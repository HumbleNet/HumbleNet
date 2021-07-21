#ifndef HUMBLENET_LOBBIES_H
#define HUMBLENET_LOBBIES_H

#include "humblenet.h"
#include "humblenet_attributes.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum humblenet_LobbyType {
	HUMBLENET_LOBBY_TYPE_UNKNOWN = 0,
	HUMBLENET_LOBBY_TYPE_PRIVATE,
	HUMBLENET_LOBBY_TYPE_PUBLIC,
} humblenet_LobbyType;

typedef enum humblenet_LobbyComparison {
	HUMBLENET_LOBBY_COMPARE_EQUAL = 0,
	HUMBLENET_LOBBY_COMPARE_NOT_EQUAL,
	HUMBLENET_LOBBY_COMPARE_GREATER,
	HUMBLENET_LOBBY_COMPARE_GREATER_OR_EQUAL,
	HUMBLENET_LOBBY_COMPARE_LESS,
	HUMBLENET_LOBBY_COMPARE_LESS_OR_EQUAL,
} humblenet_LobbyComparison;

/** Lobby handling */

/*
 * Create a new lobby
 */
HUMBLENET_API ha_requestId HUMBLENET_CALL humblenet_lobby_create(humblenet_LobbyType, uint16_t maxMembers);

/*
 * Join a lobby
 */
HUMBLENET_API ha_requestId HUMBLENET_CALL humblenet_lobby_join(LobbyId);

/*
 * Leave a lobby
 */
HUMBLENET_API ha_requestId HUMBLENET_CALL humblenet_lobby_leave(LobbyId);

/** Querying lobby information */

/*
 * Get the lobby owner
 */
HUMBLENET_API PeerId HUMBLENET_CALL humblenet_lobby_owner(LobbyId);

/*
 * Get the lobby max member count
 */
HUMBLENET_API uint16_t HUMBLENET_CALL humblenet_lobby_max_members(LobbyId);

/*
 * Get the lobby type
 */
HUMBLENET_API humblenet_LobbyType HUMBLENET_CALL humblenet_lobby_type(LobbyId);

/*
 * Get lobby member count
 */
HUMBLENET_API uint16_t HUMBLENET_CALL humblenet_lobby_member_count(LobbyId);

/*
 * Fetch the list of members in the lobby
 */
HUMBLENET_API uint16_t HUMBLENET_CALL humblenet_lobby_get_members(LobbyId, PeerId*, uint16_t nelts);

/*
 * Returns a copy of the lobby attributes
 */
HUMBLENET_API humblenet_AttributeMap* HUMBLENET_CALL humblenet_lobby_get_attributes(LobbyId);

/*
 * Returns a copy of the lobby member attributes
 */
HUMBLENET_API humblenet_AttributeMap* HUMBLENET_CALL humblenet_lobby_get_member_attributes(LobbyId, PeerId);

/** Modifying a lobby */

/*
 * Set the lobby max member count
 */
HUMBLENET_API ha_requestId HUMBLENET_CALL humblenet_lobby_set_max_members(LobbyId, uint16_t maxMembers);

/*
 * Set the lobby type
 */
HUMBLENET_API ha_requestId HUMBLENET_CALL humblenet_lobby_set_type(LobbyId, humblenet_LobbyType);

/*
 * Sets attributes on the lobby.
 *
 * when replace is true it will replace all attributes with only the set provided
 * when replace is false it will set/override only the attributes provided (unspecified attributes will not be touched)
 */
HUMBLENET_API ha_requestId HUMBLENET_CALL humblenet_lobby_set_attributes(LobbyId, humblenet_AttributeMap*, ha_bool replace);

/*
 * Sets attributes on a member of the lobby.
 *
 * when replace is true it will replace all attributes with only the set provided
 * when replace is false it will set/override only the attributes provided (unspecified attributes will not be touched)
 */
HUMBLENET_API ha_requestId HUMBLENET_CALL humblenet_lobby_set_member_attributes(LobbyId, PeerId, humblenet_AttributeMap*, ha_bool replace);

#ifdef __cplusplus
}
#endif

#endif /*HUMBLENET_LOBBIES_H */