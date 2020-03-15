#include "humblenet_p2p.h"
#include "humblenet_events.h"
#include "humblenet_lobbies.h"

#ifdef EMSCRIPTEN
#include <emscripten.h>
#else
#define EMSCRIPTEN_KEEPALIVE

#include <atomic>
#include <thread>

#endif

#include <locale>
#include <iostream>
#include <iterator>
#include <string>

static PeerId myPeer = 0;
static bool connected = false;
const uint8_t CHANNEL = 52;

std::string toLower(const std::string& in)
{
	std::locale loc;
	std::string out;

	for (auto c: in) {
		out.push_back( std::tolower( c, loc ));
	}
	return out;
}

void doQuit();

void processCommand(const std::string& command);

#ifdef EMSCRIPTEN
extern "C" {
EMSCRIPTEN_KEEPALIVE void sendCommand(const char* message);
}

void doQuit() {
	//noop
}

void sendCommand(const char* message)
{
	processCommand(message);
}

#else
std::atomic<bool> run( true );

void doQuit()
{
	run.store( false );
}

void readInput()
{
	std::string buffer;

	while (run.load()) {
		std::getline( std::cin, buffer );

		processCommand( buffer );
	}
}

#endif

void printHelp()
{
	std::cout << "Available commands:" << std::endl;
	std::cout << "  /create max    : Create a new private lobby" << std::endl;
	std::cout << "  /join lobby#   : Connect to a lobby by ID" << std::endl;
	std::cout << "  /leave lobby#  : Leave a lobby by ID" << std::endl;
	std::cout << "  /quit          : Quit the application" << std::endl;
	std::cout << "  /help          : Print this help" << std::endl;
}


struct Command {
	typedef bool (* CommandFunc)(const std::string& args);

	std::string command;
	CommandFunc func;

	bool process(const std::string& c)
	{
		auto match = c.compare( 0, command.size(), command ) == 0;
		if (match) {
			match = func( c.substr( command.size()));
		}
		return match;
	}
};

static Command commands[] = {
		{"/quit",    [](const std::string&) {
			doQuit();
			return true;
		}},
		{"/help",    [](const std::string&) {
			printHelp();
			return true;
		}},
		{"/create ", [](const std::string& arg) {
			uint16_t max = std::stoul( arg );
			humblenet_lobby_create( HUMBLENET_LOBBY_TYPE_PRIVATE, max );
			return true;
		}},
		{"/join ",   [](const std::string& arg) {
			LobbyId lobby = std::stoul( arg );
			humblenet_lobby_join( lobby );
			return true;
		}},
		{"/leave ",  [](const std::string& arg) {
			LobbyId lobby = std::stoul( arg );
			humblenet_lobby_leave( lobby );
			return true;
		}},
};

void processCommand(const std::string& message)
{
	std::string command = toLower( message );

	bool found = false;

	for (auto& x : commands) {
		if (x.process( command )) {
			found = true;
			break;
		}
	}

	if (!found && command.compare( 0, 1, "/" ) == 0) {
		std::cout << "Unknown command: " << command << std::endl;
		std::cout << "Use /help to see available commands: " << std::endl;
	} else {
		// what do we do
	}
}

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
			case HUMBLENET_EVENT_LOBBY_CREATE_SUCCESS:
				std::cout << "Created lobby " << event.lobby.lobby_id << std::endl;
				break;
			case HUMBLENET_EVENT_LOBBY_JOIN:
				std::cout << "Joined lobby " << event.lobby.lobby_id << std::endl;
				break;
			case HUMBLENET_EVENT_LOBBY_LEAVE:
				std::cout << "Left lobby " << event.lobby.lobby_id << std::endl;
				break;
			case HUMBLENET_EVENT_LOBBY_MEMBER_JOIN:
				std::cout << "Member joined lobby " << event.lobby.lobby_id
						  << " :: " << event.lobby.peer_id << std::endl;
				break;
			case HUMBLENET_EVENT_LOBBY_MEMBER_LEAVE:
				std::cout << "Member left lobby " << event.lobby.lobby_id
						  << " :: " << event.lobby.peer_id << std::endl;
				break;
			default:
				std::cout << "Unhandled event: 0x" << std::hex << event.type << std::endl;
				break;
		}
	}

	// Poll if connected
	if (connected) {
		bool done = false;
		do {
			char buff[1024];
			PeerId otherPeer = 0;
			int ret = humblenet_p2p_recvfrom( buff, sizeof( buff ), &otherPeer, CHANNEL );
			if (ret < 0) {
				if (otherPeer == 0) {
					done = true;
				}
			} else if (ret > 0) {
				// process message
			} else {
				done = true;
			}
		} while (!done);
	}
}

int main(int argc, char* argv[])
{
	std::string peerServer = HUMBLENET_SERVER_URL;
	if (argc == 2) {
		peerServer = argv[1];
	}
	humblenet_p2p_init( peerServer.c_str(), "client_token", "client_secret", nullptr );

#ifdef EMSCRIPTEN
	EM_ASM(
		var canvasParent = Module.canvas.parentNode;
		var wrap = document.createElement('div');
		canvasParent.insertBefore(wrap, Module.canvas);

		var sendCommand = Module.cwrap('sendCommand', 'void', ['string']);

		var commandInput = document.createElement('input');
		commandInput.setAttribute('type','text');
		commandInput.setAttribute('placeholder', 'Command');
		commandInput.addEventListener('keydown', function(e) {
			if (e.code == "Enter") {
				sendCommand(commandInput.value);
			}
		});
		wrap.appendChild(commandInput);

		var button = document.createElement('button');
		button.innerText = 'Send';
		button.setAttribute('type','button');
		button.addEventListener('click', function(e) {
			sendCommand(commandInput.value);
		});

		wrap.appendChild(button);
	);

	emscripten_set_main_loop( main_loop, 60, 1 );
#else
	std::thread cinThread( readInput );

	while (run.load()) {
		main_loop();
	}

	run.store( false );
	cinThread.join();

	humblenet_shutdown();
#endif
	return 0;
}
