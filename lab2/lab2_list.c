#include "SortedList.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <pthread.h>

// for elapse time calculation
#define BILLION 1000000000L

// for lock
#define MUTEX 1
#define SPINLOCK 2

pthread_mutex_t counter_mutex;
volatile int spin_lock;
int lock;

SortedList_t *list;
SortedListElement_t *elem;
char name[20] = "";

int opt_yield;
char yield_str[4] = "";

// The worker function
void *worker (void *num) {
	int iterations = *((int *) num);
	int i;
	// insert elements into list
	for (i = 0; i < iterations; i++) {
		switch (lock) {
        case 1:
			printf("Inside mutex\n");
            pthread_mutex_lock(&counter_mutex);
            SortedList_insert(list, &elem[i]);
            pthread_mutex_unlock(&counter_mutex);
			printf("Outside mutex\n");
            break;
        case 2:
            while (__sync_lock_test_and_set(&spin_lock, 1)) while(spin_lock);
            SortedList_insert(list, &elem[i]);
            __sync_lock_release(&spin_lock);
            break;
		default:
			SortedList_insert(list, &elem[i]);
			break;
    }
	}

	// get length
	int length;
	switch (lock) {
		case 1:
            pthread_mutex_lock(&counter_mutex);
            length = SortedList_length(list);
			if (length < 0) {
				fprintf(stderr, "Cannot get the length of sorted list.\n");
				exit(2);
			}
            pthread_mutex_unlock(&counter_mutex);
            break;
        case 2:
            while (__sync_lock_test_and_set(&spin_lock, 1)) while(spin_lock);
            length = SortedList_length(list);
			if (length < 0) {
				fprintf(stderr, "Cannot get the length of sorted list.\n");
				exit(2);
			}
            __sync_lock_release(&spin_lock);
            break;
		default:
			length = SortedList_length(list);
			if (length < 0) {
				fprintf(stderr, "Cannot get the length of sorted list.\n");
				exit(2);
			}
			break;
    }

	// looks up and deletes
	for (i = 0; i < iterations; i++) {
		SortedListElement_t *cur;
		switch (lock) {
		case 1:
            pthread_mutex_lock(&counter_mutex);
			cur = SortedList_lookup (list, elem[i].key);
			if (cur == NULL) {
				fprintf(stderr, "Cannot find the element.\n");
				exit(2);
			}
            if (SortedList_delete(cur) != 0) {
				fprintf(stderr, "Cannot delete the element.\n");
				exit(2);
			};
            pthread_mutex_unlock(&counter_mutex);
            break;
        case 2:
            while (__sync_lock_test_and_set(&spin_lock, 1)) while(spin_lock);
            cur = SortedList_lookup (list, elem[i].key);
			if (cur == NULL) {
				fprintf(stderr, "Cannot find the element.\n");
				exit(2);
			}
            if (SortedList_delete(cur) != 0) {
				fprintf(stderr, "Cannot delete the element.\n");
				exit(2);
			};
            __sync_lock_release(&spin_lock);
            break;
		default:
			cur = SortedList_lookup(list, elem[i].key);
			if (cur == NULL) {
				fprintf(stderr, "Cannot find the element.\n");
				exit(2);
			}
            if (SortedList_delete(cur) != 0) {
				fprintf(stderr, "Cannot delete the element.\n");
				exit(2);
			};
			break;
    }
	}

}

void thread_mangager(int num_threads, int num_iterations) {
    pthread_t *threads = (pthread_t*) malloc(num_threads * sizeof(pthread_t));
    long i;
    int rc;

	int *arg = (int*) malloc(num_threads * sizeof(int));

    // thread creation
    for (i = 0; i < num_threads; i++) {
		arg[i] = i;
        printf("In main: creating thread %ld\n", i);
		//The value of arg is the key to avoid deadlock!!!
        rc = pthread_create(&threads[i], NULL, worker, arg);
        if (rc){
           printf("ERROR; return code from pthread_create() is %d\n", rc);
           exit(1);
       }
    }
    // wait for all threads completion
    for (i = 0; i < num_threads; ++i) {
		if (pthread_join(threads[i], NULL) != 0) {
			fprintf(stderr, "%s\n", "Some errors on joining threads");
            exit(1);
		}
	}
	printf("Thread join succeeds!!\n");

    free(threads);
	free(arg);

}

// program usage information
char *USAGE =
	"Usage: lab2_add [--threads=5] [--iterations=3] [--yield=[idl]] [--sync=[ms]]\n";

// handle bad option
void exit_with_usage() {
	fprintf(stderr, "%s", USAGE);
	exit(EXIT_SUCCESS);
}

