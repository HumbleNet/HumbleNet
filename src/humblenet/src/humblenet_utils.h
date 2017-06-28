#ifndef HUMBLENET_UTILS_H
#define HUMBLENET_UTILS_H

template <typename A, typename B>
class BidiractionalMap {
public:
	typedef A key_type;
	typedef B mapped_type;
	typedef std::map<A, B> AtoB;
	typedef std::map<B, A> BtoA;
private:
	AtoB list_a;
	BtoA list_b;
public:
	void insert(const A& a, const B& b) {
		list_a.emplace(a, b);
		list_b.emplace(b, a);
	}

	void erase(const A& a) {
		auto it = list_a.find(a);
		if (it != list_a.end()) {
			list_b.erase(it->second);
			list_a.erase(it);
		}
	}

	void erase(const B& a) {
		auto it = list_b.find(a);
		if (it != list_b.end()) {
			list_a.erase(it->second);
			list_b.erase(it);
		}
	}

	void erase(typename AtoB::iterator& it) {
		list_b.erase(it->second);
		list_a.erase(it);
	}

	void erase(typename BtoA::iterator& it) {
		list_a.erase(it->second);
		list_b.erase(it);
	}

	typename AtoB::iterator find(const A& a) { return list_a.find(a); }

	typename BtoA::iterator find(const B& b) { return list_b.find(b); }

	bool is_end(typename AtoB::iterator& it) {
		return list_a.end() == it;
	}

	bool is_end(typename BtoA::iterator& it) {
		return list_b.end() == it;
	}
};

template< class MapType >
void erase_value( MapType& map, const typename MapType::mapped_type& value ) {
	for( auto it = map.begin(); it != map.end(); ){
		if( it->second == value )
			map.erase( it++ );
		else
			++it;
	}
}

#endif // HUMBLENET_UTILS_H