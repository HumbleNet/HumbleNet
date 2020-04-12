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
