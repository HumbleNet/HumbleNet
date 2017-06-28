/*
 *  webrtc_native.cpp
 *  webrtc_native
 *
 *  Created by Chris Rudd on 6/12/16.
 *
 *
 */

#include "libwebrtc.h"

#define WEBRTC_API 


#include "webrtc/api/peerconnection.h"
#include "webrtc/api/datachannel.h"
#include "webrtc/base/ssladapter.h"

#include "webrtc/api/test/fakeconstraints.h"
#define WEBRTC_BASE_GUNIT_H_
#include "webrtc/media/engine/fakewebrtcvideoengine.h"
#include "webrtc/api/test/fakeaudiocapturemodule.h"

#include "webrtc/base/asyncinvoker.h"

#include <assert.h>
#include <vector>

#undef LOG
#define LOG printf

#define WEBRTC_PRE_52   // 52 changes some API's

struct libwebrtc_context {
    lwrtc_callback_function callback;
    std::vector<std::string> stunServers;

    std::unique_ptr<rtc::Thread> network_thread;
    std::unique_ptr<rtc::Thread> worker_thread;
    std::unique_ptr<rtc::Thread> signaling_thread;

    rtc::AsyncInvoker invoker;

    rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> factory;

    template<class F>
    void AsyncInvokeOnSignalingThread( const F& functor ) {
        invoker.AsyncInvoke<void>( signaling_thread.get(), functor );
    }

    template<class F>
    void AsyncInvokeOnSignalingThread( long ms, const F& functor ) {
        invoker.AsyncInvokeDelayed<void>( signaling_thread.get(), functor, ms );
    }

};

struct libwebrtc_connection : public webrtc::PeerConnectionObserver {
    libwebrtc_context* ctx;
    void* user_data;

    rtc::scoped_refptr<webrtc::PeerConnectionInterface> connection;
    rtc::scoped_refptr<webrtc::DataChannelInterface> firstChannel;

    virtual ~libwebrtc_connection() {
        LOG("Destroying connection\n");
    }

    void OnSignalingChange(
                           webrtc::PeerConnectionInterface::SignalingState new_state);
    void OnIceConnectionChange(
                               webrtc::PeerConnectionInterface::IceConnectionState new_state);
    void OnIceGatheringChange(
                              webrtc::PeerConnectionInterface::IceGatheringState new_state){}
    void OnIceCandidate( const webrtc::IceCandidateInterface* candidate );
    void OnRenegotiationNeeded(){
        LOG("Renegotiation needed\n");
    };
    void OnDataChannel( rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel );

#ifdef WEBRTC_PRE_52
    void OnDataChannel( webrtc::DataChannelInterface* data_channel ) {
        OnDataChannel( rtc::scoped_refptr<webrtc::DataChannelInterface>( data_channel ) );
    }
    void OnAddStream(webrtc::MediaStreamInterface* stream) {}
    void OnRemoveStream(webrtc::MediaStreamInterface* stream) {}
#endif
};

struct libwebrtc_data_channel : public webrtc::DataChannelObserver{
    libwebrtc_connection* parent;
    void* user_data;

    rtc::scoped_refptr<webrtc::DataChannelInterface> channel;
    bool inbound;

    libwebrtc_data_channel( libwebrtc_connection* parent, rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel, bool inbound = false )
    :parent(parent)
    ,user_data(parent->user_data)
    ,channel( data_channel )
    ,inbound( inbound )
    {
        channel->RegisterObserver(this);
    }

    virtual ~libwebrtc_data_channel() {
        LOG("Destroying channel\n");
    }

    void OnStateChange() {
        assert( parent != NULL );
        assert( parent->ctx != NULL );
        assert( channel != NULL );

        assert( parent->ctx->signaling_thread->IsCurrent() );

        switch( channel->state() ) {
            case webrtc::DataChannelInterface::kOpen:
                LOG("Data channel opened\n");
                parent->ctx->callback( parent->ctx, parent, this, inbound ? LWRTC_CALLBACK_CHANNEL_ACCEPTED : LWRTC_CALLBACK_CHANNEL_CONNECTED, user_data, (void*)(channel->label().c_str()), (int)channel->label().size() );
                break;
            case webrtc::DataChannelInterface::kClosed:
                LOG("Data channel closed\n");
                parent->ctx->callback( parent->ctx, parent, this, LWRTC_CALLBACK_CHANNEL_CLOSED, user_data, (void*)(channel->label().c_str()), (int)channel->label().size() );

                channel->UnregisterObserver();
                channel.release();

                delete this;

                break;
            default:
                break;
        }
    }

