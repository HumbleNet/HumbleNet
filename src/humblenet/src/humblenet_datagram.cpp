#include "humblenet_datagram.h"

#include "humblenet_p2p_internal.h"

// TODO : If this had access to the internals of Connection it could be further optimized.

#include <map>
#include <vector>
#include <stdlib.h>
#include <cstring>
#include <stdio.h>
#include <algorithm>

struct datagram_connection {
	Connection*			conn;			// established connection.
	PeerId				peer;			// "address"

	std::vector<char>	buf_in;			// data we have received buy not yet processed.
	std::vector<char>	buf_out;		// packet combining...
	int					queued;

	uint32_t seq_out = 0;
	uint32_t seq_in = 0;
	
	datagram_connection( Connection* conn, bool outgoing )
	:conn( conn )
	,peer( humblenet_connection_get_peer_id( conn ) )
	,queued( 0 )
	,seq_out( 0 )
	,seq_in( 0 )
	{
	}
};

typedef std::map<Connection*, datagram_connection> ConnectionMap;

static ConnectionMap	connections;
static bool				queuedPackets = false;

struct datagram_header {
    uint16_t size;
    uint8_t  channel;
    uint32_t seq;

    uint8_t data[];
};

static int datagram_get_message( datagram_connection& conn, const void* buffer, size_t length, int flags, uint8_t channel ) {

	std::vector<char>& in = conn.buf_in;
	std::vector<char>::iterator start = in.begin();

restart:

	uint16_t available = in.end() - start;

	if( available <= sizeof( datagram_header ) )
		return -1;

	datagram_header* hdr = reinterpret_cast<datagram_header*>(&in[start - in.begin()]);

	if ((hdr->size + sizeof(datagram_header)) > available) {
		// incomplete packet
		LOG("Incomplete packet from %u. %d but only have %d\n", conn.peer, hdr->size, available );
		return -1;
	}

	auto end = start + sizeof(datagram_header) + hdr->size;

	if( hdr->channel != channel ) {
		TRACE("Incorrect channel: wanted %d but got %d(%u) from %u\n", channel, hdr->channel, hdr->size, conn.peer );

		// skip to next packet.
		start = end;

		goto restart;
	}

	if( flags & HUMBLENET_MSG_PEEK )
		return hdr->size;

	// prevent buffer overrins on read.
	// this WILL truncate the message if the supplied buffer is not big enough -- see IEEE Std -> recvfrom.
	length = std::min<size_t>( length, hdr->size );
	memcpy((char*)buffer, hdr->data, length);

	// since we are accesing hdr directly out of the buffer, we need a local to store the msg size that was read.
	uint16_t size = hdr->size;
	in.erase(start, end);
	
	return size;
}

static void datagram_flush( datagram_connection& dg, const char* reason ) {
	if( ! dg.buf_out.empty() )
	{
		if( ! humblenet_connection_is_writable( dg.conn ) ) {
			LOG("Waiting(%s) %d packets (%zu bytes) to  %p\n", reason, dg.queued, dg.buf_out.size(), dg.conn );
			return;
		}

		if( dg.queued > 1 )
			LOG("Flushing(%s) %d packets (%zu bytes) to  %p\n", reason, dg.queued, dg.buf_out.size(), dg.conn );
		int ret = humblenet_connection_write( dg.conn, &dg.buf_out[0], dg.buf_out.size() );
		if( ret < 0 ) {
			LOG("Error flushing packets: %s\n", humblenet_get_error() );
			humblenet_clear_error();
		}
		dg.buf_out.clear();
		dg.queued = 0;
	}
}

