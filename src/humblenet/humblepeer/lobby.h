#pragma once

#include "humblenet_lobbies.h"
#include "humblenet_types_internal.h"
#include <string>
#include <unordered_map>
#include <utility>

namespace humblenet {
	struct Lobby {
		struct MemberDetails {
			AttributeMap attributes;

			void applyAttributes(const AttributeMap& attribs, bool merge = false);
		};

		LobbyId lobbyId;

		PeerId ownerPeerId;

		humblenet_LobbyType type;

		uint16_t maxMembers;

		AttributeMap attributes;

		std::unordered_map<PeerId, MemberDetails> members;

		Lobby(LobbyId _id, PeerId _owner, humblenet_LobbyType _type, uint16_t _max) : lobbyId( _id ),
																					  ownerPeerId( _owner ),
																					  type( _type ),
																					  maxMembers( _max )
		{
			addPeer( _owner );
		}

		decltype(members)::iterator addPeer(PeerId peerId)
		{
			auto it = members.emplace( std::piecewise_construct,
							 std::forward_as_tuple( peerId ),
							 std::forward_as_tuple());
			return it.first;
		}

		void removePeer(PeerId peerId)
		{
			members.erase(peerId);
		}

		void applyAttributes(const AttributeMap& attribs, bool merge = false);
	};
}


