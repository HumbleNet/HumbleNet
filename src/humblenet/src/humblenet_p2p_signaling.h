#ifndef HUMBLENET_SIGNALING
#define HUMBLENET_SIGNALING

#include "humblepeer.h"
#include "libsocket.h"
#include <vector>

namespace humblenet {
    enum SignalingState {
        SIGNALING_DISCONNECTED,
        SIGNALING_WAITING,
        SIGNALING_CONNECTING,
        SIGNALING_CONNECTED
    };

    struct P2PSignalConnection {
        internal_socket_t *wsi;
        SignalingState state;
        uint64_t last_attempt;
        std::vector<uint8_t> recvBuf;
        std::vector<char> sendBuf;

        P2PSignalConnection();
        ~P2PSignalConnection();

        bool connect();
        bool valid();
        void disconnect();

        bool reconnect();
    };
    
    void register_protocol( internal_context_t* contet );
}

ha_bool humblenet_signaling_connect();

#endif // HUMBLENET_SIGNALING
