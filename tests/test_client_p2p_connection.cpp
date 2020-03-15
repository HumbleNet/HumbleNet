#define CATCH_CONFIG_MAIN

#include <catch2/catch.hpp>

#include "humblepeer.h"
#include "humblenet_events.h"
#include "humblenet_p2p_internal.h"
#include "libpoll.h"
#include "libsocket.h"

struct Message {
	std::vector<uint8_t> buffer;

	const humblenet::HumblePeer::Message* message = nullptr;
};
std::vector<Message> messages;

std::vector<std::string> calls;

// region Define stubs

void humblenet_p2p_shutdown() {}

bool poll_was_init() {
	calls.emplace_back( "poll_was_init" );
	return true;
}

void poll_lock()
{
	calls.emplace_back( "poll_lock" );
}

void poll_timeout(poll_timeout_t callback, int timeout_ms, void* user_data)
{
	callback( user_data );
	calls.emplace_back( "poll_timeout" );
}

void poll_unlock()
{
	calls.emplace_back( "poll_unlock" );
}

void poll_interrupt()
{
	calls.emplace_back( "poll_interrupt" );
}

void poll_deinit()
{
	calls.emplace_back( "poll_deinit" );
}

internal_context_t* internal_init(internal_callbacks_t*)
{
	calls.emplace_back( "internal_init" );
	return nullptr;
}

void internal_poll_io()
{
	calls.emplace_back( "internal_poll_io" );
}

bool internal_supports_webRTC(internal_context_t*)
{
	calls.emplace_back( "internal_supports_webRTC" );
	return true;
}

void internal_close_socket(internal_socket_t*)
{
	calls.emplace_back( "internal_close_socket" );
}

int internal_create_channel(internal_socket_t* socket, const char* name)
{
	calls.push_back( std::string( "internal_create_channel ::" ) + name );
	return 1;
}

int internal_create_offer(internal_socket_t* socket)
{
	calls.emplace_back( "internal_create_offer" );
	return 1;
}

internal_socket_t* internal_create_webrtc(internal_context_t*)
{
	calls.emplace_back( "internal_create_webrtc" );
	return nullptr;
}

int internal_add_ice_candidate(internal_socket_t*, const char* candidate)
{
	calls.push_back( std::string( "internal_add_ice_candidate :: " ) + candidate );
	return 1;
}

int internal_set_answer(internal_socket_t* socket, const char* offer)
{
	calls.push_back( std::string( "internal_set_answer" ) + offer );
	return 1;
}

void internal_set_data(internal_socket_t*, void* user_data)
{
	calls.emplace_back( "internal_set_data" );
}

int internal_set_offer(internal_socket_t* socket, const char* offer)
{
	calls.push_back( std::string( "internal_set_offer :: " ) + offer );
	return 1;
}

void internal_set_stun_servers(internal_context_t*, const char** servers, int count)
{
	calls.emplace_back( "internal_set_stun_servers :: " + std::to_string( count ));
}

int internal_write_socket(internal_socket_t*, const void*, int buf_size)
{
	calls.push_back( std::string( "internal_write_socket :: " ) + std::to_string( buf_size ));
	return buf_size;
}

void internal_alias_resolved_to(const std::string& alias, PeerId peer)
{
	calls.push_back( std::string( "internal_alias_resolved_to :: " ) + alias + " :: " + std::to_string( peer ));
}

void internal_alias_remove_connection(Connection*)
{
	calls.emplace_back( "internal_alias_remove_connection" );
}

namespace humblenet {
	void register_protocol(internal_context_t*)
	{
		calls.emplace_back( "humblenet::register_protocol" );
	}

	static ha_bool processMessage(const humblenet::HumblePeer::Message* msg, void* user_data)
	{
		auto item = reinterpret_cast<Message*>(user_data);

		item->message = msg;

		return 1;
	}

