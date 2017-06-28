#include "game.h"
#include "p2p_connection.h"
#include "humblenet_utils.h"

namespace humblenet {

	void Game::erasePeerAliases(PeerId p)
	{
		erase_value(aliases, p);
	}
}

