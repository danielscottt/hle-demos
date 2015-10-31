#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <mqueue.h>
#include <stdlib.h>
#include <errno.h>

void print_help()
{
        printf("hle-demo: a program that takes a bunch of locks at random\n");
        printf("\nUSAGE:\n");
        printf("  hle-demo [COMMAND] [COMMAND OPTIONS]\n");
        printf("\nCOMMANDS:\n");
        printf("  run           run the demo\n");
        printf("  cpus          print cpu count\n");
}

void work(union sigval sv)
{
        // queue attrs, current and previous
        struct mq_attr c_attr, p_attr;
        // the buffer we read our messages into
        void *buf;
        // our queue
        mqd_t wkq = *((mqd_t*)sv.sival_ptr);

        if (mq_getattr(wkq, &c_attr) == -1) {
                printf("worker queue: failed to retrieve mq attributes");
                exit(EXIT_FAILURE);
        }
        buf = malloc(c_attr.mq_msgsize);
        if (buf == NULL) {
                printf("worker queue: failed ro create buffer\n");
                exit(EXIT_FAILURE);
        }
        c_attr.mq_flags = O_NONBLOCK;
        mq_setattr(wkq, &c_attr, &p_attr);
        while (mq_receive(wkq, buf, c_attr.mq_msgsize, NULL) != -1) {
                int ts = atoi(buf);
                printf("received int: %d\n", ts);
        }
        if (errno != EAGAIN) {
                printf("worker queue: error receiving message");
                exit(EXIT_FAILURE);
        }
        mq_setattr(wkq, &p_attr, 0);
        free(buf);
        exit(EXIT_SUCCESS);
}

int get_cpu_cnt()
{
        return sysconf(_SC_NPROCESSORS_ONLN);
}

int parse_limit_flag(char *argv[])
{
        for (int i = 0; i < sizeof(argv); i++) {
                if (strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--limit") == 0)
                        return (int)(argv[i+1][0] - '0');
        }
        return 0;
}

void run(int argc, char *argv[])
{
        int cpus = 0;
        if (argc >= 4) { // 1. binary 2. command 3. flag 4. value
                cpus = parse_limit_flag(argv);
        }
        int cpu_c = get_cpu_cnt();
        if (cpus < 1 || cpus > cpu_c) {
                cpus = cpu_c;
        }
        printf("using %d worker pools pinned to %d cores\n", cpus, cpus);
        struct sigevent sig_event;
        mqd_t wkq = mq_open("/avg", O_RDWR | O_CREAT, 0664, 0);
        if (errno == EINVAL) {
                printf("error opening queue");
                exit(1);
        }
        mq_unlink("/avg");
        sig_event.sigev_notify = SIGEV_THREAD;
        sig_event.sigev_notify_function = work;
        sig_event.sigev_notify_attributes = NULL;
        sig_event.sigev_value.sival_ptr = &wkq;
        if (mq_notify(wkq, &sig_event) == -1) {
                printf("failed to configure mq_notify %d\n", errno);
                exit(1);
        }
        const char* msg = "7";
        if (mq_send(wkq, msg, sizeof(msg), 0) == -1) {
                printf("error sending to worker queue\n");
                exit(1);
        }
        sleep(1);
}

int main(int argc, char *argv[])
{
        if (argc == 1) {
                print_help();
                return 0;
        }
        if (strcmp(argv[1], "cpu") == 0) {
                int cpu_c = get_cpu_cnt();
                printf("cpu count: %d\n", cpu_c);
        }
        else if (strcmp(argv[1], "run") == 0) {
                run(argc, argv);
        }
        else {
                print_help();
        }
        return 0;
}
