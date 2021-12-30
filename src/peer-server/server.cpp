#include "server.h"
#include "game_db.h"
#include "logging.h"
#include "hmac.h"

namespace humblenet {
	Server::Server(std::shared_ptr<GameDB> _gameDB)
	: context(NULL)
	, m_gameDB(_gameDB)
	{
	}

	Game *Server::getVerifiedGame(const HumblePeer::HelloServer* hello)
	{
		GameRecord record;
		auto gameToken = hello->game_token();
		auto gameSignature = hello->game_signature();

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

			auto authToken = hello->auth_token();
			if (authToken) {
				HMACInput(&hmac, authToken->Data(), authToken->size());
			}
			auto reconnect = hello->reconnect_token();
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
}