	ha_bool sendP2PMessage(P2PSignalConnection* conn, const uint8_t* buff, size_t length)
	{
		messages.emplace_back();

		Message& item = messages.back();

		item.buffer.resize( length );
		memcpy( item.buffer.data(), buff, length );

		parseMessage( item.buffer, processMessage, &item );

		return true;
	}
}
// endregion

namespace HumblePeer = humblenet::HumblePeer;

class ConnectedPeerFixture {
protected:
	humblenet::P2PSignalConnection conn;
public:
	ConnectedPeerFixture()
	{
		// Setup the hello
		runCommand( conn, [&](humblenet::P2PSignalConnection& c) {
			humbleNetState = HumbleNetState();

			humblenet::sendHelloClient( &conn, 1, "reconnectToken", {{"stun.server"},
																	 {"turn.server", "username", "pass"}} );
		} );
	}

	~ConnectedPeerFixture() = default;

	template<typename Function>
	bool runCommand(humblenet::P2PSignalConnection& c, Function block)
	{
		messages.clear();
		block( c );
		c.processMsg( messages[0].message );
		messages.clear();

		HumbleNet_Event e = {};
		while (humblenet_event_poll( &e )) {}

		calls.clear();

		return true;
	}
};

SCENARIO( "client processMsg : HelloClient", "[hello]" )
{
	humblenet::P2PSignalConnection conn;

	GIVEN( "a hello client response" ) {
		messages.clear();

		humblenet::sendHelloClient( &conn, 1, "reconnectToken", {{"stun.server"},
																 {"turn.server", "username", "pass"}} );

		humbleNetState = HumbleNetState();
		calls.clear();

		HumbleNet_Event e = {};
		while (humblenet_event_poll( &e )) {}

		conn.processMsg( messages[0].message );

		THEN( "it sets properties on the humbleNetState" ) {
			CHECK_NOFAIL( humbleNetState.reconnectToken == "reconnectToken" );

			CHECK( humbleNetState.myPeerId == 1 );

			REQUIRE( humbleNetState.iceServers.size() == 2 );

			CHECK( humbleNetState.iceServers[0].type == HumblePeer::ICEServerType::STUNServer );
			CHECK( humbleNetState.iceServers[0].server == "stun.server" );

			CHECK( humbleNetState.iceServers[1].type == HumblePeer::ICEServerType::TURNServer );
			CHECK( humbleNetState.iceServers[1].server == "turn.server" );
			CHECK( humbleNetState.iceServers[1].username == "username" );
			CHECK( humbleNetState.iceServers[1].password == "pass" );
		}

		THEN( "it sets the ice servers" ) {
			CHECK_THAT( calls, Catch::VectorContains<std::string>( "internal_set_stun_servers :: 1" ));
		}

		THEN( "it generates a P2P connected event and an assign peer event" ) {
			REQUIRE( humblenet_event_poll( &e ));

			CHECK( e.type == HUMBLENET_EVENT_P2P_CONNECTED );

			CHECK( e.common.request_id == 0 );

			REQUIRE( humblenet_event_poll( &e ));

			CHECK( e.type == HUMBLENET_EVENT_P2P_ASSIGN_PEER );

			CHECK( e.peer.peer_id == 1 );
		}
	}
}

SCENARIO_METHOD( ConnectedPeerFixture, "client processMsg : AliasResolved", "[alias]" )
{
	GIVEN( "A resolved with no peer" ) {
		humblenet::sendAliasResolved( &conn, "server.host", 0, 0xdeadbeef );

		conn.processMsg( messages[0].message );

		THEN( "it calls the internal_alias_resolved_to method" ) {
			CHECK_THAT( calls, Catch::VectorContains<std::string>( "internal_alias_resolved_to :: server.host :: 0" ));
		}

		THEN( "it creates an alias not found event" ) {
			HumbleNet_Event e = {};

			REQUIRE( humblenet_event_poll( &e ));

			CHECK( e.type == HUMBLENET_EVENT_ALIAS_NOT_FOUND );
			CHECK( e.common.request_id == 0xdeadbeef );
			CHECK( std::string( e.error.error ) == "Failed to resolve alias" );
		}
	}

	GIVEN( "A resolved with a peer" ) {
		humblenet::sendAliasResolved( &conn, "server.host", 5, 0xdeadbeef );

		conn.processMsg( messages[0].message );

		THEN( "it calls the internal_alias_resolved_to method" ) {
			CHECK_THAT( calls, Catch::VectorContains<std::string>( "internal_alias_resolved_to :: server.host :: 5" ));
		}

		THEN( "it creates an alias resolved event" ) {
			HumbleNet_Event e = {};

			REQUIRE( humblenet_event_poll( &e ));

			CHECK( e.type == HUMBLENET_EVENT_ALIAS_RESOLVED );
			CHECK( e.common.request_id == 0xdeadbeef );
			CHECK( e.peer.peer_id == 5 );
		}
	}
}

