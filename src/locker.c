#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <immintrin.h>

struct locker *locker_init(int type)
{
        struct locker *l = malloc(sizeof(struct locker));
        assert(l != NULL);
        l->lock = (enum lock_states) malloc(sizeof(unlocked));
        l->lock = unlocked;
        l->lock_type = type;
        return l;
}

void acquire(struct locker *l)
{
        int ll, ul;
        ll = locked;
        ul = unlocked;
        int *ulp = (int*)&ul;
        do {
                ul = unlocked;
                ulp = (int*)&ul;
                _mm_pause();
        } while (
                __atomic_compare_exchange_n(&l->lock,
                                            ulp,
                                            (int*)&ll,
                                            true,
                                            l->lock_type,
                                            l->lock_type) != true);
}

void release(struct locker *l)
{
        __atomic_store_n(&l->lock, unlocked, l->lock_type);
}
