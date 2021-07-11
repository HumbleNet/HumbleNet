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

// region Lobby system
SCENARIO_METHOD( ConnectedPeerFixture, "server processMsg : LobbyCreate" )
{
	GIVEN( "A valid lobby create request" ) {
		ha_requestId reqId = humblenet::sendLobbyCreate( &conn, HUMBLENET_LOBBY_TYPE_PUBLIC,
														 4 );

		conn.processMsg( messages[0].message );

		THEN( "it generates a lobby did create event" ) {
			REQUIRE( messages.size() == 2 );

			REQUIRE( messages[1].message->message_type() == HumblePeer::MessageType::LobbyDidCreate );

			auto msg = reinterpret_cast<const HumblePeer::LobbyDidCreate*>(messages[1].message->message());

			CHECK( messages[1].message->requestId() == reqId );

			CHECK( msg->lobbyId() == (game.nextLobbyId - 1));

			REQUIRE( msg->lobby() != nullptr );

			CHECK( msg->lobby()->ownerPeer() == conn.peerId );

			CHECK( msg->lobby()->lobbyType() == HUMBLENET_LOBBY_TYPE_PUBLIC );

			CHECK( msg->lobby()->maxMembers() == 4 );

			REQUIRE( msg->lobby()->attributeSet() != nullptr );

			CHECK( msg->lobby()->attributeSet()->mode() == HumblePeer::AttributeMode::Replace );

			CHECK( msg->lobby()->attributeSet()->attributes() == nullptr );
		}

		THEN( "it adds the lobby to the game" ) {
			REQUIRE( game.lobbies.size() == 1 );

			auto& lobby = game.lobbies.begin()->second;

			CHECK( lobby.maxMembers == 4 );
			CHECK( lobby.ownerPeerId == conn.peerId );
			CHECK( lobby.lobbyId == (game.nextLobbyId - 1));
			CHECK( lobby.type == HUMBLENET_LOBBY_TYPE_PUBLIC );

			CHECK( lobby.members.size() == 1 );
			CHECK( lobby.members.count( conn.peerId ) == 1 );
		}
	}
}

SCENARIO_METHOD( ConnectedPeerFixture, "server processMsg : LobbyJoin" )
{
	GIVEN( "A lobby join request for an existing lobby" ) {
		auto lobbyId = conn.game->generateNewLobbyId();
		conn.game->lobbies.insert(
				std::make_pair( lobbyId, humblenet::Lobby( lobbyId, connAlt.peerId, HUMBLENET_LOBBY_TYPE_PUBLIC, 4 )));

		ha_requestId reqId = humblenet::sendLobbyJoin( &conn, lobbyId );

		conn.processMsg( messages[0].message );

		THEN( "it generates a lobby did join event for each member" ) {
			REQUIRE( messages.size() == 4 );

			{
				auto& curMsg = messages[1];

				// Message 1 for new member about themselves (success response)

				REQUIRE( curMsg.message->message_type() == HumblePeer::MessageType::LobbyDidJoin );

				auto msg = reinterpret_cast<const HumblePeer::LobbyDidJoin*>(curMsg.message->message());

				CHECK ( curMsg.peer == conn.peerId );

				CHECK( curMsg.message->requestId() == reqId );

				CHECK( msg->lobbyId() == lobbyId );

				CHECK( msg->peerId() == conn.peerId );

				REQUIRE( msg->lobby() != nullptr );

				CHECK( msg->lobby()->ownerPeer() == connAlt.peerId );

				CHECK( msg->lobby()->lobbyType() == HUMBLENET_LOBBY_TYPE_PUBLIC );

				CHECK( msg->lobby()->maxMembers() == 4 );

				REQUIRE( msg->lobby()->attributeSet() != nullptr );

				CHECK( msg->lobby()->attributeSet()->mode() == HumblePeer::AttributeMode::Replace );

				CHECK( msg->lobby()->attributeSet()->attributes() == nullptr );

				CHECK( msg->memberDetails() == nullptr );
			}

			{
				auto& curMsg = messages[2];

				// Message 2 for existing member about newly added member

				REQUIRE( curMsg.message->message_type() == HumblePeer::MessageType::LobbyDidJoin );

				auto msg = reinterpret_cast<const HumblePeer::LobbyDidJoin*>(curMsg.message->message());

				CHECK ( curMsg.peer == connAlt.peerId );

				CHECK( curMsg.message->requestId() == 0 );

				CHECK( msg->lobbyId() == lobbyId );

				CHECK( msg->peerId() == conn.peerId );

				CHECK( msg->lobby() == nullptr );

				REQUIRE( msg->memberDetails() != nullptr );

				REQUIRE( msg->memberDetails()->attributeSet() != nullptr );

				auto attribSet = msg->memberDetails()->attributeSet();

				CHECK( attribSet->mode() == HumblePeer::AttributeMode::Replace );

				REQUIRE( attribSet->attributes() == nullptr );
			}

			{
				auto& curMsg = messages[3];

				// Message 3 for new member about existing members

				REQUIRE( curMsg.message->message_type() == HumblePeer::MessageType::LobbyDidJoin );

				auto msg = reinterpret_cast<const HumblePeer::LobbyDidJoin*>(curMsg.message->message());

				CHECK ( curMsg.peer == conn.peerId );

				CHECK( curMsg.message->requestId() == 0 );

				CHECK( msg->lobbyId() == lobbyId );

				CHECK( msg->peerId() == connAlt.peerId );

				CHECK( msg->lobby() == nullptr );

				CHECK( msg->memberDetails() != nullptr );

				REQUIRE( msg->memberDetails()->attributeSet() != nullptr );

				auto attribSet = msg->memberDetails()->attributeSet();

				CHECK( attribSet->mode() == HumblePeer::AttributeMode::Replace );

				REQUIRE( attribSet->attributes() == nullptr );
			}
		}

		THEN( "it adds the member to the lobby" ) {
			REQUIRE( game.lobbies.size() == 1 );

			auto& lobby = game.lobbies.at( lobbyId );

			CHECK( lobby.members.size() == 2 );
			CHECK( lobby.members.count( conn.peerId ) == 1 );
		}

		THEN ( "it does not change the owner to another member" ) {
			REQUIRE( game.lobbies.size() == 1 );

			auto& lobby = game.lobbies.at( lobbyId );

			CHECK( lobby.maxMembers == 4 );
			CHECK( lobby.ownerPeerId == connAlt.peerId );
		}
	}
}

