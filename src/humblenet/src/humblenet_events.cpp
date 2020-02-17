#include "humblenet_events_internal.h"

static_assert( sizeof( HumbleNet_Event ) == 64, "HumbleNet_Event size incorrect" );

#include "circular_buffer.h"
#include "mutex.h"

static CircularBuffer<HumbleNet_Event> g_eventQueue( 64 );
static mutex_t* g_eventLock = nullptr;

void humblenet_event_init()
{
	if (!g_eventLock) {
		g_eventLock = mutex_create();
	}
}


ha_bool HUMBLENET_CALL humblenet_event_poll(HumbleNet_Event* event)
{
	humblenet_event_init();

	basic_lock_t l( g_eventLock );

	if (event) {
		return g_eventQueue.pop( *event );
	} else {
		return g_eventQueue.used() > 0;
	}
}

bool humblenet_event_push(HumbleNet_Event& event)
{
	humblenet_event_init();

	basic_lock_t l( g_eventLock );

	return g_eventQueue.push( event );
}