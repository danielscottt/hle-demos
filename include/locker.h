#define UNLOCKED 0
#define LOCKED 1

void acquire(int* lock);
void release(int* lock);

void acquire_hle(unsigned int* lock);
void release_hle(unsigned int* lock);
