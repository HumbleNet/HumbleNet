#pragma once

#include "game.h"
#include "p2p_connection.h"

#include <unordered_map>
#include <string>
#include <memory>

struct libwebsocket_context;
struct libwebsocket;

namespace humblenet {
	class GameDB;

	struct Server {
		struct libwebsocket_context *context;

		// we need to be able to lookup connections on both
		// websocket (for the receive callback) and peer id (when sending stuff)
		std::unordered_map<struct libwebsocket *, std::unique_ptr<P2PSignalConnection> > signalConnections;

		std::unordered_map<GameId, std::unique_ptr<Game> > games;

		std::string stunServerAddress;


		Server(std::shared_ptr<GameDB> _gameDB);

		Game *getVerifiedGame(const HumblePeer::HelloServer* hello);
		void populateStunServers(std::vector<ICEServer>& servers);
		void triggerWrite(struct libwebsocket* wsi);

	private:
		std::shared_ptr<GameDB> m_gameDB;
	};

}