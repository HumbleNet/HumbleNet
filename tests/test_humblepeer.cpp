#define CATCH_CONFIG_MAIN

#include <catch2/catch.hpp>

#include "humblepeer.h"


namespace humblenet {
	struct P2PSignalConnection {
		std::vector<uint8_t> buffer;

		const HumblePeer::Message* message = nullptr;
	};

	static ha_bool processMessage(const humblenet::HumblePeer::Message* msg, void* user_data)
	{
		auto conn = reinterpret_cast<P2PSignalConnection*>(user_data);

		conn->message = msg;

		return 1;
	}

	ha_bool sendP2PMessage(P2PSignalConnection* conn, const uint8_t* buff, size_t length)
	{
		conn->buffer.resize( length );
		memcpy( conn->buffer.data(), buff, length );

		parseMessage( conn->buffer, processMessage, conn );

		return true;
	}
}

namespace HumblePeer = humblenet::HumblePeer;

humblenet::P2PSignalConnection conn;

// region Initial connection handling

SCENARIO( "sendHelloServer", "[client][hello]" )
{
	std::map<std::string, std::string> attributes = {{"key", "value"}};

	GIVEN( "No Auth nor reconnect token" ) {
		humblenet::sendHelloServer( &conn, 0x3f, "gameToken", "gameSecret", "", "", attributes );

		THEN( "it creates a HelloServer message" ) {
			REQUIRE( conn.message->message_type() == HumblePeer::MessageType::HelloServer );
		}

		auto msg = reinterpret_cast<const HumblePeer::HelloServer*>(conn.message->message());

		CHECK( msg->version() == 0 );
		CHECK( msg->flags() == 0x3f );

		THEN( "it has no authToken nor reconnectToken" ) {
			CHECK_FALSE( msg->authToken());
			CHECK_FALSE( msg->reconnectToken());
		}

		THEN( "it has the game token and game signature" ) {
			CHECK( msg->gameToken()->str() == "gameToken" );
			CHECK( msg->gameSignature() != nullptr );
		}

		THEN( "has the correct attributes" ) {
			auto attribs = msg->attributes();
			CHECK( attribs->size() == 2 );

			auto v = attribs->LookupByKey( "key" );
			CHECK( v->value()->str() == "value" );

			CHECK( attribs->LookupByKey( "timestamp" ));
		}
	}

	GIVEN( "with auth and reconnect token" ) {
		humblenet::sendHelloServer( &conn, 0x3f, "gameToken", "gameSecret", "authToken", "reconnectToken", attributes );

		THEN( "it creates a HelloServer message" ) {
			REQUIRE( conn.message->message_type() == HumblePeer::MessageType::HelloServer );
		}

		auto msg = reinterpret_cast<const HumblePeer::HelloServer*>(conn.message->message());

		CHECK( msg->version() == 0 );
		CHECK( msg->flags() == 0x3f );

		THEN( "it has no authToken nor reconnectToken" ) {
			CHECK( msg->authToken()->str() == "authToken" );
			CHECK( msg->reconnectToken()->str() == "reconnectToken" );
		}

		THEN( "it has the game token and game signature" ) {
			CHECK( msg->gameToken()->str() == "gameToken" );
			CHECK( msg->gameSignature() != nullptr );
		}

		THEN( "has the correct attributes" ) {
			auto attribs = msg->attributes();
			CHECK( attribs->size() == 2 );

			auto v = attribs->LookupByKey( "key" );
			CHECK( v->value()->str() == "value" );

			CHECK( attribs->LookupByKey( "timestamp" ));
		}
	}
}

