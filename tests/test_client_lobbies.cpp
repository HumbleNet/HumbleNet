#define CATCH_CONFIG_MAIN

#include <catch2/catch.hpp>

#include "humblepeer.h"
#include "humblenet_lobbies.h"
#include "humblenet_p2p_internal.h"

HumbleNetState humbleNetState;

struct Message {
	std::vector<uint8_t> buffer;

	const humblenet::HumblePeer::Message* message = nullptr;
};
std::vector<Message> messages;

std::vector<std::string> calls;

// region Define stubs

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

		item.buffer.resize( length );
		memcpy( item.buffer.data(), buff, length );

		parseMessage( item.buffer, processMessage, &item );

		return true;
	}
}

// endregion

namespace HumblePeer = humblenet::HumblePeer;

class ConnectedPeerFixture {
public:
	ConnectedPeerFixture()
	{
		humbleNetState = HumbleNetState();
		messages.clear();
		calls.clear();
	}

	~ConnectedPeerFixture() = default;

	template<typename Function>
	bool runSetup(Function block)
	{
		block();
		messages.clear();
		calls.clear();

		return true;
	}
};

SCENARIO_METHOD( ConnectedPeerFixture, "humblenet_lobby_create", "[create]" )
{
	GIVEN( "a create lobby call" ) {
		auto req = humblenet_lobby_create( HUMBLENET_LOBBY_TYPE_PRIVATE, 4 );

		THEN( "it returns a request Id" ) {
			CHECK( req != 0 );
		}

		THEN( "it sends a lobby create event to the peer server" ) {
			REQUIRE( messages.size() == 1 );

			CHECK( messages[0].message->requestId() == req );

			REQUIRE( messages[0].message->message_type() == HumblePeer::MessageType::LobbyCreate );

			auto msg = reinterpret_cast<const HumblePeer::LobbyCreate*>(messages[0].message->message());

			CHECK( msg->lobbyType() == HUMBLENET_LOBBY_TYPE_PRIVATE );

			CHECK( msg->maxMembers() == 4 );
		}
	}

	GIVEN( "an invalid lobby type" ) {
		auto req = humblenet_lobby_create( HUMBLENET_LOBBY_TYPE_UNKNOWN, 4 );

		THEN( "it returns a request Id" ) {
			CHECK( req != 0 );
		}

		THEN( "it sends a lobby create event for a private lobby" ) {
			REQUIRE( messages.size() == 1 );

			CHECK( messages[0].message->requestId() == req );

			REQUIRE( messages[0].message->message_type() == HumblePeer::MessageType::LobbyCreate );

			auto msg = reinterpret_cast<const HumblePeer::LobbyCreate*>(messages[0].message->message());

			CHECK( msg->lobbyType() == HUMBLENET_LOBBY_TYPE_PRIVATE );

			CHECK( msg->maxMembers() == 4 );
		}
	}
}

SCENARIO_METHOD( ConnectedPeerFixture, "humblenet_lobby_join", "[create]" )
{
	GIVEN( "join a lobby" ) {
		auto req = humblenet_lobby_join( 0xdeadbeef );

		THEN( "it sends a lobby join event to the peer server" ) {
			REQUIRE( messages.size() == 1 );

			CHECK( messages[0].message->requestId() == req );

			REQUIRE( messages[0].message->message_type() == HumblePeer::MessageType::LobbyJoin );

			auto msg = reinterpret_cast<const HumblePeer::LobbyJoin*>(messages[0].message->message());

			CHECK( msg->lobbyId() == 0xdeadbeef );
		}
	}
}

SCENARIO_METHOD( ConnectedPeerFixture, "humblenet_lobby_leave", "[create]" )
{
	GIVEN( "leave a lobby" ) {
		auto req = humblenet_lobby_leave( 0xdeadbeef );

		THEN( "it sends a lobby leave event to the peer server" ) {
			REQUIRE( messages.size() == 1 );

			CHECK( messages[0].message->requestId() == req );

			REQUIRE( messages[0].message->message_type() == HumblePeer::MessageType::LobbyLeave );

			auto msg = reinterpret_cast<const HumblePeer::LobbyLeave*>(messages[0].message->message());

			CHECK( msg->lobbyId() == 0xdeadbeef );
		}
	}
}