SCENARIO_METHOD( ConnectedPeerFixture, "server processMsg : LobbyLeave" )
{
	GIVEN( "a request to leave a lobby I am a member of" ) {
		auto lobbyId = conn.game->generateNewLobbyId();
		conn.game->lobbies.insert(
				std::make_pair( lobbyId, humblenet::Lobby( lobbyId, connAlt.peerId, HUMBLENET_LOBBY_TYPE_PUBLIC, 4 )));

		conn.game->lobbies.begin()->second.addPeer( conn.peerId );

		ha_requestId reqId = humblenet::sendLobbyLeave( &conn, lobbyId );

		conn.processMsg( messages[0].message );

		THEN( "it generates a lobby did leave event for each member" ) {
			REQUIRE( messages.size() == 3 );
			{
				auto& curMsg = messages[1];

				REQUIRE( curMsg.message->message_type() == HumblePeer::MessageType::LobbyDidLeave );

				auto msg = reinterpret_cast<const HumblePeer::LobbyDidLeave*>(curMsg.message->message());

				CHECK ( curMsg.peer == conn.peerId );

				CHECK( curMsg.message->requestId() == reqId );

				CHECK( msg->lobbyId() == lobbyId );

				CHECK( msg->peerId() == conn.peerId );
			}

			{
				auto& curMsg = messages[2];

				REQUIRE( curMsg.message->message_type() == HumblePeer::MessageType::LobbyDidLeave );

				auto msg = reinterpret_cast<const HumblePeer::LobbyDidLeave*>(curMsg.message->message());

				CHECK ( curMsg.peer == connAlt.peerId );

				CHECK( curMsg.message->requestId() == 0 );

				CHECK( msg->lobbyId() == lobbyId );

				CHECK( msg->peerId() == conn.peerId );
			}
		}

		THEN( "it removes the member from the lobby" ) {
			REQUIRE( game.lobbies.size() == 1 );

			auto& lobby = game.lobbies.at( lobbyId );

			CHECK( lobby.members.size() == 1 );
			CHECK( lobby.members.count( conn.peerId ) == 0 );
		}

		THEN ( "it does not change the owner to another member" ) {
			REQUIRE( game.lobbies.size() == 1 );

			auto& lobby = game.lobbies.at( lobbyId );

			CHECK( lobby.maxMembers == 4 );
			CHECK( lobby.ownerPeerId == connAlt.peerId );
		}
	}

	GIVEN( "a request to leave a lobby I am the owner of" ) {
		auto lobbyId = conn.game->generateNewLobbyId();
		conn.game->lobbies.insert(
				std::make_pair( lobbyId, humblenet::Lobby( lobbyId, conn.peerId, HUMBLENET_LOBBY_TYPE_PUBLIC, 4 )));
		conn.game->lobbies.begin()->second.addPeer( connAlt.peerId );

		ha_requestId reqId = humblenet::sendLobbyLeave( &conn, lobbyId );

		conn.processMsg( messages[0].message );

		THEN( "it generates a lobby did leave event for each member" ) {
			REQUIRE( messages.size() == 3 );

			{
				auto& curMsg = messages[1];

				REQUIRE( curMsg.message->message_type() == HumblePeer::MessageType::LobbyDidLeave );

				auto msg = reinterpret_cast<const HumblePeer::LobbyDidLeave*>(curMsg.message->message());

				CHECK ( curMsg.peer == conn.peerId );

				CHECK( curMsg.message->requestId() == reqId );

				CHECK( msg->lobbyId() == lobbyId );

				CHECK( msg->peerId() == conn.peerId );
			}

			{
				auto& curMsg = messages[2];

				REQUIRE( curMsg.message->message_type() == HumblePeer::MessageType::LobbyDidLeave );

				auto msg = reinterpret_cast<const HumblePeer::LobbyDidLeave*>(curMsg.message->message());

				CHECK ( curMsg.peer == connAlt.peerId );

				CHECK( curMsg.message->requestId() == 0 );

				CHECK( msg->lobbyId() == lobbyId );

				CHECK( msg->peerId() == conn.peerId );
			}
		}

		THEN( "it removes the member from the lobby" ) {
			REQUIRE( game.lobbies.size() == 1 );

			auto& lobby = game.lobbies.at( lobbyId );

			CHECK( lobby.members.size() == 1 );
			CHECK( lobby.members.count( conn.peerId ) == 0 );
		}

		THEN ( "it changes the owner to another member" ) {
			REQUIRE( game.lobbies.size() == 1 );

			auto& lobby = game.lobbies.at( lobbyId );

			CHECK( lobby.maxMembers == 4 );
			CHECK( lobby.ownerPeerId == connAlt.peerId );
		}
	}
}

