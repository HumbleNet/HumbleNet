#include <cassert>
#include <cstdio>
#include <cstring>

#include <limits>

#include "humblepeer.h"

#include "crc.h"

#include "hmac.h"

#include <ctime>

#define LOG printf

#define DEFAULT_FBB_SIZE 512


namespace humblenet {
	const size_t PEER_OFFSET_SIZE = 8;

	static_assert( sizeof(flatbuffers::uoffset_t) * 2 == PEER_OFFSET_SIZE, "PEER_OFFSET_SIZE must equal 2x flatbuffers::uoffset_t");

	// Custom allocator so we can pre-build our 2 uoffset_t's one for message size and one for crc
	class peer_allocator : public flatbuffers::DefaultAllocator {
		const size_t _offset;
	public:
		peer_allocator(size_t offset) : _offset(offset) {}

		uint8_t *allocate(size_t size) const {

			uint8_t *p = new uint8_t[size + _offset];
			return p ? (p + _offset) : nullptr;
		}
		void deallocate(uint8_t *p) const {
			if (p) {
				delete[] (p - _offset);
			}
		}
	};

	static peer_allocator peer_fbb_allocator(PEER_OFFSET_SIZE);

	flatbuffers::Offset<flatbuffers::String> CreateFBBStringIfNotEmpty(flatbuffers::FlatBufferBuilder &fbb, const std::string &str)
	{
		if (str.empty()) {
			return 0;
		} else {
			return fbb.CreateString(str);
		}
	}

	template<typename T>
	flatbuffers::Offset<flatbuffers::Vector<T>> CreateFBBVectorIfNotEmpty(flatbuffers::FlatBufferBuilder &fbb, const std::vector<T> &v)
	{
		if (v.empty()) {
			return 0;
		} else {
			return fbb.CreateVector(v);
		}
	}

	flatbuffers::Offset<HumblePeer::AttributeSet> buildAttributeSet(flatbuffers::FlatBufferBuilder& fbb,
																	HumblePeer::AttributeMode mode,
																	const AttributeMap& attributes)
	{
		if (mode == HumblePeer::AttributeMode::Merge && attributes.empty()) {
			// result is nothing so don't bother creating an attribute set
			return 0;
		}

		std::vector<flatbuffers::Offset<HumblePeer::Attribute>> tempAttribs;
		tempAttribs.reserve( attributes.size());

		for (auto& it : attributes) {
			tempAttribs.emplace_back( HumblePeer::CreateAttributeDirect(fbb, it.first.c_str(), it.second.c_str()) );
		}

		return HumblePeer::CreateAttributeSetDirect( fbb, mode, tempAttribs.empty() ? nullptr : &tempAttribs );
	}

	enum class RequestType : uint8_t {
		Alias = 0x1,
		Lobby = 0x2,
	};

	ha_requestId generateRequestId(RequestType type) {
		static uint32_t id_counter = 0;
		return (static_cast<uint32_t>(type) << 24u) | ((++id_counter) % 0xffffff);
	}

	// Sending and parsing

	inline ha_bool WARN_UNUSED_RESULT sendP2PMessage(P2PSignalConnection *conn, const flatbuffers::FlatBufferBuilder& fbb)
	{
		uint8_t *buff = fbb.GetBufferPointer();
		flatbuffers::uoffset_t size = fbb.GetSize();

		uint8_t *base = buff - PEER_OFFSET_SIZE;

		memset(base, 0, PEER_OFFSET_SIZE);
		flatbuffers::WriteScalar(base, size);

		auto crc = crc_init();
		crc = crc_update(crc, buff, size);
		crc = crc_finalize(crc);

		flatbuffers::WriteScalar(base + sizeof(flatbuffers::uoffset_t), flatbuffers::uoffset_t(crc));

		return sendP2PMessage(conn, base, size + PEER_OFFSET_SIZE);
	}

