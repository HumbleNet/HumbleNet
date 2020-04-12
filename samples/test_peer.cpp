#include "humblenet_p2p.h"
#include "humblenet_events.h"

#ifdef EMSCRIPTEN
#include <emscripten.h>
#else
#define EMSCRIPTEN_KEEPALIVE

#include <atomic>
#include <thread>

#endif

#include <locale>
#include <iostream>
#include <unordered_set>
#include <algorithm>
#include <iterator>

#include "test_peer_generated.h"

static PeerId myPeer = 0;
static bool connected = false;
const uint8_t CHANNEL = 52;

std::unordered_set<PeerId> pendingPeers;
std::unordered_set<PeerId> connectedPeers;

extern "C" {
EMSCRIPTEN_KEEPALIVE void connectToPeer(PeerId peer);
EMSCRIPTEN_KEEPALIVE void sendChat(const char* message);
}

void disconnectPeer(PeerId otherPeer)
{
	std::cout << "Disconnecting peer " << otherPeer << std::endl;
	pendingPeers.erase( otherPeer );
	connectedPeers.erase( otherPeer );
	humblenet_p2p_disconnect( otherPeer );
}

void connectToPeer(PeerId peer)
{
	if (peer != myPeer && pendingPeers.find( peer ) == pendingPeers.end() &&
		connectedPeers.find( peer ) == connectedPeers.end()) {
		std::cout << "Connecting to peer " << peer << std::endl;
		pendingPeers.insert( peer );
	}
}

void connectedToPeer(PeerId peer)
{
	std::cout << "Connected to peer " << peer << std::endl;

	pendingPeers.erase( peer );
	connectedPeers.insert( peer );
}

flatbuffers::Offset<flatbuffers::Vector<uint32_t>> buildKnownPeers(flatbuffers::FlatBufferBuilder& builder)
{
	std::vector<PeerId> kp;

	std::copy( connectedPeers.begin(), connectedPeers.end(), std::back_inserter( kp ));
	std::copy( pendingPeers.begin(), pendingPeers.end(), std::back_inserter( kp ));

	return builder.CreateVector( kp );
}

void processKnownPeers(const flatbuffers::Vector<uint32_t>* peers)
{
	if (peers) {
		std::cout << "Processing KPs: " << peers->Length() << ": ";

		for (size_t i = 0; i < peers->Length(); ++i) {
			PeerId newPeer = peers->Get( i );
			std::cout << newPeer << " ";
			connectToPeer( newPeer );
		}
		std::cout << std::endl;
	}
}

int sendMessage(PeerId peer, const flatbuffers::FlatBufferBuilder& buffer)
{
	int ret = humblenet_p2p_sendto( buffer.GetBufferPointer(), buffer.GetSize(), peer, SEND_RELIABLE, CHANNEL );
	return ret;
}

void sendChat(const char* message)
{
	flatbuffers::FlatBufferBuilder builder;
	auto chatmsg = builder.CreateString( message );
	auto hello = TestClient::CreateChat( builder, chatmsg );
	auto msg = TestClient::CreateMessage( builder, TestClient::MessageSwitch_Chat, hello.Union());
	builder.Finish( msg );

	std::unordered_set<PeerId> toRemove;

	for (auto peer : connectedPeers) {
		int ret = sendMessage( peer, builder );
		if (ret < 0) {
			toRemove.insert( peer );
		}
	}
	for (auto p : toRemove) {
		disconnectPeer( p );
	}
}

void sendChat(const std::string& message)
{
	sendChat( message.c_str());
}

bool sendHello(PeerId peer, bool isResponse = false)
{
	std::cout << "Try hello " << peer << std::endl;

	flatbuffers::FlatBufferBuilder builder;
	auto hello = TestClient::CreateHelloPeer( builder, buildKnownPeers( builder ), isResponse );
	auto msg = TestClient::CreateMessage( builder, TestClient::MessageSwitch_HelloPeer, hello.Union());
	builder.Finish( msg );

	return sendMessage( peer, builder ) >= 0;
}