SCENARIO( "sendHelloClient", "[server][hello]" )
{
	std::map<std::string, std::string> attributes = {{"key", "value"}};

	GIVEN( "no reconnect token nor ice servers" ) {
		std::vector<humblenet::ICEServer> iceServers;

		humblenet::sendHelloClient( &conn, 0xdeadbeef, "", iceServers );

		THEN( "it creates a HelloClient message" ) {
			REQUIRE( conn.message->message_type() == HumblePeer::MessageType::HelloClient );
		}

		auto msg = reinterpret_cast<const HumblePeer::HelloClient*>(conn.message->message());

		CHECK( msg->peerId() == 0xdeadbeef );

		THEN( "it has no reconnect token" ) {
			CHECK_FALSE( msg->reconnectToken());
		}

		THEN( "it has no ice servers" ) {
			CHECK_FALSE( msg->iceServers());
		}
	}

	GIVEN( "a reconnect token and ice servers" ) {
		std::vector<humblenet::ICEServer> iceServers = {{"stun.server"},
														{"turn.server", "username", "pass"}};

		humblenet::sendHelloClient( &conn, 0xdeadbeef, "reconnectToken", iceServers );

		THEN( "it creates a HelloClient message" ) {
			REQUIRE( conn.message->message_type() == HumblePeer::MessageType::HelloClient );
		}

		auto msg = reinterpret_cast<const HumblePeer::HelloClient*>(conn.message->message());

		CHECK( msg->peerId() == 0xdeadbeef );

		THEN( "it has a reconnect token" ) {
			CHECK( msg->reconnectToken());
		}

		THEN( "it has ice servers" ) {
			auto srvs = msg->iceServers();
			CHECK( srvs->size() == 2 );

			auto v = srvs->Get( 0 );
			CHECK( v->type() == HumblePeer::ICEServerType::STUNServer );
			CHECK( v->server()->str() == "stun.server" );
			CHECK_FALSE( v->username());
			CHECK_FALSE( v->password());

			v = srvs->Get( 1 );
			CHECK( v->type() == HumblePeer::ICEServerType::TURNServer );
			CHECK( v->server()->str() == "turn.server" );
			CHECK( v->username()->str() == "username" );
			CHECK( v->password()->str() == "pass" );
		}
	}
}

// endregion

// region Generic error/success handling

SCENARIO( "sendSuccess", "[server]" )
{

}

SCENARIO( "sendError", "[server]" )
{

}

// endregion

// region Peer connection handling

SCENARIO( "sendNoSuchPeer", "[server][peer]" )
{

}

SCENARIO( "sendPeerRefused", "[peer]" )
{

}

SCENARIO( "sendP2POffer", "[peer]" )
{

}

SCENARIO( "sendP2PResponse", "[peer]" )
{

}

SCENARIO( "sendICECandidate", "[peer]" )
{

}

SCENARIO( "sendP2PRelayData", "[peer]" )
{

}

// endregion

// region Alias handling

SCENARIO( "sendAliasRegister", "[client][alias]" )
{
	GIVEN( "a request to register an alias" ) {
		auto reqId = humblenet::sendAliasRegister( &conn, "server.host" );

		THEN("it returns a non zero request ID") {
			REQUIRE( reqId != 0 );
		}

		THEN( "it creates a AliasRegister message" ) {
			REQUIRE( conn.message->message_type() == HumblePeer::MessageType::AliasRegister );
		}

		auto msg = reinterpret_cast<const HumblePeer::AliasRegister*>(conn.message->message());

		THEN( "it has the correct properties" ) {
			CHECK( msg->alias()->str() == "server.host" );

			CHECK( conn.message->requestId() == reqId );
		}
	}
}

SCENARIO( "sendAliasUnregister", "[client][alias]" )
{
	GIVEN( "a request to unregister an alias" ) {
		auto reqId = humblenet::sendAliasUnregister( &conn, "server.host" );

		THEN("it returns a non zero request ID") {
			REQUIRE( reqId != 0 );
		}

		THEN( "it creates a AliasUnregister message" ) {
			REQUIRE( conn.message->message_type() == HumblePeer::MessageType::AliasUnregister );
		}

		auto msg = reinterpret_cast<const HumblePeer::AliasUnregister*>(conn.message->message());

		THEN( "it has the correct properties" ) {
			CHECK( msg->alias()->str() == "server.host" );

			CHECK( conn.message->requestId() == reqId );
		}
	}

	GIVEN( "a request to unregister every alias" ) {
		auto reqId = humblenet::sendAliasUnregister( &conn, "" );

		THEN("it returns a non zero request ID") {
			REQUIRE( reqId != 0 );
		}

		THEN( "it creates a AliasUnregister message" ) {
			REQUIRE( conn.message->message_type() == HumblePeer::MessageType::AliasUnregister );
		}

		auto msg = reinterpret_cast<const HumblePeer::AliasUnregister*>(conn.message->message());

		THEN( "it has the correct properties" ) {
			CHECK( msg->alias() == nullptr );

			CHECK( conn.message->requestId() == reqId );
		}
	}
}

