#ifndef HUMBLENET_EVENTS_H
#define HUMBLENET_EVENTS_H

#include "humblenet.h"

typedef enum {
	/* P2P events */
	HUMBLENET_EVENT_P2P_CONNECTED = 0x10,           /**< Sent when P2P connection is established */
	HUMBLENET_EVENT_P2P_DISCONNECTED,               /**< Sent when P2P connection is lost */
	HUMBLENET_EVENT_P2P_ASSIGN_PEER,                /**< Sent when P2P is assigned a peer ID */

	/* Peer events */
	HUMBLENET_EVENT_PEER_CONNECTED = 0x50,          /**< Sent when a connection to a peer is connected */
	HUMBLENET_EVENT_PEER_DISCONNECTED,              /**< Sent when a connection to a peer is disconnected */
	HUMBLENET_EVENT_PEER_REJECTED,                  /**< Sent when a connection to a peer is rejected */
	HUMBLENET_EVENT_PEER_NOT_FOUND,                 /**< Sent when a connection to a peer is not found */

	/* Alias events */
	HUMBLENET_EVENT_ALIAS_REGISTER_SUCCESS = 0x100, /**< Sent when an alias is registered successfully */
	HUMBLENET_EVENT_ALIAS_REGISTER_ERROR,           /**< Sent when alias registration fails */
	HUMBLENET_EVENT_ALIAS_RESOLVED,                 /**< Sent when an alias is resolved */
	HUMBLENET_EVENT_ALIAS_NOT_FOUND,                /**< Sent when an alias fails to resolve */
} HumbleNet_EventType;

typedef struct {
	HumbleNet_EventType type; /**< The EventType */
	uint32_t request_id;      /**< The original request ID for this event response */
} HumbleNet_Event_Common;

typedef struct {
	HumbleNet_EventType type; /**< ALIAS_NOT_FOUND, ALIAS_REGISTER_ERRO */
	uint32_t request_id;      /**< The original request ID for this event response */
	char error[48];           /**< Extra error detail */
} HumbleNet_Event_Error;

typedef struct {
	HumbleNet_EventType type; /**< P2P_ASSIGN_PEER, ALIAS_RESOLVED */
	uint32_t request_id;      /**< The original request ID for this event response */
	PeerId peer_id;           /**< A peer ID. */
} HumbleNet_Event_Peer;

union HumbleNet_Event {
	HumbleNet_EventType type;
	HumbleNet_Event_Common common;
	HumbleNet_Event_Error error;
	HumbleNet_Event_Peer peer;

	uint8_t padding[64];
};

/**
 * Pulls an event from the event queue
 *
 * @param event A pointer to an event structure or NULL
 * @return true if there are any events.
 *
 * If event parameter is null then this method will only check if there are events on the queue, but not remove anything from the queue.
 * If event parameter is not null, then and event will be popped from the queue and written to the param pointer.
 */
ha_bool HUMBLENET_CALL humblenet_event_poll(HumbleNet_Event* event);

#endif /* HUMBLENET_EVENTS_H */