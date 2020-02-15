#ifndef MUTEX_H
#define MUTEX_H

struct mutex_t;

mutex_t *mutex_create();

void mutex_destroy(mutex_t *m);

void mutex_lock(mutex_t *m);

int mutex_try_lock(mutex_t *m, long ms);

void mutex_unlock(mutex_t *m);

class basic_lock_t {
public:
	basic_lock_t() = delete;

	basic_lock_t(const basic_lock_t &) = delete;

	basic_lock_t &operator=(const basic_lock_t &) = delete;

	explicit basic_lock_t(mutex_t *m) : m_lock(m)
	{
		mutex_lock(m_lock);
	}

	~basic_lock_t()
	{
		mutex_unlock(m_lock);
	}

private:

	mutex_t *m_lock;
};

#endif // MUTEX_H