SCENARIO( "sendAliasLookup", "[client][alias]" )
{
	GIVEN( "a request for an alias" ) {
		auto reqId = humblenet::sendAliasLookup( &conn, "server.host" );

		THEN("it returns a non zero request ID") {
			REQUIRE( reqId != 0 );
		}

		THEN( "it creates a AliasLookup message" ) {
			REQUIRE( conn.message->message_type() == HumblePeer::MessageType::AliasLookup );
		}

		auto msg = reinterpret_cast<const HumblePeer::AliasLookup*>(conn.message->message());

		THEN( "it has the correct properties" ) {
			CHECK( msg->alias()->str() == "server.host" );

			CHECK( conn.message->requestId() == reqId );
		}
	}
}

SCENARIO( "sendAliasResolved", "[server][alias]" )
{
	GIVEN( "a resolved peer" ) {
		humblenet::sendAliasResolved( &conn, "server.host", 0xdeadbeef, 0xbeefdead );

		THEN( "it creates a AliasResolved message" ) {
			REQUIRE( conn.message->message_type() == HumblePeer::MessageType::AliasResolved );
		}

		auto msg = reinterpret_cast<const HumblePeer::AliasResolved*>(conn.message->message());

		THEN( "it has the correct properties" ) {
			CHECK( msg->alias()->str() == "server.host" );

			CHECK( msg->peerId() == 0xdeadbeef );

			CHECK( conn.message->requestId() == 0xbeefdead );
		}
	}

	GIVEN( "a unresolved peer" ) {
		humblenet::sendAliasResolved( &conn, "server.host", 0, 0xbeefdead );

		THEN( "it creates a AliasResolved message" ) {
			REQUIRE( conn.message->message_type() == HumblePeer::MessageType::AliasResolved );
		}

		auto msg = reinterpret_cast<const HumblePeer::AliasResolved*>(conn.message->message());

		THEN( "it has the correct properties" ) {
			CHECK( msg->alias()->str() == "server.host" );

			CHECK( msg->peerId() == 0 );

			CHECK( conn.message->requestId() == 0xbeefdead );
		}
	}
}

// endregion

// region Lobby

SCENARIO( "sendLobbyCreate", "[client][lobby]" )
{
	GIVEN( "a request for private with unlimited members" ) {
		humblenet::sendLobbyCreate( &conn, HUMBLENET_LOBBY_TYPE_PRIVATE, 0 );

		THEN( "it creates a LobbyCreate message" ) {
			REQUIRE( conn.message->message_type() == HumblePeer::MessageType::LobbyCreate );
		}

		auto msg = reinterpret_cast<const HumblePeer::LobbyCreate*>(conn.message->message());

		THEN( "it has the correct properties" ) {
			CHECK( msg->lobbyType() == static_cast<uint8_t>(HUMBLENET_LOBBY_TYPE_PRIVATE));
			CHECK( msg->maxMembers() == 0 );

			CHECK( conn.message->requestId() != 0 );
		}
	}

	GIVEN( "a request for public with max members" ) {
		humblenet::sendLobbyCreate( &conn, HUMBLENET_LOBBY_TYPE_PUBLIC, 8 );

		THEN( "it creates a LobbyCreate message" ) {
			REQUIRE( conn.message->message_type() == HumblePeer::MessageType::LobbyCreate );
		}

		auto msg = reinterpret_cast<const HumblePeer::LobbyCreate*>(conn.message->message());

		THEN( "it has the correct properties" ) {
			CHECK( msg->lobbyType() == static_cast<uint8_t>(HUMBLENET_LOBBY_TYPE_PUBLIC));
			CHECK( msg->maxMembers() == 8 );

			CHECK( conn.message->requestId() != 0 );
		}
	}
}

SCENARIO( "sendLobbyDidCreate", "[server][lobby]" )
{
	humblenet::Lobby lobby( 0xbeefdead, 0xd00d, HUMBLENET_LOBBY_TYPE_PUBLIC, 4 );
	lobby.attributes.insert( {"key", "value"} );

	GIVEN( "a created lobby response" ) {
		humblenet::sendLobbyDidCreate( &conn, 0xdeadbeef, lobby );

		THEN( "it creates a LobbyDidCreate message" ) {
			REQUIRE( conn.message->message_type() == HumblePeer::MessageType::LobbyDidCreate );
		}

		auto msg = reinterpret_cast<const HumblePeer::LobbyDidCreate*>(conn.message->message());

		THEN( "it has the correct properties" ) {
			CHECK( msg->lobbyId() == 0xbeefdead );

			CHECK( conn.message->requestId() == 0xdeadbeef );

			REQUIRE( msg->lobby() != nullptr );

			CHECK( msg->lobby()->ownerPeer() == lobby.ownerPeerId );

			CHECK( msg->lobby()->lobbyType() == lobby.type );

			CHECK( msg->lobby()->maxMembers() == lobby.maxMembers );

			REQUIRE( msg->lobby()->attributeSet() != nullptr );

			auto attribSet = msg->lobby()->attributeSet();

			CHECK( attribSet->mode() == HumblePeer::AttributeMode::Replace );

			REQUIRE( attribSet->attributes() != nullptr );

			auto attribs = attribSet->attributes();

			CHECK( attribs->size() == 1 );

			auto v = attribs->LookupByKey( "key" );

			CHECK( v->value()->str() == "value" );
		}
	}
}