    void OnMessage(const webrtc::DataBuffer& buffer ) {
        assert( parent != NULL );
        assert( parent->ctx != NULL );

        assert( parent->ctx->signaling_thread->IsCurrent() );

        if( parent->ctx->callback( parent->ctx, parent, this, LWRTC_CALLBACK_CHANNEL_RECEIVE, user_data, (void*)buffer.data.data<uint8_t>(), (int)buffer.size() ) ) {
            // callback failed, close channel
            LOG("CHANNEL_RECEIVE closed connection\n");
            channel->Close();
        }
    }

    void OnBufferedAmountChange(uint64_t previous_amount){
    }
};

class OnSetSessionDescriptor: public rtc::RefCountedObject<webrtc::SetSessionDescriptionObserver> {
public:
    virtual void OnSuccess() {
        LOG("Descriptor set\n");
    }

    virtual void OnFailure(const std::string& error ) {
        LOG("Failed to set session descriptor: %s\n", error.c_str() );
    }
private:
};

class OnCreateSessionDescription : public rtc::RefCountedObject<webrtc::CreateSessionDescriptionObserver> {
public:
    OnCreateSessionDescription( libwebrtc_connection* c )
    :conn(c)
    {}

    virtual void OnSuccess(webrtc::SessionDescriptionInterface* desc) {
        assert( conn != NULL );
        assert( conn->ctx != NULL );
        assert( conn->ctx->signaling_thread->IsCurrent() );

        conn->connection->SetLocalDescription( new OnSetSessionDescriptor(), desc );

        std::string sdp;

        if( ! desc->ToString(&sdp) ) {
            LOG("Failed to convert session descriotion to sdp string!!!\n");
        } else if( conn->ctx->callback( conn->ctx, conn, NULL, LWRTC_CALLBACK_LOCAL_DESCRIPTION, conn->user_data, (void*)(sdp.c_str()), (int)(sdp.size())) ) {
            LOG("LOCAL_DESCRIPTION callback failed\n");
        }
    }

    virtual void OnFailure(const std::string& error ) {
        assert( conn != NULL );
        assert( conn->ctx != NULL );
        assert( conn->ctx->signaling_thread->IsCurrent() );

        LOG("Failed to create local session descriptor: %s\n", error.c_str() );
        conn->connection->Close();
    }
private:
    libwebrtc_connection* conn;
};

void libwebrtc_connection::OnSignalingChange(
                       webrtc::PeerConnectionInterface::SignalingState new_state){
    assert( ctx != NULL );
    assert( ctx->signaling_thread->IsCurrent() );

    LOG("Signaling state changed to : %d\n", new_state );

    if( new_state == webrtc::PeerConnectionInterface::SignalingState::kClosed ) {
        // defer the destroy callback
        ctx->AsyncInvokeOnSignalingThread(1000, [this] {
            if( ctx->callback( ctx, this, NULL, LWRTC_CALLBACK_DESTROY, user_data, NULL, 0 ) ) {

            }
            delete this;
        });

        connection.release();
    }
};

void libwebrtc_connection::OnIceConnectionChange(
                           webrtc::PeerConnectionInterface::IceConnectionState new_state){
    assert( ctx != NULL );
    assert( ctx->signaling_thread->IsCurrent() );

    LOG("Ice Connection state changed to : %d on connection: %p\n", new_state, connection.get() );

    switch ( new_state ) {
        case webrtc::PeerConnectionInterface::kIceConnectionConnected:
            if( ctx->callback( ctx, this, NULL, LWRTC_CALLBACK_ESTABLISHED, user_data, NULL, 0 ) ) {
                connection->Close();
            }
            break;
        case webrtc::PeerConnectionInterface::kIceConnectionDisconnected:
            connection->Close();
            break;
        case webrtc::PeerConnectionInterface::kIceConnectionClosed:
            if( ctx->callback( ctx, this, NULL, LWRTC_CALLBACK_DISCONNECTED, user_data, NULL, 0 ) ) {
            }
            break;
        default:
            break;
    }
};