int humblenet_datagram_send( const void* message, size_t length, int flags, Connection* conn, uint8_t channel )
{
	// if were still connecting, we cant write yet
	// TODO: Should we queue that data up?
	switch( humblenet_connection_status( conn ) ) {
		case HUMBLENET_CONNECTION_CONNECTING: {
			// If send using buffered, then allow messages to be queued till were connected.
			if( flags & HUMBLENET_MSG_BUFFERED )
				break;
			
			humblenet_set_error("Connection is still being establisted");
			return 0;
		}
		case HUMBLENET_CONNECTION_CLOSED: {
			humblenet_set_error("Connection is closed");
			// should this wipe the state ?
			return -1;
		}
		default: {
			break;
		}
	}

	ConnectionMap::iterator it = connections.find( conn );
	if( it == connections.end() ) {
		connections.insert( ConnectionMap::value_type( conn, datagram_connection( conn, false ) ) );
		it = connections.find( conn );
	}

	datagram_connection& dg = it->second;

	datagram_header hdr;

	hdr.size = length;
	hdr.channel = channel;
	hdr.seq = dg.seq_out++;

	dg.buf_out.reserve( dg.buf_out.size() + length + sizeof(datagram_header) );

	dg.buf_out.insert( dg.buf_out.end(), reinterpret_cast<const char*>( &hdr ), reinterpret_cast<const char*>( &hdr ) + sizeof( datagram_header ) );
	dg.buf_out.insert( dg.buf_out.end(), reinterpret_cast<const char*>( message ), reinterpret_cast<const char*>( message ) + length );

	dg.queued++;

	if( !( flags & HUMBLENET_MSG_BUFFERED ) ) {
		datagram_flush( dg, "no-delay" );
	} else if( dg.buf_out.size() > 1024 ) {
		datagram_flush( dg, "max-length" );
	} else {
		queuedPackets = true;
		//if( dg.queued > 1 )
		//    LOG("Queued %d packets (%zu bytes) for  %p\n", dg.queued, dg.buf_out.size(), dg.conn );
	}
	return length;
}

int humblenet_datagram_recv( void* buffer, size_t length, int flags, Connection** fromconn, uint8_t channel )
{
	// flush queued packets
	if( queuedPackets ) {
		for( ConnectionMap::iterator it = connections.begin(); it != connections.end(); ++it ) {
			datagram_flush( it->second, "auto" );
		}
		queuedPackets = false;
	}

	// first we drain all existing packets.
	for( ConnectionMap::iterator it = connections.begin(); it != connections.end(); ++it ) {
		int ret = datagram_get_message( it->second, buffer, length, flags, channel );
		if( ret > 0 ) {
			*fromconn = it->second.conn;
			return ret;
		}
	}

	// next check for incomming data on existing connections...
	while(true) {
		Connection* conn = humblenet_poll_all(0);
		if( conn == NULL )
			break;

		PeerId peer = humblenet_connection_get_peer_id( conn );

		ConnectionMap::iterator it = connections.find( conn );
		if( it == connections.end() ) {
			// hmm connection not cleaned up properly...
			LOG("received data from peer %u, but we have no datagram_connection for them\n", peer );
			connections.insert( ConnectionMap::value_type( conn, datagram_connection( conn, false ) ) );
			it = connections.find( conn );
		}

		// read whatever we can...
		int retval = 0;
		char* buf = (char*)humblenet_connection_read_all(conn, &retval );
		if( retval < 0 ) {
			free( buf );
			connections.erase( it );
			LOG("read from peer %u(%p) failed with %s\n", peer, conn, humblenet_get_error() );
			humblenet_clear_error();
			if( humblenet_connection_status( conn ) == HUMBLENET_CONNECTION_CLOSED ) {
				*fromconn = conn;
				return -1;
			}
			continue;
		} else {
			it->second.buf_in.insert( it->second.buf_in.end(), buf, buf+retval );
			free( buf );
			retval = datagram_get_message( it->second, buffer, length, flags, channel );
			if( retval > 0 ) {
				*fromconn = it->second.conn;
				return retval;
			}
		}
	}

	// no existing connections have a packet ready, see if we have any new connections
	while( true ) {
		Connection* conn = humblenet_connection_accept();
		if( conn == NULL )
			break;

		PeerId peer = humblenet_connection_get_peer_id( conn );
		if( peer == 0 ) {
			// Not a peer connection?
			LOG("Accepted connection, but not a peer connection: %p\n", conn);
		   // humblenet_connection_close( conn );
			continue;
		}

		connections.insert( ConnectionMap::value_type( conn, datagram_connection( conn, false ) ) );
	}

	return 0;
}

/*
* See if there is a message waiting on the specified channel
*/
ha_bool humblenet_datagram_select( size_t* length, uint8_t channel ) {
	Connection* conn = NULL;
	int ret = humblenet_datagram_recv( NULL, 0, HUMBLENET_MSG_PEEK, &conn, channel );
	if( ret > 0 )
		*length = ret;
	else
		*length = 0;

	return *length > 0;
}

ha_bool humblenet_datagram_flush() {
	// flush queued packets
	if( queuedPackets ) {
		for( ConnectionMap::iterator it = connections.begin(); it != connections.end(); ++it ) {
			datagram_flush( it->second, "manual" );
		}
		queuedPackets = false;
		return true;
	}
	return false;
}
