#include "humblenet_p2p.h"

#ifdef EMSCRIPTEN
#include <emscripten.h>
#else
#define EMSCRIPTEN_KEEPALIVE
#include <atomic>
#endif

#include <iostream>
#include <cstring>
#include <algorithm>
#include <string>
#include <ctime>

enum class PeerStatus : uint8_t{
	NONE,
	WAITING,
	CONNECTING,
	CONNECTED,
	SENT,
};

enum class MessageType : uint8_t{
	HELLO = 1,
	ACK = 2,
	TEXT = 3,
};

static PeerId myPeer = 0;

static time_t startPeerLastHello;
static PeerId startPeerId = 0;
PeerStatus startPeerStatus = PeerStatus::NONE;

const uint8_t CHANNEL = 42;
const uint16_t MAX_MESSAGE_SIZE = 512;

const char client_token[] = "hello_world";
const char client_secret[] = "secret";

extern "C" {
	EMSCRIPTEN_KEEPALIVE void connectToPeer(PeerId peer);
	EMSCRIPTEN_KEEPALIVE void sendChat(PeerId peer, const char* message);
}

void send_message(PeerId peer, MessageType type, const char* text, int size)
{
	if (size > 255) {
		return;
	}
	uint8_t buff[MAX_MESSAGE_SIZE];

	buff[0] = (uint8_t)type;
	buff[1] = (uint8_t)size;
	if (size > 0) {
		memcpy(buff + 2, text, size);
	}
	humblenet_p2p_sendto(buff, size + 2, peer, SEND_RELIABLE, CHANNEL);
}

void process_message(PeerId remotePeer, const uint8_t* buffer, int buffer_size, int message_size)
{
	// NOTE message_size is how big the message actually was. if it was bigger than our buffer size it WILL be truncated!
	int length = std::max(buffer_size, message_size);

	if (length > 2) {
		MessageType type = (MessageType)buffer[0];
		switch (type) {
			case MessageType::TEXT:
				// We received a text message.. Lets display it!
			{
				int size = buffer[1];
				if (size > 0) {
					// make sure the buffer was not truncated
					if (size > (length - 2)) return;

					// copy the text out of it (skipping the first 2 bytes which are out type + length)
					std::string text((const char *)(buffer + 2), size);

					std::cout << "Message TEXT from " << remotePeer << " : " << text << std::endl;
				}
			}
				break;
			case MessageType::HELLO:
				// We received a hello message. Send an ack.
				std::cout << "Received HELLO from " << remotePeer << std::endl;
				send_message(remotePeer, MessageType::ACK, "", 0);
				break;
			case MessageType::ACK:
				// we received an ACK message. lets check if it is the right peer.
				std::cout << "Received ACK from " << remotePeer << std::endl;
				if (remotePeer == startPeerId) {
					startPeerStatus = PeerStatus::CONNECTED;
				}
			default:
				break;
		}
	}
}


void sendChat(PeerId peer, const char* message)
{
	send_message(peer, MessageType::TEXT, message, (int)strlen(message));
}

void connectToPeer(PeerId peer)
{
	startPeerId = peer;
	startPeerStatus = PeerStatus::CONNECTING;

	send_message(peer, MessageType::HELLO, "", 0);
}

void main_loop()
{
	humblenet_p2p_wait(100);

	if (myPeer == 0) {
		// check to see if we finished connecting to the peer server
		myPeer = humblenet_p2p_get_my_peer_id();
		if (myPeer != 0) {
			std::cout << "We connected to the peer server with peer " << myPeer << std::endl;
			if (startPeerStatus == PeerStatus::WAITING) {
				// initiate the connection to the remote peer
				startPeerLastHello = time(NULL);
				startPeerStatus = PeerStatus::CONNECTING;
				send_message(startPeerId, MessageType::HELLO, "", 0);
			}
		}
	} else {
		// check if we have connected to the other peer
		switch (startPeerStatus) {
			case PeerStatus::CONNECTING:
			{
				time_t now = time(NULL);
				if (now > startPeerLastHello) {
					// try once a second
					send_message(startPeerId, MessageType::HELLO, "", 0);
					startPeerLastHello = now;
				}
			}
				break;
			case PeerStatus::CONNECTED:
				// we are connected
				sendChat(startPeerId, "Hello World!");
				startPeerStatus = PeerStatus::SENT;
#ifdef EMSCRIPTEN
				EM_ASM(
					var button = document.getElementById('ChatButton');
					if (button) {
						button.disabled = false;
					}
				);
#endif
				break;
			default:
				break;
		}

		// we are connected so lets see if we have any data to process
		// this buffer should be LARGE enough to handle any packet you might send OR use peek to see how big the next one is
		uint8_t buff[MAX_MESSAGE_SIZE];
		bool done = false;

		while (!done) {
			PeerId remotePeer = 0;
			int ret = humblenet_p2p_recvfrom(buff, sizeof(buff), &remotePeer, CHANNEL);
			if (ret < 0) {
				if (remotePeer != 0) {
					// disconnected client
				} else {
					// error
					done = true;
				}
			} else if (ret > 0) {
				// we received data process it
				process_message(remotePeer, buff, sizeof(buff), ret);
			} else {
				// 0 return value means no more data to read
				done = true;
			}
		}
	}
}

int main(int argc, char *argv[])
{
	if (argc > 1) {
		startPeerStatus = PeerStatus::WAITING;
		startPeerId = std::stol(argv[1]);
	}
	// Initialize the core library
	humblenet_init();

	// Initialize the P2P subsystem connecting to the named peer server
	// Client token and secret are defined in the peer server configuration
	// the 4th argument is for user authentication (future feature)
	humblenet_p2p_init(HUMBLENET_SERVER_URL, client_token, client_secret, NULL);

#ifdef EMSCRIPTEN
	EM_ASM(
		var canvasParent = Module.canvas.parentNode;
		var wrap = document.createElement('div');
		canvasParent.insertBefore(wrap, Module.canvas);

		// data
		var remotePeer = 0;

		// Messages
		var connectToPeer = Module.cwrap('connectToPeer', 'void', ['number']);
		var sendChat = Module.cwrap('sendChat', 'void', ['number', 'string']);

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
				remotePeer = val;
				connectToPeer(val);
			}
		});
		wrap.appendChild(button);

		wrap.appendChild(document.createElement('br'));

		var chatInput = document.createElement('input');
		chatInput.setAttribute('type','text');
		chatInput.setAttribute('placeholder', 'Message');
		wrap.appendChild(chatInput);

		var button = document.createElement('button');
		button.id = 'ChatButton';
		button.innerText = 'Chat';
		button.setAttribute('type','button');
		button.disabled = true;
		button.addEventListener('click', function(e) {
			var val = chatInput.value;
			sendChat(remotePeer, val);
		});
		wrap.appendChild(button);

	);
	emscripten_set_main_loop( main_loop, 60, 1 );
#else
	std::atomic<bool> run(true);

	while (run.load()) {
		main_loop();
	}
	humblenet_shutdown();
#endif
	return 0;
}
