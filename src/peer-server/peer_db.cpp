#include "peer_db.h"

#include <fstream>

namespace humblenet {
	// Anonymous DB

	bool PeerDBAnonymous::findByToken(const std::string &token, humblenet::PeerRecord &record)
	{
		auto it = m_peers.find(token);
		if (it == m_peers.end()) {
			return false;
		} else {
			record = it->second;
		}
		return true;
	}

	void PeerDBAnonymous::erase(const std::string& token) {
		auto it = m_peers.find(token);
		if( it != m_peers.end() ) {
			m_peers.erase(it);
		}
	}

	void PeerDBAnonymous::insert(const humblenet::PeerRecord &record) {
		m_peers.erase(record.token);
		m_peers.insert( std::make_pair(record.token, record) );
	}
	// Flat File DB

	PeerDBFlatFile::PeerDBFlatFile(const std::string& filename)
	: m_filename(filename)
	{
		loadFile();
	}

	void PeerDBFlatFile::loadFile()
	{
		m_peers.clear();
		std::ifstream in(m_filename.c_str());

		if (!in) return;

		std::string line;
		for (std::getline(in, line); in; std::getline(in, line))
		{
			// format
			// game_id, token, peer_id [,alias]*

			// skip comments
			if (line[0] == '#') continue;

			auto gameEnd = line.find(',');
			if (gameEnd == line.npos) continue;
			auto tokEnd = line.find(',', gameEnd + 1);
			if (tokEnd == line.npos) continue;
			auto peerEnd = line.find(',', tokEnd + 1);
			if (peerEnd == line.npos) peerEnd = line.size()-1;

			PeerRecord record;

			record.game_id = (GameId)std::stoul(line.substr(0, gameEnd));
			record.token = line.substr(gameEnd+1, tokEnd - gameEnd - 1);
			record.peer_id = (PeerId)std::stoul(line.substr(tokEnd+1, peerEnd));

			auto aliasStart = peerEnd+1;

			while( aliasStart != line.npos && aliasStart < line.size() ) {
				auto aliasEnd = line.find( ',', aliasStart+1 );
				if( aliasEnd == line.npos ) {
					record.aliases.insert( line.substr(aliasStart) );
					break;
				} else {
					record.aliases.insert( line.substr(aliasStart, aliasEnd) );
					aliasStart = aliasEnd + 1;
				}
			}

			m_peers.emplace(record.token, record);
		}
	}

	void PeerDBFlatFile::flush() {
		std::ofstream out(m_filename.c_str(), std::ofstream::out | std::ofstream::trunc);

		if (!out) return;

		for( auto it = m_peers.begin(); it != m_peers.end(); ++it ) {
			const PeerRecord& record = it->second;

			out << record.game_id << "," << record.token << "," << record.peer_id;
			for( auto ait = record.aliases.begin(); ait != record.aliases.end(); ++ait ) {
				out << "," << *ait;
			}
			out << std::endl;
		}
	}

	bool PeerDBFlatFile::findByToken(const std::string &token, humblenet::PeerRecord &record)
	{
		return PeerDBAnonymous::findByToken(token,record);
	}

	void PeerDBFlatFile::erase(const std::string& token) {
		PeerDBAnonymous::erase( token );
		flush();
	}

	void PeerDBFlatFile::insert(const humblenet::PeerRecord &record) {
		PeerDBAnonymous::insert( record );
#pragma message("TODO: if no record to erase, do simple append")
		flush();
	}
}