SCENARIO_METHOD( ConnectedPeerFixture, "server processMsg : LobbyUpdate" )
{
	humblenet::AttributeMap attributes = {{"key",  "value"},
										  {"test", "two"}};

	GIVEN( "I am the owner of the lobby " ) {
		GIVEN( "and only merging attributes" ) {
			auto lobbyId = conn.game->generateNewLobbyId();
			conn.game->lobbies.insert(
					std::make_pair( lobbyId, humblenet::Lobby( lobbyId, conn.peerId, HUMBLENET_LOBBY_TYPE_PUBLIC, 4 )));
			auto& lobby = conn.game->lobbies.at( lobbyId );
			lobby.attributes["test"] = "some";
			lobby.attributes["another"] = "one";
			CHECK( lobby.attributes.size() == 2 );

			conn.game->lobbies.begin()->second.addPeer( connAlt.peerId );

			ha_requestId reqId = humblenet::sendLobbyUpdate( &conn, lobbyId,
															 HUMBLENET_LOBBY_TYPE_UNKNOWN, 0,
															 HumblePeer::AttributeMode::Merge, attributes );

			conn.processMsg( messages[0].message );

			THEN( "it generates a LobbyDidUpdate message for each member" ) {
				REQUIRE( messages.size() == 3 );
				{
					auto& curMsg = messages[1];

					REQUIRE( curMsg.message->message_type() == HumblePeer::MessageType::LobbyDidUpdate );

					auto msg = reinterpret_cast<const HumblePeer::LobbyDidUpdate*>(curMsg.message->message());

					CHECK ( curMsg.peer == conn.peerId );

					CHECK( curMsg.message->requestId() == reqId );

					CHECK( msg->lobbyId() == lobbyId );

					CHECK( msg->revision() == 0 );

					CHECK( msg->lobbyType() == 0 );

					CHECK( msg->maxMembers() == 0 );

					REQUIRE( msg->attributeSet() != nullptr );

					REQUIRE( msg->attributeSet()->attributes() != nullptr );

					CHECK( msg->attributeSet()->mode() == HumblePeer::AttributeMode::Merge );

					auto attribs = msg->attributeSet()->attributes();

					REQUIRE( attribs->size() == 2 );

					auto v = attribs->LookupByKey( "key" );

					CHECK( v->value()->str() == "value" );

					v = attribs->LookupByKey( "test" );

					CHECK( v->value()->str() == "two" );
				}

				{
					auto& curMsg = messages[2];

					REQUIRE( curMsg.message->message_type() == HumblePeer::MessageType::LobbyDidUpdate );

					auto msg = reinterpret_cast<const HumblePeer::LobbyDidUpdate*>(curMsg.message->message());

					CHECK ( curMsg.peer == connAlt.peerId );

					CHECK( curMsg.message->requestId() == 0 );

					CHECK( msg->lobbyId() == lobbyId );

					CHECK( msg->revision() == 0 );

					CHECK( msg->lobbyType() == 0 );

					CHECK( msg->maxMembers() == 0 );

					REQUIRE( msg->attributeSet() != nullptr );

					REQUIRE( msg->attributeSet()->attributes() != nullptr );

					CHECK( msg->attributeSet()->mode() == HumblePeer::AttributeMode::Merge );

					auto attribs = msg->attributeSet()->attributes();

					REQUIRE( attribs->size() == 2 );

					auto v = attribs->LookupByKey( "key" );

					CHECK( v->value()->str() == "value" );

					v = attribs->LookupByKey( "test" );

					CHECK( v->value()->str() == "two" );
				}
			}

			THEN( "it does not change the type nor max members" ) {
				CHECK( lobby.type == HUMBLENET_LOBBY_TYPE_PUBLIC );

				CHECK( lobby.maxMembers == 4 );
			}

			THEN( "it updates the attributes on the lobby" ) {
				CHECK( lobby.attributes.size() == 3 );

				CHECK( lobby.attributes["key"] == "value" );
				CHECK( lobby.attributes["test"] == "two" );
				CHECK( lobby.attributes["another"] == "one" );
			}
		}

		GIVEN( "and only replacing attributes" ) {
			auto lobbyId = conn.game->generateNewLobbyId();
			conn.game->lobbies.insert(
					std::make_pair( lobbyId, humblenet::Lobby( lobbyId, conn.peerId, HUMBLENET_LOBBY_TYPE_PUBLIC, 4 )));
			auto& lobby = conn.game->lobbies.at( lobbyId );
			lobby.attributes["test"] = "some";
			lobby.attributes["another"] = "one";
			CHECK( lobby.attributes.size() == 2 );

			conn.game->lobbies.begin()->second.addPeer( connAlt.peerId );

			ha_requestId reqId = humblenet::sendLobbyUpdate( &conn, lobbyId,
															 HUMBLENET_LOBBY_TYPE_UNKNOWN, 0,
															 HumblePeer::AttributeMode::Replace, attributes );

			conn.processMsg( messages[0].message );

			THEN( "it generates a lobbyUpdate message for each member" ) {
				REQUIRE( messages.size() == 3 );
				{
					auto& curMsg = messages[1];

					REQUIRE( curMsg.message->message_type() == HumblePeer::MessageType::LobbyDidUpdate );

					auto msg = reinterpret_cast<const HumblePeer::LobbyDidUpdate*>(curMsg.message->message());

					CHECK ( curMsg.peer == conn.peerId );

					CHECK( curMsg.message->requestId() == reqId );

					CHECK( msg->lobbyId() == lobbyId );

					CHECK( msg->lobbyType() == 0 );

					CHECK( msg->maxMembers() == 0 );

					REQUIRE( msg->attributeSet() != nullptr );

					REQUIRE( msg->attributeSet()->attributes() != nullptr );

					CHECK( msg->attributeSet()->mode() == HumblePeer::AttributeMode::Replace );

					auto attribs = msg->attributeSet()->attributes();

					REQUIRE( attribs->size() == 2 );

					auto v = attribs->LookupByKey( "key" );

					CHECK( v->value()->str() == "value" );

					v = attribs->LookupByKey( "test" );

					CHECK( v->value()->str() == "two" );
				}

				{
					auto& curMsg = messages[2];

					REQUIRE( curMsg.message->message_type() == HumblePeer::MessageType::LobbyDidUpdate );

					auto msg = reinterpret_cast<const HumblePeer::LobbyDidUpdate*>(curMsg.message->message());

					CHECK ( curMsg.peer == connAlt.peerId );

					CHECK( curMsg.message->requestId() == 0 );

					CHECK( msg->lobbyId() == lobbyId );

					CHECK( msg->lobbyType() == 0 );

					CHECK( msg->maxMembers() == 0 );

					REQUIRE( msg->attributeSet() != nullptr );

					REQUIRE( msg->attributeSet()->attributes() != nullptr );

					CHECK( msg->attributeSet()->mode() == HumblePeer::AttributeMode::Replace );

					auto attribs = msg->attributeSet()->attributes();

					REQUIRE( attribs->size() == 2 );

					auto v = attribs->LookupByKey( "key" );

					CHECK( v->value()->str() == "value" );

					v = attribs->LookupByKey( "test" );

					CHECK( v->value()->str() == "two" );
				}
			}

			THEN( "it does not change the type nor max members" ) {
				CHECK( lobby.type == HUMBLENET_LOBBY_TYPE_PUBLIC );

				CHECK( lobby.maxMembers == 4 );
			}

			THEN( "it updates the attributes on the lobby" ) {
				CHECK( lobby.attributes.size() == 2 );

				CHECK( lobby.attributes["key"] == "value" );
				CHECK( lobby.attributes["test"] == "two" );
			}
		}

		GIVEN( "and merging a blank value for an attribute" ) {
			auto lobbyId = conn.game->generateNewLobbyId();
			conn.game->lobbies.insert(
					std::make_pair( lobbyId, humblenet::Lobby( lobbyId, conn.peerId, HUMBLENET_LOBBY_TYPE_PUBLIC, 4 )));
			auto& lobby = conn.game->lobbies.at( lobbyId );
			lobby.addPeer( connAlt.peerId );

			lobby.attributes["test"] = "some";
			lobby.attributes["another"] = "one";
			CHECK( lobby.attributes.size() == 2 );

			attributes["test"] = "";

			ha_requestId reqId = humblenet::sendLobbyUpdate( &conn, lobbyId,
															 HUMBLENET_LOBBY_TYPE_UNKNOWN, 0,
															 HumblePeer::AttributeMode::Merge, attributes );

			conn.processMsg( messages[0].message );

			THEN( "it clears that attribute" ) {
				CHECK( lobby.attributes.size() == 2 );

				CHECK( lobby.attributes["key"] == "value" );
				CHECK( lobby.attributes["another"] == "one" );
			}
		}

		GIVEN( "and only updating the lobby type" ) {
			auto lobbyId = conn.game->generateNewLobbyId();
			conn.game->lobbies.insert(
					std::make_pair( lobbyId, humblenet::Lobby( lobbyId, conn.peerId, HUMBLENET_LOBBY_TYPE_PUBLIC, 4 )));
			auto& lobby = conn.game->lobbies.at( lobbyId );
			lobby.addPeer( connAlt.peerId );

			lobby.attributes["test"] = "some";
			lobby.attributes["another"] = "one";
			CHECK( lobby.attributes.size() == 2 );

			ha_requestId reqId = humblenet::sendLobbyUpdate( &conn, lobbyId,
															 HUMBLENET_LOBBY_TYPE_PRIVATE, 0,
															 HumblePeer::AttributeMode::Merge, {} );

			conn.processMsg( messages[0].message );

			THEN( "it generates a lobbyUpdate message for each member" ) {
				REQUIRE( messages.size() == 3 );

				{
					auto& curMsg = messages[1];

					REQUIRE( curMsg.message->message_type() == HumblePeer::MessageType::LobbyDidUpdate );

					auto msg = reinterpret_cast<const HumblePeer::LobbyDidUpdate*>(curMsg.message->message());

					CHECK ( curMsg.peer == conn.peerId );

					CHECK( curMsg.message->requestId() == reqId );

					CHECK( msg->lobbyId() == lobbyId );

					CHECK( msg->lobbyType() == HUMBLENET_LOBBY_TYPE_PRIVATE );

					CHECK( msg->maxMembers() == 0 );

					CHECK( msg->attributeSet() == nullptr );
				}

				{
					auto& curMsg = messages[2];

					REQUIRE( curMsg.message->message_type() == HumblePeer::MessageType::LobbyDidUpdate );

					auto msg = reinterpret_cast<const HumblePeer::LobbyDidUpdate*>(curMsg.message->message());

					CHECK ( curMsg.peer == connAlt.peerId );

					CHECK( curMsg.message->requestId() == 0 );

					CHECK( msg->lobbyId() == lobbyId );

					CHECK( msg->lobbyType() == HUMBLENET_LOBBY_TYPE_PRIVATE );

					CHECK( msg->maxMembers() == 0 );

					REQUIRE( msg->attributeSet() == nullptr );
				}
			}

			THEN( "it changes the lobby type" ) {
				CHECK( lobby.type == HUMBLENET_LOBBY_TYPE_PRIVATE );
			}

			THEN( "it does not change the max members" ) {
				CHECK( lobby.maxMembers == 4 );
			}

			THEN( "it does not change the attributes" ) {
				CHECK( lobby.attributes.size() == 2 );

				CHECK( lobby.attributes["test"] == "some" );
				CHECK( lobby.attributes["another"] == "one" );
			}
		}

		GIVEN( "and only updating the max members" ) {
			auto lobbyId = conn.game->generateNewLobbyId();
			conn.game->lobbies.insert(
					std::make_pair( lobbyId, humblenet::Lobby( lobbyId, conn.peerId, HUMBLENET_LOBBY_TYPE_PUBLIC, 4 )));
			auto& lobby = conn.game->lobbies.at( lobbyId );
			lobby.addPeer( connAlt.peerId );

			lobby.attributes["test"] = "some";
			lobby.attributes["another"] = "one";
			CHECK( lobby.attributes.size() == 2 );

			ha_requestId reqId = humblenet::sendLobbyUpdate( &conn, lobbyId,
															 HUMBLENET_LOBBY_TYPE_UNKNOWN, 8,
															 HumblePeer::AttributeMode::Merge, {} );

			conn.processMsg( messages[0].message );

			THEN( "it generates a lobbyUpdate message for each member" ) {
				REQUIRE( messages.size() == 3 );

				{
					auto& curMsg = messages[1];

					REQUIRE( curMsg.message->message_type() == HumblePeer::MessageType::LobbyDidUpdate );

					auto msg = reinterpret_cast<const HumblePeer::LobbyDidUpdate*>(curMsg.message->message());

					CHECK ( curMsg.peer == conn.peerId );

					CHECK( curMsg.message->requestId() == reqId );

					CHECK( msg->lobbyId() == lobbyId );

					CHECK( msg->lobbyType() == 0 );

					CHECK( msg->maxMembers() == 8 );

					CHECK( msg->attributeSet() == nullptr );
				}

				{
					auto& curMsg = messages[2];

					REQUIRE( curMsg.message->message_type() == HumblePeer::MessageType::LobbyDidUpdate );

					auto msg = reinterpret_cast<const HumblePeer::LobbyDidUpdate*>(curMsg.message->message());

					CHECK ( curMsg.peer == connAlt.peerId );

					CHECK( curMsg.message->requestId() == 0 );

					CHECK( msg->lobbyId() == lobbyId );

					CHECK( msg->lobbyType() == 0 );

					CHECK( msg->maxMembers() == 8 );

					REQUIRE( msg->attributeSet() == nullptr );
				}
			}

			THEN( "it does not change the lobby type" ) {
				CHECK( lobby.type == HUMBLENET_LOBBY_TYPE_PUBLIC );
			}

			THEN( "it changes the max members" ) {
				CHECK( lobby.maxMembers == 8 );
			}

			THEN( "it does not change the attributes" ) {
				CHECK( lobby.attributes.size() == 2 );

				CHECK( lobby.attributes["test"] == "some" );
				CHECK( lobby.attributes["another"] == "one" );
			}
		}
	}

	GIVEN( "I am not the lobby owner" ) {
		auto lobbyId = conn.game->generateNewLobbyId();
		conn.game->lobbies.insert(
				std::make_pair( lobbyId, humblenet::Lobby( lobbyId, connAlt.peerId, HUMBLENET_LOBBY_TYPE_PUBLIC, 4 )));
		conn.game->lobbies.begin()->second.addPeer( conn.peerId );
		auto& lobby = conn.game->lobbies.at( lobbyId );

		ha_requestId reqId = humblenet::sendLobbyUpdate( &conn, lobbyId,
														 HUMBLENET_LOBBY_TYPE_PRIVATE, 8,
														 HumblePeer::AttributeMode::Merge, attributes );

		conn.processMsg( messages[0].message );

		THEN( "it generates a lobby Error message" ) {
			REQUIRE( messages.size() == 2 );

			REQUIRE( messages[1].message->message_type() == HumblePeer::MessageType::LobbyUpdateError );

			auto msg = reinterpret_cast<const HumblePeer::Error*>(messages[1].message->message());

			CHECK ( messages[1].peer == conn.peerId );

			CHECK( messages[1].message->requestId() == reqId );

			CHECK( msg->error()->str() == "Lobby can only be updated by the lobby owner" );
		}

		THEN( "it does not change the type" ) {
			CHECK( lobby.type == HUMBLENET_LOBBY_TYPE_PUBLIC );
		}

		THEN( "it does not change the max members" ) {
			CHECK( lobby.maxMembers == 4 );
		}

		THEN( "it does not change the lobby attributes" ) {
			CHECK( lobby.attributes.empty());
		}
	}
}