	ha_bool parseMessage(std::vector<uint8_t> &recvBuf, ProcessMsgFunc processFunc, void *user_data)
	{
		if (recvBuf.empty()) {
			// nothing to do
			return 1;
		}

		// first PEER_OFFSET_SIZE bytes are our packet header
		flatbuffers::uoffset_t fbSize = flatbuffers::ReadScalar<flatbuffers::uoffset_t>(recvBuf.data());
		// make sure we have enough data!

		if (recvBuf.size() < (fbSize + PEER_OFFSET_SIZE)) {
			// partial payload, try again later
			return 1;
		}

		const uint8_t* buff = recvBuf.data() + PEER_OFFSET_SIZE;

		auto crc = crc_init();
		crc = crc_update(crc, buff, fbSize);
		crc = crc_finalize(crc);

		flatbuffers::uoffset_t fbCrc = flatbuffers::ReadScalar<flatbuffers::uoffset_t>(recvBuf.data() + sizeof(flatbuffers::uoffset_t));

		if (fbCrc != crc) {
			// TODO should we disconnect in this case?
			return 0;
		}

		// Now validate our buffer based on the expected size
		flatbuffers::Verifier v(buff, fbSize);
		if (!HumblePeer::VerifyMessageBuffer(v)) {
			// TODO should we disconnect in this case?
			return 0;
		}

		auto message = HumblePeer::GetMessage(buff);

		// process it
		ha_bool messageOk = processFunc(message, user_data);
		if (!messageOk) {
			// processFunc didn't like this message for some reason
			return 0;
		}

		recvBuf.erase(recvBuf.begin(), recvBuf.begin() + fbSize + PEER_OFFSET_SIZE);

		// no need to check if recvBuf is empty since parseMessage will do it
		return parseMessage(recvBuf, processFunc, user_data);
	}

	// ** Peer server connection

	ha_bool sendHelloServer(P2PSignalConnection *conn, uint8_t flags,
							const std::string& gametoken, const std::string& gamesecret,
							const std::string& authToken, const std::string& reconnectToken,
							const std::map<std::string, std::string>& attributes)
	{
		assert(!gametoken.empty());
		assert(!gamesecret.empty());

		flatbuffers::FlatBufferBuilder fbb(DEFAULT_FBB_SIZE, &peer_fbb_allocator);

		std::vector<flatbuffers::Offset<HumblePeer::Attribute>> tempAttribs;

		// build up attributes
		tempAttribs.reserve(attributes.size() + 1);

		for (auto& it : attributes) {
			if (it.first == "timestamp") continue;
			tempAttribs.emplace_back( HumblePeer::CreateAttribute(fbb, fbb.CreateString(it.first), fbb.CreateString(it.second)) );
		}

		tempAttribs.emplace_back( HumblePeer::CreateAttribute(fbb, fbb.CreateString("timestamp"), fbb.CreateString(std::to_string(time(NULL)))) );

		uint32_t version = 0;

		HMACContext hmac;
		HMACInit(&hmac, (const uint8_t*)gamesecret.data(), gamesecret.size());

		HMACInput(&hmac, (const uint8_t*)authToken.data(), authToken.size());
		HMACInput(&hmac, (const uint8_t*)reconnectToken.data(), reconnectToken.size());

		flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<HumblePeer::Attribute>>> attribs;
		if (!tempAttribs.empty()) {
			attribs = fbb.CreateVectorOfSortedTables(&tempAttribs);
			// tempAttribs is sorted now
			for (const auto& it : tempAttribs) {
				auto attr = flatbuffers::EndianScalar(reinterpret_cast<const HumblePeer::Attribute *>(fbb.GetCurrentBufferPointer() + fbb.GetSize() - it.o));
				auto key = attr->key();
				auto value = attr->value();
				HMACInput(&hmac, key->Data(), key->size());
				HMACInput(&hmac, value->Data(), value->size());
			}
		}

		uint8_t hmacresult[HMAC_DIGEST_SIZE];

		HMACResult(&hmac, hmacresult);

		std::string signature;

		HMACResultToHex(hmacresult, signature);

		auto packet = HumblePeer::CreateHelloServer(fbb,
													version,
													flags,
													fbb.CreateString(gametoken),
													fbb.CreateString(signature),
													CreateFBBStringIfNotEmpty(fbb, authToken),
													CreateFBBStringIfNotEmpty(fbb, reconnectToken),
													attribs);

		auto msg = HumblePeer::CreateMessage(fbb, HumblePeer::MessageType::HelloServer, packet.Union());
		fbb.Finish(msg);

		return sendP2PMessage(conn, fbb);
	}