SCENARIO( "sendLobbyJoin", "[client][lobby]" )
{
	GIVEN( "a join lobby request" ) {
		humblenet::sendLobbyJoin( &conn, 0xbeefdead );

		THEN( "it creates a LobbyJoin message" ) {
			REQUIRE( conn.message->message_type() == HumblePeer::MessageType::LobbyJoin );
		}

		auto msg = reinterpret_cast<const HumblePeer::LobbyJoin*>(conn.message->message());

		THEN( "it has the correct properties" ) {
			CHECK( msg->lobbyId() == 0xbeefdead );

			CHECK( conn.message->requestId() != 0 );
		}
	}
}

SCENARIO( "sendLobbyDidJoinSelf", "[server][lobby]" )
{
	humblenet::Lobby lobby( 0xbeefdead, 0xd00d1, HUMBLENET_LOBBY_TYPE_PUBLIC, 4 );
	lobby.attributes.insert( {"key", "value"} );

	GIVEN( "a did join lobby response with lobby details" ) {
		humblenet::sendLobbyDidJoinSelf( &conn, 0xdeadbeef, lobby, 0xd00d );

		THEN( "it creates a LobbyDidJoin message" ) {
			REQUIRE( conn.message->message_type() == HumblePeer::MessageType::LobbyDidJoin );
		}

		auto msg = reinterpret_cast<const HumblePeer::LobbyDidJoin*>(conn.message->message());

		THEN( "it has the correct properties" ) {
			CHECK( msg->lobbyId() == lobby.lobbyId );

			CHECK( conn.message->requestId() == 0xdeadbeef );

			CHECK( msg->peerId() == 0xd00d );

			REQUIRE( msg->lobby() != nullptr );

			CHECK( msg->lobby()->ownerPeer() == lobby.ownerPeerId );

			CHECK( msg->lobby()->lobbyType() == lobby.type );

			CHECK( msg->lobby()->maxMembers() == lobby.maxMembers );

			REQUIRE( msg->lobby()->attributeSet() != nullptr );

			auto attribSet = msg->lobby()->attributeSet();

			CHECK( attribSet->mode() == HumblePeer::AttributeMode::Replace );

			REQUIRE( attribSet->attributes() != nullptr );

			auto attribs = attribSet->attributes();

			CHECK( attribs->size() == 1 );

			auto v = attribs->LookupByKey( "key" );

			CHECK( v->value()->str() == "value" );

			CHECK( msg->memberDetails() == nullptr );
		}
	}
}

SCENARIO( "sendLobbyDidJoin", "[server][lobby]" )
{
	GIVEN( "a did join lobby response with member attribute details" ) {
		humblenet::sendLobbyDidJoin( &conn, 0xbeefdead, 0xd00d, {{"skill", "1"}} );

		THEN( "it creates a LobbyDidJoin message" ) {
			REQUIRE( conn.message->message_type() == HumblePeer::MessageType::LobbyDidJoin );
		}

		auto msg = reinterpret_cast<const HumblePeer::LobbyDidJoin*>(conn.message->message());

		THEN( "it has the correct properties" ) {
			CHECK( msg->lobbyId() == 0xbeefdead );

			CHECK( conn.message->requestId() == 0 );

			CHECK( msg->peerId() == 0xd00d );

			CHECK( msg->lobby() == nullptr );

			REQUIRE( msg->memberDetails() != nullptr );

			auto attribSet = msg->memberDetails()->attributeSet();

			CHECK( attribSet->mode() == HumblePeer::AttributeMode::Replace );

			REQUIRE( attribSet->attributes() != nullptr );

			auto attribs = attribSet->attributes();

			CHECK( attribs->size() == 1 );

			auto v = attribs->LookupByKey( "skill" );

			CHECK( v->value()->str() == "1" );
		}
	}
}

