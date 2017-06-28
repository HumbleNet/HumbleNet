#include <cassert>
#include <cstdio>
#include <cstring>

#include <limits>

#include "humblenet.h"
#include "humblepeer.h"

#include "crc.h"

#include "hmac.h"

#include <ctime>

#define LOG printf

#define DEFAULT_FBB_SIZE 512


namespace humblenet {
	const size_t PEER_OFFSET_SIZE = 8;

	class peer_allocator : public flatbuffers::simple_allocator {
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

	ha_bool sendP2PConnect(P2PSignalConnection *conn, PeerId peerId, uint8_t flags, const char* offer)
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

	ha_bool sendAliasRegister(P2PSignalConnection *conn, const std::string& alias)
	{
		flatbuffers::FlatBufferBuilder fbb(DEFAULT_FBB_SIZE, &peer_fbb_allocator);
		auto packet = HumblePeer::CreateAliasRegister(fbb, fbb.CreateString(alias));
		auto msg = HumblePeer::CreateMessage(fbb, HumblePeer::MessageType::AliasRegister, packet.Union());
		fbb.Finish(msg);

		return sendP2PMessage(conn, fbb);
	}

	ha_bool sendAliasUnregister(P2PSignalConnection *conn, const std::string& alias)
	{
		flatbuffers::FlatBufferBuilder fbb(DEFAULT_FBB_SIZE, &peer_fbb_allocator);
		auto packet = HumblePeer::CreateAliasUnregister(fbb, CreateFBBStringIfNotEmpty(fbb, alias));
		auto msg = HumblePeer::CreateMessage(fbb, HumblePeer::MessageType::AliasUnregister, packet.Union());
		fbb.Finish(msg);

		return sendP2PMessage(conn, fbb);
	}

	ha_bool sendAliasLookup(P2PSignalConnection *conn, const std::string& alias)
	{
		flatbuffers::FlatBufferBuilder fbb(DEFAULT_FBB_SIZE, &peer_fbb_allocator);
		auto packet = HumblePeer::CreateAliasLookup(fbb, fbb.CreateString(alias));
		auto msg = HumblePeer::CreateMessage(fbb, HumblePeer::MessageType::AliasLookup, packet.Union());
		fbb.Finish(msg);

		return sendP2PMessage(conn, fbb);
	}

	ha_bool sendAliasResolved(P2PSignalConnection *conn, const std::string& alias, PeerId peer)
	{
		flatbuffers::FlatBufferBuilder fbb(DEFAULT_FBB_SIZE, &peer_fbb_allocator);
		auto packet = HumblePeer::CreateAliasResolved(fbb, fbb.CreateString(alias), peer);
		auto msg = HumblePeer::CreateMessage(fbb, HumblePeer::MessageType::AliasResolved, packet.Union());
		fbb.Finish(msg);

		return sendP2PMessage(conn, fbb);
	}
}  // namespace humblenet