SCENARIO_METHOD( ConnectedPeerFixture, "client processMsg : AliasRegisterError", "[alias]" )
{
	GIVEN( "A register error event" ) {
		humblenet::sendError( &conn, 0xdeadbeef, "Alias already registered",
							  HumblePeer::MessageType::AliasRegisterError );

		conn.processMsg( messages[0].message );

		THEN( "it creates an alias register error event" ) {
			HumbleNet_Event e = {};

			REQUIRE( humblenet_event_poll( &e ));

			CHECK( e.type == HUMBLENET_EVENT_ALIAS_REGISTER_ERROR );
			CHECK( e.common.request_id == 0xdeadbeef );
			CHECK( std::string( e.error.error ) == "Failed to register alias" );
		}
	}
}

SCENARIO_METHOD( ConnectedPeerFixture, "client processMsg : AliasRegisterSuccess", "[alias]" )
{
	GIVEN( "A register success event" ) {
		humblenet::sendSuccess( &conn, 0xdeadbeef, HumblePeer::MessageType::AliasRegisterSuccess );

		conn.processMsg( messages[0].message );

		THEN( "it creates an alias register success event" ) {
			HumbleNet_Event e = {};

			REQUIRE( humblenet_event_poll( &e ));

			CHECK( e.type == HUMBLENET_EVENT_ALIAS_REGISTER_SUCCESS );
			CHECK( e.common.request_id == 0xdeadbeef );
		}
	}
}

SCENARIO_METHOD( ConnectedPeerFixture, "client processMsg : LobbyDidCreate", "[lobby]" )
{
	humblenet::Lobby lobby( 0xa0, humbleNetState.myPeerId, HUMBLENET_LOBBY_TYPE_PUBLIC, 4 );
	lobby.attributes.insert( {"name", "My lobby"} );

	GIVEN( "A lobby did create" ) {
		humblenet::sendLobbyDidCreate( &conn, 0xdeadbeef, lobby );

		conn.processMsg( messages[0].message );

		THEN( "it creates a lobby create success event" ) {
			HumbleNet_Event e = {};

			REQUIRE( humblenet_event_poll( &e ));

			CHECK( e.type == HUMBLENET_EVENT_LOBBY_CREATE_SUCCESS );
			CHECK( e.common.request_id == 0xdeadbeef );
			CHECK( e.lobby.lobby_id == lobby.lobbyId );
			CHECK( e.lobby.peer_id == humbleNetState.myPeerId );
		}

		THEN( "It registers the lobby in the internal state" ) {
			REQUIRE( humbleNetState.lobbies.size() == 1 );

			const auto it = humbleNetState.lobbies.find( lobby.lobbyId );

			REQUIRE( it != humbleNetState.lobbies.end());

			CHECK( it->second.lobbyId == lobby.lobbyId );
			CHECK( it->second.type == lobby.type );
			CHECK( it->second.ownerPeerId == humbleNetState.myPeerId );
			CHECK( it->second.maxMembers == lobby.maxMembers );
			CHECK( it->second.attributes == lobby.attributes );
		}
	}
}

