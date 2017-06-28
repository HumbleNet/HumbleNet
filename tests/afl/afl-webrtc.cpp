#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#else
#include <unistd.h>
#include <semaphore.h>
#endif

#include <cassert>
#include <cstdio>
#include <cstring>
#include <sys/types.h>
#include <sys/stat.h>

#include <vector>


// helper program for AFLing webrtc microstack
// if you don't know what this is you don't need it


#include <openssl/ssl.h>


extern "C" {


#include <ILibAsyncSocket.h>
#include <ILibParsers.h>
#include <ILibWebRTC.h>
#include <ILibWrapperWebRTC.h>


void WebRTCOnConnectionCandidate(ILibWrapper_WebRTC_Connection connection, struct sockaddr_in6* candidate) {
	printf("WebRTCOnConnectionCandidate %p %p\n", connection, candidate);
}


void WebRTCConnectionSink(ILibWrapper_WebRTC_Connection webRTCConnection, int connected) {
	printf("WebRTCConnectionSink %p %d\n", webRTCConnection, connected);
}


void WebRTCDataChannelSink(ILibWrapper_WebRTC_Connection webRTCConnection, ILibWrapper_WebRTC_DataChannel *dataChannel) {
	printf("WebRTCDataChannelSink %p %p\n", webRTCConnection, dataChannel);
}


void WebRTCConnectionSendOkSink(ILibWrapper_WebRTC_Connection connection) {
	printf("WebRTCConnectionSendOkSink %p\n", connection);
}


}  // extern "C"


int main(int argc, char *argv[]) {
	if (argc < 2) {
		return 1;
	}

	void *webRTCChain = ILibCreateChain();	// Create the MicrostackChain, to which we'll attach the WebRTC ConnectionFactory
	assert(webRTCChain);
	// Create the connection factory, and bind to port "0", to use a random port
	ILibWrapper_WebRTC_ConnectionFactory webRTCConnectionFactory = ILibWrapper_WebRTC_ConnectionFactory_CreateConnectionFactory(webRTCChain, 0);
	assert(webRTCConnectionFactory);

	ILibWrapper_WebRTC_Connection webRTCConnection = ILibWrapper_WebRTC_ConnectionFactory_CreateConnection(webRTCConnectionFactory, &WebRTCConnectionSink, &WebRTCDataChannelSink, &WebRTCConnectionSendOkSink);

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
		if (msgBuf.back() != '\0') {
			msgBuf.push_back('\0');
		}

		char *offer = ILibWrapper_WebRTC_Connection_SetOffer(webRTCConnection, &msgBuf[0], msgBuf.size(), &WebRTCOnConnectionCandidate);

		printf("temp: \"%s\"\n", offer);
		free(offer);
	}

	fclose(f);

	ILibStopChain(webRTCChain); // Shutdown the chain

	ILibStartChain(webRTCChain);

	return 0;
}
