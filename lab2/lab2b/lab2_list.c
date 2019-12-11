#include "SortedList.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <pthread.h>
#include <signal.h>

// for elapse time calculation
#define BILLION 1000000000L

// for lock
#define MUTEX 1
#define SPINLOCK 2

// lock type
int lock = 0;

// Number of threads the user specified
int num_threads = 1;

// Number of iterations the user specified
int num_iterations = 1;

int num_elements = 0;

int num_lists = 1;

//wait time for different thread numbers with mutex
long long wait_time;

SortedListElement_t *elem;
SortedList_t *all_lists = NULL;
pthread_mutex_t *m_locks;
int *s_locks;

char name[20] = "";

int opt_yield;
char yield_str[4] = "";

void sig_handler(int signo)
{
    fprintf(stderr, "Signal %d is caught.\n", signo);
    exit(2);
}

// pick a sublist
unsigned long hash_func(const char *str)
{
    unsigned long hash = 5381;
    int c;

    while ((c = *str++))
        hash = ((hash << 5) + hash) + c;

    return hash;
}


long long calc_diff(struct timespec *start, struct timespec *end)
{
    long long elapsed = BILLION * (end->tv_sec - start->tv_sec) + end->tv_nsec - \
                        start->tv_nsec;

    return elapsed;
}


// The worker function
void *worker (void *num)
{
    int n = *((int *) num);
    struct timespec start, end;

    int i;
    // insert elements into list
    for (i = n; i < num_elements; i += num_threads)
    {
        unsigned long index = hash_func(elem[i].key) % num_lists;
       
        switch (lock)
        {
        case 1:
            clock_gettime(CLOCK_MONOTONIC, &start);
            pthread_mutex_lock(&m_locks[index]);
            clock_gettime(CLOCK_MONOTONIC, &end);

            SortedList_insert(&all_lists[index], &elem[i]);

            pthread_mutex_unlock(&m_locks[index]);
            wait_time += calc_diff(&start, &end);
            break;
        case 2:
            clock_gettime(CLOCK_MONOTONIC, &start);
            while (__sync_lock_test_and_set(&s_locks[index], 1));
            clock_gettime(CLOCK_MONOTONIC, &end);

            SortedList_insert(&all_lists[index], &elem[i]);
            __sync_lock_release(&s_locks[index]);
            wait_time += calc_diff(&start, &end);
            break;
        default:
            SortedList_insert(&all_lists[index], &elem[i]);
            break;
        }
    }

    // get length
    int length = 0;

    switch (lock)
    {
    case 1:
        for (int i = 0; i < num_lists; i++)
        {
            clock_gettime(CLOCK_MONOTONIC, &start);
            pthread_mutex_lock(&m_locks[i]);
            clock_gettime(CLOCK_MONOTONIC, &end);

            length += SortedList_length(&all_lists[i]);
            pthread_mutex_unlock(&m_locks[i]);
            wait_time += calc_diff(&start, &end);
        }
        break;
    case 2:
        for (int i = 0; i < num_lists; i++)
        {
            clock_gettime(CLOCK_MONOTONIC, &start);
            while (__sync_lock_test_and_set(&s_locks[i], 1));
            clock_gettime(CLOCK_MONOTONIC, &end);
            length += SortedList_length(&all_lists[i]);
            __sync_lock_release(&s_locks[i]);
            wait_time += calc_diff(&start, &end);
        }
        break;
    default:
        for (int i = 0; i < num_lists; i++)
        {
            length += SortedList_length(&all_lists[i]);
        }
        break;
    }

    if (length < 0)
    {
        fprintf(stderr, "List corrupted in finding length.\n");
        exit(2);
    }

    // looks up and deletes
    for (i = n; i < num_elements; i += num_threads)
    {
        int index = hash_func(elem[i].key) % num_lists;
        SortedListElement_t *cur;
        switch (lock)
        {
        case 1:
            clock_gettime(CLOCK_MONOTONIC, &start);
            pthread_mutex_lock(&m_locks[index]);
            clock_gettime(CLOCK_MONOTONIC, &end);

            cur = SortedList_lookup (&all_lists[index], elem[i].key);
            if (cur == NULL)
            {
                fprintf(stderr, "List corrupted in lookup.\n");
                exit(2);
            }
            if (SortedList_delete(cur))
            {
                fprintf(stderr, "List corrupted in deletion.\n");
                exit(2);
            };
            pthread_mutex_unlock(&m_locks[index]);
            wait_time += calc_diff(&start, &end);
            break;
        case 2:
            clock_gettime(CLOCK_MONOTONIC, &start);
            while (__sync_lock_test_and_set(&s_locks[index], 1));
            clock_gettime(CLOCK_MONOTONIC, &end);
            cur = SortedList_lookup (&all_lists[index], elem[i].key);
            if (cur == NULL)
            {
                fprintf(stderr, "List corrupted in lookup.\n");
                exit(2);
            }
            if (SortedList_delete(cur))
            {
                fprintf(stderr, "Cannot delete the element.\n");
                exit(2);
            };
            __sync_lock_release(&s_locks[index]);
            wait_time += calc_diff(&start, &end);
            break;
        default:
            cur = SortedList_lookup(&all_lists[index], elem[i].key);
            if (cur == NULL)
            {
                fprintf(stderr, "List corrupted in lookup.\n");
                exit(2);
            }
            if (SortedList_delete(cur))
            {
                fprintf(stderr, "List corrupted in deletion.\n");
                exit(2);
            };
            break;
        }
    }

    return NULL;
}