SCENARIO_METHOD( ConnectedPeerFixture, "humblenet_lobby_owner", "[read]" )
{
	LobbyId lobbyId = 0xdeadbeef;

	GIVEN( "a joined lobby" ) {
		runSetup( [&]() {
			auto r = humbleNetState.lobbies.emplace( std::piecewise_construct,
													 std::forward_as_tuple( lobbyId ),
													 std::forward_as_tuple( lobbyId, 0xd00d,
																			HUMBLENET_LOBBY_TYPE_PRIVATE,
																			4 ));
			r.first->second.addPeer( 0xd00d1 );
		} );

		auto owner = humblenet_lobby_owner( lobbyId );

		THEN( "it returns the owner" ) {
			CHECK( owner == 0xd00d );
		}
	}

	GIVEN( "a unjoined lobby" ) {
		auto owner = humblenet_lobby_owner( lobbyId );

		THEN( "it returns 0" ) {
			CHECK( owner == 0 );
		}
	}
}

SCENARIO_METHOD( ConnectedPeerFixture, "humblenet_lobby_max_members", "[read]" )
{
	LobbyId lobbyId = 0xdeadbeef;

	GIVEN( "a joined lobby" ) {
		runSetup( [&]() {
			auto r = humbleNetState.lobbies.emplace( std::piecewise_construct,
													 std::forward_as_tuple( lobbyId ),
													 std::forward_as_tuple( lobbyId, 0xd00d,
																			HUMBLENET_LOBBY_TYPE_PRIVATE,
																			4 ));
			r.first->second.addPeer( 0xd00d1 );
		} );

		auto num = humblenet_lobby_max_members( lobbyId );

		THEN( "it returns the max members" ) {
			CHECK( num == 4 );
		}
	}

	GIVEN( "a unjoined lobby" ) {
		auto num = humblenet_lobby_max_members( lobbyId );

		THEN( "it returns 0" ) {
			CHECK( num == 0 );
		}
	}
}

SCENARIO_METHOD( ConnectedPeerFixture, "humblenet_lobby_type", "[read]" )
{
	LobbyId lobbyId = 0xdeadbeef;

	GIVEN( "a joined lobby" ) {
		runSetup( [&]() {
			auto r = humbleNetState.lobbies.emplace( std::piecewise_construct,
													 std::forward_as_tuple( lobbyId ),
													 std::forward_as_tuple( lobbyId, 0xd00d,
																			HUMBLENET_LOBBY_TYPE_PRIVATE,
																			4 ));
			r.first->second.addPeer( 0xd00d1 );
		} );

		auto ret = humblenet_lobby_type( lobbyId );

		THEN( "it returns the max members" ) {
			CHECK( ret == HUMBLENET_LOBBY_TYPE_PRIVATE );
		}
	}

	GIVEN( "a unjoined lobby" ) {
		auto ret = humblenet_lobby_type( lobbyId );

		THEN( "it returns UNKNOWN" ) {
			CHECK( ret == HUMBLENET_LOBBY_TYPE_UNKNOWN );
		}
	}
}

SCENARIO_METHOD( ConnectedPeerFixture, "humblenet_lobby_member_count", "[read]" )
{
	LobbyId lobbyId = 0xdeadbeef;

	GIVEN( "a joined lobby" ) {
		runSetup( [&]() {
			auto r = humbleNetState.lobbies.emplace( std::piecewise_construct,
													 std::forward_as_tuple( lobbyId ),
													 std::forward_as_tuple( lobbyId, 0xd00d,
																			HUMBLENET_LOBBY_TYPE_PRIVATE,
																			4 ));
			r.first->second.addPeer( 0xd00d1 );
		} );

		auto num = humblenet_lobby_member_count( lobbyId );

		THEN( "it returns the number of members" ) {
			CHECK( num == 2 );
		}
	}

	GIVEN( "a unjoined lobby" ) {
		auto num = humblenet_lobby_member_count( lobbyId );

		THEN( "it returns 0" ) {
			CHECK( num == 0 );
		}
	}
}

