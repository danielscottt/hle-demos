#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/msg.h>
#include <pthread.h>
#include <mach/clock.h>
#include <mach/mach.h>
#include <signal.h>

#include <hle-emulation.h>
#include <locker.h>

#define HLE_TYPE 0
#define NON_HLE_TYPE 1
#define THREAD_POOL_SIZE 50

// the int we flip with hle
static unsigned int hle_lock;

// the int we flip without hle
static int non_hle_lock;

// the averager doubleton.  Both the hle and non-hle
// samples have their own global copy.
// The watch_averages thread reads both while the
// work_upstream_q threads write to it.
typedef struct averager {
        unsigned long total;
        unsigned long count;
} averager;

averager hle_avg = {.total = 0, .count = 0};
averager non_hle_avg = {.total = 0, .count = 0};

// structure passed to each worker thread
typedef struct worker_arg_t {
        int q;
        int touch;
        int type;
        int id;
} worker_arg_t;

// a structure used to track all threads which need to be 
// cancelled at exit.
typedef struct thread_tracker_t {
        pthread_t hle[THREAD_POOL_SIZE];
        pthread_t nonhle[THREAD_POOL_SIZE];
} thread_tracker_t;

// the message structure passed to the upstream queue.
typedef struct msg_t {
        long mtype;
        int msg_type;
        int diff;
} msg_t;

void print_help()
{
        printf("avg: A progam that showcases the performance benefits of Intel® HLE\n");
        printf("\nUSAGE:\n");
        printf("  avg [COMMAND]\n");
        printf("\nCOMMANDS:\n");
        printf("  run           run the demo\n");
}

// this is the function run on the recieving side of each queue.
// the workers send their time spent in the "critical section" here.
void* work_upstream_q(void* qid_ptr)
{
        int* qid = (int*)qid_ptr;
        msg_t msg;

        while (msgrcv(*qid, &msg, sizeof(msg_t)-sizeof(long), 1, 0) > 0) {
                switch (msg.msg_type) {
                case HLE_TYPE:
                        hle_avg.total = hle_avg.total + msg.diff;
                        hle_avg.count = hle_avg.count + 1;
                        break;
                case NON_HLE_TYPE:
                        non_hle_avg.total = non_hle_avg.total + msg.diff;
                        non_hle_avg.count = non_hle_avg.count + 1;
                        break;
                default:
                        break;
                }
        }
        return NULL;
}

// initializes and returns a reference to the id for
// a kernel-based queue.
int init_mq(key_t key)
{
        pthread_t tid;
        int* id = malloc(sizeof(int));
        int msgflg = IPC_CREAT | 0666;

        // open the queue. create it if it does
        // not exist.
        if ((*id = msgget(key, msgflg)) < 0)
                return -1;

        // create the which binds to the recieving side of the queue.
        if (pthread_create(&tid, NULL, work_upstream_q, id) != 0)
                return -1;

         return *id;
}

// the routine each worker executes.
void* worker_routine(void* warg_ptr)
{
        worker_arg_t* warg = (worker_arg_t*)warg_ptr;
        int i = 0;
        clock_serv_t cclock;

        host_get_clock_service(mach_host_self(), SYSTEM_CLOCK, &cclock);

        while (1) {
                int x;
                struct mach_timespec t1, t2;
                msg_t msg;

                switch (warg->type) {
                case HLE_TYPE:

                        // take our start time
                        clock_get_time(cclock, &t1);

                        // take the lock with hle.
                        acquire_hle(&hle_lock);

                        // set our value
                        warg->touch = i;

                        // release our lock with hle
                        release_hle(&hle_lock);

                        //take our finish time
                        clock_get_time(cclock, &t2);
                        break;
                case NON_HLE_TYPE:

                        // get our start time
                        clock_get_time(cclock, &t1);

                        // retrieve a non-hle lock
                        acquire(&non_hle_lock);

                        // set our value
                        warg->touch = i;

                        // release the lock
                        release(&non_hle_lock);

                        // take our finish time
                        clock_get_time(cclock, &t2);
                        break;
                default:
                        // something bad has happened
                        exit(1);
                }

                // this happens very fast. check to make sure observable time actually moved forward.
                if (t2.tv_nsec - t1.tv_nsec > 0) {
                        // set the difference in our message as well as the type (for the kernel)
                        // and the type (for us). Also, send that message.
                        msg.diff = t2.tv_nsec - t1.tv_nsec;
                        msg.mtype = 1;
                        msg.msg_type = warg->type;
                        msgsnd(warg->q, &msg, sizeof(msg_t)-sizeof(long), 0);
                }

                // tick our value
                i++;
        }

        // tear down our clock
        mach_port_deallocate(mach_task_self(), cclock);
        return NULL;
}

