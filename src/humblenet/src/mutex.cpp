#include "mutex.h"

#ifdef WIN32
#include <windows.h>

struct mutex_t {
	CRITICAL_SECTION lock;
};

mutex_t* mutex_create() {
	auto m = new mutex_t;
	InitializeCriticalSectionAndSpinCount( &(m->lock), 2000 );
	return m;
}

void mutex_destroy(mutex_t* m)
{
	DeleteCriticalSection( &(m->lock) );
	delete m;
}

void mutex_lock(mutex_t* m)
{
	EnterCriticalSection( &(m->lock) );
}

int mutex_try_lock(mutex_t* m, long ms)
{
	if (ms > 0) {
		int ret = TryEnterCriticalSection( &(m->lock) );
		while (ret ==0 && ms > 0) {
			ms -= 10;
			Sleep( 10 );
			ret = TryEnterCriticalSection( &(m->lock) );
		}
		return ret != 0 ? 0 : 1;
	} else {
		return TryEnterCriticalSection( &(m->lock) ) != 0 ? 0 : 1;
	}
}

void mutex_unlock(mutex_t* m)
{
	LeaveCriticalSection( &(m->lock) );
}

#elif defined(EMSCRIPTEN)
// noop implementations

mutex_t *mutex_create() { return nullptr; }

void mutex_destroy(mutex_t *m) {}

void mutex_lock(mutex_t *m) {}

int mutex_try_lock(mutex_t *m, long ms) { return 0; }

void mutex_unlock(mutex_t *m) {}

#else
#include <pthread.h>
#include <unistd.h>

struct mutex_t {
	pthread_mutex_t lock;

	mutex_t() : lock(PTHREAD_MUTEX_INITIALIZER) {}
};

mutex_t *mutex_create()
{
	auto m = new mutex_t;
	pthread_mutexattr_t attr;

	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

	pthread_mutex_init(&m->lock, &attr);

	return m;
}

void mutex_destroy(mutex_t *m)
{
	pthread_mutex_destroy(&m->lock);
	delete m;
}

void mutex_lock(mutex_t *m)
{
	pthread_mutex_lock(&m->lock);
}


int mutex_try_lock(mutex_t *m, long ms)
{
	if (ms > 0) {
#if _POSIX_TIMEOUTS > 0
		struct timespec ts;

		ts.tv_sec = ms / 1000;
		ts.tv_nsec = 1000 * (ms % 1000);

		return pthread_mutex_timedlock(&m->lock, &ts);
#else
		int ret = 0;
		do {
			ret = pthread_mutex_trylock(&m->lock);
			if (!ret) break;
			usleep(1000); // 1ms
		} while (--ms > 0);

		return ret;
#endif
	} else {
		return pthread_mutex_trylock(&m->lock);
	}
}

void mutex_unlock(mutex_t *m)
{
	pthread_mutex_unlock(&m->lock);
}

#endif