void processMessage(PeerId peer, const char* buff, size_t length)
{
	flatbuffers::Verifier v( reinterpret_cast<const uint8_t*>(buff), length );

	if (!TestClient::VerifyMessageBuffer( v )) {
		std::cout << "Invalid buffer received" << std::endl;
		return;
	}
	auto message = TestClient::GetMessage( buff );

	auto message_type = message->message_type();

	if (message_type == TestClient::MessageSwitch_HelloPeer) {
		auto hello = reinterpret_cast<const TestClient::HelloPeer*>(message->message());
		std::cout << "Received hello from peer " << peer << " response " << hello->is_response() << std::endl;
		connectedToPeer( peer );

		processKnownPeers( hello->peers());


		if (!hello->is_response()) {
			sendHello( peer, true );
		}
	} else if (message_type == TestClient::MessageSwitch_Chat) {
		auto chat = reinterpret_cast<const TestClient::Chat*>(message->message());
		std::cout << "Peer " << peer << ": " << chat->message()->str() << std::endl;
	} else {
		std::cout << "Unexpected message type received " << message_type << std::endl;
	}
}

// input helpers for desktop builds

#ifdef EMSCRIPTEN
void printHelp()
{
}
#else

std::string toLower(const std::string& in)
{
	std::locale loc;
	std::string out;

	for (auto c: in) {
		out.push_back( std::tolower( c, loc ));
	}
	return out;
}

void printHelp()
{
	std::cout << "Available commands:" << std::endl;
	std::cout << "  /peer #       : Connect to a peer by ID" << std::endl;
	std::cout << "  /disconnect # : Disconnect from a peer by ID" << std::endl;
	std::cout << "  /alias name   : Register an alias of name for this peer" << std::endl;
	std::cout << "  /unalias name : Unregister a specific alias name for this peer" << std::endl;
	std::cout << "  /unalias      : Unregister all aliases for this peer" << std::endl;
	std::cout << "  /lookup name  : Lookup an alias" << std::endl;
	std::cout << "  /quit         : Quit the application" << std::endl;
	std::cout << "  /help         : Print this help" << std::endl;
}

void readInput(std::atomic<bool>& run)
{
	std::string buffer;

	while (run.load()) {
		std::getline( std::cin, buffer );

		std::string command = toLower( buffer );

		if (command == "/quit") {
			run.store( false );
		} else if (command.compare( 0, 6, "/peer " ) == 0) {
			PeerId peer = strtoul( &command[6], NULL, 10 );
			if (peer != 0) {
				connectToPeer( peer );
			} else {
				connectToPeer( humblenet_p2p_virtual_peer_for_alias( &command[6] ));
			}
		} else if (command.compare( 0, 7, "/alias " ) == 0) {
			humblenet_p2p_register_alias( &command[7] );
		} else if (command.compare( 0, 8, "/lookup " ) == 0) {
			humblenet_p2p_lookup_alias( &command[8] );
		} else if (command == "/unalias") {
			humblenet_p2p_unregister_alias( nullptr );
		} else if (command.compare( 0, 9, "/unalias " ) == 0) {
			humblenet_p2p_unregister_alias( &command[9] );
		} else if (command.compare( 0, 12, "/disconnect " ) == 0) {
			PeerId peer = strtoul( &command[12], NULL, 10 );
			if (peer != 0) {
				disconnectPeer( peer );
			} else {
				disconnectPeer( humblenet_p2p_virtual_peer_for_alias( &command[12] ));
			}
		} else if (command == "/help") {
			printHelp();
		} else if (command.compare( 0, 1, "/" ) == 0) {
			std::cout << "Unknown command: " << command << std::endl;
			std::cout << "Use /help to see available commands: " << std::endl;
		} else {
			sendChat( buffer );
		}
	}
}

#endif

static time_t last = 0;