	ha_bool sendHelloClient(P2PSignalConnection *conn, PeerId peerId, const std::string& reconnectToken, const std::vector<ICEServer>& iceServers)
	{
		flatbuffers::FlatBufferBuilder fbb(DEFAULT_FBB_SIZE, &peer_fbb_allocator);

		std::vector<flatbuffers::Offset<HumblePeer::ICEServer>> tempServers;
		tempServers.reserve(iceServers.size());

		for( auto& it : iceServers) {
			auto server = fbb.CreateString(it.server);
			flatbuffers::Offset<HumblePeer::ICEServer> s;

			if (it.type == HumblePeer::ICEServerType::STUNServer) {
				s = HumblePeer::CreateICEServer(fbb, HumblePeer::ICEServerType::STUNServer, server);
			} else { // turn server
				s = HumblePeer::CreateICEServer(fbb, HumblePeer::ICEServerType::TURNServer, server, fbb.CreateString(it.username), fbb.CreateString(it.password));
			}
			tempServers.emplace_back( s );
		}

		auto packet = HumblePeer::CreateHelloClient(fbb, peerId,
													CreateFBBStringIfNotEmpty(fbb, reconnectToken),
													CreateFBBVectorIfNotEmpty(fbb, tempServers));
		auto msg = HumblePeer::CreateMessage(fbb, HumblePeer::MessageType::HelloClient, packet.Union());
		fbb.Finish(msg);

		return sendP2PMessage(conn, fbb);
	}

	// ** generic messages

	ha_bool sendSuccess(P2PSignalConnection *conn, ha_requestId requestId, HumblePeer::MessageType successType)
	{
		flatbuffers::FlatBufferBuilder fbb(DEFAULT_FBB_SIZE, &peer_fbb_allocator);
		auto packet = HumblePeer::CreateSuccess(fbb);
		auto msg = HumblePeer::CreateMessage(fbb, successType, packet.Union(), requestId);
		fbb.Finish(msg);

		return sendP2PMessage(conn, fbb);
	}

	ha_bool sendError(P2PSignalConnection *conn, ha_requestId requestId, const std::string& error, HumblePeer::MessageType errorType)
	{
		flatbuffers::FlatBufferBuilder fbb(DEFAULT_FBB_SIZE, &peer_fbb_allocator);
		auto packet = HumblePeer::CreateError(fbb, fbb.CreateString(error));
		auto msg = HumblePeer::CreateMessage(fbb, errorType, packet.Union(), requestId);
		fbb.Finish(msg);

		return sendP2PMessage(conn, fbb);
	}

	// ** P2P Negotiation messages

	ha_bool sendNoSuchPeer(P2PSignalConnection *conn, PeerId peerId)
	{
		flatbuffers::FlatBufferBuilder fbb(DEFAULT_FBB_SIZE, &peer_fbb_allocator);
		auto packet = HumblePeer::CreateP2PReject(fbb, peerId, HumblePeer::P2PRejectReason::NotFound);
		auto msg = HumblePeer::CreateMessage(fbb, HumblePeer::MessageType::P2PReject, packet.Union());
		fbb.Finish(msg);

		return sendP2PMessage(conn, fbb);
	}

	ha_bool sendPeerRefused(P2PSignalConnection *conn, PeerId peerId)
	{
		flatbuffers::FlatBufferBuilder fbb(DEFAULT_FBB_SIZE, &peer_fbb_allocator);
		auto packet = HumblePeer::CreateP2PReject(fbb, peerId, HumblePeer::P2PRejectReason::PeerRefused);
		auto msg = HumblePeer::CreateMessage(fbb, HumblePeer::MessageType::P2PReject, packet.Union());
		fbb.Finish(msg);

		return sendP2PMessage(conn, fbb);
	}