SCENARIO_METHOD( ConnectedPeerFixture, "server processMsg : LobbyMemberUpdate" )
{
	humblenet::AttributeMap attributes = {{"key",  "value"},
										  {"test", "two"}};

	GIVEN( "I am the member being updated (even when not owner)" ) {
		auto lobbyId = conn.game->generateNewLobbyId();
		conn.game->lobbies.insert(
				std::make_pair( lobbyId, humblenet::Lobby( lobbyId, connAlt.peerId, HUMBLENET_LOBBY_TYPE_PUBLIC, 4 )));
		auto& lobby = conn.game->lobbies.at( lobbyId );
		lobby.addPeer( conn.peerId );
		auto& memberDetails = lobby.members.at( conn.peerId );

		lobby.attributes["lobby"] = "one";

		memberDetails.attributes["test"] = "some";
		memberDetails.attributes["another"] = "one";
		CHECK( memberDetails.attributes.size() == 2 );

		GIVEN( "and merging attributes" ) {
			ha_requestId reqId = humblenet::sendLobbyMemberUpdate( &conn, lobbyId, conn.peerId,
																   HumblePeer::AttributeMode::Merge,
																   attributes );

			conn.processMsg( messages[0].message );

			THEN( "it generates a lobbyUpdate message for each member" ) {
				REQUIRE( messages.size() == 3 );
				{
					auto& curMsg = messages[1];

					REQUIRE( curMsg.message->message_type() == HumblePeer::MessageType::LobbyMemberDidUpdate );

					auto msg = reinterpret_cast<const HumblePeer::LobbyMemberDidUpdate*>(curMsg.message->message());

					CHECK ( curMsg.peer == conn.peerId );

					CHECK( curMsg.message->requestId() == reqId );

					CHECK( msg->lobbyId() == lobbyId );

					CHECK( msg->peerId() == conn.peerId );

					REQUIRE( msg->attributeSet() != nullptr );

					REQUIRE( msg->attributeSet()->attributes() != nullptr );

					CHECK( msg->attributeSet()->mode() == HumblePeer::AttributeMode::Merge );

					auto attribs = msg->attributeSet()->attributes();

					REQUIRE( attribs->size() == 2 );

					auto v = attribs->LookupByKey( "key" );

					CHECK( v->value()->str() == "value" );

					v = attribs->LookupByKey( "test" );

					CHECK( v->value()->str() == "two" );
				}

				{
					auto& curMsg = messages[2];

					REQUIRE( curMsg.message->message_type() == HumblePeer::MessageType::LobbyMemberDidUpdate );

					auto msg = reinterpret_cast<const HumblePeer::LobbyMemberDidUpdate*>(curMsg.message->message());

					CHECK ( curMsg.peer == connAlt.peerId );

					CHECK( curMsg.message->requestId() == 0 );

					CHECK( msg->lobbyId() == lobbyId );

					CHECK( msg->peerId() == conn.peerId );

					REQUIRE( msg->attributeSet() != nullptr );

					REQUIRE( msg->attributeSet()->attributes() != nullptr );

					CHECK( msg->attributeSet()->mode() == HumblePeer::AttributeMode::Merge );

					auto attribs = msg->attributeSet()->attributes();

					REQUIRE( attribs->size() == 2 );

					auto v = attribs->LookupByKey( "key" );

					CHECK( v->value()->str() == "value" );

					v = attribs->LookupByKey( "test" );

					CHECK( v->value()->str() == "two" );
				}
			}

			THEN( "it does not update the attributes on the lobby" ) {
				CHECK( lobby.attributes.size() == 1 );
				CHECK( lobby.attributes["lobby"] == "one" );
			}

			THEN( "it updates the attributes on the lobby member" ) {
				CHECK( memberDetails.attributes.size() == 3 );

				CHECK( memberDetails.attributes["key"] == "value" );
				CHECK( memberDetails.attributes["test"] == "two" );
				CHECK( memberDetails.attributes["another"] == "one" );
			}
		}

		GIVEN( "and replacing attributes" ) {
			ha_requestId reqId = humblenet::sendLobbyMemberUpdate( &conn, lobbyId, conn.peerId,
																   HumblePeer::AttributeMode::Replace,
																   attributes );

			conn.processMsg( messages[0].message );

			THEN( "it generates a lobbyUpdate message for each member" ) {
				REQUIRE( messages.size() == 3 );

				{
					auto& curMsg = messages[1];

					REQUIRE( curMsg.message->message_type() == HumblePeer::MessageType::LobbyMemberDidUpdate );

					auto msg = reinterpret_cast<const HumblePeer::LobbyMemberDidUpdate*>(curMsg.message->message());

					CHECK ( curMsg.peer == conn.peerId );

					CHECK( curMsg.message->requestId() == reqId );

					CHECK( msg->lobbyId() == lobbyId );

					CHECK( msg->peerId() == conn.peerId );

					REQUIRE( msg->attributeSet() != nullptr );

					REQUIRE( msg->attributeSet()->attributes() != nullptr );

					CHECK( msg->attributeSet()->mode() == HumblePeer::AttributeMode::Replace );

					auto attribs = msg->attributeSet()->attributes();

					REQUIRE( attribs->size() == 2 );

					auto v = attribs->LookupByKey( "key" );

					CHECK( v->value()->str() == "value" );

					v = attribs->LookupByKey( "test" );

					CHECK( v->value()->str() == "two" );
				}

				{
					auto& curMsg = messages[2];

					REQUIRE( curMsg.message->message_type() == HumblePeer::MessageType::LobbyMemberDidUpdate );

					auto msg = reinterpret_cast<const HumblePeer::LobbyMemberDidUpdate*>(curMsg.message->message());

					CHECK ( curMsg.peer == connAlt.peerId );

					CHECK( curMsg.message->requestId() == 0 );

					CHECK( msg->lobbyId() == lobbyId );

					CHECK( msg->peerId() == conn.peerId );

					REQUIRE( msg->attributeSet() != nullptr );

					REQUIRE( msg->attributeSet()->attributes() != nullptr );

					CHECK( msg->attributeSet()->mode() == HumblePeer::AttributeMode::Replace );

					auto attribs = msg->attributeSet()->attributes();

					REQUIRE( attribs->size() == 2 );

					auto v = attribs->LookupByKey( "key" );

					CHECK( v->value()->str() == "value" );

					v = attribs->LookupByKey( "test" );

					CHECK( v->value()->str() == "two" );
				}
			}

			THEN( "it does not update the attributes on the lobby" ) {
				CHECK( lobby.attributes.size() == 1 );
				CHECK( lobby.attributes["lobby"] == "one" );
			}

			THEN( "it updates the attributes on the lobby" ) {
				CHECK( memberDetails.attributes.size() == 2 );

				CHECK( memberDetails.attributes["key"] == "value" );
				CHECK( memberDetails.attributes["test"] == "two" );
			}
		}

		GIVEN( "and a blank value for an attribute" ) {
			attributes["test"] = "";

			ha_requestId reqId = humblenet::sendLobbyMemberUpdate( &conn, lobbyId, conn.peerId,
																   HumblePeer::AttributeMode::Merge,
																   attributes );

			conn.processMsg( messages[0].message );

			THEN( "it clears that attribute" ) {
				CHECK( memberDetails.attributes.size() == 2 );

				CHECK( memberDetails.attributes["key"] == "value" );
				CHECK( memberDetails.attributes["another"] == "one" );
			}
		}
	}

	GIVEN( "I am not the member being updated" ) {
		auto lobbyId = conn.game->generateNewLobbyId();
		conn.game->lobbies.insert(
				std::make_pair( lobbyId, humblenet::Lobby( lobbyId, conn.peerId, HUMBLENET_LOBBY_TYPE_PUBLIC, 4 )));
		auto& lobby = conn.game->lobbies.at( lobbyId );
		lobby.addPeer( connAlt.peerId );
		lobby.attributes["lobby"] = "one";
		auto& memberDetails = lobby.members.at( connAlt.peerId );
		memberDetails.attributes["member"] = "two";

		ha_requestId reqId = humblenet::sendLobbyMemberUpdate( &conn, lobbyId, connAlt.peerId,
															   HumblePeer::AttributeMode::Merge,
															   attributes );

		conn.processMsg( messages[0].message );

		THEN( "it generates a lobby Error message" ) {
			REQUIRE( messages.size() == 2 );

			REQUIRE( messages[1].message->message_type() == HumblePeer::MessageType::LobbyMemberUpdateError );

			auto msg = reinterpret_cast<const HumblePeer::Error*>(messages[1].message->message());

			CHECK ( messages[1].peer == conn.peerId );

			CHECK( messages[1].message->requestId() == reqId );

			CHECK( msg->error()->str() == "Lobby member can only be updated by the member" );
		}

		THEN( "it does not update the attributes on the lobby" ) {
			CHECK( lobby.attributes.size() == 1 );
			CHECK( lobby.attributes["lobby"] == "one" );
		}

		THEN( "it does not change the lobby member attributes" ) {
			CHECK( memberDetails.attributes.size() == 1 );
			CHECK( memberDetails.attributes["member"] == "two" );
		}
	}
}

// endregion