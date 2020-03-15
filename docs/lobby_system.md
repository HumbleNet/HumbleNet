# Lobby system

## Concepts

### Basic lobby definition
A lobby is an object to allow discovering games either via direct invites or match making.

A lobby consists of the following properties

- owner &mdash; The user who created the lobby and can change its properties
- type &mdash; The type of the lobby (e.g. private, public, etc..)
- max_peers &mdash; The maximum number of members for the lobby
- serverPeer &mdash; the primary peer. Use this when a game session has a dedicated server 
- attributes &mdash; A series of game-specific attributes
   * Represented as simple key/value string pairs
   * Can be used to transport game specific information (e.g. map, description, etc.)
   * Can be used for matching making and search filters
- members &mdash; A set of peers of users who have joined the lobby.
- member properties &mdash; Each peer has a set game-defined properties.
   * Can be used to track "ready" state of each player. 

### Searching lobbies
Lobbies can be discovered in a several ways:

- Invite code &mdash; A short code can be generated for a lobby which can
   * be entered into your game (e.g. local-play join party)
   * embedded in a link (e.g. as a startup parameter for wasm game)
   - TODO do we really need a specific thing like this? 
        or can we simply use a custom attribute?
- Search API
   * Searches can be requested based on the lobby attributes.
   * Supports simple comparison operators (=, >, <, >=, <=, !=)
   * Supports interpreting as certain data types (string, number, float)
TODO do we need passwords for lobbies?

### Lobby broadcast channel

Lobbies also provide a lightweight broadcast channel that allows you to send simple messages over the lobby channel to all of the joined members.
This can be used to build a simple pre-game chat or push pre-game member state changes (e.g. player ready)

## Data types

- LobbyId &mdash; the internal lobby handle
   * should not be exposed outside of the game
   * use th short code to share an identifier to invite outside players
- LobbyType &mdash; The type of lobby
   * Private &mdash; The lobby is not searchable and is invite only
   * Public &mdash; The lobby is searchable
- AttributeMap &mdash; A generic attribute map
   * consists of a string key and a string value

## Methods

### Creating, joining, and leaving lobbies

`requestId humblenet_lobby_create(LobbyType type, uint16_t maxMembers)`
- create a new lobby

`requestId humblenet_lobby_join(LobbyId)`
- request to join a lobby

`requestId humblenet_lobby_leave(LobbyId)`
- You must already be a member of the lobby 
- leave the lobby

### Lobby property methods

- You must already be a member of the lobby (local request)

`PeerId humblenet_lobby_owner(LobbyId)`
- Returns the owner Peer for the lobby

`uint16_t humblenet_lobby_max_members(LobbyId)`
- Returns the max members for the lobby

`LobbyType humblenet_lobby_type(LobbyId)`
- Returns the lobby type

`uint16_t humblenet_lobby_member_count(LobbyId)`
- Returns the number of members in a lobby

`uint16_t humblenet_lobby_get_members(LobbyId, PeerId*, uint16_t nelts)`
- Fetches the list of peer IDs in the lobby

`AttributeMap* humblenet_lobby_get_attributes(LobbyId)`
- Returns the attributes for the lobby

`AttributeMap* humblenet_lobby_get_member_attributes(LobbyId)`
- Returns the attributes for the lobby

### Modifying lobby methods

`ha_requestId humblenet_lobby_set_max_members(LobbyId, uint16_t maxMembers);`
- adjusts the max members 

`ha_requestId humblenet_lobby_set_type(LobbyId, humblenet_LobbyType);`
- adjusts the lobby type

## Events

### LOBBY_CREATE_SUCCESS

Triggered when the lobby is successfully created. Includes a reference to the requestId and the lobbyId of the new lobby.

### LOBBY_JOIN

Triggered when you join a lobby

### LOBBY_LEAVE

Triggered when you leave a lobby

### LOBBY_UPDATE

Triggered when lobby attributes are updated.  

### LOBBY_MEMBER_JOIN

Triggered when a member joins the lobby. Also sent when creating a lobby (because you joined the newly created lobby)

### LOBBY_MEMBER_LEAVE

Triggered when a member leaves the lobby.

### LOBBY_MEMBER_UPDATE

Triggered when a member's attributes are updated.

### LOBBY_CREATE_ERROR

Triggered when creating a lobby failed

### LOBBY_JOIN_ERROR

Triggered when failing to join a lobby.  e.g. lobby didn't exist

### LOBBY_LEAVE_ERROR

Triggered when failing to leave a lobby

## Peer server messages

### LobbyCreate [client]
Client request to create a lobby

### LobbyJoin [client]
Client request to join a lobby

### LobbyLeave [client]
Client request to leave a lobby

### LobbyUpdate [client]
Client sending updates for a lobby

### LobbyMemberUpdate [client]
Client sending member updates

### LobbyDidCreate [server]
Server response for the created lobby
- includes lobby details (should be exactly what was requested)

### LobbyDidJoin [server]
Server response for 
- successful join (myself) (requestId != 0)
    - includes lobby details
    - includes member attributes
- new member added (myself to other member) (requestId == 0)
    - includes member attributes
- existing member added (other members to myself) (requestId == 0)
    - includes member attributes

### LobbyDidUpdate [server]
Server response including lobby metadata

### LobbyMemberDidUpdate [server]
Server response including lobby member metadata

### LobbyDidLeave [server]
Server response for 
- successful leave (myself) (requestId != 0)
- member removed (other member) (requestId == 0)

### Errors
- LobbyCreateError
    - ?
- LobbyJoinError
    - when lobby ID is invalid
- LobbyLeaveError
    - when lobby ID is invalid
- LobbyUpdateError
    - when requester is not owner of lobby
- LobbyMemberUpdateError
    - when requester is not the member

## TODO

- [ ] parameter validation
  - [ ] max_size <= 1 ?
  - [ ] attribute length limits?
- [ ] enforce max members
- [ ] when changing max members what to do about current member count being above
- [ ] implement kicking members
- [ ] implement search API
- [ ] limit number of lobbies a user may be a member? (e.g. 1!)
- [ ] implement deleting lobby on last leave
- [ ] implement leaving lobby on peer disconnect
- [ ] implement passwords on lobbies?
- [ ] implement lobby chat
- [ ] implement joinable property?
- [ ] implement serverPeer concept