SCENARIO( "sendLobbyLeave", "[client][lobby]" )
{
	GIVEN( "a lave lobby request" ) {
		humblenet::sendLobbyLeave( &conn, 0xbeefdead );

		THEN( "it creates a LobbyLeave message" ) {
			REQUIRE( conn.message->message_type() == HumblePeer::MessageType::LobbyLeave );
		}

		auto msg = reinterpret_cast<const HumblePeer::LobbyLeave*>(conn.message->message());

		THEN( "it has the correct properties" ) {
			CHECK( msg->lobbyId() == 0xbeefdead );

			CHECK( conn.message->requestId() != 0 );
		}
	}
}

SCENARIO( "sendLobbyDidLeave", "[server][lobby]" )
{
	GIVEN( "a did leave lobby response" ) {
		humblenet::sendLobbyDidLeave( &conn, 0xdeadbeef, 0xbeefdead, 0xd00d );

		THEN( "it creates a LobbyDidJoin message" ) {
			REQUIRE( conn.message->message_type() == HumblePeer::MessageType::LobbyDidLeave );
		}

		auto msg = reinterpret_cast<const HumblePeer::LobbyDidLeave*>(conn.message->message());

		THEN( "it has the correct properties" ) {
			CHECK( msg->lobbyId() == 0xbeefdead );

			CHECK( conn.message->requestId() == 0xdeadbeef );

			CHECK( msg->peerId() == 0xd00d );
		}
	}
}

SCENARIO( "sendLobbyUpdate", "[client][lobby]" )
{
	humblenet::AttributeMap attributes = {{"key", "value"}};

	LobbyId lobbyId = 0xbeefdead;

	GIVEN( "only updating attributes " ) {
		auto reqId = humblenet::sendLobbyUpdate( &conn, lobbyId,
												 HUMBLENET_LOBBY_TYPE_UNKNOWN, 0,
												 HumblePeer::AttributeMode::Merge, attributes );

		THEN( "it creates a LobbyUpdate message" ) {
			REQUIRE( conn.message->message_type() == HumblePeer::MessageType::LobbyUpdate );
		}

		auto msg = reinterpret_cast<const HumblePeer::LobbyUpdate*>(conn.message->message());

		THEN( "it has the correct properties" ) {
			CHECK( conn.message->requestId() == reqId );

			CHECK( msg->lobbyId() == lobbyId );

			CHECK( msg->lobbyType() == 0 );

			CHECK( msg->maxMembers() == 0 );

			REQUIRE( msg->attributeSet() != nullptr );

			auto attribSet = msg->attributeSet();

			CHECK( attribSet->mode() == HumblePeer::AttributeMode::Merge );

			auto attribs = attribSet->attributes();

			REQUIRE( attribs->size() == 1 );

			auto v = attribs->LookupByKey( "key" );

			CHECK( v->value()->str() == "value" );
		}
	}

	GIVEN( "only updating lobby type" ) {
		auto reqId = humblenet::sendLobbyUpdate( &conn, lobbyId,
												 HUMBLENET_LOBBY_TYPE_PRIVATE, 0,
												 HumblePeer::AttributeMode::Merge, {} );

		THEN( "it creates a LobbyUpdate message" ) {
			REQUIRE( conn.message->message_type() == HumblePeer::MessageType::LobbyUpdate );
		}

		auto msg = reinterpret_cast<const HumblePeer::LobbyUpdate*>(conn.message->message());

		THEN( "it has the correct properties" ) {
			CHECK( conn.message->requestId() == reqId );

			CHECK( msg->lobbyId() == lobbyId );

			CHECK( msg->lobbyType() == HUMBLENET_LOBBY_TYPE_PRIVATE );

			CHECK( msg->maxMembers() == 0 );

			CHECK( msg->attributeSet() == nullptr );
		}
	}

	GIVEN( "only updating max members" ) {
		auto reqId = humblenet::sendLobbyUpdate( &conn, lobbyId,
												 HUMBLENET_LOBBY_TYPE_UNKNOWN, 8,
												 HumblePeer::AttributeMode::Merge, {} );

		THEN( "it creates a LobbyUpdate message" ) {
			REQUIRE( conn.message->message_type() == HumblePeer::MessageType::LobbyUpdate );
		}

		auto msg = reinterpret_cast<const HumblePeer::LobbyUpdate*>(conn.message->message());

		THEN( "it has the correct properties" ) {
			CHECK( conn.message->requestId() == reqId );

			CHECK( msg->lobbyId() == lobbyId );

			CHECK( msg->lobbyType() == 0 );

			CHECK( msg->maxMembers() == 8 );

			CHECK( msg->attributeSet() == nullptr );
		}
	}

	GIVEN( "only updating empty attributes as a replace" ) {
		auto reqId = humblenet::sendLobbyUpdate( &conn, lobbyId,
												 HUMBLENET_LOBBY_TYPE_UNKNOWN, 0,
												 HumblePeer::AttributeMode::Replace, {} );

		THEN( "it creates a LobbyUpdate message" ) {
			REQUIRE( conn.message->message_type() == HumblePeer::MessageType::LobbyUpdate );
		}

		auto msg = reinterpret_cast<const HumblePeer::LobbyUpdate*>(conn.message->message());

		THEN( "it has the correct properties" ) {
			CHECK( conn.message->requestId() == reqId );

			CHECK( msg->lobbyId() == lobbyId );

			CHECK( msg->lobbyType() == 0 );

			CHECK( msg->maxMembers() == 0 );

			REQUIRE( msg->attributeSet() != nullptr );

			auto attribSet = msg->attributeSet();

			CHECK( attribSet->mode() == HumblePeer::AttributeMode::Replace );

			REQUIRE( attribSet->attributes() == nullptr );
		}
	}
}