void main_loop()
{
	humblenet_p2p_wait( 100 );

	HumbleNet_Event event;
	while (humblenet_event_poll( &event )) {
		switch (event.type) {
			case HUMBLENET_EVENT_P2P_CONNECTED:
				std::cout << "P2P connected" << std::endl;
				break;
			case HUMBLENET_EVENT_P2P_ASSIGN_PEER:
				std::cout << "P2P Peer assigned: " << event.peer.peer_id << std::endl;
				myPeer = event.peer.peer_id;
				connected = true;
				printHelp();
				break;
//			case HUMBLENET_EVENT_PEER_CONNECTED:
//			case HUMBLENET_EVENT_PEER_DISCONNECTED:
			case HUMBLENET_EVENT_PEER_REJECTED:
				std::cout << "Peer rejected: " << event.peer.peer_id << std::endl;
				break;
			case HUMBLENET_EVENT_PEER_NOT_FOUND:
				std::cout << "Peer not found: " << event.peer.peer_id << std::endl;
				break;
			case HUMBLENET_EVENT_ALIAS_RESOLVED:
				std::cout << "Alias resolved: " << event.peer.peer_id << std::endl;
				break;
			case HUMBLENET_EVENT_ALIAS_NOT_FOUND:
				std::cout << "Alias not found: " << event.error.error << std::endl;
				break;
			case HUMBLENET_EVENT_ALIAS_REGISTER_SUCCESS:
				std::cout << "Alias registered" << std::endl;
				break;
			case HUMBLENET_EVENT_ALIAS_REGISTER_ERROR:
				std::cout << "Alias registration error: " << event.error.error << std::endl;
				break;
			default:
				std::cout << "Unhandled event: 0x" << std::hex << event.type << std::endl;
				break;
		}
	}

	// Poll if connected
	if (connected) {
		std::unordered_set<PeerId> toRemove;

		bool done = false;
		do {
			char buff[1024];
			PeerId otherPeer = 0;
			int ret = humblenet_p2p_recvfrom( buff, sizeof( buff ), &otherPeer, CHANNEL );
			if (ret < 0) {
				if (otherPeer != 0) {
					toRemove.insert( otherPeer );
				} else {
					done = true;
				}
			} else if (ret > 0) {
				processMessage( otherPeer, buff, ret );
			} else {
				done = true;
			}
		} while (!done);

		time_t now = time( NULL );
		if (now > last) {
			for (auto p: pendingPeers) {
				if (!sendHello( p )) {
					toRemove.insert( p );
				}
			}
			last = now;
		}

		// disconnect lost peers
		for (auto p: toRemove) {
			disconnectPeer( p );
		}
	}
}

int main(int argc, char* argv[])
{
	std::string peerServer = HUMBLENET_SERVER_URL;
	if (argc == 2) {
		peerServer = argv[1];
	}
	humblenet_p2p_init( peerServer.c_str(), "client_token", "client_secret", NULL );

#ifdef EMSCRIPTEN
	EM_ASM(
		var canvasParent = Module.canvas.parentNode;
		var wrap = document.createElement('div');
		canvasParent.insertBefore(wrap, Module.canvas);

		// Connect
		var connectToPeer = Module.cwrap('connectToPeer', 'void', ['number']);

		var peerInput = document.createElement('input');
		peerInput.setAttribute('type','number');
		peerInput.setAttribute('placeholder', 'Peer #');

		wrap.appendChild(peerInput);

		var button = document.createElement('button');
		button.innerText = 'Connect';
		button.setAttribute('type','button');
		button.addEventListener('click', function(e) {
			var val = parseInt(peerInput.value);
			if (val > 0) {
				connectToPeer(val);
			}
		});

		wrap.appendChild(button);

		wrap.appendChild(document.createElement('br'));

		// Chat
		var sendChat = Module.cwrap('sendChat', 'void', ['string']);

		var chatInput = document.createElement('input');
		chatInput.setAttribute('type','text');
		chatInput.setAttribute('placeholder', 'Message');

		wrap.appendChild(chatInput);

		var button = document.createElement('button');
		button.innerText = 'Chat';
		button.setAttribute('type','button');
		button.addEventListener('click', function(e) {
			var val = chatInput.value;
			sendChat(val);
		});

		wrap.appendChild(button);
	);

	emscripten_set_main_loop( main_loop, 60, 1 );
#else
	std::atomic<bool> run( true );
	std::thread cinThread( readInput, std::ref( run ));

	while (run.load()) {
		main_loop();
	}

	run.store( false );
	cinThread.join();

	humblenet_shutdown();
#endif
	return 0;
}
