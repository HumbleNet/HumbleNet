#ifndef HUMBLENET_SIGNALING
#define HUMBLENET_SIGNALING

#include "humblepeer.h"
#include "libsocket.h"
#include <vector>

namespace humblenet {
    struct P2PSignalConnection {
        internal_socket_t *wsi;
        std::vector<uint8_t> recvBuf;
        std::vector<char> sendBuf;

        P2PSignalConnection()
        : wsi(NULL)
        {
        }

        void disconnect() {
            if( wsi )
                internal_close_socket(wsi);
            wsi = NULL;
        }

        ha_bool sendMessage(const uint8_t* buff, size_t length);

        ha_bool processMsg(const HumblePeer::Message* msg);
    };
    
    void register_protocol( internal_context_t* context );
}

ha_bool humblenet_signaling_connect();

#endif // HUMBLENET_SIGNALING