	ha_bool sendP2POffer(P2PSignalConnection *conn, PeerId peerId, uint8_t flags, const char* offer)
	{
		flatbuffers::FlatBufferBuilder fbb(DEFAULT_FBB_SIZE, &peer_fbb_allocator);
		auto packet = HumblePeer::CreateP2POffer(fbb, peerId, flags, fbb.CreateString(offer));
		auto msg = HumblePeer::CreateMessage(fbb, HumblePeer::MessageType::P2POffer, packet.Union());
		fbb.Finish(msg);

		return sendP2PMessage(conn, fbb);
	}

	ha_bool sendP2PResponse(P2PSignalConnection *conn, PeerId peerId, const char* offer)
	{
		flatbuffers::FlatBufferBuilder fbb(DEFAULT_FBB_SIZE, &peer_fbb_allocator);
		auto packet = HumblePeer::CreateP2PAnswer(fbb, peerId, fbb.CreateString(offer));
		auto msg = HumblePeer::CreateMessage(fbb, HumblePeer::MessageType::P2PAnswer, packet.Union());
		fbb.Finish(msg);

		return sendP2PMessage(conn, fbb);
	}

	ha_bool sendICECandidate(P2PSignalConnection *conn, PeerId peerId, const char* offer)
	{
		flatbuffers::FlatBufferBuilder fbb(DEFAULT_FBB_SIZE, &peer_fbb_allocator);
		auto packet = HumblePeer::CreateICECandidate(fbb, peerId, fbb.CreateString(offer));
		auto msg = HumblePeer::CreateMessage(fbb, HumblePeer::MessageType::ICECandidate, packet.Union());
		fbb.Finish(msg);

		return sendP2PMessage(conn, fbb);
	}

	ha_bool sendP2PConnected(P2PSignalConnection *conn, PeerId peerId)
	{
		flatbuffers::FlatBufferBuilder fbb(DEFAULT_FBB_SIZE, &peer_fbb_allocator);
		auto packet = HumblePeer::CreateP2PConnected(fbb, peerId);
		auto msg = HumblePeer::CreateMessage(fbb, HumblePeer::MessageType::P2PConnected, packet.Union());
		fbb.Finish(msg);

		return sendP2PMessage(conn, fbb);

	}

	ha_bool sendP2PDisconnect(P2PSignalConnection *conn, PeerId peerId)
	{
		flatbuffers::FlatBufferBuilder fbb(DEFAULT_FBB_SIZE, &peer_fbb_allocator);
		auto packet = HumblePeer::CreateP2PDisconnect(fbb, peerId);
		auto msg = HumblePeer::CreateMessage(fbb, HumblePeer::MessageType::P2PDisconnect, packet.Union());
		fbb.Finish(msg);

		return sendP2PMessage(conn, fbb);

	}

	ha_bool sendP2PRelayData(humblenet::P2PSignalConnection *conn, PeerId peerId, const void* data, uint16_t length) {
		flatbuffers::FlatBufferBuilder fbb(DEFAULT_FBB_SIZE, &peer_fbb_allocator);
		auto packet = HumblePeer::CreateP2PRelayData(fbb, peerId, fbb.CreateVector((int8_t*)data, length));
		auto msg = HumblePeer::CreateMessage(fbb, HumblePeer::MessageType::P2PRelayData, packet.Union());
		fbb.Finish(msg);

		return sendP2PMessage(conn, fbb);
	}

	// ** Name Alias messages

