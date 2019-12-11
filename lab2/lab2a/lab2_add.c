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
#define DEFAULT 0
#define MUTEX 1
#define SPINLOCK 2
#define COMPARE_AND_SWAP 3

long long counter;
pthread_mutex_t counter_mutex;
volatile int spin_lock = 0;
int lock;

// for yield option
int opt_yield;

// program name
char name[20] = "";

// program usage information
char *USAGE =
  "Usage: lab2_add [--threads 5] [--iterations 3] [yield] [sync=[msc]]\n";

// handle bad option
void exit_with_usage() {
  fprintf(stderr, "%s", USAGE);
  exit(1);
}

// Critical section
void add(long long *pointer, long long value) {
  long long sum = *pointer + value;
  if (opt_yield) // the calling thread relinquish the CPU
    sched_yield(); // always succeeds
  *pointer = sum;
}

// synchronization for each thread
void lock_helper(long long value) {
  switch (lock) {
  case 1:
    pthread_mutex_lock(&counter_mutex);
    add(&counter, value);
    pthread_mutex_unlock(&counter_mutex);
    break;
  case 2:
    while (__sync_lock_test_and_set(&spin_lock, 1)) while(spin_lock);
    add(&counter, value);
    __sync_lock_release(&spin_lock);
    break;
  case 3:
    ;
    long long old_val, new_val;
    do {
      old_val = counter;
      new_val = old_val + value;
      if (opt_yield) sched_yield();
    } while (__sync_val_compare_and_swap(&counter, old_val, new_val) != old_val);
    break;
  default:
    add(&counter, value);
    break;
  }

}

// The worker function
void *worker (void *num) {
  int iterations = *((int *) num);
  int i;
  for (i = 0; i < iterations; i++) {
    lock_helper(1);
  }
  for (i = 0; i < iterations; i++) {
    lock_helper(-1);
  }
  return NULL;
}

void thread_mangager(int num_threads, int num_iterations) {
  pthread_t *threads = (pthread_t*) malloc(num_threads * sizeof(pthread_t));
  long i;
  int rc;

  int *arg = malloc(sizeof(int));
  *arg = num_iterations;

  // thread creation
  for (i = 0; i < num_threads; i++) {
    rc = pthread_create(&threads[i], NULL, worker, arg);
    if (rc) {
      fprintf(stderr,"ERROR; return code from pthread_create() is %d\n", rc);
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

  free(threads);
  free(arg);

}

// set the program name
void set_tag () {
  strcat(name, "add");
  if (opt_yield)
    strcat(name, "-yield");
  switch (lock) {
  case 1:
    strcat(name, "-m");
    break;
  case 2:
    strcat(name, "-s");
    break;
  case 3:
    strcat(name, "-c");
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

  // Initialize the long long counterer
  counter = 0;

  int long_index = 0;
  int opt = 0;

  static struct option long_options[] = {
    {"threads",       required_argument, 0,  'T' },
    {"iterations",    required_argument, 0,  'I' },
    {"yield",     no_argument,       0,  'Y' },
    {"sync",          required_argument, 0,  'S' },
    {  0,0, 0,  0  }
  };

  while ((opt = getopt_long(argc, argv, " ", long_options, &long_index)) != -1) {
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
      break;
    case 'Y':
      opt_yield = 1;
      break;
    case 'S':
      switch (optarg[0]) {
      case 'm':     // Mutex
	lock = MUTEX;
	pthread_mutex_init(&counter_mutex, NULL);
	break;
      case 's':     //Spinlock
	lock = SPINLOCK;
	break;
      case 'c':     //Compare and swap
	lock = COMPARE_AND_SWAP;
	break;
      }
      break;
    default:
      fprintf(stderr, "Unrecognized option: %s\n", optarg);
      exit_with_usage();
    }
  }

  //uint64_t elapsed;
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

  // Data preparation
  long long elapsed =
    BILLION * (end.tv_sec - start.tv_sec) + end.tv_nsec - start.tv_nsec;

  long long num_operations = num_threads * num_iterations  * 2;
  long long avg_op_time = elapsed / num_operations;

  // get the program name
  set_tag();

  // Data report
  printf("%s,%d,%d,%lld,%lld,%lld,%lld\n",
	 name, num_threads, num_iterations, num_operations,
	 elapsed, avg_op_time, counter);
  exit(0);
}