SCENARIO_METHOD( ConnectedPeerFixture, "client processMsg : LobbyCreateError", "[lobby]" )
{
	GIVEN( "A create error event" ) {
		humblenet::sendError( &conn, 0xdeadbeef, "Lobby failed to create", HumblePeer::MessageType::LobbyCreateError );

		conn.processMsg( messages[0].message );

		THEN( "it creates a lobby create error event" ) {
			HumbleNet_Event e = {};

			REQUIRE( humblenet_event_poll( &e ));

			CHECK( e.type == HUMBLENET_EVENT_LOBBY_CREATE_ERROR );
			CHECK( e.common.request_id == 0xdeadbeef );
			CHECK( std::string( e.error.error ) == "Failed to create lobby" );
		}
	}
}

SCENARIO_METHOD( ConnectedPeerFixture, "client processMsg : LobbyDidJoin", "[lobby]" )
{
	humblenet::Lobby lobby( 0xa0, 0xd00d1, HUMBLENET_LOBBY_TYPE_PUBLIC, 4 );
	lobby.attributes.insert( {"name", "My lobby"} );

	GIVEN( "A lobby did join (self)" ) {

		humblenet::sendLobbyDidJoinSelf( &conn, 0xdeadbeef, lobby, humbleNetState.myPeerId );

		conn.processMsg( messages[0].message );

		THEN( "it creates a lobby join event" ) {
			HumbleNet_Event e = {};

			REQUIRE( humblenet_event_poll( &e ));

			CHECK( e.type == HUMBLENET_EVENT_LOBBY_JOIN );
			CHECK( e.common.request_id == 0xdeadbeef );
			CHECK( e.lobby.lobby_id == lobby.lobbyId );
			CHECK( e.lobby.peer_id == humbleNetState.myPeerId );
		}

		THEN( "It registers the lobby in the internal state" ) {
			REQUIRE( humbleNetState.lobbies.size() == 1 );

			const auto it = humbleNetState.lobbies.find( lobby.lobbyId );

			REQUIRE( it != humbleNetState.lobbies.end());

			CHECK( it->second.lobbyId == lobby.lobbyId );
			CHECK( it->second.type == lobby.type );
			CHECK( it->second.ownerPeerId == 0xd00d1 );
			CHECK( it->second.maxMembers == lobby.maxMembers );
			CHECK( it->second.attributes == lobby.attributes );
		}
	}

	GIVEN( "A lobby did join (another member)" ) {
		runCommand( conn, [&](humblenet::P2PSignalConnection& c) {
			humblenet::sendLobbyDidCreate( &c, 0xdeadbeef, lobby );
		} );
		humblenet::sendLobbyDidJoin( &conn, lobby.lobbyId, 0xd00d, {{"skill", "2"}} );

		conn.processMsg( messages[0].message );

		THEN( "it creates a lobby member join event" ) {
			HumbleNet_Event e = {};

			REQUIRE( humblenet_event_poll( &e ));

			CHECK( e.type == HUMBLENET_EVENT_LOBBY_MEMBER_JOIN );
			CHECK( e.common.request_id == 0 );
			CHECK( e.lobby.lobby_id == lobby.lobbyId );
			CHECK( e.lobby.peer_id == 0xd00d );
		}

		THEN( "It adds the member to the lobby in the internal state" ) {
			const auto it = humbleNetState.lobbies.find( lobby.lobbyId );

			REQUIRE( it != humbleNetState.lobbies.end());

			auto mit = it->second.members.find( 0xd00d );

			REQUIRE( mit != it->second.members.end());

			CHECK( mit->second.attributes == humblenet::AttributeMap( {{"skill", "2"}} ));
		}
	}
}

