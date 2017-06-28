#pragma once

#include "humblenet.h"
#include "humblepeer.h"
#include "game.h"

struct libwebsocket;

namespace humblenet {
	struct Game;
	struct Server;

	enum HumblePeerState {
		Opening
		, Open
		, Closing
		, Closed
	};

	struct P2PSignalConnection {
		Server* peerServer;

		std::vector<uint8_t> recvBuf;
		std::vector<char> sendBuf;
		struct libwebsocket *wsi;
		PeerId peerId;
		HumblePeerState state;

		bool webRTCsupport;
		bool trickleICE;

		Game *game;

		std::string url;

		// peers which have a P2P connection with this one
		// pointer not owned
		std::unordered_set<P2PSignalConnection *> connectedPeers;


		P2PSignalConnection(Server* s)
		: peerServer(s)
		, wsi(NULL)
		, peerId (0)
		, state(Opening)
		, webRTCsupport(false)
		, trickleICE(true)
		, game(NULL)
		{
		}

		ha_bool processMsg(const HumblePeer::Message* msg);

		void sendMessage(const uint8_t *buff, size_t length);
	};

}
