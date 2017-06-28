#ifndef HUMBLENET_DATAGRAM_H
#define HUMBLENET_DATAGRAM_H

#include "humblenet.h"

#define HUMBLENET_MSG_PEEK 0x1
#define HUMBLENET_MSG_BUFFERED 0x02

/*
* Send a message to a connection
*/
int humblenet_datagram_send( const void* message, size_t length, int flags, struct Connection* toconn, uint8_t channel );

/*
* Receive a message sent from a connection
*/
int humblenet_datagram_recv( void* buffer, size_t length, int flags, struct Connection** fromconn, uint8_t channel );

/*
* See if there is a message waiting on the specified channel
*/
ha_bool humblenet_datagram_select( size_t* length, uint8_t channel );

/*
* Flush any pending packets
*/
ha_bool humblenet_datagram_flush();

#endif // HUMBLENET_DATAGRAM_H