void libwebrtc_connection::OnIceCandidate( const webrtc::IceCandidateInterface* candidate ) {
    assert( ctx != NULL );
    assert( ctx->signaling_thread->IsCurrent() );

    std::string spd;

    if( ! candidate->ToString(&spd) ) {
        LOG("Failed to convert ice candidate to spd string!!!!\n");
    } else if( ctx->callback(ctx, this, NULL, LWRTC_CALLBACK_ICE_CANDIDATE, user_data, (void*)(spd.c_str()), (int)(spd.size()) ) ) {
        LOG("Callback failed the ice candidate request\n");
    }
}

void libwebrtc_connection::OnDataChannel( rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel ) {
    assert( ctx != NULL );
    assert( ctx->signaling_thread->IsCurrent() );

    libwebrtc_data_channel* channel = new libwebrtc_data_channel( this, data_channel, true );

    // status changes on the DC will propagate the connection to the callback.
}


//
// Start of public API implementation
//

WEBRTC_API struct libwebrtc_context* libwebrtc_create_context(lwrtc_callback_function callback) {
    assert( callback != NULL );
    libwebrtc_context* ctx = new libwebrtc_context();


    rtc::InitializeSSL();

    ctx->callback = callback;
    // we cant use the default create, as that binds to the current thread, which we dont have a run loop for, thus the queue is not processed on it.
#ifdef WEBRTC_PRE_52
    ctx->worker_thread.reset( new rtc::Thread()  );
    ctx->signaling_thread.reset( new rtc::Thread()  );

    ctx->worker_thread->Start();
    ctx->signaling_thread->Start();

    ctx->factory = webrtc::CreatePeerConnectionFactory( ctx->worker_thread.get(), ctx->signaling_thread.get(), 
	FakeAudioCaptureModule::Create(),
	new cricket::FakeWebRtcVideoEncoderFactory(),
	new cricket::FakeWebRtcVideoDecoderFactory() );
#else
    ctx->network_thread.reset( rtc::Thread::CreateWithSocketServer().release() );
    ctx->worker_thread.reset( rtc::Thread::Create().release() );
    ctx->signaling_thread.reset( rtc::Thread::Create().release() );

    ctx->network_thread->Start();
    ctx->worker_thread->Start();
    ctx->signaling_thread->Start();

    ctx->factory = webrtc::CreatePeerConnectionFactory( ctx->network_thread.get(), ctx->worker_thread.get(), ctx->signaling_thread.get(), nullptr, nullptr, nullptr );
#endif
    return ctx;
}

WEBRTC_API void libwebrtc_destroy_context( struct libwebrtc_context* ctx ) {

//    ctx->network_thread->Stop();
//    ctx->signaling_thread->Stop();
//    ctx->worker_thread->Stop();

    delete ctx;
}

WEBRTC_API void libwebrtc_set_stun_servers( struct libwebrtc_context* ctx, const char** servers, int count)
{
    ctx->stunServers.clear();
    for( int i = 0; i < count; ++i ) {
        ctx->stunServers.push_back( *servers );
        servers++;
    }
}

WEBRTC_API struct libwebrtc_connection* libwebrtc_create_connection_extended( struct libwebrtc_context* ctx, void* user_data )
{
    webrtc::PeerConnectionInterface::RTCConfiguration config;

    for( const std::string& server : ctx->stunServers ) {
        webrtc::PeerConnectionInterface::IceServer ice_server;
        ice_server.uri = "stun:" + server;
        config.servers.push_back(ice_server);
    }

    webrtc::FakeConstraints constraints;

    constraints.AddMandatory(webrtc::MediaConstraintsInterface::kEnableDtlsSrtp, "true");

    libwebrtc_connection* c = new libwebrtc_connection();

    c->ctx = ctx;
    c->user_data = user_data;
    c->connection = ctx->factory->CreatePeerConnection( config, &constraints, nullptr, nullptr, c );

    return c;
}

