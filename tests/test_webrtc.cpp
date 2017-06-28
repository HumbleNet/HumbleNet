#include "libsocket.h"
#include "libpoll.h"

#include <string>
#include <vector>
#include <cstring>

static internal_context_t* context;

bool wait( int* pms ) {
	
	int ms = *pms;
	
	struct timeval tv;
	tv.tv_sec = ms / 1000;
	tv.tv_usec = 1000 * (ms % 1000);
	
	poll_select( 0, NULL, NULL, NULL, &tv );
	
	*pms = tv.tv_sec * 1000 + tv.tv_usec / 1000;
	
	return false;
}

bool wait( int ms ) {
	return wait( &ms );
}

/*
 * user_data is expected to be a vector to write the data received into.
 */
static int on_data(internal_socket_t*, const void* data, int len, void* user_data )
{
	auto mgs_queue = reinterpret_cast<std::vector<std::string>*>(user_data);
	
	mgs_queue->push_back( std::string( (const char*)data, len ) );

	return 0;
}

/*
 * user_data is expected to be the socket to send the offer to
 */
static int on_sdp_offer( internal_socket_t*, const char* offer, void* user_data ) {
	auto socket = reinterpret_cast< internal_socket_t* >( user_data );
	
	internal_set_offer( socket, offer );

	printf("sdp offer: %s\n", offer);

	return 0;
}

/*
 * user_data is expected to be the socket to send the anwser to
 */
static int on_sdp_answer( internal_socket_t*, const char* offer, void* user_data ) {
	auto socket = reinterpret_cast< internal_socket_t* >( user_data );
	
	internal_set_answer( socket, offer );

	printf("sdp answer: %s\n", offer);

	return 0;
}

/*
 * user_data is expected to be the socket to send the candidate to
 */
static int on_ice_candidate( internal_socket_t*, const char* candidate, void* user_data ) {
	auto socket = reinterpret_cast< internal_socket_t* >( user_data );
	
	internal_add_ice_candidate( socket, candidate );
	
	printf("ice: %s\n", candidate);
	
	return 0;
}

static int on_connected( internal_socket_t* socket, void* user_data ) {
	internal_create_channel( socket, "testing" );
	
	return 0;
}

static int on_channel_connected( internal_socket_t* socket, const char* name, void* user_data ) {
	// not sure...
	return 0;
}

static bool run_message_test(const char *label, internal_socket_t *left, internal_socket_t *right)
{
	internal_callbacks_t events;
	
	memset(&events, 0, sizeof(events));
	
	events.on_data = on_data;
	
	internal_set_callbacks(left, &events );
	internal_set_callbacks(right, &events );
	
	const char* binary_message;
	int received_message_count = 0;
	int expected_message_count = 2;
	
	std::vector<std::string> msg_queue;
	
	printf("[%s] starting\n", label);
	
//    g_signal_connect(left, "on-data", G_CALLBACK(on_data), msg_queue);
//    g_signal_connect(right, "on-data", G_CALLBACK(on_data), msg_queue);
//    g_signal_connect(left, "on-binary-data", G_CALLBACK(on_binary_data), msg_queue);
//    g_signal_connect(right, "on-binary-data", G_CALLBACK(on_binary_data), msg_queue);
	
	internal_set_data( left, &msg_queue );
	internal_set_data( right, &msg_queue );
	
	printf("[%s] sending messages\n", label);
	
	binary_message = "binary: left->right";
	internal_write_socket(left, binary_message, strlen(binary_message));
	binary_message = "binary: right->left";
	internal_write_socket(right, binary_message, strlen(binary_message));
	
	printf("[%s] expecting messages\n", label);
	
	int timeout = 5000;
	for (received_message_count = 0; received_message_count < expected_message_count; ) {
		wait(&timeout);
	
		// detect time has run out...
		if( timeout <= 0 )
		{
			printf("[%s] *** timeout while waiting for message\n", label);
			break;
		}
		
		for( auto it = msg_queue.begin(); it != msg_queue.end(); ) {
			const char* received_message = it->c_str();
			
			printf("[%s] received message: %s\n", label, received_message);
			
			received_message_count++;
			
			msg_queue.erase(it);
			
			it = msg_queue.begin();
		}
	}
	
	internal_close_socket( left );
	internal_close_socket( right );
	
	if (received_message_count >= expected_message_count) {
		printf("[%s] Success, ", label);
	} else {
		printf("[%s] Failure, ", label);
	}
	printf("received %d / %d messages\n", received_message_count, expected_message_count);
	
	return received_message_count >= expected_message_count;
}

/*
 * This sets up a left and right socket, then sends data and verifies it was all received.
 *
 */
bool connected = false;
int run_webrtc_data_test() {
	
	internal_callbacks_t caller;
	internal_callbacks_t callee;
	
	memset(&caller, 0, sizeof(caller));
	memset(&callee, 0, sizeof(callee));
	
	internal_socket_t* left = internal_create_webrtc(context);
	internal_socket_t* right = internal_create_webrtc(context);
	
	caller.on_sdp = on_sdp_offer;
	caller.on_ice_candidate = on_ice_candidate;
	caller.on_connect = on_connected;
	caller.on_connect_channel = [] ( internal_socket_t* socket, const char* name, void* user_data ) -> int {
		printf("connected sockets using channel: %s\n", name );
		connected = true;
		return 0;
	};
	
	callee.on_sdp = on_sdp_answer;
	callee.on_ice_candidate = on_ice_candidate;
	
	internal_set_callbacks( left, &caller );
	internal_set_callbacks( right, &callee );
	
	internal_set_data(left, right);
	internal_set_data(right,left);
	
	// this will trigger the whole process.
	internal_create_offer(left);
	
	int timeout = 5000;
	connected = false;
	while( ! connected ) {
		wait(&timeout);

		if( timeout <= 0 ) {
			printf("call setup timed out\n");
			return 0;
		}
	}
	
	return run_message_test("webrtc", left, right) ? 1 : 0;
}


int main(int argc, char **argv)
{
	int success_count = 0;
	int test_count = 0;
	
	internal_callbacks_t callbacks;
	
	memset(&callbacks, 0, sizeof(callbacks));
	
	context = internal_init(  &callbacks );
	
	const char* stun[] = { HUMBLENET_SERVER_URL };
	internal_set_stun_servers(context, stun, 1);

	{
		success_count += run_webrtc_data_test(); test_count++;
		success_count += run_webrtc_data_test(); test_count++;
		success_count += run_webrtc_data_test(); test_count++;

		//success_count += run_test(HUMBLENET_SERVER_URL); test_count++;
		
		printf("\n%d / %d test were successful\n", success_count, test_count);
	}
	
	internal_deinit(context);
	
	return test_count - success_count;
}
