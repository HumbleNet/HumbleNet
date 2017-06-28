#include "humblenet.h"
#include "humblepeer.h"

#include <cassert>
#include <cstdio>
#include <cstring>
#include <sys/types.h>
#include <sys/stat.h>
#ifndef _WIN32
#include <unistd.h>
#endif


// helper program for AFLing parseMessage
// if you don't know what this is you don't need it


using namespace humblenet;


// these are unused but necessary so humblepeer.cpp can link
struct humblenet::P2PSignalConnection {
};


extern "C" {


static ha_bool parseCallback(HumblePeerMessageType msgType, const HumblePeerMessage *msg, const char *payload, void *user) {
	assert(user != NULL);

	// we use temp to make sure we have side-effects the compiler can't remove
	uintptr_t &temp = *reinterpret_cast<uintptr_t *>(user);

	unsigned int payloadLen = 0;

	switch(msgType) {
	case P2PConnect:
		{
			const P2PConnectMsg &p2pmsg = msg->p2pConnect;
			temp = temp ^ p2pmsg.peer;
			temp = temp ^ p2pmsg.flags;
			payloadLen = p2pmsg.payloadLen;
		}
		break;

	case P2PResponse:
		{
			const P2PResponseMsg &p2pmsg = msg->p2pResponse;
			temp = temp ^ p2pmsg.peer;
			payloadLen = p2pmsg.payloadLen;
		}
		break;

	case ICECandidate:
		{
			const ICECandidateMsg &p2pmsg = msg->iceCandidate;
			temp = temp ^ p2pmsg.peer;
			payloadLen = p2pmsg.payloadLen;
		}
		break;

	case ErrNoSuchPeer:
	case ErrPeerRefused:
		{
			const P2PErrMsg &err = msg->err;
			temp = temp ^ err.peer;
		}
		break;

	case HelloServer:
		{
			const HelloServerMsg &server = msg->serverHello;
			payloadLen = server.authTokenLen + server.browserInfoLen;
		}
		break;

	case HelloClient:
		{
			const HelloClientMsg &client = msg->clientHello;
			temp = temp ^ client.peerId;
			payloadLen = client.credentialLen;
		}
		break;

	case ConnectionEstablished:
		break;

	case CreateLobby:
		{
			const CreateLobbyMsg &lobbymsg = msg->createLobby;
			temp = temp ^ lobbymsg.maxUsers;
			payloadLen = lobbymsg.lobbyNameLen + lobbymsg.passwordLen;
		}
		break;

	case JoinLobby:
		{
			const JoinLobbyMsg &lobbymsg = msg->joinLobby;
			payloadLen = lobbymsg.lobbyNameLen + lobbymsg.passwordLen;
		}
		break;

	case CloseLobby:
		{
			const CloseLobbyMsg &lobbymsg = msg->closeLobby;
			payloadLen = lobbymsg.lobbyNameLen;
		}
		break;

	case LobbyPeerId:
		{
			const LobbyPeerIdMsg &lobbymsg = msg->lobbyPeerId;
			temp = temp ^ lobbymsg.peerId;
			payloadLen = lobbymsg.lobbyNameLen;
		}
		break;

	case ErrLobbyError:
		{
			const LobbyErrorMsg &lobbymsg = msg->lobbyError;
			payloadLen = lobbymsg.lobbyNameLen;
		}
		break;

	case LobbyEstablished:
		{
			const LobbyEstablishedMsg &lobbymsg = msg->lobbyEstablished;
			payloadLen = lobbymsg.lobbyNameLen;
		}
		break;

	case P2PData:
		{
			const P2PDataMsg &p2pdata = msg->p2pData;
			temp = temp ^ p2pdata.peer;
			payloadLen = p2pdata.payloadLen;
		}
		break;

	case P2PClosed:
		{
			const P2PClosedMsg &p2pClosed = msg->p2pClosed;
			temp = temp ^ p2pClosed.peer;
		}
		break;

	case LobbyJoined:
		{
			const LobbyJoinedMsg &lobbymsg = msg->lobbyJoined;
			temp = temp ^ lobbymsg.peerId;
			temp = temp ^ lobbymsg.lobbyId;
			payloadLen = lobbymsg.lobbyNameLen;
		}
		break;

	default:
		assert(false);
	}

	// read every byte of payload
	for (unsigned int i = 0; i < payloadLen; i++) {
		temp = (temp * 65537) ^ payload[i];
	}

	return true;
}


}  // extern "C"


int main(int argc, char *argv[]) {
	if (argc < 2) {
		return 1;
	}

	struct stat statbuf;
	memset(&statbuf, 0, sizeof(struct stat));

	FILE *f = fopen(argv[1], "r");
	if (!f) {
		return 2;
	}

	int retval = fstat(fileno(f), &statbuf);
	if (retval == 0) {
		size_t filesize = statbuf.st_size;
		std::vector<char> msgBuf(filesize, 0);
		fread(&msgBuf[0], 1, filesize, f);

		uintptr_t temp = 0;
		parseMessage(msgBuf, parseCallback, &temp);
		printf("temp: %lu\n", temp);
	}

	fclose(f);

	return 0;
}