WEBRTC_API struct libwebrtc_data_channel* libwebrtc_create_channel( struct libwebrtc_connection* c, const char* name )
{
    webrtc::DataChannelInit config;

    config.ordered = true;
    config.reliable = true;

    rtc::scoped_refptr<webrtc::DataChannelInterface> dc = c->firstChannel.get() ? c->firstChannel : c->connection->CreateDataChannel( name, &config);
    assert( dc.get() != NULL );

    libwebrtc_data_channel* channel = new libwebrtc_data_channel( c, dc );

    if( c->firstChannel.get() ) {
        c->firstChannel.release();

        // Not sure why status change doesnt occur for this channel...
        c->ctx->AsyncInvokeOnSignalingThread( [channel] {
            channel->OnStateChange();
        });
    }

    return channel;
}

WEBRTC_API int libwebrtc_create_offer( struct libwebrtc_connection* c ) {

    webrtc::PeerConnectionInterface::RTCOfferAnswerOptions options;

    options.offer_to_receive_audio = false;
    options.offer_to_receive_video = false;

    webrtc::DataChannelInit config;

    config.ordered = true;
    config.reliable = true;

    c->firstChannel = c->connection->CreateDataChannel( "default", &config);

    c->connection->CreateOffer( new OnCreateSessionDescription(c), options );

    return 1;
}

WEBRTC_API int libwebrtc_set_offer( struct libwebrtc_connection* conn, const char* sdp ) {

    webrtc::SdpParseError error;

    webrtc::SessionDescriptionInterface* description = webrtc::CreateSessionDescription(webrtc::SessionDescriptionInterface::kOffer, sdp, &error );

    if( !description ) {
        LOG("set_offer failed @ %s : %s\n", error.line.c_str(), error.description.c_str() );
        return 0;
    }

    // TODO: should this be a blocking call (wait on result)
    conn->connection->SetRemoteDescription( new OnSetSessionDescriptor(/* completion block */), description );

    webrtc::PeerConnectionInterface::RTCOfferAnswerOptions options;

    options.offer_to_receive_audio = false;
    options.offer_to_receive_video = false;

    conn->connection->CreateAnswer( new OnCreateSessionDescription(conn), options);

    return 1;
}

WEBRTC_API int libwebrtc_set_answer( struct libwebrtc_connection* conn, const char* sdp ) {
    webrtc::SdpParseError error;

    webrtc::SessionDescriptionInterface* description = webrtc::CreateSessionDescription(webrtc::SessionDescriptionInterface::kAnswer, sdp, &error );

    if( !description ) {
        LOG("set_answer failed [%s] @ %s : %s\n", sdp, error.line.c_str(), error.description.c_str() );
        return 0;
    }

    // TODO: should this be a blocking call (wait on result)
    conn->connection->SetRemoteDescription( new OnSetSessionDescriptor(), description );

    return 1;
}

WEBRTC_API int libwebrtc_add_ice_candidate( struct libwebrtc_connection* conn, const char* candidate ) {

    webrtc::SdpParseError error;

    std::string spd_mid = "data"; // ??? Candidate->spd_mid(), but we dont currently transfer that
    int spd_mline_index = 0; // ??? Candidate->spd_mline_index(), but we dont currently transfer that

    webrtc::IceCandidateInterface* ice = webrtc::CreateIceCandidate(spd_mid, spd_mline_index, candidate, &error);

    if( !ice ) {
        LOG("add_ice_candidate failed @ %s : %s\n", error.line.c_str(), error.description.c_str() );
        return 0;
    }

    conn->connection->AddIceCandidate( ice );

    return 1;
}

WEBRTC_API int libwebrtc_write( struct libwebrtc_data_channel* dc, const void* data, int len ) {

    rtc::CopyOnWriteBuffer buffer( (uint8_t*)data, len );

    // Not sure if send makes a copy....
    if( ! dc->channel->Send( webrtc::DataBuffer(buffer, true) ) ) {
        return -1;
    }

    return len;
}

WEBRTC_API void libwebrtc_close_channel( struct libwebrtc_data_channel* channel ) {
    assert( channel != NULL );
    assert( channel->channel.get() != NULL );

    channel->channel->Close();
}

WEBRTC_API void libwebrtc_close_connection( struct libwebrtc_connection* conn ) {
    assert( conn != NULL );
    assert( conn->connection.get() != NULL );

    conn->connection->Close();
}