SCENARIO_METHOD( ConnectedPeerFixture, "humblenet_lobby_get_members", "[read]" )
{
	LobbyId lobbyId = 0xdeadbeef;

	GIVEN( "a joined lobby" ) {
		runSetup( [&]() {
			auto r = humbleNetState.lobbies.emplace( std::piecewise_construct,
													 std::forward_as_tuple( lobbyId ),
													 std::forward_as_tuple( lobbyId, 0xd00d,
																			HUMBLENET_LOBBY_TYPE_PRIVATE,
																			4 ));
			r.first->second.addPeer( 0xd00d1 );
			r.first->second.addPeer( 0xd00d2 );
		} );

		GIVEN( "a peer list that is big enough" ) {
			std::vector<PeerId> peers;
			peers.resize( 3 );

			auto num = humblenet_lobby_get_members( lobbyId, peers.data(), peers.size());

			THEN( "it returns the number of members copied" ) {
				CHECK( num == 3 );
			}
		}

		GIVEN( "a peer list that is too small" ) {
			std::vector<PeerId> peers;
			peers.resize( 2 );

			auto num = humblenet_lobby_get_members( lobbyId, peers.data(), peers.size());

			THEN( "it returns only 2 members copied" ) {
				CHECK( num == 2 );
			}
		}
	}

	GIVEN( "a unjoined lobby" ) {
		PeerId peers[2];
		auto num = humblenet_lobby_get_members( lobbyId, peers, sizeof( peers ) / sizeof( peers[0] ));

		THEN( "it returns 0" ) {
			CHECK( num == 0 );
		}
	}
}

SCENARIO_METHOD( ConnectedPeerFixture, "humblenet_lobby_get_attributes", "[read]" )
{
	LobbyId lobbyId = 0xdeadbeef;

	GIVEN( "a joined lobby" ) {
		runSetup( [&]() {
			auto r = humbleNetState.lobbies.emplace( std::piecewise_construct,
													 std::forward_as_tuple( lobbyId ),
													 std::forward_as_tuple( lobbyId, 0xd00d,
																			HUMBLENET_LOBBY_TYPE_PRIVATE,
																			4 ));
			r.first->second.applyAttributes( {{"key", "value"}} );
		} );

		auto attrs = humblenet_lobby_get_attributes( lobbyId );

		THEN( "it returns an AttributeMap" ) {
			REQUIRE( attrs != nullptr );

			CHECK( humblenet_attributes_size( attrs ) == 1 );
		}

		humblenet_attributes_free( attrs );
	}

	GIVEN( "a unjoined lobby" ) {
		auto attrs = humblenet_lobby_get_attributes( lobbyId );

		THEN( "it returns nullptr" ) {
			CHECK( attrs == nullptr );
		}
	}
}

SCENARIO_METHOD( ConnectedPeerFixture, "humblenet_lobby_get_member_attributes", "[read]" )
{
	LobbyId lobbyId = 0xdeadbeef;
	LobbyId peerId = 0xd00d;

	GIVEN( "a joined lobby where the peer is a member" ) {
		runSetup( [&]() {
			auto r = humbleNetState.lobbies.emplace( std::piecewise_construct,
													 std::forward_as_tuple( lobbyId ),
													 std::forward_as_tuple( lobbyId, peerId,
																			HUMBLENET_LOBBY_TYPE_PRIVATE,
																			4 ));
			r.first->second.members.begin()->second.applyAttributes( {{"key", "value"}} );
		} );

		auto attrs = humblenet_lobby_get_member_attributes( lobbyId, peerId );

		THEN( "it returns an AttributeMap" ) {
			REQUIRE( attrs != nullptr );

			CHECK( humblenet_attributes_size( attrs ) == 1 );
		}

		humblenet_attributes_free( attrs );
	}

	GIVEN( "a joined lobby where the peer is not a member" ) {
		runSetup( [&]() {
			auto r = humbleNetState.lobbies.emplace( std::piecewise_construct,
													 std::forward_as_tuple( lobbyId ),
													 std::forward_as_tuple( lobbyId, peerId + 1,
																			HUMBLENET_LOBBY_TYPE_PRIVATE,
																			4 ));
			r.first->second.members.begin()->second.applyAttributes( {{"key", "value"}} );
		} );

		auto attrs = humblenet_lobby_get_member_attributes( lobbyId, peerId );

		THEN( "it returns nullptr" ) {
			CHECK( attrs == nullptr );
		}
	}

	GIVEN( "a unjoined lobby" ) {
		auto attrs = humblenet_lobby_get_member_attributes( lobbyId, peerId );

		THEN( "it returns nullptr" ) {
			CHECK( attrs == nullptr );
		}
	}
}

