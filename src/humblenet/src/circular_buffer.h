#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

#include <vector>
#include <string>
#include <sstream>
#include <cassert>

template<typename T>
class CircularBuffer {
public:
	explicit CircularBuffer(uint16_t capacity);

	~CircularBuffer();

	uint16_t available() const noexcept;

	uint16_t used() const noexcept;

	bool push(const T& item) noexcept;

	bool pop(T& item) noexcept;

	std::string debug() noexcept;

private:
	struct Entry {
		T item;
		Entry* next;
		Entry* prev;

		Entry() : next( nullptr ), prev( nullptr ) {}
	};

	void remove(Entry* entry) noexcept;

	uint16_t m_capacity;
	uint16_t m_count;
	uint16_t m_max;

	Entry* m_head;
	Entry* m_tail;
	Entry* m_free;
};

template<typename T>
CircularBuffer<T>::CircularBuffer(uint16_t capacity) : m_capacity( capacity ),
													   m_count( 0 ),
													   m_max( 0 ),
													   m_head( nullptr ),
													   m_tail( nullptr ),
													   m_free( nullptr )
{
}

template<typename T>
CircularBuffer<T>::~CircularBuffer()
{
	for (auto entry = m_head; entry;) {
		auto next = entry->next;
		if (entry) {
			delete entry;
		}
		entry = next;
	}

	for (auto entry = m_free; entry;) {
		auto next = entry->next;
		if (entry) {
			delete entry;
		}
		entry = next;
	}
}

template<typename T>
uint16_t CircularBuffer<T>::available() const noexcept
{
	return m_capacity - m_count;
}

template<typename T>
uint16_t CircularBuffer<T>::used() const noexcept
{
	return m_count;
}

template<typename T>
bool CircularBuffer<T>::push(const T& item) noexcept
{
	Entry* entry = nullptr;

	if (m_count >= m_capacity) {
		return false; // Overflow
	}

	if (m_free == nullptr) {
		entry = new Entry;
	} else {
		entry = m_free;
		m_free = m_free->next;
	}

	entry->item = item;

	if (m_tail) {
		m_tail->next = entry;
		entry->prev = m_tail;
		m_tail = entry;
		entry->next = nullptr;
	} else {
		assert( !m_head );
		m_tail = entry;
		m_head = entry;
		entry->next = nullptr;
		entry->prev = nullptr;
	}

	++m_count;
	if (m_count > m_max) {
		m_max = m_count;
	}

	return true;
}

template<typename T>
bool CircularBuffer<T>::pop(T& item) noexcept
{
	Entry* entry = m_head;
	if (entry) {
		item = entry->item;
		remove( entry );
		return true;
	} else {
		return false;
	}
}

template<typename T>
void CircularBuffer<T>::remove(Entry* entry) noexcept
{
	if (entry->prev) {
		entry->prev->next = entry->next;
	}
	if (entry->next) {
		entry->next->prev = entry->prev;
	}
	if (entry == m_head) {
		assert( entry->prev == nullptr );
		m_head = entry->next;
	}
	if (entry == m_tail) {
		assert( entry->next == nullptr );
		m_tail = entry->prev;
	}

	entry->next = m_free;
	m_free = entry;
	--m_count;
}

template<typename T>
std::string CircularBuffer<T>::debug() noexcept
{
	std::stringstream ss;
	ss <<
	   "Head: " << std::hex << m_head <<
	   " Tail: " << std::hex << m_tail <<
	   " Free: " << std::hex << m_free <<
	   " Used: " << used() <<
	   " Available: " << available();
	return ss.str();
}

#endif // CIRCULAR_BUFFER_H
