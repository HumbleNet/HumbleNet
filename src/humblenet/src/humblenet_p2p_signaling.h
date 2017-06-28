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
    };
    
    void register_protocol( internal_context_t* contet );
}

ha_bool humblenet_signaling_connect();

#endif // HUMBLENET_SIGNALING
