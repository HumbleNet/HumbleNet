#pragma once

#include "humblenet.h"
#include "lobby.h"

#include <unordered_map>
#include <string>

namespace humblenet {

	struct P2PSignalConnection;

	typedef uint32_t GameId;

	struct Game {
		// TODO make this not generate incrementing IDs
		PeerId generateNewPeerId()
		{
			PeerId peerId = nextPeerId;
			nextPeerId++;
			return peerId;
		}

		// TODO make this not generate incrementing IDs
		LobbyId generateNewLobbyId()
		{
			LobbyId lobbyId = nextLobbyId;
			nextLobbyId++;
			return lobbyId;
		}

		GameId gameId;

		PeerId nextPeerId;

		LobbyId nextLobbyId;

		std::unordered_map<PeerId, P2PSignalConnection *> peers;  // not owned, must also be in signalConnections

		std::unordered_map<std::string, PeerId> aliases;

		std::unordered_map<LobbyId, Lobby> lobbies;

		Game(GameId game_id) : gameId(game_id), nextPeerId(1), nextLobbyId(1) { }

		void erasePeerAliases(PeerId p);
	};
}