	ha_requestId sendAliasRegister(P2PSignalConnection *conn, const std::string& alias)
	{
		flatbuffers::FlatBufferBuilder fbb(DEFAULT_FBB_SIZE, &peer_fbb_allocator);
		auto requestId = generateRequestId(RequestType::Alias);
		auto packet = HumblePeer::CreateAliasRegister(fbb, fbb.CreateString(alias));
		auto msg = HumblePeer::CreateMessage(fbb, HumblePeer::MessageType::AliasRegister, packet.Union(), requestId);
		fbb.Finish(msg);

		return sendP2PMessage(conn, fbb) ? requestId : 0;
	}

	ha_requestId sendAliasUnregister(P2PSignalConnection *conn, const std::string& alias)
	{
		flatbuffers::FlatBufferBuilder fbb(DEFAULT_FBB_SIZE, &peer_fbb_allocator);
		auto requestId = generateRequestId(RequestType::Alias);
		auto packet = HumblePeer::CreateAliasUnregister(fbb, CreateFBBStringIfNotEmpty(fbb, alias));
		auto msg = HumblePeer::CreateMessage(fbb, HumblePeer::MessageType::AliasUnregister, packet.Union(), requestId);
		fbb.Finish(msg);

		return sendP2PMessage(conn, fbb) ? requestId : 0;
	}

	ha_requestId sendAliasLookup(P2PSignalConnection *conn, const std::string& alias)
	{
		flatbuffers::FlatBufferBuilder fbb(DEFAULT_FBB_SIZE, &peer_fbb_allocator);
		auto requestId = generateRequestId(RequestType::Alias);
		auto packet = HumblePeer::CreateAliasLookup(fbb, fbb.CreateString(alias));
		auto msg = HumblePeer::CreateMessage(fbb, HumblePeer::MessageType::AliasLookup, packet.Union(), requestId);
		fbb.Finish(msg);

		return sendP2PMessage(conn, fbb) ? requestId : 0;
	}

	ha_bool sendAliasResolved(P2PSignalConnection *conn, const std::string& alias, PeerId peer, ha_requestId requestId)
	{
		flatbuffers::FlatBufferBuilder fbb(DEFAULT_FBB_SIZE, &peer_fbb_allocator);
		auto packet = HumblePeer::CreateAliasResolved(fbb, fbb.CreateString(alias), peer);
		auto msg = HumblePeer::CreateMessage(fbb, HumblePeer::MessageType::AliasResolved, packet.Union(), requestId);
		fbb.Finish(msg);

		return sendP2PMessage(conn, fbb);
	}

	// region Lobby messages

	flatbuffers::Offset<HumblePeer::LobbyDetails> buildLobbyDetails(flatbuffers::FlatBufferBuilder& fbb,
																	PeerId ownerPeerId,
																	humblenet_LobbyType lobbyType, uint16_t maxMembers,
																	const AttributeMap& attributes)
	{
		auto attribSet = buildAttributeSet(fbb, HumblePeer::AttributeMode::Replace, attributes );

		return HumblePeer::CreateLobbyDetails(fbb, ownerPeerId, lobbyType, maxMembers, attribSet);

	}

	ha_requestId sendLobbyCreate(P2PSignalConnection *conn, humblenet_LobbyType type, uint16_t maxMembers)
	{
		flatbuffers::FlatBufferBuilder fbb(DEFAULT_FBB_SIZE, &peer_fbb_allocator);
		auto requestId = generateRequestId(RequestType::Lobby);
		auto packet = HumblePeer::CreateLobbyCreate(fbb, static_cast<uint8_t>(type), maxMembers);
		auto msg = HumblePeer::CreateMessage(fbb, HumblePeer::MessageType::LobbyCreate, packet.Union(), requestId);
		fbb.Finish(msg);

		return sendP2PMessage(conn, fbb) ? requestId : 0;
	}

	ha_bool sendLobbyDidCreate(P2PSignalConnection* conn, ha_requestId requestId, const Lobby& lobby)
	{
		flatbuffers::FlatBufferBuilder fbb(DEFAULT_FBB_SIZE, &peer_fbb_allocator);

		auto lobbyDetails = buildLobbyDetails(fbb, lobby.ownerPeerId, lobby.type, lobby.maxMembers, lobby.attributes);

		auto packet = HumblePeer::CreateLobbyDidCreate(fbb, lobby.lobbyId, lobbyDetails);

		auto msg = HumblePeer::CreateMessage(fbb, HumblePeer::MessageType::LobbyDidCreate, packet.Union(), requestId);
		fbb.Finish(msg);

		return sendP2PMessage(conn, fbb);
	}

