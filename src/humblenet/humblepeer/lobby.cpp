#include "lobby.h"

namespace humblenet {
	void applyAttributes(AttributeMap& dest, const AttributeMap& attribs, bool merge = false)
	{
		if (!merge) {
			dest.clear();
		}

		for (auto& p: attribs) {
			if (p.second.empty()) {
				dest.erase( p.first );
			} else {
				dest[p.first] = p.second;
			}
		}
	}

	void Lobby::MemberDetails::applyAttributes(const AttributeMap& attribs, bool merge)
	{
		::humblenet::applyAttributes( attributes, attribs, merge);
	}

	void Lobby::applyAttributes(const AttributeMap& attribs, bool merge)
	{
		::humblenet::applyAttributes( attributes, attribs, merge);
	}

}