// blocks until SIGINT and then tears down threads.
void wait_for_threads(const thread_tracker_t* tt)
{
        // wait for the SIGINT signal
        sigset_t waitset;
        sigemptyset(&waitset);
        sigaddset(&waitset, SIGINT);
        int sig;
        sigwait(&waitset, &sig);

        // tear down threads
        for (int i = 2; i < THREAD_POOL_SIZE; i++) {
                pthread_cancel(tt->hle[i]);
        }
        for (int i = 0; i < THREAD_POOL_SIZE; i++) {
                pthread_cancel(tt->nonhle[i]);
        }
}

void spawn_workers(thread_tracker_t* tt, int hle_q, int nonhle_q)
{
        pthread_t* hle = (pthread_t*)malloc(sizeof(pthread_t)*THREAD_POOL_SIZE);
        pthread_t* nonhle = (pthread_t*)malloc(sizeof(pthread_t)*THREAD_POOL_SIZE);

        for (int i = 0; i < THREAD_POOL_SIZE; i++) {
                worker_arg_t hle_warg;
                hle_warg.q = hle_q;
                hle_warg.touch = 0;
                hle_warg.type = HLE_TYPE;
                hle_warg.id = i;

                pthread_t id;
                pthread_create(&id, NULL, worker_routine, &hle_warg);

                hle[i] = id;
        }

        for (int i = 0; i < THREAD_POOL_SIZE; i++) {
                worker_arg_t nonhle_warg;
                nonhle_warg.q = nonhle_q;
                nonhle_warg.touch = 0;
                nonhle_warg.type = NON_HLE_TYPE;
                nonhle_warg.id = i;

                pthread_t id;
                pthread_create(&id, NULL, worker_routine, &nonhle_warg);

                nonhle[i] = id;
        }

        memcpy(tt->hle, hle, sizeof(pthread_t)*THREAD_POOL_SIZE);
        memcpy(tt->nonhle, nonhle, sizeof(pthread_t)*THREAD_POOL_SIZE);

        wait_for_threads(tt);

        msgctl(hle_q, IPC_RMID, NULL);
        msgctl(nonhle_q, IPC_RMID, NULL);
        free(hle);
        free(nonhle);
}

void* watch_averages(void* _)
{
        while (1) {
                sleep(1);

                if (hle_avg.count && non_hle_avg.count) {
                        unsigned long hle = hle_avg.total / hle_avg.count;
                        unsigned long nhle = non_hle_avg.total / non_hle_avg.count;

                        double percentage = 100.0-(100.0*((double)hle / (double)nhle));

                        printf("speed up per lock with HLE: %.2f%%\n", percentage);
                }
        }
}

void run(int argc, char *argv[])
{
        pthread_t watcher_id;

        int hle_qid = init_mq(HLE_TYPE);
        int nonhle_qid = init_mq(NON_HLE_TYPE);

        thread_tracker_t* tt = malloc(sizeof(thread_tracker_t));
        pthread_create(&watcher_id, NULL, watch_averages, NULL);

        spawn_workers(tt, hle_qid, nonhle_qid);

        free(tt);
        exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
        if (argc == 1) {
                print_help();
                return 0;
        }
        if (strcmp(argv[1], "run") == 0) {
                run(argc, argv);
        }
        else {
                print_help();
        }
        return 0;
}