	ha_requestId sendLobbyJoin(P2PSignalConnection *conn, LobbyId lobbyId)
	{
		flatbuffers::FlatBufferBuilder fbb(DEFAULT_FBB_SIZE, &peer_fbb_allocator);
		auto requestId = generateRequestId(RequestType::Lobby);
		auto packet = HumblePeer::CreateLobbyJoin(fbb, lobbyId);
		auto msg = HumblePeer::CreateMessage(fbb, HumblePeer::MessageType::LobbyJoin, packet.Union(), requestId);
		fbb.Finish(msg);

		return sendP2PMessage(conn, fbb) ? requestId : 0;
	}

	ha_bool sendLobbyDidJoinSelf(P2PSignalConnection* conn, ha_requestId requestId, const Lobby& lobby, PeerId peerId,
							 const AttributeMap& memberAttributes)
	{
		flatbuffers::FlatBufferBuilder fbb(DEFAULT_FBB_SIZE, &peer_fbb_allocator);

		auto lobbyDetails = buildLobbyDetails(fbb, lobby.ownerPeerId, lobby.type, lobby.maxMembers, lobby.attributes);

		flatbuffers::Offset<HumblePeer::LobbyMemberDetails> memberDetails;
		if (!memberAttributes.empty()) {
			auto attribSet = buildAttributeSet(fbb, HumblePeer::AttributeMode::Replace, memberAttributes);

			memberDetails = HumblePeer::CreateLobbyMemberDetails(fbb, attribSet);
		}

		auto packet = HumblePeer::CreateLobbyDidJoin(fbb, lobby.lobbyId, peerId, lobbyDetails, memberDetails);
		auto msg = HumblePeer::CreateMessage(fbb, HumblePeer::MessageType::LobbyDidJoin, packet.Union(), requestId);
		fbb.Finish(msg);

		return sendP2PMessage(conn, fbb);
	}

	ha_bool sendLobbyDidJoin(P2PSignalConnection* conn, LobbyId lobbyId, PeerId peerId,
							 const AttributeMap& memberAttributes)
	{
		flatbuffers::FlatBufferBuilder fbb(DEFAULT_FBB_SIZE, &peer_fbb_allocator);

		auto attribSet = buildAttributeSet(fbb, HumblePeer::AttributeMode::Replace, memberAttributes);

		auto memberDetails = HumblePeer::CreateLobbyMemberDetails(fbb, attribSet);

		auto packet = HumblePeer::CreateLobbyDidJoin(fbb, lobbyId, peerId, 0, memberDetails);
		auto msg = HumblePeer::CreateMessage(fbb, HumblePeer::MessageType::LobbyDidJoin, packet.Union());
		fbb.Finish(msg);

		return sendP2PMessage(conn, fbb);
	}

	ha_requestId sendLobbyLeave(P2PSignalConnection *conn, LobbyId lobbyId)
	{
		flatbuffers::FlatBufferBuilder fbb(DEFAULT_FBB_SIZE, &peer_fbb_allocator);
		auto requestId = generateRequestId(RequestType::Lobby);
		auto packet = HumblePeer::CreateLobbyLeave(fbb, lobbyId);
		auto msg = HumblePeer::CreateMessage(fbb, HumblePeer::MessageType::LobbyLeave, packet.Union(), requestId);
		fbb.Finish(msg);

		return sendP2PMessage(conn, fbb) ? requestId : 0;
	}