SCENARIO_METHOD( ConnectedPeerFixture, "client processMsg : LobbyJoinError", "[lobby]" )
{
	GIVEN( "A join error event" ) {
		humblenet::sendError( &conn, 0xdeadbeef, "Lobby failed to join", HumblePeer::MessageType::LobbyJoinError );

		conn.processMsg( messages[0].message );

		THEN( "it creates a lobby join error event" ) {
			HumbleNet_Event e = {};

			REQUIRE( humblenet_event_poll( &e ));

			CHECK( e.type == HUMBLENET_EVENT_LOBBY_JOIN_ERROR );
			CHECK( e.common.request_id == 0xdeadbeef );
			CHECK( std::string( e.error.error ) == "Failed to join lobby" );
		}
	}
}

SCENARIO_METHOD( ConnectedPeerFixture, "client processMsg : LobbyDidLeave", "[lobby]" )
{
	humblenet::Lobby lobby( 0xa0, 0xd00d1, HUMBLENET_LOBBY_TYPE_PUBLIC, 4 );
	lobby.attributes.insert( {"name", "My lobby"} );
	runCommand( conn, [&](humblenet::P2PSignalConnection& c) {
		humblenet::sendLobbyDidCreate( &c, 0, lobby );
	} );
	REQUIRE( humbleNetState.lobbies.size() == 1 );

	GIVEN( "A lobby did leave (self)" ) {
		humblenet::sendLobbyDidLeave( &conn, 0xdeadbeef, lobby.lobbyId, humbleNetState.myPeerId );

		conn.processMsg( messages[0].message );

		THEN( "it creates a lobby leave event" ) {
			HumbleNet_Event e = {};

			REQUIRE( humblenet_event_poll( &e ));

			CHECK( e.type == HUMBLENET_EVENT_LOBBY_LEAVE );
			CHECK( e.common.request_id == 0xdeadbeef );
			CHECK( e.lobby.lobby_id == lobby.lobbyId );
			CHECK( e.lobby.peer_id == humbleNetState.myPeerId );
		}

		THEN( "It removes the lobby from the internal state" ) {
			REQUIRE( humbleNetState.lobbies.size() == 0 );

			const auto it = humbleNetState.lobbies.find( lobby.lobbyId );

			CHECK( it == humbleNetState.lobbies.end());
		}
	}

	GIVEN( "A lobby did leave (another member)" ) {
		humblenet::sendLobbyDidLeave( &conn, 0, lobby.lobbyId, 0xd00d );

		conn.processMsg( messages[0].message );

		THEN( "it creates a lobby member leave event" ) {
			HumbleNet_Event e = {};

			REQUIRE( humblenet_event_poll( &e ));

			CHECK( e.type == HUMBLENET_EVENT_LOBBY_MEMBER_LEAVE );
			CHECK( e.common.request_id == 0 );
			CHECK( e.lobby.lobby_id == lobby.lobbyId );
			CHECK( e.lobby.peer_id == 0xd00d );
		}

		THEN( "It removes the member from the lobby in the internal state" ) {
			const auto it = humbleNetState.lobbies.find( lobby.lobbyId );

			REQUIRE( it != humbleNetState.lobbies.end());

			auto mit = it->second.members.find( 0xd00d );

			CHECK( mit == it->second.members.end());
		}
	}
}

SCENARIO_METHOD( ConnectedPeerFixture, "client processMsg : LobbyLeaveError", "[lobby]" )
{
	GIVEN( "A leave error event" ) {
		humblenet::sendError( &conn, 0xdeadbeef, "Lobby failed to leave", HumblePeer::MessageType::LobbyLeaveError );

		conn.processMsg( messages[0].message );

		THEN( "it creates a lobby leave error event" ) {
			HumbleNet_Event e = {};

			REQUIRE( humblenet_event_poll( &e ));

			CHECK( e.type == HUMBLENET_EVENT_LOBBY_LEAVE_ERROR );
			CHECK( e.common.request_id == 0xdeadbeef );
			CHECK( std::string( e.error.error ) == "Failed to leave lobby" );
		}
	}
}

