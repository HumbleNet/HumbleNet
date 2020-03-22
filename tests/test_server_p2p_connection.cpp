#define CATCH_CONFIG_MAIN

#include <catch2/catch.hpp>

#include <cstdarg>
#include <cstdio>

#include "../src/peer-server/p2p_connection.h"
#include "../src/peer-server/server.h"

struct Message {
	std::vector<uint8_t> buffer;

	PeerId peer = 0;

	const humblenet::HumblePeer::Message* message = nullptr;
};
std::vector<Message> messages;

humblenet::Game* theGame;

namespace humblenet {
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

		item.peer = conn->peerId;

		item.buffer.resize( length );
		memcpy( item.buffer.data(), buff, length );

		parseMessage( item.buffer, processMessage, &item );

		return true;
	}

	// Stub Server
	Server::Server(std::shared_ptr<GameDB> _gameDB)
			: context( nullptr ), m_gameDB( std::move( _gameDB ))
	{
	}

	void Server::triggerWrite(struct libwebsocket* wsi)
	{
		// noop
	}

	Game* Server::getVerifiedGame(const HumblePeer::HelloServer* hello)
	{
		return theGame;
	}

	void Server::populateStunServers(std::vector<ICEServer>& servers)
	{
		// noop
	}
}

void log_func_var(int level, const char* fmt, ...)
{
	char buff[512];

	va_list vl;
	va_start( vl, fmt );
	vsnprintf( buff, sizeof( buff ), fmt, vl );
	va_end( vl );

	INFO( buff );
	// noop
}

std::shared_ptr<humblenet::GameDB> gameDB;
humblenet::Server server( gameDB );

namespace HumblePeer = humblenet::HumblePeer;

class ConnectedPeerFixture {
protected:
	humblenet::P2PSignalConnection conn;
	humblenet::P2PSignalConnection connAlt;
	humblenet::Game game;
public:
	ConnectedPeerFixture() : conn( &server ), connAlt( &server ), game( 1 )
	{
		theGame = &game;

		// Setup the hello
		runCommand( conn, [&](humblenet::P2PSignalConnection& c) {
			std::map<std::string, std::string> attributes = {{"key", "value"}};
			humblenet::sendHelloServer( &c, 0x1, "gameToken", "gameSecret", "", "", attributes );
		} );

		runCommand( connAlt, [&](humblenet::P2PSignalConnection& c) {
			std::map<std::string, std::string> attributes = {{"key", "value"}};
			humblenet::sendHelloServer( &c, 0x1, "gameToken", "gameSecret", "", "", attributes );
		} );
	}

	~ConnectedPeerFixture()
	{
		theGame = nullptr;
	}

	template<typename Function>
	bool runCommand(humblenet::P2PSignalConnection& c, Function block)
	{
		messages.clear();
		block( c );
		c.processMsg( messages[0].message );
		messages.clear();

		return true;
	}
};

// region Basic hello
SCENARIO( "server processMsg : HelloServer" )
{
	humblenet::P2PSignalConnection conn( &server );

	std::map<std::string, std::string> attributes = {{"key", "value"}};

	GIVEN( "An invalid game token" ) {
		messages.clear();

		humblenet::sendHelloServer( &conn, 0x1, "gameToken", "gameSecret", "", "", attributes );

		THEN( "It returns false and produces no messages" ) {
			CHECK_FALSE( conn.processMsg( messages[0].message ));

			CHECK( messages.size() == 1 );
		}
	}

	GIVEN( "a valid game token" ) {
		messages.clear();

		humblenet::sendHelloServer( &conn, 0x1, "gameToken", "gameSecret", "", "", attributes );

		humblenet::Game myGame( 1 );
		theGame = &myGame;

		THEN( "it produces a HelloClient message" ) {
			CHECK( conn.processMsg( messages[0].message ));

			REQUIRE( messages.size() == 2 );

			REQUIRE( messages[1].message->message_type() == HumblePeer::MessageType::HelloClient );

			auto msg = reinterpret_cast<const HumblePeer::HelloClient*>(messages[1].message->message());

			CHECK( msg->peerId() == 1 );

			CHECK( msg->reconnectToken() == nullptr );

			CHECK( msg->iceServers() == nullptr );
		}

		THEN( "It sets up the game state on the peer connection" ) {
			CHECK( conn.processMsg( messages[0].message ));

			REQUIRE( messages.size() == 2 );

			CHECK( conn.peerId == 1 );
			CHECK( conn.game == &myGame );
			CHECK( conn.game->peers[1] == &conn );
		}

		theGame = nullptr;
	}
}

