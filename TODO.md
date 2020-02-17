# HumbleNet TODO list

## Known Bugs

### WebRTC Stun handling (Desktop. Not an issue when using chromium WebRTC stack)
- [x] get it connecting to coturn correctly
- [ ] get it handling multiple STUN requests at a time
- [ ] get it not crashing with canceled STUN requests

### Browser issues
- [ ] figure out why/workaround an issue where some installs of Firefox do not send correct lifecycle events

## Components

### P2P API
The core WebRTC P2P Component of HumbleNet.  Provides a simple P2P contract that allows you to send data to a known Peer

#### P2P TODO
- [ ] Reconnect API
    - when connecting a reconnect token should be issues that the client will use when reconnecting to the peer-server
    - reconnecting with this token will provide the same Peer ID to the client
    - this will provide continuity in peer IDs and lobby state etc during brief network outages.
- [x] Ability to directly lookup a name alias (either a polling method or an event callback)
- [ ] Cleanup internal socket handling to no longer need the old Connection API contract
- [ ] connection retry
- [x] add events 
  - [ ] peer-server connection failure
  - [x] peer-server connect
  - [ ] peer-server disconnect
  - [ ] peer-server reconnect
  - [x] Assignment of My peer ID
  - [ ] new incoming peer (for explicit accept/reject policy)
    - [ ] if event is disabled then policy would be auto-accept any incoming connection
  - [x] connected / disconnected to/from peer
  - [x] name alias resolved
 
#### Questions
- [ ] How should we handle connecting to my self?
- [ ] How should we handle cleaner notification of disconnected peer state?

### Event API (TODO)
This is a simple API (build similar to how the SDL2 Event system works) that allows a game to opt-in to certain events via a polling loop or watcher.

- [ ] basic functionality would be
  - [x] PollEvent
  - [ ] Enable/Disable Event,
    - [ ] If an event is disabled it will have a predefined behavior.
  - [ ] and Add/Remove Event Watcher

### Lobby API (TODO)
This is the core lobby functionality for advanced peer discovery. A lobby is simply a group of peers that can be searched by certain attributes.

- [ ] build up the final spec for a lobby
  - [ ] lobbies have a unique non-guessable ID
  - [ ] lobbies have an owner which can be "re-assigned"
  - [ ] lobbies have a set of attributes (e.g. name, server, min-level, etc..  fully dynamic to the game)
  - [ ] lobbies can be searched based on the attribute values or attribute value ranges. (e.g. map=demo1 or skill>5 & skill < 10)
  - [ ] lobbies can have a password to restrict joining
  - [ ] lobbies can be public (searchable), or private
  - [ ] lobbies can have a maximum number of members
- [ ] add events for lobbies
  - [ ] created lobby
  - [ ] joined lobby
  - [ ] member joined
  - [ ] member left
  - [ ] attributes changed

### WebSocket API (TODO)
The websocket API provides a simple cross platform API to connect to existing WebSocket servers (for more client-server applications that do not need WebRTC)

- [ ] method to connect to a server with a list of protocols to attempt to connect with
- [ ] ability to query what protocol was accepted
- [ ] send data
- [ ] receive data
- [ ] poll
- [ ] disconnect
