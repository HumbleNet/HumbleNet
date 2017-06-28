#pragma once

#include "humblenet.h"

#include <unordered_map>
#include <string>

namespace humblenet {

	struct P2PSignalConnection;

	typedef uint32_t GameId;

	struct Game {
		PeerId generateNewPeerId()
		{
			PeerId peerId = nextPeerId;
			nextPeerId++;
			return peerId;
		}

		GameId gameId;

		PeerId nextPeerId;

		std::unordered_map<PeerId, P2PSignalConnection *> peers;  // not owned, must also be in signalConnections

		std::unordered_map<std::string, PeerId> aliases;

		Game(GameId game_id) : gameId(game_id), nextPeerId(1) { }

		void erasePeerAliases(PeerId p);
	};
}