SCENARIO_METHOD( ConnectedPeerFixture, "humblenet_lobby_set_max_members", "[write]" )
{
	LobbyId lobbyId = 0xdeadbeef;

	GIVEN( "a joined lobby" ) {
		runSetup( [&]() {
			auto r = humbleNetState.lobbies.emplace( std::piecewise_construct,
													 std::forward_as_tuple( lobbyId ),
													 std::forward_as_tuple( lobbyId, 0xd00d,
																			HUMBLENET_LOBBY_TYPE_PRIVATE,
																			4 ));
			r.first->second.addPeer( 0xd00d1 );
		} );

		auto req = humblenet_lobby_set_max_members( lobbyId, 8 );

		THEN( "it returns the request ID" ) {
			CHECK( req != 0 );
		}

		THEN( "it sends a lobby update request to the peer server" ) {
			REQUIRE( messages.size() == 1 );

			CHECK( messages[0].message->requestId() == req );

			REQUIRE( messages[0].message->message_type() == HumblePeer::MessageType::LobbyUpdate );

			auto msg = reinterpret_cast<const HumblePeer::LobbyUpdate*>(messages[0].message->message());

			CHECK( msg->lobbyId() == lobbyId );

			CHECK( msg->lobbyType() == HUMBLENET_LOBBY_TYPE_UNKNOWN );

			CHECK( msg->maxMembers() == 8 );

			CHECK( msg->attributeSet() == nullptr );
		}
	}

	GIVEN( "a unjoined lobby" ) {
		auto req = humblenet_lobby_set_max_members( lobbyId, 8 );

		THEN( "it returns 0" ) {
			CHECK( req == 0 );
		}
	}
}

SCENARIO_METHOD( ConnectedPeerFixture, "humblenet_lobby_set_type", "[write]" )
{
	LobbyId lobbyId = 0xdeadbeef;

	GIVEN( "a joined lobby" ) {
		runSetup( [&]() {
			auto r = humbleNetState.lobbies.emplace( std::piecewise_construct,
													 std::forward_as_tuple( lobbyId ),
													 std::forward_as_tuple( lobbyId, 0xd00d,
																			HUMBLENET_LOBBY_TYPE_PRIVATE,
																			4 ));
			r.first->second.addPeer( 0xd00d1 );
		} );

		auto req = humblenet_lobby_set_type( lobbyId, HUMBLENET_LOBBY_TYPE_PUBLIC );

		THEN( "it returns the request ID" ) {
			CHECK( req != 0 );
		}

		THEN( "it sends a lobby update request to the peer server" ) {
			REQUIRE( messages.size() == 1 );

			CHECK( messages[0].message->requestId() == req );

			REQUIRE( messages[0].message->message_type() == HumblePeer::MessageType::LobbyUpdate );

			auto msg = reinterpret_cast<const HumblePeer::LobbyUpdate*>(messages[0].message->message());

			CHECK( msg->lobbyId() == lobbyId );

			CHECK( msg->lobbyType() == HUMBLENET_LOBBY_TYPE_PUBLIC );

			CHECK( msg->maxMembers() == 0 );

			CHECK( msg->attributeSet() == nullptr );
		}
	}

	GIVEN( "a unjoined lobby" ) {
		auto req = humblenet_lobby_set_type( lobbyId, HUMBLENET_LOBBY_TYPE_PUBLIC );

		THEN( "it returns 0" ) {
			CHECK( req == 0 );
		}
	}
}

