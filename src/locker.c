#include <stdlib.h>
#include <immintrin.h>

#include <hle-emulation.h>
#include <locker.h>

void acquire(int* lock)
{
        int ll = LOCKED;
        int ul = UNLOCKED;
        do {
                ul = UNLOCKED;
                _mm_pause();
        } while (
                __atomic_compare_exchange_n(lock,
                                            &ul,
                                            ll,
                                            1,
                                            __ATOMIC_ACQUIRE,
                                            __ATOMIC_ACQUIRE) != 1);
}

void acquire_hle(unsigned int* lock)
{
        while (__hle_acquire_test_and_set4(lock) == 1) {
                while (*lock == 0)
                        _mm_pause();
        }
}

void release_hle(unsigned int* lock)
{
        __hle_release_clear4(lock);
}


void release(int* lock)
{
        __atomic_store_n(lock, UNLOCKED, __ATOMIC_RELEASE);
}