SCENARIO( "sendLobbyMemberUpdate", "[client][lobby]" )
{
	humblenet::AttributeMap attributes = {{"key", "value"}};
	LobbyId lobbyId = 0xbeefdead;
	PeerId peerId = 0xd00d;

	GIVEN( "a setting attributes" ) {
		ha_requestId reqId = humblenet::sendLobbyMemberUpdate( &conn, lobbyId, peerId,
															   HumblePeer::AttributeMode::Merge, attributes );

		THEN( "it creates a LobbyMemberUpdate message" ) {
			REQUIRE( conn.message->message_type() == HumblePeer::MessageType::LobbyMemberUpdate );
		}

		auto msg = reinterpret_cast<const HumblePeer::LobbyMemberUpdate*>(conn.message->message());

		THEN( "it has the correct properties" ) {
			CHECK( conn.message->requestId() == reqId );

			CHECK( msg->lobbyId() == lobbyId );

			CHECK( msg->peerId() == peerId );

			REQUIRE( msg->attributeSet() != nullptr );

			auto attribSet = msg->attributeSet();

			CHECK( attribSet->mode() == HumblePeer::AttributeMode::Merge );

			auto attribs = attribSet->attributes();

			REQUIRE( attribs->size() == 1 );

			auto v = attribs->LookupByKey( "key" );

			CHECK( v->value()->str() == "value" );
		}
	}

	GIVEN( "no attributes as a replace" ) {
		ha_requestId reqId = humblenet::sendLobbyMemberUpdate( &conn, lobbyId, peerId,
															   HumblePeer::AttributeMode::Replace, {} );

		THEN( "it creates a LobbyMemberUpdate message" ) {
			REQUIRE( conn.message->message_type() == HumblePeer::MessageType::LobbyMemberUpdate );
		}

		auto msg = reinterpret_cast<const HumblePeer::LobbyMemberUpdate*>(conn.message->message());

		THEN( "it has the correct properties" ) {
			CHECK( conn.message->requestId() == reqId );

			CHECK( msg->lobbyId() == lobbyId );

			CHECK( msg->peerId() == peerId );

			REQUIRE( msg->attributeSet() != nullptr );

			auto attribSet = msg->attributeSet();

			CHECK( attribSet->mode() == HumblePeer::AttributeMode::Replace );

			CHECK( attribSet->attributes() == nullptr );
		}
	}

	GIVEN( "no attributes as a merge" ) {
		ha_requestId reqId = humblenet::sendLobbyMemberUpdate( &conn, lobbyId, peerId,
															   HumblePeer::AttributeMode::Merge, {} );

		THEN( "it creates a LobbyMemberUpdate message" ) {
			REQUIRE( conn.message->message_type() == HumblePeer::MessageType::LobbyMemberUpdate );
		}

		auto msg = reinterpret_cast<const HumblePeer::LobbyMemberUpdate*>(conn.message->message());

		THEN( "it has the correct properties" ) {
			CHECK( conn.message->requestId() == reqId );

			CHECK( msg->lobbyId() == lobbyId );

			CHECK( msg->peerId() == peerId );

			CHECK( msg->attributeSet() == nullptr );
		}
	}
}

