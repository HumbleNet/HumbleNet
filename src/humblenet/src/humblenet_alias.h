#ifndef HUMBLENET_ALIAS_H
#define HUMBLENET_ALIAS_H

#ifndef HUMBLENET_P2P_INTERNAL
#error Cannot use this header outside humblenet
#endif

/**
 * Register an alias for this peer.
 *
 *
 * NOTE: The peer server can reject the alias, but that will notify via the events.
 */
ha_bool internal_alias_register(const char* alias );

/**
 * Unregister an alias for this peer.
 *
 *
 * Passing NULL means unregister ALL aliases
 */
ha_bool internal_alias_unregister(const char* alias );

/**
 * Return a virtual PeerId that represents the alias.
 *
 */
PeerId internal_alias_lookup(const char* alias );

/**
 * Is this peer id a vritual peer id
 */
ha_bool internal_alias_is_virtual_peer( PeerId peer );


/**
 * Get the virtual peer associated with this conenction (if there is one)
 *
 */
PeerId internal_alias_get_virtual_peer( Connection* conn );

/**
 * Find the connection associated with the virtual peer
 *
 */
Connection* internal_alias_find_connection( PeerId );

/**
 * Setup a connection to the virtual peer
 *
 */
Connection* internal_alias_create_connection( PeerId );

/**
 * Remove the connection from the alias system
 *
 */
void internal_alias_remove_connection( Connection* conn );

/**
 * Called when the peer server resolves the peerid for a a name
 *
 */
void internal_alias_resolved_to( const std::string& alias, PeerId peer );

#endif // HUMBLENET_ALIAS_H