SCENARIO_METHOD( ConnectedPeerFixture, "client processMsg : LobbyDidUpdate", "[lobby]" )
{
	humblenet::Lobby lobby( 0xa0, 0xd00d1, HUMBLENET_LOBBY_TYPE_PUBLIC, 4 );
	lobby.attributes.insert( {"name", "My lobby"} );
	runCommand( conn, [&](humblenet::P2PSignalConnection& c) {
		humblenet::sendLobbyDidCreate( &c, 0, lobby );
	} );
	REQUIRE( humbleNetState.lobbies.size() == 1 );

	GIVEN( "A lobby attribute merge update" ) {
		humblenet::sendLobbyDidUpdate( &conn, 0xdeadbeef, lobby.lobbyId,
									   HUMBLENET_LOBBY_TYPE_UNKNOWN, 0,
									   HumblePeer::AttributeMode::Merge, {{"skill", "1"}} );

		conn.processMsg( messages[0].message );

		THEN( "it creates a lobby update event" ) {
			HumbleNet_Event e = {};

			REQUIRE( humblenet_event_poll( &e ));

			CHECK( e.type == HUMBLENET_EVENT_LOBBY_UPDATE );
			CHECK( e.common.request_id == 0xdeadbeef );
			CHECK( e.lobby.lobby_id == lobby.lobbyId );
			CHECK( e.lobby.peer_id == 0 );
		}

		THEN( "It updates the lobby attributes in the internal state" ) {
			REQUIRE( humbleNetState.lobbies.size() == 1 );

			const auto it = humbleNetState.lobbies.find( lobby.lobbyId );

			REQUIRE( it != humbleNetState.lobbies.end());

			CHECK( it->second.lobbyId == lobby.lobbyId );
			CHECK( it->second.attributes == humblenet::AttributeMap( {{"name",  "My lobby"},
																	  {"skill", "1"}} ));
		}

		THEN( "it does not update the type" ) {
			const auto it = humbleNetState.lobbies.find( lobby.lobbyId );

			CHECK( it->second.type == HUMBLENET_LOBBY_TYPE_PUBLIC );
		}

		THEN( "it does not update the max members" ) {
			const auto it = humbleNetState.lobbies.find( lobby.lobbyId );

			CHECK( it->second.maxMembers == 4 );
		}
	}

	GIVEN( "A lobby attribute replace update" ) {
		humblenet::sendLobbyDidUpdate( &conn, 0xdeadbeef, lobby.lobbyId,
									   HUMBLENET_LOBBY_TYPE_UNKNOWN, 0,
									   HumblePeer::AttributeMode::Replace, {{"skill", "1"}} );

		conn.processMsg( messages[0].message );

		THEN( "it creates a lobby update event" ) {
			HumbleNet_Event e = {};

			REQUIRE( humblenet_event_poll( &e ));

			CHECK( e.type == HUMBLENET_EVENT_LOBBY_UPDATE );
			CHECK( e.common.request_id == 0xdeadbeef );
			CHECK( e.lobby.lobby_id == lobby.lobbyId );
			CHECK( e.lobby.peer_id == 0 );
		}

		THEN( "It updates the lobby attributes in the internal state" ) {
			REQUIRE( humbleNetState.lobbies.size() == 1 );

			const auto it = humbleNetState.lobbies.find( lobby.lobbyId );

			REQUIRE( it != humbleNetState.lobbies.end());

			CHECK( it->second.lobbyId == lobby.lobbyId );
			CHECK( it->second.attributes == humblenet::AttributeMap( {{"skill", "1"}} ));
		}

		THEN( "it does not update the type" ) {
			const auto it = humbleNetState.lobbies.find( lobby.lobbyId );

			CHECK( it->second.type == HUMBLENET_LOBBY_TYPE_PUBLIC );
		}

		THEN( "it does not update the max members" ) {
			const auto it = humbleNetState.lobbies.find( lobby.lobbyId );

			CHECK( it->second.maxMembers == 4 );
		}
	}

	GIVEN( "A lobby type update" ) {
		humblenet::sendLobbyDidUpdate( &conn, 0xdeadbeef, lobby.lobbyId,
									   HUMBLENET_LOBBY_TYPE_PRIVATE, 0,
									   HumblePeer::AttributeMode::Merge, {} );

		conn.processMsg( messages[0].message );

		THEN( "it creates a lobby update event" ) {
			HumbleNet_Event e = {};

			REQUIRE( humblenet_event_poll( &e ));

			CHECK( e.type == HUMBLENET_EVENT_LOBBY_UPDATE );
			CHECK( e.common.request_id == 0xdeadbeef );
			CHECK( e.lobby.lobby_id == lobby.lobbyId );
			CHECK( e.lobby.peer_id == 0 );
		}

		THEN( "it does not update the lobby attributes" ) {
			REQUIRE( humbleNetState.lobbies.size() == 1 );

			const auto it = humbleNetState.lobbies.find( lobby.lobbyId );

			REQUIRE( it != humbleNetState.lobbies.end());

			CHECK( it->second.lobbyId == lobby.lobbyId );
			CHECK( it->second.attributes == humblenet::AttributeMap( {{"name", "My lobby"}} ));
		}

		THEN( "it updates the type" ) {
			const auto it = humbleNetState.lobbies.find( lobby.lobbyId );

			CHECK( it->second.type == HUMBLENET_LOBBY_TYPE_PRIVATE );
		}

		THEN( "it does not update the max members" ) {
			const auto it = humbleNetState.lobbies.find( lobby.lobbyId );

			CHECK( it->second.maxMembers == 4 );
		}
	}

	GIVEN( "A lobby max members update" ) {
		humblenet::sendLobbyDidUpdate( &conn, 0xdeadbeef, lobby.lobbyId,
									   HUMBLENET_LOBBY_TYPE_UNKNOWN, 8,
									   HumblePeer::AttributeMode::Merge, {} );

		conn.processMsg( messages[0].message );

		THEN( "it creates a lobby update event" ) {
			HumbleNet_Event e = {};

			REQUIRE( humblenet_event_poll( &e ));

			CHECK( e.type == HUMBLENET_EVENT_LOBBY_UPDATE );
			CHECK( e.common.request_id == 0xdeadbeef );
			CHECK( e.lobby.lobby_id == lobby.lobbyId );
			CHECK( e.lobby.peer_id == 0 );
		}

		THEN( "it does not update the lobby attributes" ) {
			REQUIRE( humbleNetState.lobbies.size() == 1 );

			const auto it = humbleNetState.lobbies.find( lobby.lobbyId );

			REQUIRE( it != humbleNetState.lobbies.end());

			CHECK( it->second.lobbyId == lobby.lobbyId );
			CHECK( it->second.attributes == humblenet::AttributeMap( {{"name", "My lobby"}} ));
		}

		THEN( "it does not update the type" ) {
			const auto it = humbleNetState.lobbies.find( lobby.lobbyId );

			CHECK( it->second.type == HUMBLENET_LOBBY_TYPE_PUBLIC );
		}

		THEN( "it updates the max members" ) {
			const auto it = humbleNetState.lobbies.find( lobby.lobbyId );

			CHECK( it->second.maxMembers == 8 );
		}
	}
}

