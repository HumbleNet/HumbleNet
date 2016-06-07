#pragma once

#include "game.h"

#include <unordered_map>
#include <unordered_set>
#include <string>

namespace humblenet {
	struct PeerRecord {
		std::string token;
		GameId game_id;
		PeerId peer_id;
		std::unordered_set<std::string> aliases;

		PeerRecord() : game_id(0), peer_id(0) {}
	};

	class PeerDB {
	public:
		PeerDB() {};
		virtual ~PeerDB() {};

		virtual bool findByToken(const std::string& token, PeerRecord& record) = 0;
		virtual void insert(const PeerRecord& record) = 0;
		virtual void erase(const std::string& token) = 0;
	};

	class PeerDBAnonymous : public PeerDB {
	protected:
		std::unordered_map<std::string, PeerRecord> m_peers;
	public:
		PeerDBAnonymous() {}
		virtual bool findByToken(const std::string& token, PeerRecord& record);
		virtual void insert(const PeerRecord& record);
		virtual void erase(const std::string& token);
	};

	class PeerDBFlatFile : public PeerDBAnonymous {
		std::string m_filename;

		void loadFile();
		void flush();
	public:
		PeerDBFlatFile(const std::string& filename);
		virtual bool findByToken(const std::string &token, humblenet::PeerRecord &record);
		virtual void insert(const PeerRecord& record);
		virtual void erase(const std::string& token);
	};
}
