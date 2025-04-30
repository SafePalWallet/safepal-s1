#ifndef _WALLET_UTILS_MUTEX_H
#define _WALLET_UTILS_MUTEX_H

#include <stdint.h>
#include <sys/types.h>
#include <pthread.h>

class Mutex {
public:

	Mutex();
	~Mutex();
	// lock or unlock the mutex
	int lock();
	void unlock();

	// lock if possible; returns 0 on success, error otherwise
	int tryLock();

	// Manages the mutex automatically. It'll be locked when Autolock is
	// constructed and released when Autolock goes out of scope.
	class Autolock {
	public:
		inline Autolock(Mutex &mutex) : mLock(mutex) { mLock.lock(); }

		inline Autolock(Mutex *mutex) : mLock(*mutex) { mLock.lock(); }

		inline ~Autolock() { mLock.unlock(); }

	private:
		Mutex &mLock;
	};

private:
	// A mutex cannot be copied
	pthread_mutex_t mMutex;
};

inline Mutex::Mutex() {
	pthread_mutex_init(&mMutex, NULL);
}

inline Mutex::~Mutex() {
	pthread_mutex_destroy(&mMutex);
}

inline int Mutex::lock() {
	return -pthread_mutex_lock(&mMutex);
}

inline void Mutex::unlock() {
	pthread_mutex_unlock(&mMutex);
}

inline int Mutex::tryLock() {
	return -pthread_mutex_trylock(&mMutex);
}

#endif