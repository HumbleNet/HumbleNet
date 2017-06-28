#include "game_db.h"

#include <fstream>

namespace humblenet {
	// Anonymous DB

	bool GameDBAnonymous::findByToken(const std::string &token, humblenet::GameRecord &record)
	{
		auto it = m_games.find(token);
		if (it == m_games.end()) {
			record.game_id = ++m_lastGameId;
			record.secret = "anonymous";
			record.verify = false;
			m_games.emplace(token, record);
		} else {
			record = it->second;
		}
		return true;
	}

	// Flat File DB

	GameDBFlatFile::GameDBFlatFile(const std::string& filename)
	: m_filename(filename)
	{
		loadFile();
	}

	void GameDBFlatFile::loadFile()
	{
		m_games.clear();
		std::ifstream in(m_filename.c_str());

		if (!in) return;

		std::string line;
		for (std::getline(in, line); in; std::getline(in, line))
		{
			// skip comments
			if (line[0] == '#') continue;

			auto gameEnd = line.find(',');
			if (gameEnd == line.npos) continue;
			auto tokEnd = line.find(',', gameEnd + 1);
			if (tokEnd == line.npos) continue;
			auto secEnd = line.find(',', tokEnd + 1);
			if (secEnd == line.npos) continue;
			auto valEnd = line.find(',', secEnd + 1);
			if (valEnd == line.npos) continue;

			GameRecord record;
			bool active = std::stoi(line.substr(valEnd + 1)) != 0;
			if (!active) continue;

			record.game_id = (GameId)std::stoul(line.substr(0, gameEnd));
			std::string token = line.substr(gameEnd+1, tokEnd - gameEnd - 1);
			record.secret = line.substr(tokEnd+1, secEnd - tokEnd - 1);
			record.verify = std::stoi(line.substr(secEnd+1, valEnd - secEnd - 1)) != 0;

			m_games.emplace(token, record);
		}
	}

	bool GameDBFlatFile::findByToken(const std::string &token, humblenet::GameRecord &record)
	{
		auto it = m_games.find(token);
		if (it != m_games.end()) {
			record = it->second;
			return true;
		}
		return false;
	}
}