SCENARIO( "sendLobbyDidUpdate", "[server][lobby]" )
{
	humblenet::AttributeMap attributes = {{"key", "value"}};

	ha_requestId reqId = 0xbeef;
	LobbyId lobbyId = 0xbeefdead;

	GIVEN( "a updating attributes only" ) {
		humblenet::sendLobbyDidUpdate( &conn, reqId, lobbyId,
									   HUMBLENET_LOBBY_TYPE_UNKNOWN, 0,
									   HumblePeer::AttributeMode::Merge, attributes );

		THEN( "it creates a LobbyDidUpdate message" ) {
			REQUIRE( conn.message->message_type() == HumblePeer::MessageType::LobbyDidUpdate );
		}

		auto msg = reinterpret_cast<const HumblePeer::LobbyDidUpdate*>(conn.message->message());

		THEN( "it has the correct properties" ) {
			CHECK( conn.message->requestId() == reqId );

			CHECK( msg->lobbyId() == lobbyId );

			CHECK( msg->revision() == 0);

			CHECK( msg->lobbyType() == 0 );

			CHECK( msg->maxMembers() == 0 );

			REQUIRE( msg->attributeSet() != nullptr );

			auto attribSet = msg->attributeSet();

			CHECK( attribSet->mode() == HumblePeer::AttributeMode::Merge );

			auto attribs = attribSet->attributes();

			REQUIRE( attribs->size() == 1 );

			auto v = attribs->LookupByKey( "key" );

			CHECK( v->value()->str() == "value" );
		}
	}

	GIVEN( "updating only lobby type" ) {
		humblenet::sendLobbyDidUpdate( &conn, reqId, lobbyId,
									   HUMBLENET_LOBBY_TYPE_PRIVATE, 0,
									   HumblePeer::AttributeMode::Merge, {} );

		THEN( "it creates a LobbyDidUpdate message" ) {
			REQUIRE( conn.message->message_type() == HumblePeer::MessageType::LobbyDidUpdate );
		}

		auto msg = reinterpret_cast<const HumblePeer::LobbyDidUpdate*>(conn.message->message());

		THEN( "it has the correct properties" ) {
			CHECK( conn.message->requestId() == reqId );

			CHECK( msg->lobbyId() == lobbyId );

			CHECK( msg->revision() == 0);

			CHECK( msg->lobbyType() == HUMBLENET_LOBBY_TYPE_PRIVATE );

			CHECK( msg->maxMembers() == 0 );

			CHECK( msg->attributeSet() == nullptr );
		}
	}

	GIVEN( "updating only max members" ) {
		humblenet::sendLobbyDidUpdate( &conn, reqId, lobbyId,
									   HUMBLENET_LOBBY_TYPE_UNKNOWN, 8,
									   HumblePeer::AttributeMode::Merge, {} );

		THEN( "it creates a LobbyDidUpdate message" ) {
			REQUIRE( conn.message->message_type() == HumblePeer::MessageType::LobbyDidUpdate );
		}

		auto msg = reinterpret_cast<const HumblePeer::LobbyDidUpdate*>(conn.message->message());

		THEN( "it has the correct properties" ) {
			CHECK( conn.message->requestId() == reqId );

			CHECK( msg->lobbyId() == lobbyId );

			CHECK( msg->revision() == 0);

			CHECK( msg->lobbyType() == 0 );

			CHECK( msg->maxMembers() == 8 );

			CHECK( msg->attributeSet() == nullptr );
		}
	}

	GIVEN( "blank attributes as replace" ) {
		humblenet::sendLobbyDidUpdate( &conn, reqId, lobbyId,
									   HUMBLENET_LOBBY_TYPE_UNKNOWN, 0,
									   HumblePeer::AttributeMode::Replace, {} );

		THEN( "it creates a LobbyDidUpdate message" ) {
			REQUIRE( conn.message->message_type() == HumblePeer::MessageType::LobbyDidUpdate );
		}

		auto msg = reinterpret_cast<const HumblePeer::LobbyDidUpdate*>(conn.message->message());

		THEN( "it has the correct properties" ) {
			CHECK( conn.message->requestId() == reqId );

			CHECK( msg->lobbyId() == lobbyId );

			CHECK( msg->revision() == 0);

			CHECK( msg->lobbyType() == 0 );

			CHECK( msg->maxMembers() == 0 );

			REQUIRE( msg->attributeSet() != nullptr );

			auto attribSet = msg->attributeSet();

			CHECK( attribSet->mode() == HumblePeer::AttributeMode::Replace );

			CHECK( attribSet->attributes() == nullptr );
		}
	}

	GIVEN( "blank attributes as merge" ) {
		humblenet::sendLobbyDidUpdate( &conn, reqId, lobbyId,
									   HUMBLENET_LOBBY_TYPE_UNKNOWN, 0,
									   HumblePeer::AttributeMode::Merge, {} );

		THEN( "it creates a LobbyDidUpdate message" ) {
			REQUIRE( conn.message->message_type() == HumblePeer::MessageType::LobbyDidUpdate );
		}

		auto msg = reinterpret_cast<const HumblePeer::LobbyDidUpdate*>(conn.message->message());

		THEN( "it has the correct properties" ) {
			CHECK( conn.message->requestId() == reqId );

			CHECK( msg->lobbyId() == lobbyId );

			CHECK( msg->revision() == 0);

			CHECK( msg->lobbyType() == 0 );

			CHECK( msg->maxMembers() == 0 );

			CHECK( msg->attributeSet() == nullptr );
		}
	}
}