SCENARIO_METHOD( ConnectedPeerFixture, "humblenet_lobby_set_attributes", "[write]" )
{
	LobbyId lobbyId = 0xdeadbeef;

	auto attributes = humblenet_attributes_new();
	humblenet_attributes_set( attributes, "key", "value" );

	GIVEN( "a joined lobby" ) {
		runSetup( [&]() {
			auto r = humbleNetState.lobbies.emplace( std::piecewise_construct,
													 std::forward_as_tuple( lobbyId ),
													 std::forward_as_tuple( lobbyId, 0xd00d,
																			HUMBLENET_LOBBY_TYPE_PRIVATE,
																			4 ));
			r.first->second.addPeer( 0xd00d1 );
		} );

		auto req = humblenet_lobby_set_attributes( lobbyId, attributes, true );

		THEN( "it returns the request ID" ) {
			CHECK( req != 0 );
		}

		THEN( "it sends a lobby update request to the peer server" ) {
			REQUIRE( messages.size() == 1 );

			CHECK( messages[0].message->requestId() == req );

			REQUIRE( messages[0].message->message_type() == HumblePeer::MessageType::LobbyUpdate );

			auto msg = reinterpret_cast<const HumblePeer::LobbyUpdate*>(messages[0].message->message());

			CHECK( msg->lobbyId() == lobbyId );

			CHECK( msg->lobbyType() == HUMBLENET_LOBBY_TYPE_UNKNOWN );

			CHECK( msg->maxMembers() == 0 );

			CHECK( msg->attributeSet() != nullptr );

			auto attribSet = msg->attributeSet();

			CHECK( attribSet->mode() == HumblePeer::AttributeMode::Replace );

			REQUIRE( attribSet->attributes() != nullptr );

			auto attribs = attribSet->attributes();

			REQUIRE( attribs->size() == 1);

			auto v = attribs->LookupByKey( "key" );

			CHECK( v->value()->str() == "value" );
		}
	}

	GIVEN( "a unjoined lobby" ) {
		auto req = humblenet_lobby_set_type( lobbyId, HUMBLENET_LOBBY_TYPE_PUBLIC );

		THEN( "it returns 0" ) {
			CHECK( req == 0 );
		}
	}

	humblenet_attributes_free( attributes );
}

SCENARIO_METHOD( ConnectedPeerFixture, "humblenet_lobby_set_member_attributes", "[write]" )
{
	LobbyId lobbyId = 0xdeadbeef;

	auto attributes = humblenet_attributes_new();
	humblenet_attributes_set( attributes, "key", "value" );

	GIVEN( "a joined lobby" ) {
		runSetup( [&]() {
			auto r = humbleNetState.lobbies.emplace( std::piecewise_construct,
													 std::forward_as_tuple( lobbyId ),
													 std::forward_as_tuple( lobbyId, 0xd00d,
																			HUMBLENET_LOBBY_TYPE_PRIVATE,
																			4 ));
			r.first->second.addPeer( 0xd00d1 );
		} );

		auto req = humblenet_lobby_set_member_attributes( lobbyId, 0xd00d, attributes, true );

		THEN( "it returns the request ID" ) {
			CHECK( req != 0 );
		}

		THEN( "it sends a lobby member update request to the peer server" ) {
			REQUIRE( messages.size() == 1 );

			CHECK( messages[0].message->requestId() == req );

			REQUIRE( messages[0].message->message_type() == HumblePeer::MessageType::LobbyMemberUpdate );

			auto msg = reinterpret_cast<const HumblePeer::LobbyMemberUpdate*>(messages[0].message->message());

			CHECK( msg->lobbyId() == lobbyId );

			CHECK( msg->peerId() == 0xd00d );

			CHECK( msg->attributeSet() != nullptr );

			auto attribSet = msg->attributeSet();

			CHECK( attribSet->mode() == HumblePeer::AttributeMode::Replace );

			REQUIRE( attribSet->attributes() != nullptr );

			auto attribs = attribSet->attributes();

			REQUIRE( attribs->size() == 1);

			auto v = attribs->LookupByKey( "key" );

			CHECK( v->value()->str() == "value" );
		}
	}

	GIVEN( "a unjoined lobby" ) {
		auto req = humblenet_lobby_set_type( lobbyId, HUMBLENET_LOBBY_TYPE_PUBLIC );

		THEN( "it returns 0" ) {
			CHECK( req == 0 );
		}
	}

	humblenet_attributes_free( attributes );
}