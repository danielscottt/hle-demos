enum lock_states {unlocked, locked};

struct locker {
        enum lock_states lock;
        int lock_type;
};

struct locker *locker_init(int type);
void acquire(struct locker *l);
void release(struct locker *l);