SCENARIO( "sendLobbyDidUpdateMember", "[server][lobby]" )
{
	humblenet::AttributeMap attributes = {{"key", "value"}};

	ha_requestId reqId = 0xbeef;
	LobbyId lobbyId = 0xbeefdead;
	PeerId peerId = 0xd00d;

	GIVEN( "a attribute update" ) {
		humblenet::sendLobbyMemberDidUpdate( &conn, reqId, lobbyId, peerId,
											 HumblePeer::AttributeMode::Merge, attributes );

		THEN( "it creates a LobbyMemberDidUpdate message" ) {
			REQUIRE( conn.message->message_type() == HumblePeer::MessageType::LobbyMemberDidUpdate );
		}

		auto msg = reinterpret_cast<const HumblePeer::LobbyMemberDidUpdate*>(conn.message->message());

		THEN( "it has the correct properties" ) {
			CHECK( conn.message->requestId() == reqId );

			CHECK( msg->lobbyId() == lobbyId );

			CHECK( msg->peerId() == peerId );

			CHECK( msg->revision() == 0);

			REQUIRE( msg->attributeSet() != nullptr );

			auto attribSet = msg->attributeSet();

			CHECK( attribSet->mode() == HumblePeer::AttributeMode::Merge );

			auto attribs = attribSet->attributes();

			REQUIRE( attribs->size() == 1 );

			auto v = attribs->LookupByKey( "key" );

			CHECK( v->value()->str() == "value" );
		}
	}

	GIVEN( "no attributes as a replace" ) {
		humblenet::sendLobbyMemberDidUpdate( &conn, reqId, lobbyId, peerId,
											 HumblePeer::AttributeMode::Replace, {} );

		THEN( "it creates a LobbyMemberDidUpdate message" ) {
			REQUIRE( conn.message->message_type() == HumblePeer::MessageType::LobbyMemberDidUpdate );
		}

		auto msg = reinterpret_cast<const HumblePeer::LobbyMemberDidUpdate*>(conn.message->message());

		THEN( "it has the correct properties" ) {
			CHECK( conn.message->requestId() == reqId );

			CHECK( msg->lobbyId() == lobbyId );

			CHECK( msg->peerId() == peerId );

			CHECK( msg->revision() == 0);

			REQUIRE( msg->attributeSet() != nullptr );

			auto attribSet = msg->attributeSet();

			CHECK( attribSet->mode() == HumblePeer::AttributeMode::Replace );

			auto attribs = attribSet->attributes();

			CHECK( attribSet->attributes() == nullptr );
		}
	}

	GIVEN( "no attributes as a merge" ) {
		humblenet::sendLobbyMemberDidUpdate( &conn, reqId, lobbyId, peerId,
											 HumblePeer::AttributeMode::Merge, {} );

		THEN( "it creates a LobbyMemberDidUpdate message" ) {
			REQUIRE( conn.message->message_type() == HumblePeer::MessageType::LobbyMemberDidUpdate );
		}

		auto msg = reinterpret_cast<const HumblePeer::LobbyMemberDidUpdate*>(conn.message->message());

		THEN( "it has the correct properties" ) {
			CHECK( conn.message->requestId() == reqId );

			CHECK( msg->lobbyId() == lobbyId );

			CHECK( msg->peerId() == peerId );

			CHECK( msg->revision() == 0);

			CHECK( msg->attributeSet() == nullptr );
		}
	}
}

// endregion