// endregion

// region P2P offer handling
// TODO implement tests for P2POffer, P2PAnswer, ICECandidate, P2PReject, P2PConnected, P2PDisconnect, P2P RelayData
// endregion

// region Aliasing

SCENARIO_METHOD( ConnectedPeerFixture, "server processMsg : AliasRegister" )
{
	GIVEN( "A valid hostname" ) {
		ha_requestId reqId = humblenet::sendAliasRegister( &conn, "server.host" );

		THEN( "it generates a register success event" ) {
			conn.processMsg( messages[0].message );

			REQUIRE( messages.size() == 2 );

			{
				auto& curMsg = messages[1].message;

				REQUIRE( curMsg->message_type() == HumblePeer::MessageType::AliasRegisterSuccess );

				auto msg = reinterpret_cast<const HumblePeer::Success*>(curMsg->message());

				CHECK( curMsg->requestId() == reqId );
			}
		}

		THEN( "it adds the alias to the game" ) {
			conn.processMsg( messages[0].message );

			REQUIRE( game.aliases.find( "server.host" ) != game.aliases.end());

			CHECK( game.aliases.find( "server.host" )->second == conn.peerId );
		}
	}

	GIVEN( "when the host is registered to another peer" ) {
		game.aliases.emplace( "server.host", connAlt.peerId );

		ha_requestId reqId = humblenet::sendAliasRegister( &conn, "server.host" );

		THEN( "it does not register the peer" ) {
			conn.processMsg( messages[0].message );

			REQUIRE( game.aliases.size() == 1 );

			REQUIRE( game.aliases.find( "server.host" ) != game.aliases.end());

			REQUIRE( game.aliases.find( "server.host" )->second == connAlt.peerId );
		}

		THEN( "it generates an error response" ) {
			conn.processMsg( messages[0].message );

			REQUIRE( messages.size() == 2 );
			{
				auto& curMsg = messages[1].message;

				REQUIRE( curMsg->message_type() == HumblePeer::MessageType::AliasRegisterError );

				auto msg = reinterpret_cast<const HumblePeer::Error*>(curMsg->message());

				CHECK( curMsg->requestId() == reqId );
				CHECK( msg->error()->str() == "Alias already registered" );
			}
		}
	}
}

SCENARIO_METHOD( ConnectedPeerFixture, "server processMsg : AliasUnregister" )
{
	GIVEN( "A previously registered hostname" ) {
		runCommand( conn, [&](humblenet::P2PSignalConnection& c) {
			humblenet::sendAliasRegister( &c, "server.host" );
		} );

		runCommand( conn, [&](humblenet::P2PSignalConnection& c) {
			humblenet::sendAliasRegister( &c, "server2.host" );
		} );

		CHECK( game.aliases.size() == 2 );

		ha_requestId reqId = humblenet::sendAliasUnregister( &conn, "server.host" );

		THEN( "it generates a Unregister success response" ) {
			conn.processMsg( messages[0].message );

			REQUIRE( messages.size() == 2 );

			{
				auto& curMsg = messages[1].message;

				REQUIRE( curMsg->message_type() == HumblePeer::MessageType::AliasUnregisterSuccess );

				auto msg = reinterpret_cast<const HumblePeer::Success*>(curMsg->message());

				CHECK( curMsg->requestId() == reqId );
			}
		}

		THEN( "it unregisters the specified alias only" ) {
			conn.processMsg( messages[0].message );

			CHECK( game.aliases.size() == 1 );

			REQUIRE( game.aliases.find( "server.host" ) == game.aliases.end());
			REQUIRE( game.aliases.find( "server2.host" ) != game.aliases.end());
		}
	}

	GIVEN( "when the host is registered to another peer" ) {
		game.aliases.emplace( "server.host", connAlt.peerId );

		CHECK( game.aliases.size() == 1 );

		ha_requestId reqId = humblenet::sendAliasUnregister( &conn, "server.host" );

		THEN( "it does not unregister the peer" ) {
			conn.processMsg( messages[0].message );

			REQUIRE( game.aliases.size() == 1 );
		}

		THEN( "it generates an error response" ) {
			conn.processMsg( messages[0].message );

			REQUIRE( messages.size() == 2 );
			{
				auto& curMsg = messages[1].message;

				REQUIRE( curMsg->message_type() == HumblePeer::MessageType::AliasUnregisterError );

				auto msg = reinterpret_cast<const HumblePeer::Error*>(curMsg->message());

				CHECK( curMsg->requestId() == reqId );
				CHECK( msg->error()->str() == "Unregister failed" );
			}
		}
	}

	GIVEN( "no specific hostname" ) {
		runCommand( conn, [&](humblenet::P2PSignalConnection& c) {
			humblenet::sendAliasRegister( &c, "server.host" );
		} );

		runCommand( conn, [&](humblenet::P2PSignalConnection& c) {
			humblenet::sendAliasRegister( &c, "server2.host" );
		} );

		CHECK( game.aliases.size() == 2 );

		game.aliases.emplace( "server3.host", connAlt.peerId );

		ha_requestId reqId = humblenet::sendAliasUnregister( &conn, "" );

		THEN( "it generates a Unregister success response" ) {
			conn.processMsg( messages[0].message );

			REQUIRE( messages.size() == 2 );

			{
				auto& curMsg = messages[1].message;

				REQUIRE( curMsg->message_type() == HumblePeer::MessageType::AliasUnregisterSuccess );

				auto msg = reinterpret_cast<const HumblePeer::Success*>(curMsg->message());

				CHECK( curMsg->requestId() == reqId );
			}
		}

		THEN( "it unregisters all aliases for the peer" ) {
			conn.processMsg( messages[0].message );

			CHECK( game.aliases.size() == 1 );

			CHECK( game.aliases.find( "server.host" ) == game.aliases.end());
			CHECK( game.aliases.find( "server2.host" ) == game.aliases.end());
			CHECK( game.aliases.find( "server3.host" ) != game.aliases.end());
		}
	}
}