	ha_bool sendLobbyDidLeave(P2PSignalConnection *conn, ha_requestId requestId, LobbyId lobbyId, PeerId peerId)
	{
		flatbuffers::FlatBufferBuilder fbb(DEFAULT_FBB_SIZE, &peer_fbb_allocator);
		auto packet = HumblePeer::CreateLobbyDidLeave(fbb, lobbyId, peerId);
		auto msg = HumblePeer::CreateMessage(fbb, HumblePeer::MessageType::LobbyDidLeave, packet.Union(), requestId);
		fbb.Finish(msg);

		return sendP2PMessage(conn, fbb);
	}

	ha_requestId sendLobbyUpdate(P2PSignalConnection* conn,
								 LobbyId lobbyId,
								 humblenet_LobbyType type, uint16_t maxMembers,
								 HumblePeer::AttributeMode mode, const AttributeMap& attributes)
	{
		flatbuffers::FlatBufferBuilder fbb( DEFAULT_FBB_SIZE, &peer_fbb_allocator );

		auto requestId = generateRequestId( RequestType::Lobby );

		auto attribSet = buildAttributeSet(fbb, mode, attributes );

		auto packet = HumblePeer::CreateLobbyUpdate( fbb, lobbyId, static_cast<uint8_t>(type), maxMembers, attribSet );
		auto msg = HumblePeer::CreateMessage( fbb, HumblePeer::MessageType::LobbyUpdate, packet.Union(), requestId );

		fbb.Finish(msg);

		return sendP2PMessage(conn, fbb) ? requestId : 0;
	}

	ha_requestId sendLobbyMemberUpdate(P2PSignalConnection* conn,
									   LobbyId lobbyId, PeerId peerId,
									   HumblePeer::AttributeMode mode, const AttributeMap& attributes)
	{
		flatbuffers::FlatBufferBuilder fbb( DEFAULT_FBB_SIZE, &peer_fbb_allocator );

		auto requestId = generateRequestId( RequestType::Lobby );

		auto attribSet = buildAttributeSet(fbb, mode, attributes );

		auto packet = HumblePeer::CreateLobbyMemberUpdate( fbb, lobbyId, peerId, attribSet );
		auto msg = HumblePeer::CreateMessage( fbb, HumblePeer::MessageType::LobbyMemberUpdate, packet.Union(), requestId);
		fbb.Finish( msg );

		return sendP2PMessage( conn, fbb ) ? requestId : 0;
	}

	ha_bool sendLobbyDidUpdate(P2PSignalConnection* conn, ha_requestId requestId,
							   LobbyId lobbyId,
							   humblenet_LobbyType type, uint16_t maxMembers,
							   HumblePeer::AttributeMode mode, const AttributeMap& attributes)
	{
		flatbuffers::FlatBufferBuilder fbb( DEFAULT_FBB_SIZE, &peer_fbb_allocator );

		auto attribSet = buildAttributeSet(fbb, mode, attributes );

		auto packet = HumblePeer::CreateLobbyDidUpdate( fbb, lobbyId, 0, static_cast<uint8_t>(type), maxMembers, attribSet );
		auto msg = HumblePeer::CreateMessage( fbb, HumblePeer::MessageType::LobbyDidUpdate, packet.Union(), requestId );

		fbb.Finish(msg);

		return sendP2PMessage(conn, fbb);
	}

	ha_bool sendLobbyMemberDidUpdate(P2PSignalConnection* conn, ha_requestId requestId,
								  LobbyId lobbyId, PeerId peerId,
								  HumblePeer::AttributeMode mode,
								  const AttributeMap& attributes)
	{
		flatbuffers::FlatBufferBuilder fbb( DEFAULT_FBB_SIZE, &peer_fbb_allocator );

		auto attribSet = buildAttributeSet(fbb, mode, attributes );

		auto packet = HumblePeer::CreateLobbyMemberDidUpdate( fbb, lobbyId, peerId, 0, attribSet );
		auto msg = HumblePeer::CreateMessage( fbb, HumblePeer::MessageType::LobbyMemberDidUpdate, packet.Union(), requestId );

		fbb.Finish(msg);

		return sendP2PMessage(conn, fbb);
	}

	// endregion
}  // namespace humblenet
