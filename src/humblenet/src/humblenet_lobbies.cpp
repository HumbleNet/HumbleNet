#include "humblenet_lobbies.h"

#include "humblepeer.h"

#include "humblenet_p2p_internal.h"

#include "humblenet_attributes_internal.h"

ha_requestId HUMBLENET_CALL humblenet_lobby_create(humblenet_LobbyType type, uint16_t maxMembers)
{
	if (type == HUMBLENET_LOBBY_TYPE_UNKNOWN) {
		type = HUMBLENET_LOBBY_TYPE_PRIVATE;
	}
	return humblenet::sendLobbyCreate( humbleNetState.p2pConn.get(), type, maxMembers );
}

ha_requestId HUMBLENET_CALL humblenet_lobby_join(LobbyId lobbyId)
{
	return humblenet::sendLobbyJoin( humbleNetState.p2pConn.get(), lobbyId );
}

ha_requestId HUMBLENET_CALL humblenet_lobby_leave(LobbyId lobbyId)
{
	return humblenet::sendLobbyLeave( humbleNetState.p2pConn.get(), lobbyId );
}

PeerId HUMBLENET_CALL humblenet_lobby_owner(LobbyId lobbyId)
{
	auto it = humbleNetState.lobbies.find( lobbyId );
	if (it != humbleNetState.lobbies.end()) {
		return it->second.ownerPeerId;
	}

	return 0;
}

uint16_t HUMBLENET_CALL humblenet_lobby_max_members(LobbyId lobbyId)
{
	auto it = humbleNetState.lobbies.find( lobbyId );
	if (it != humbleNetState.lobbies.end()) {
		return it->second.maxMembers;
	}

	return 0;
}

humblenet_LobbyType HUMBLENET_CALL humblenet_lobby_type(LobbyId lobbyId)
{
	auto it = humbleNetState.lobbies.find( lobbyId );
	if (it != humbleNetState.lobbies.end()) {
		return it->second.type;
	}

	return HUMBLENET_LOBBY_TYPE_UNKNOWN;
}

uint16_t HUMBLENET_CALL humblenet_lobby_member_count(LobbyId lobbyId)
{
	auto it = humbleNetState.lobbies.find( lobbyId );
	if (it != humbleNetState.lobbies.end()) {
		return static_cast<uint16_t>(it->second.members.size());
	}

	return 0;
}

uint16_t HUMBLENET_CALL humblenet_lobby_get_members(LobbyId lobbyId, PeerId* peerList, uint16_t nelts)
{
	auto it = humbleNetState.lobbies.find( lobbyId );
	if (it != humbleNetState.lobbies.end() && peerList) {
		uint16_t i = 0;
		for (const auto& p : it->second.members) {
			if ((i + 1) > nelts) break;
			peerList[i] = p.first;
			++i;
		}
		return i;
	}

	return 0;
}

humblenet_AttributeMap* HUMBLENET_CALL humblenet_lobby_get_attributes(LobbyId lobbyId)
{
	auto it = humbleNetState.lobbies.find( lobbyId );
	if (it != humbleNetState.lobbies.end()) {
		return humblenet_attributes_new_from( it->second.attributes );
	}

	return nullptr;
}

humblenet_AttributeMap* HUMBLENET_CALL humblenet_lobby_get_member_attributes(LobbyId lobbyId, PeerId peerId)
{
	auto it = humbleNetState.lobbies.find( lobbyId );
	if (it != humbleNetState.lobbies.end()) {
		auto pit = it->second.members.find( peerId );
		if (pit != it->second.members.end()) {
			return humblenet_attributes_new_from( pit->second.attributes );
		}
	}

	return nullptr;
}

ha_requestId HUMBLENET_CALL humblenet_lobby_set_type(LobbyId lobbyId, humblenet_LobbyType type)
{
	auto it = humbleNetState.lobbies.find( lobbyId );
	if (it != humbleNetState.lobbies.end()) {
		return humblenet::sendLobbyUpdate( humbleNetState.p2pConn.get(), lobbyId, type, 0,
										   humblenet::HumblePeer::AttributeMode::Merge, {} );
	}

	return 0;
}

ha_requestId HUMBLENET_CALL humblenet_lobby_set_max_members(LobbyId lobbyId, uint16_t maxMembers)
{
	auto it = humbleNetState.lobbies.find( lobbyId );
	if (it != humbleNetState.lobbies.end()) {
		return humblenet::sendLobbyUpdate( humbleNetState.p2pConn.get(), lobbyId, HUMBLENET_LOBBY_TYPE_UNKNOWN,
										   maxMembers,
										   humblenet::HumblePeer::AttributeMode::Merge, {} );
	}

	return 0;
}

ha_requestId HUMBLENET_CALL humblenet_lobby_set_attributes(LobbyId lobbyId,
														   humblenet_AttributeMap* attributes, ha_bool replace)
{
	auto it = humbleNetState.lobbies.find( lobbyId );
	if (it != humbleNetState.lobbies.end()) {
		auto& attribs = humblenet_attributes_get_map( attributes );
		auto mode = replace ? humblenet::HumblePeer::AttributeMode::Replace
							: humblenet::HumblePeer::AttributeMode::Merge;

		return humblenet::sendLobbyUpdate( humbleNetState.p2pConn.get(), lobbyId,
										   HUMBLENET_LOBBY_TYPE_UNKNOWN, 0,
										   mode, attribs );
	}

	return 0;
}

ha_requestId HUMBLENET_CALL humblenet_lobby_set_member_attributes(LobbyId lobbyId, PeerId peerId,
																  humblenet_AttributeMap* attributes, ha_bool replace)
{
	auto it = humbleNetState.lobbies.find( lobbyId );
	if (it != humbleNetState.lobbies.end()) {
		auto& attribs = humblenet_attributes_get_map( attributes );
		auto mode = replace ? humblenet::HumblePeer::AttributeMode::Replace
							: humblenet::HumblePeer::AttributeMode::Merge;

		return humblenet::sendLobbyMemberUpdate( humbleNetState.p2pConn.get(),
												 lobbyId, peerId,
												 mode, attribs );
	}

	return 0;
}