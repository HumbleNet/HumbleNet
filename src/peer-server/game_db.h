#pragma once

#include "game.h"

#include <unordered_map>
#include <string>

namespace humblenet {
	struct GameRecord {
		GameId game_id;
		std::string secret;
		bool verify;

		GameRecord() : game_id(0), verify(false) {}
	};

	class GameDB {
	public:
		GameDB() {};
		virtual ~GameDB() {};

		virtual bool findByToken(const std::string& token, GameRecord& record) = 0;
	};

	class GameDBAnonymous : public GameDB {
		GameId m_lastGameId;
		std::unordered_map<std::string, GameRecord> m_games;
	public:
		GameDBAnonymous() : m_lastGameId(0) {}
		virtual bool findByToken(const std::string& token, GameRecord& record);
	};

	class GameDBFlatFile : public GameDB {
		std::string m_filename;
		std::unordered_map<std::string, GameRecord> m_games;

		void loadFile();
	public:
		GameDBFlatFile(const std::string& filename);
		virtual bool findByToken(const std::string &token, humblenet::GameRecord &record);
	};
}