SCENARIO_METHOD( ConnectedPeerFixture, "server processMsg : AliasLookup" )
{
	GIVEN( "A previously registered hostname" ) {
		runCommand( conn, [&](humblenet::P2PSignalConnection& c) {
			humblenet::sendAliasRegister( &c, "server.host" );
		} );

		CHECK( game.aliases.size() == 1 );

		ha_requestId reqId = humblenet::sendAliasLookup( &conn, "server.host" );

		THEN( "it generates a Resolved response" ) {
			conn.processMsg( messages[0].message );

			REQUIRE( messages.size() == 2 );

			{
				auto& curMsg = messages[1].message;

				REQUIRE( curMsg->message_type() == HumblePeer::MessageType::AliasResolved );

				auto msg = reinterpret_cast<const HumblePeer::AliasResolved*>(curMsg->message());

				CHECK( curMsg->requestId() == reqId );
				CHECK( msg->peerId() == conn.peerId );
			}
		}
	}

	GIVEN( "when the host is registered to another peer" ) {
		game.aliases.emplace( "server.host", connAlt.peerId );

		CHECK( game.aliases.size() == 1 );

		ha_requestId reqId = humblenet::sendAliasLookup( &conn, "server.host" );

		THEN( "it generates a Resolved response" ) {
			conn.processMsg( messages[0].message );

			REQUIRE( messages.size() == 2 );

			{
				auto& curMsg = messages[1].message;

				REQUIRE( curMsg->message_type() == HumblePeer::MessageType::AliasResolved );

				auto msg = reinterpret_cast<const HumblePeer::AliasResolved*>(curMsg->message());

				CHECK( curMsg->requestId() == reqId );
				CHECK( msg->peerId() == connAlt.peerId);
			}
		}
	}

	GIVEN( "when an unknown hostname" ) {
		game.aliases.emplace( "server.host", connAlt.peerId );

		CHECK( game.aliases.size() == 1 );

		ha_requestId reqId = humblenet::sendAliasLookup( &conn, "server2.host" );

		THEN( "it generates a Resolved response with no peer" ) {
			conn.processMsg( messages[0].message );

			REQUIRE( messages.size() == 2 );

			{
				auto& curMsg = messages[1].message;

				REQUIRE( curMsg->message_type() == HumblePeer::MessageType::AliasResolved );

				auto msg = reinterpret_cast<const HumblePeer::AliasResolved*>(curMsg->message());

				CHECK( curMsg->requestId() == reqId );
				CHECK( msg->peerId() == 0 );
			}
		}
	}
}

// endregion