SCENARIO_METHOD( ConnectedPeerFixture, "client processMsg : LobbyUpdateError", "[lobby]" )
{
	GIVEN( "A lobby update error event" ) {
		humblenet::sendError( &conn, 0xdeadbeef, "Lobby failed to update", HumblePeer::MessageType::LobbyUpdateError );

		conn.processMsg( messages[0].message );

		THEN( "it creates a lobby update error event" ) {
			HumbleNet_Event e = {};

			REQUIRE( humblenet_event_poll( &e ));

			CHECK( e.type == HUMBLENET_EVENT_LOBBY_UPDATE_ERROR );
			CHECK( e.common.request_id == 0xdeadbeef );
			CHECK( std::string( e.error.error ) == "Failed to update lobby" );
		}
	}
}

SCENARIO_METHOD( ConnectedPeerFixture, "client processMsg : LobbyMemberDidUpdate", "[lobby]" )
{
	humblenet::Lobby lobby( 0xa0, 0xd00d1, HUMBLENET_LOBBY_TYPE_PUBLIC, 4 );
	lobby.attributes.insert( {"name", "My lobby"} );

	runCommand( conn, [&](humblenet::P2PSignalConnection& c) {
		humblenet::sendLobbyDidJoinSelf( &c, 0x12345, lobby, humbleNetState.myPeerId, {{"skill", "2"}} );
	} );

	REQUIRE( humbleNetState.lobbies.size() == 1 );

	GIVEN( "A lobby member merge update" ) {
		humblenet::sendLobbyMemberDidUpdate( &conn, 0xdeadbeef, lobby.lobbyId, humbleNetState.myPeerId,
											 HumblePeer::AttributeMode::Merge, {{"ready", "1"}} );

		conn.processMsg( messages[0].message );

		THEN( "it creates a lobby member update event" ) {
			HumbleNet_Event e = {};

			REQUIRE( humblenet_event_poll( &e ));

			CHECK( e.type == HUMBLENET_EVENT_LOBBY_MEMBER_UPDATE );
			CHECK( e.common.request_id == 0xdeadbeef );
			CHECK( e.lobby.lobby_id == lobby.lobbyId );
			CHECK( e.lobby.peer_id == humbleNetState.myPeerId );
		}

		THEN( "It updates the lobby member in the internal state" ) {
			REQUIRE( humbleNetState.lobbies.size() == 1 );

			const auto it = humbleNetState.lobbies.find( lobby.lobbyId );

			REQUIRE( it != humbleNetState.lobbies.end());

			const auto mit = it->second.members.find( humbleNetState.myPeerId );

			REQUIRE( mit != it->second.members.end());

			CHECK( mit->second.attributes == humblenet::AttributeMap( {{"skill", "2"},
																	   {"ready", "1"}} ));
		}
	}

	GIVEN( "A lobby member replace update" ) {
		humblenet::sendLobbyMemberDidUpdate( &conn, 0xdeadbeef, lobby.lobbyId, humbleNetState.myPeerId,
											 HumblePeer::AttributeMode::Replace, {{"ready", "1"}} );

		conn.processMsg( messages[0].message );

		THEN( "it creates a lobby member update event" ) {
			HumbleNet_Event e = {};

			REQUIRE( humblenet_event_poll( &e ));

			CHECK( e.type == HUMBLENET_EVENT_LOBBY_MEMBER_UPDATE );
			CHECK( e.common.request_id == 0xdeadbeef );
			CHECK( e.lobby.lobby_id == lobby.lobbyId );
			CHECK( e.lobby.peer_id == humbleNetState.myPeerId );
		}

		THEN( "It updates the lobby in the internal state" ) {
			REQUIRE( humbleNetState.lobbies.size() == 1 );

			const auto it = humbleNetState.lobbies.find( lobby.lobbyId );

			REQUIRE( it != humbleNetState.lobbies.end());

			const auto mit = it->second.members.find( humbleNetState.myPeerId );

			REQUIRE( mit != it->second.members.end());

			CHECK( mit->second.attributes == humblenet::AttributeMap( {{"ready", "1"}} ));
		}
	}
}

SCENARIO_METHOD( ConnectedPeerFixture, "client processMsg : LobbyMemberUpdateError", "[lobby]" )
{
	GIVEN( "A member update error event" ) {
		humblenet::sendError( &conn, 0xdeadbeef, "Lobby failed to update member", HumblePeer::MessageType::LobbyMemberUpdateError );

		conn.processMsg( messages[0].message );

		THEN( "it creates a lobby member update error event" ) {
			HumbleNet_Event e = {};

			REQUIRE( humblenet_event_poll( &e ));

			CHECK( e.type == HUMBLENET_EVENT_LOBBY_MEMBER_UPDATE_ERROR );
			CHECK( e.common.request_id == 0xdeadbeef );
			CHECK( std::string( e.error.error ) == "Failed to update lobby member" );
		}
	}
}