void get_yield_op (char *optarg) {
	int len = strlen(optarg);

	if (len > 3) {
		printf("Too many arguments for yield\n");
		exit_with_usage();
	}

	int i;
	for (i = 0; i < len; i++) {
		switch(optarg[i]) {
			case 'i':
				opt_yield |= INSERT_YIELD;
				strcat(yield_str, "i");
				break;
			case 'd':
				opt_yield |= DELETE_YIELD;
				strcat(yield_str, "d");
				break;
			case 'l':
				opt_yield |= LOOKUP_YIELD;
				strcat(yield_str, "l");
				break;
			default:
				printf("Unrecognized argument for yield.\n");
				exit_with_usage();
		}
	}
}

// Random string generator
static char *rand_str (size_t size) {
	char *str;
	const char charset[] =
		"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789,.-#'?!";
	if (size) {
		str = malloc(sizeof(char) * (size + 1));

		if (str) {
			int l = (int) (sizeof(charset) - 1);
			int key;
			int i;
			for (i = 0; i < size; i++) {
				key = rand() % l;
				str[i] = charset[key];
			}

			str[size] = '\0';
		}
	}
	return str;
}

// set the program name
void set_tag () {
	strcat(name, "list");
	if (opt_yield) {
		strcat(name, "-");
		if (yield_str == "")
			strcat(name, "none");
		else
			strcat(name, yield_str);
	}

	switch (lock) {
		case 1 :
			strcat(name, "-m");
			break;
		case 2:
			strcat(name, "-s");
			break;
		default:
			strcat(name, "-none");
			break;
	}
}

int main(int argc, char* argv[]) {
	// Number of threads the user specified
	int num_threads = 1;

	// Number of iterations the user specified
	int num_iterations = 1;

	int long_index = 0;
	int opt = 0;

	static struct option long_options[] = {
		{"threads",       required_argument, 0,  'T' },
		{"iterations",    required_argument, 0,  'I' },
        {"yield",   required_argument, 0, 'Y'},
        {"sync", required_argument, 0, 'S'},
		{0,0,0,0}
	};

	while ((opt = getopt_long(argc, argv, "T:I:Y:S:", long_options, &long_index)) != -1) {
		switch(opt) {
		case 'T':
			num_threads = atoi(optarg);
			if (num_threads < 1) {
				fprintf(stderr, "Expected positive integer after --threads\n");
				exit_with_usage();
			}
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
            get_yield_op(optarg);
            break;
        case 'S':
            switch (optarg[0]) {
                case 'm': // Mutex
                    lock = MUTEX;
					pthread_mutex_init(&counter_mutex, NULL);
                    break;
                case 's': //Spinlock
                    lock = SPINLOCK;
					spin_lock = 0;
                    break;
                default:
					lock = 0;
					fprintf(stderr, "Unrecognized argument for sync: %s\n", optarg[0]);
                    exit_with_usage();
            }
            break;
		default:
			fprintf(stderr, "Unrecognized option: %s\n", opt);
			exit_with_usage();
		}
	}

	// number of elements in the SortedList
	const int num_elements = num_threads * num_iterations;

	// Initialize an empty list
    list = (SortedList_t *) malloc (sizeof(SortedList_t));
	list->prev = list;
	list->next = list;
	list->key = NULL;

	elem = (SortedListElement_t*) malloc(num_elements * sizeof(SortedListElement_t));;
	int i, l;
	for (i = 0; i < num_elements; i++) {
		l = rand() % 10; // randomly pick size for the string
		elem[i].key = rand_str(l);
	}

	struct timespec start, end;

	// Collect start time
	if (clock_gettime(CLOCK_MONOTONIC, &start) < 0) {
		fprintf(stderr, "Error on getting clock time.\n");
		exit(1);
	}

    thread_mangager(num_threads, num_iterations);

	// Collect end time
	if (clock_gettime(CLOCK_MONOTONIC, &end) < 0) {
		fprintf(stderr, "Error on getting clock time.\n");
		exit(1);
	}

	if (SortedList_length(list) != 0) {
		fprintf(stderr, "List Corrupted\n");
		exit(2);
	}

	// Data preparation
	long long unsigned int elapsed = BILLION * (end.tv_sec - start.tv_sec) + end.tv_nsec - start.tv_nsec;
	printf("elapsed time = %llu nanoseconds\n", elapsed);

	int num_operations = num_threads * num_iterations  * 3;
	int avg_op_time = elapsed / num_operations;

	// get the program name
	set_tag();

	// Data report
	printf("%s, %d, %d, %d, %llu, %d\n",
			name, num_threads, num_iterations, num_operations,
			elapsed, avg_op_time);

	free(elem);
	free(list);

}
