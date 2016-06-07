#include "server.h"
#include "game_db.h"
#include "logging.h"
#include "hmac.h"
#include "peer_db.h"

namespace humblenet {
	Server::Server(std::shared_ptr<GameDB> _gameDB, std::shared_ptr<PeerDB> _peerDB)
	: context(NULL)
	, m_gameDB(_gameDB)
	, m_peerDB(_peerDB)
	{
	}

	Game *Server::getVerifiedGame(const HumblePeer::HelloServer* hello)
	{
		GameRecord record;
		auto gameToken = hello->gameToken();
		auto gameSignature = hello->gameSignature();

		if (!gameToken || !gameSignature) {
			LOG_ERROR("No game token provided!\n");
			return nullptr;
		}

		if (!m_gameDB->findByToken(gameToken->c_str(), record)) {
			// Game not found by DB
			LOG_ERROR("Invalid game token: %s.\n", gameToken->c_str());
			return nullptr;
		}

		// Verify signature
		if (record.verify) {
			HMACContext hmac;
			HMACInit(&hmac, (const uint8_t*)record.secret.c_str(), record.secret.size());

			auto authToken = hello->authToken();
			if (authToken) {
				HMACInput(&hmac, authToken->Data(), authToken->size());
			}
			auto reconnect = hello->authToken();
			if (reconnect) {
				HMACInput(&hmac, reconnect->Data(), reconnect->size());
			}

			auto attributes = hello->attributes();

			if (attributes) {
				for (const auto& it : *attributes) {
					auto key = it->key();
					auto value = it->value();
					HMACInput(&hmac, key->Data(), key->size());
					HMACInput(&hmac, value->Data(), value->size());
				}
			}

			uint8_t hmacresult[HMAC_DIGEST_SIZE];
			HMACResult(&hmac, hmacresult);
			std::string signature;
			HMACResultToHex(hmacresult, signature);

			if (signature.compare(gameSignature->c_str()) != 0) {
				LOG_ERROR("Invalid game signature provided for token: %s\n", gameToken->c_str());
				return nullptr;
			}
		}

		auto it = this->games.find(record.game_id);
		if (it == this->games.end()) {
			// not found, create
			Game *g = new Game(record.game_id);
			this->games.emplace(record.game_id, std::unique_ptr<Game>(g));
			return g;
		}

		return it->second.get();
	}

	bool Server::getPeerByToken(const std::string& token, PeerRecord& rec ) {
		return m_peerDB->findByToken( token, rec );
	}

	void Server::savePeerState(const P2PSignalConnection* conn) {
		// if client has a reconnectToken, save thier state so they can reconnect.
		if( ! conn->reconnectToken.empty() ) {
			PeerRecord record;

			record.game_id = conn->game->gameId;
			record.token = conn->reconnectToken;
			record.peer_id = conn->peerId;

			for( auto it = conn->game->aliases.begin(); it != conn->game->aliases.end(); ++ it ) {
				if( it->second == conn->peerId )
					record.aliases.insert( it->first );
			}

			m_peerDB->insert( record );
		}
	}

	void Server::triggerWrite(struct libwebsocket *wsi)
	{
		libwebsocket_callback_on_writable(context, wsi);
	}

	void Server::populateStunServers(std::vector<ICEServer> &servers)
	{
		if( ! stunServerAddress.empty() ) {
			servers.emplace_back(stunServerAddress);
		}
	}

	void Server::shutdown() {
		// save all peer state
		for( auto it = games.begin(); it != games.end(); ++it ) {
			for( auto peer = it->second->peers.begin(); peer != it->second->peers.end(); ++ peer ) {
				savePeerState( peer->second );
			}
		}
	}
}
