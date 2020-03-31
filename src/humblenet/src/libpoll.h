#ifndef LIBPOLL_H
#define LIBPOLL_H

#ifdef WIN32
#if defined(WINSOCK2)
#include <winsock2.h>
#include <ws2tcpip.h>
#elif defined(WINSOCK1)
#include <winsock.h>
#include <wininet.h>
#endif
typedef struct fd_set fd_set;
#else
#include <sys/select.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef void(*poll_pre_select)(void* object, fd_set *readset, fd_set *writeset, fd_set *errorset, int* blocktime);
typedef void(*poll_post_select)(void* object, int slct, fd_set *readset, fd_set *writeset, fd_set *errorset);
typedef void(*poll_pre_destroy)(void* object);

struct poll_module_t {
    poll_pre_select PreSelect;
    poll_post_select PostSelect;
    poll_pre_destroy Destroy;
};

typedef void (*poll_timeout_t)(void* data );

// Initialize the polling system
struct poll_context_t* poll_init();

// shutdown the polling system (will shutdown all bound modules as well)
void poll_deinit();
    
// return the poll context
bool poll_was_init();

// lock the polling system
void poll_lock();

// unlock the polling system
void poll_unlock();

// Wait at most timeout_ms for network IO to occur
void poll_wait( int timeout_ms );

// Add a module the polling system
void poll_add_module( poll_module_t* module );

// destroy a module (and remove from polling system)
void poll_destroy_module( poll_module_t* module );

// poll for triggered FDs (also lets the internal chain run )
int poll_select( int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);

// interrupt any poll select/wait in process.
void poll_interrupt();

// register a timeout callback
void poll_timeout( poll_timeout_t callback, int timeout_ms, void* user_data );

#ifdef __cplusplus
}
#endif

#endif // LIBPOLL_H