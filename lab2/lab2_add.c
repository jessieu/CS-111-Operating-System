#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <pthread.h>

#define DEFAULT 0
#define MUTEX 1
#define SPINLOCK 2
#define COMPARE_AND_SWAP 3

pthread_mutex_t counter_mutex;
volatile int spin_lock = 0;
long long counter;

int lock;


int opt_yield;
// The worker function
void add(long long *pointer, long long value) {
	long long sum = *pointer + value;
    if (opt_yield) // the calling thread relinquish the CPU
        sched_yield(); // always succeeds
	*pointer = sum;
}

void lock_helper() {
    long long value = 1;
    switch (lock) {
        case 0:
            break;
        case 1:
            pthread_mutex_lock(&counter_mutex);
            add(&counter, value);
            pthread_mutex_unlock(&counter_mutex);
            printf("%d\n", counter);
            break;
        case 2:
            while (__sync_lock_test_and_set(&spin_lock, 1)) while(spin_lock);
            add(&counter, value);
            __sync_lock_release(&spin_lock);
            printf("%d\n", counter);
            break;
        case 3:
            ;
            long long old_val, new_val;

            do {
                old_val = counter;
                new_val = old_val + value;
                if (opt_yield) sched_yield();
            } while (__sync_val_compare_and_swap(&counter, old_val, new_val) != old_val);
            printf("%d\n", counter);
            break;
    }

}

char *USAGE = "Usage: lab2_add [--threads 5] [--iterations 3]\n";

void thread_mangager(int num_threads, int num_iterations) {
    pthread_t *threads = (pthread_t*) malloc(num_threads * sizeof(pthread_t));
    long i;
    int rc;
    // thread creation
    for (i = 0; i < num_threads; i++) {
        printf("In main: creating thread %ld\n", i);
        rc = pthread_create(&threads[i], NULL, lock_helper, NULL);
        if (rc){
           printf("ERROR; return code from pthread_create() is %d\n", rc);
           exit(1);
       }
    }
    // wait for all threads completion
    for (i = 0; i < num_threads; ++i) {
		if (pthread_join(threads[i], NULL) != 0)
			fprintf(stderr, "%s\n", "Some errors on joining threads");
            exit(1);
	}

    free(threads);

}


void exit_with_usage() {
	fprintf(stderr, "%s", USAGE);
	exit(EXIT_SUCCESS);
}

int main(int argc, char* argv[]) {
	// Number of threads the user specified
	int num_threads = 1;

	// Number of iterations the user specified
	int num_iterations = 1;

    // Initialize the long long counterer
    counter = 0;

	int long_index = 0;
	int opt = 0;

	static struct option long_options[] = {
		{"threads",       required_argument, 0,  'T' },
		{"iterations",    required_argument, 0,  'I' },
        {"yield",   no_argument, 0, 'Y'},
        {"sync", required_argument, 0, 'S'},
		{0,0,0,0}
	};

	while ((opt = getopt_long(argc, argv, "T:I", long_options, &long_index)) != -1) {
		switch(opt) {
			if (optarg < 1) break;
		case 'T':
			num_threads = atoi(optarg);
			if (num_threads < 1) {
				fprintf(stderr, "Expected positive integer after --iterations\n");
				exit_with_usage();
			}
			printf("Number of threads: %d\n", num_threads);
			break;
		case 'I':
			num_iterations = atoi(optarg);
			if (num_iterations < 1) {
				fprintf(stderr, "Expected positive integer after --iterations\n");
				exit_with_usage();
			}
			printf("Number of iterations: %d\n", num_iterations);
			break;
        case 'Y':
            opt_yield = 1;
            break;
        case 'S':
            switch (optarg[0]) {
                case 'm': // Mutex
                    lock = MUTEX;
                    printf("%d\n", lock);
                    break;
                case 's': //Spinlock
                    lock = SPINLOCK;
                    break;
                case 'c': //Compare and swap
                    lock = COMPARE_AND_SWAP;
                    break;
            }
            break;
		default:
			fprintf(stderr, "Unrecognized option: %s\n", opt);
			exit_with_usage();
		}
	}

    thread_mangager(num_threads, num_iterations);

}