void thread_mangager()
{
    pthread_t *threads = (pthread_t *) malloc(num_threads * sizeof(pthread_t));
    long i;
    int rc;

    int *arg = (int *) malloc(num_threads * sizeof(int));

    // thread creation
    for (i = 0; i < num_threads; i++)
    {
        arg[i] = i;

        //The value of arg is the key to avoid deadlock!!!
        rc = pthread_create(&threads[i], NULL, worker, (void *)&arg[i]);
        if (rc)
        {
            fprintf(stderr, "ERROR on creating thread %ld with return code %d\n", i, rc);
            exit(2);
        }
    }
    // wait for all threads completion
    for (i = 0; i < num_threads; ++i)
    {
        if (pthread_join(threads[i], NULL) != 0)
        {
            fprintf(stderr, "%s\n", "Some errors on joining threads");
            exit(2);
        }
    }

    free(threads);
    free(arg);

}

// program usage information
char *USAGE =
    "Usage: lab2_list [--threads=5] [--iterations=3][--lists=2] [--yield=[idl]] [--sync=[ms]]\n";

// handle bad option
void exit_with_usage()
{
    fprintf(stderr, "%s", USAGE);
    exit(1);
}

void get_yield_op (char *optarg)
{
    int len = strlen(optarg);

    if (len > 3)
    {
        fprintf(stderr, "Too many arguments for yield\n");
        exit_with_usage();
    }

    int i;
    for (i = 0; i < len; i++)
    {
        switch(optarg[i])
        {
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
static char *rand_str (int size)
{
    char *str;
    const char charset[] =
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    if (size)
    {
        str = malloc(sizeof(char) * (size + 1));

        if (str)
        {
            int l = (int) (sizeof(charset) - 1);
            int key;
            int i;
            for (i = 0; i < size; i++)
            {
                key = rand() % l;
                str[i] = charset[key];
            }

            str[size] = '\0';
        }
    }
    return str;
}

// set the program name
void set_tag ()
{
    strcat(name, "list");
    if (opt_yield)
    {
        strcat(name, "-");
        strcat(name, yield_str);
    }
    else
    {
        strcat(name, "-none");
    }

    switch (lock)
    {
    case 1:
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

int main(int argc, char *argv[])
{
    int long_index = 0;
    int opt = 0;

    static struct option long_options[] =
    {
        {"threads",       required_argument, 0,  'T' },
        {"iterations",    required_argument, 0,  'I' },
        {"yield",     required_argument, 0,  'Y' },
        {"sync",   required_argument, 0,  'S' },
        {"lists",   required_argument, 0, 'L'},
        {  0, 0, 0,  0  }
    };

    while ((opt = getopt_long(argc, argv, "", long_options, &long_index)) != -1)
    {
        switch(opt)
        {
        case 'T':
            num_threads = atoi(optarg);
            if (num_threads < 1)
            {
                fprintf(stderr, "Expected positive integer after --threads\n");
                exit_with_usage();
            }
            break;
        case 'I':
            num_iterations = atoi(optarg);
            if (num_iterations < 1)
            {
                fprintf(stderr, "Expected positive integer after --iterations\n");
                exit_with_usage();
            }
            break;
        case 'Y':
            get_yield_op(optarg);
            break;
        case 'S':
            switch (optarg[0])
            {
            case 'm':     // Mutex
                lock = MUTEX;
		break;
            case 's':     //Spinlock
                lock = SPINLOCK;
		break;
            default:
                lock = 0;
                fprintf(stderr, "Unrecognized argument for sync: %s\n", optarg);
                exit_with_usage();
            }
            break;
        case 'L':
            num_lists = atoi(optarg);
            break;
        default:
            fprintf(stderr, "Unrecognized option\n");
            exit_with_usage();
        }
    }

    // number of elements in the SortedList
    num_elements = num_threads * num_iterations;

    // Initialize all sublists
    all_lists = (SortedList_t *)malloc(num_lists * sizeof(SortedList_t));

    for (int i = 0; i < num_lists; i++)
    {
        all_lists[i].key = NULL;
        all_lists[i].prev = &all_lists[i];
        all_lists[i].next = &all_lists[i];

    }

    // Initialize locks
    if (lock == MUTEX)
    {
        m_locks = malloc(num_lists * sizeof(pthread_mutex_t));
        for (int i = 0; i < num_lists; i++)
        {
            pthread_mutex_init(&m_locks[i], NULL);
        }
    }
    else if (lock == SPINLOCK)
    {
        s_locks = malloc(num_lists * sizeof(int));
        for (int i = 0; i < num_lists; i++)
        {
            s_locks[i] = 0;
        }
    }
   
    elem = (SortedListElement_t *) malloc(num_elements * sizeof(SortedListElement_t));;

    // allocate and preinitialize list element
    int i, l;
    for (i = 0; i < num_elements; i++)
    {
        l = rand() % 10;         // randomly pick size for the string
        elem[i].key = rand_str(l);
    }

    signal(SIGSEGV, sig_handler);
    struct timespec start, end;

    // Collect start time
    if (clock_gettime(CLOCK_MONOTONIC, &start) < 0)
    {
        fprintf(stderr, "Error on getting clock time.\n");
        exit(1);
    }

    // thread_mangager(num_threads, num_iterations);
    thread_mangager();

    // Collect end time
    if (clock_gettime(CLOCK_MONOTONIC, &end) < 0)
    {
        fprintf(stderr, "Error on getting clock time.\n");
        exit(1);
    }

    for (int i = 0; i < num_lists; i++)
    {
        if (SortedList_length(&all_lists[i]) < 0)
        {
            fprintf(stderr, "List Corrupted\n");
            exit(2);
        }
    }


    // Data preparation
    long long elapsed = BILLION * (end.tv_sec - start.tv_sec) + end.tv_nsec - start.tv_nsec;

    long long num_operations = num_threads * num_iterations  * 3;
    long long avg_op_time = elapsed / num_operations;
    long long avg_wait_time = wait_time / num_operations;

    // get the program name
    set_tag();

    printf("%s,%d,%d,%d,%lld,%lld,%lld,%lld\n",
           name, num_threads, num_iterations, num_lists, num_operations,
           elapsed, avg_op_time, avg_wait_time);
    
    free(elem);
    free(all_lists);

    if (lock == MUTEX)
    {
        free(m_locks);
    }
    else if (lock == SPINLOCK)
    {
        free(s_locks);
    }
    exit(0);
}
