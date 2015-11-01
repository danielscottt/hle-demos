#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <immintrin.h>

#include <hle-emulation.h>
#include <locker.h>

#define POOL_SIZE 4

static unsigned int hle_lock;
static int non_hle_lock;

void print_help()
{
        printf("cthreads: A progam that showcases the performance benefits of Intel® HLE\n");
        printf("\nUSAGE:\n");
        printf("  cthreads [COMMAND]\n");
        printf("\nCOMMANDS:\n");
        printf("  hle           run the program with hle\n");
        printf("  non-hle       run the program without hle\n");
}

void* hle_routine(void* _)
{
        int x;
        acquire_hle(&hle_lock);
        x = 1;
        sleep(1);
        release_hle(&hle_lock);
        return NULL;
}

void* not_hle_routine(void* _)
{
        int x;
        acquire(&non_hle_lock);
        x = 1;
        sleep(1);
        release(&non_hle_lock);
        return NULL;
}


void hle()
{
        pthread_t threads[POOL_SIZE];

        for (int i = 0; i < POOL_SIZE; i++) {
                pthread_t tid;
                pthread_create(&tid, NULL, hle_routine, NULL);
                threads[i] = tid;
        }

        for (int i = 0; i < POOL_SIZE; i++) {
                void* retval;
                pthread_join(threads[i], retval);
        }

}

void not_hle()
{
        pthread_t threads[POOL_SIZE];

        for (int i = 0; i < POOL_SIZE; i++) {
                pthread_t tid;
                pthread_create(&tid, NULL, not_hle_routine, NULL);
                threads[i] = tid;
        }

        for (int i = 0; i < POOL_SIZE; i++) {
                void* retval;
                pthread_join(threads[i], retval);
        }
}

int main(int argc, char *argv[])
{
        if (argc == 1) {
                print_help();
                return 0;
        }
        if (strcmp(argv[1], "hle") == 0) {
                hle();
        } else if (strcmp(argv[1], "not-hle") == 0) {
                not_hle();
        } else {
                print_help();
        }
        return 0;
}
