#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h> //note: O_RSYNC only works under the Linux environment
#include <unistd.h>
#include <string.h> //for strdup
#include <sys/stat.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/resource.h> //for getusage()
#include <sys/time.h>

/* collect process information - initialize in command and retrieve in wait */
struct process_info
{
    char **cmd_name;
    pid_t pid;
    int size;
} process_info;

void sig_handler (int signo)
{
    printf("received signal %d.\n", signo);
}

void get_starting_time (struct timeval *startingUserTime, struct timeval *startingSystemTime )
{
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);

    *startingUserTime = usage.ru_utime;
    startingUserTime->tv_sec = usage.ru_utime.tv_sec;
    startingUserTime->tv_usec = usage.ru_utime.tv_usec;

    *startingSystemTime = usage.ru_stime;
    startingSystemTime->tv_sec = usage.ru_stime.tv_sec;
    startingSystemTime->tv_usec = usage.ru_stime.tv_usec;
}

void print_cpu_time(struct timeval startingUserTime, struct timeval startingSystemTime)
{
    struct rusage usage;
    getrusage (RUSAGE_SELF, &usage);
    struct timeval endingUserTime = usage.ru_utime;
    struct timeval endingSystemTime = usage.ru_stime;
    long int diffUs = (long int) (endingUserTime.tv_sec - startingUserTime.tv_sec);
    long int diffUms = (long int) (endingUserTime.tv_usec - startingUserTime.tv_usec);
    long int diffSs = (long int) (endingSystemTime.tv_sec - startingSystemTime.tv_sec);
    long int diffSms = (long int) (endingSystemTime.tv_usec - startingSystemTime.tv_usec);
    printf ("CPU time: %ld.%06ld sec user, %ld.%06ld sec system\n", diffUs, diffUms, diffSs, diffSms);
}

int main (int argc, char *argv[])
{
    /* array to store file descriptors */
    int *fd_list = (int *)malloc(25 * sizeof(int));
    int fd_index = 0;
    int flag = 0;

    int close_fd = 0;

    /* array to store process information */
    struct process_info *process_arr = malloc(50 * sizeof(process_info));
    int process_num = 0;

    /* array to store process id */
    pid_t *pid_arr = malloc(25 * sizeof(pid_t));
    int pid_num = 0;

    /* for profile */
    int profile = 0;
    struct timeval startingUserTime, startingSystemTime;

    int long_index = 0;
    int opt = 0;
    int verbose = 0;

    static struct option long_options[] = {
	/* Lab 1A */
	{"rdonly",     required_argument, 0,  'R' },
	{"wronly",     required_argument, 0,  'W' },
	{"command",    required_argument, 0,  'C' },
	{"verbose",    no_argument,       0,  'V' },
	/* Lab 1B */
	{"append",     no_argument,       0,  'A' },
	{"cloexec",    no_argument,       0,  'L' },
	{"creat",      no_argument,       0,  'c' },
	{"directory",  no_argument,       0,  'D' },
	{"dsync",      no_argument,       0,  'd' },
	{"excl",       no_argument,       0,  'E' },
	{"nofollow",   no_argument,       0,  'F' },
	{"nonblock",   no_argument,       0,  'B' },
	{"rsync",      no_argument,       0,  'S' },
	{"sync",       no_argument,       0,  's' },
	{"trunc",      no_argument,       0,  'T' },
	{"rdwr",       no_argument,       0,  'r' },
	{"pipe",       no_argument,       0,  'P' },
	{"wait",       no_argument,       0,  'w' },
	{"close",      required_argument, 0,  'O' },
	{"abort",      no_argument,       0,  'a' },
	{"catch",      required_argument, 0,  't' },
	{"ignore",     required_argument, 0,  'I' },
	{"default",    required_argument, 0,  'e' },
	{"pause",      no_argument,       0,  'p' },
	/* Lab 1C */
	{"profile",    no_argument,       0,  'o' },
	{0,            0,                 0,   0  }
    };

    while ((opt = getopt_long(argc, argv, "R:W:C:VALcDdEFBSsTrPwO:at:I:e:po",
                              long_options, &long_index)) != -1) {

	if (verbose) {
	    printf("--%s ",long_options[long_index].name);
	}
	switch (opt) {
	/* -------------------rdonly------------------- */
	case 'R':
	    if (verbose) printf("%s\n",optarg);
	    if (profile) {
		get_starting_time(&startingUserTime, &startingSystemTime);
	    }
	    flag |= O_RDONLY;
	    if ((fd_list[fd_index++] = open(optarg, flag, 0)) == -1) {
		fprintf(stderr, "Cannot open file %s with read only.\n", optarg);
		exit(1);
	    }
	    flag = 0;
	    if (profile) {
		print_cpu_time(startingUserTime, startingSystemTime);
	    }
	    break;


	/* -------------------wronly------------------- */
	case 'W':
	    if (verbose) printf("%s\n",optarg);
	    if (profile) {
		get_starting_time(&startingUserTime, &startingSystemTime);
	    }
	    flag |= O_WRONLY;
	    if ((fd_list[fd_index++] = open(optarg, flag, 0)) == -1) {
		fprintf(stderr, "Cannot open file %s with write only.\n", optarg);
		exit(1);
	    }
	    flag = 0;
	    if (profile) {
		print_cpu_time(startingUserTime, startingSystemTime);
	    }
	    break;
	/* -------------------command------------------- */
	case 'C':
	    if (profile) {
		get_starting_time(&startingUserTime, &startingSystemTime);
	    }
	    optind--;
	    int count = 0;
	    char *cmd_name[256];
	    cmd_name[count++] = strdup(long_options[long_index].name);

	    while (optind < argc) {
		if (strdup(argv[optind])[0] == '-' && strdup(argv[optind])[1] == '-') break;
		cmd_name[count++] = argv[optind++];
	    }
	    cmd_name[count] = "\0";

	    int in = atoi(cmd_name[1]);
	    int out = atoi(cmd_name[2]);
	    int err = atoi(cmd_name[3]);
	    char *args[128];
	    int index = 0;
	    int start = 4;
	    while (start < count) {
		/* strdup returns a pointer to a null-terminated byte string */
		args[index++] = strdup(cmd_name[start++]);
	    }
	    args[index] = "\0";

	    if (verbose) {
		printf("%d %d %d ", in, out, err);
		int j;
		for (j = 0; j < index; j++)
		    printf("%s ",args[j]);
	    }
	    printf("\n");

	    int rc = fork();
	    pid_arr[pid_num++] = rc;
	    process_arr[process_num].pid = rc;
	    process_arr[process_num].cmd_name = cmd_name;
	    process_arr[process_num].size = count;
	    process_num++;

	    if (rc < 0) {
		fprintf(stderr, "fork failed\n");
		exit(1);
	    }else if (rc == 0) {
		//sleep(10);
		dup2(fd_list[in], 0);
		dup2(fd_list[out], 1);
		dup2(fd_list[err], 2);

		int k;
		for (k = 0; k < fd_index; k++) {
		    close (fd_list[k]);
		    fd_list[k] = -1;
		}
		if (execvp (args[0], args) < 0) {
		    fprintf(stderr, "Cannot execute command.\n");
		    exit(1);
		}
	    }else {
		// int rc_wait = wait(NULL);
		// printf("Exit with code %d\n", rc_wait);
		printf("Parent Exit\n");
	    }

	    if (profile) {
		print_cpu_time(startingUserTime, startingSystemTime);
	    }
	    //fflush(stdout);
	    break;
	/* -------------------verbose------------------- */
	case 'V':
	    verbose = 1;
	    break;
	/* -------------------append------------------- */
	case 'A':
	    if (verbose) printf("\n");
	    if (profile) {
		get_starting_time(&startingUserTime, &startingSystemTime);
	    }
	    flag |= O_APPEND;
	    if(profile) {
		print_cpu_time(startingUserTime, startingSystemTime);
	    }
	    break;
	/* -------------------cloexec------------------- */
	case 'L':
	    if (verbose) printf("\n");
	    if (profile) {
		get_starting_time(&startingUserTime, &startingSystemTime);
	    }
	    flag |= O_CLOEXEC;
	    if (profile) {
		print_cpu_time(startingUserTime, startingSystemTime);
	    }
	    break;

	/* -------------------creat------------------- */
	case 'c':
	    if (verbose) printf("\n");
	    if (profile) {
		get_starting_time(&startingUserTime, &startingSystemTime);
	    }
	    flag |= O_CREAT;
	    if (profile) {
		print_cpu_time(startingUserTime, startingSystemTime);
	    }
	    break;

	/* -------------------directory------------------- */
	case 'D':
	    if (verbose) printf("\n");
	    if (profile) {
		get_starting_time(&startingUserTime, &startingSystemTime);
	    }
	    flag |= O_DIRECTORY;
	    if (profile) {
		print_cpu_time(startingUserTime, startingSystemTime);
	    }
	    break;

	/* -------------------dsync------------------- */
	case 'd':
	    if (verbose) printf("\n");
	    if (profile) {
		get_starting_time(&startingUserTime, &startingSystemTime);
	    }
	    flag |= O_DSYNC;
	    if (profile) {
		print_cpu_time(startingUserTime, startingSystemTime);
	    }
	    break;

	/* -------------------excl------------------- */
	case 'E':
	    if (verbose) printf("\n");
	    if (profile) {
		get_starting_time(&startingUserTime, &startingSystemTime);
	    }
	    flag |= O_EXCL;
	    if (profile) {
		print_cpu_time(startingUserTime, startingSystemTime);
	    }
	    break;

	/* -------------------nofollow------------------- */
	case 'F':
	    if (verbose) printf("\n");
	    if (profile) {
		get_starting_time(&startingUserTime, &startingSystemTime);
	    }
	    flag |= O_NOFOLLOW;
	    if (profile) {
		print_cpu_time(startingUserTime, startingSystemTime);
	    }
	    break;

	/* -------------------nonblock------------------- */
	case 'B':
	    if (verbose) printf("\n");
	    if (profile) {
		get_starting_time(&startingUserTime, &startingSystemTime);
	    }
	    flag |= O_NONBLOCK;
	    if (profile) {
		print_cpu_time(startingUserTime, startingSystemTime);
	    }
	    break;

	/* -------------------rsync------------------- */
	case 'S':
	    if (verbose) printf("\n");
	    if (profile) {
		get_starting_time(&startingUserTime, &startingSystemTime);
	    }
	    flag |= O_RSYNC;
	    if (profile) {
		print_cpu_time(startingUserTime, startingSystemTime);
	    }
	    break;

	/* -------------------sync------------------- */
	case 's':
	    if (verbose) printf("\n");
	    if (profile) {
		get_starting_time(&startingUserTime, &startingSystemTime);
	    }
	    flag |= O_SYNC;
	    if (profile) {
		print_cpu_time(startingUserTime, startingSystemTime);
	    }
	    break;

	/* -------------------trunc------------------- */
	case 'T':
	    if (verbose) printf("\n");
	    if (profile) {
		get_starting_time(&startingUserTime, &startingSystemTime);
	    }
	    flag |= O_TRUNC;
	    if (profile) {
		print_cpu_time(startingUserTime, startingSystemTime);
	    }
	    break;

	/* -------------------rdwr------------------- */
	case 'r':
	    if (verbose) printf("%s\n",optarg);
	    if (profile) {
		get_starting_time(&startingUserTime, &startingSystemTime);
	    }
	    flag |= O_RDWR;
	    if ((fd_list[fd_index++] = open(optarg, flag, 0)) == -1) {
		fprintf(stderr, "RSYNC file %s failed.\n", optarg);
		exit(1);
	    }
	    flag = 0;
	    if (profile) {
		print_cpu_time(startingUserTime, startingSystemTime);
	    }
	    break;

	/* -------------------pipe------------------- */
	case 'P':
	    if (verbose) printf("\n");
	    if (profile) {
		get_starting_time(&startingUserTime, &startingSystemTime);
	    }
	    /* open a pipe */
	    int fd[2];
	    pipe(fd);
	    fd_list[fd_index++] = fd[0];
	    fd_list[fd_index++] = fd[1];
	    if(profile == 1) {
		print_cpu_time(startingUserTime, startingSystemTime);
	    }
	    break;
	/* -------------------wait------------------- */
	case 'w':
	    if (verbose) printf("\n");
	    if (profile) {
		get_starting_time(&startingUserTime, &startingSystemTime);
	    }
	    pid_t rc_pid;
	    int i = 0;
	    for (; i < pid_num; i++) {
		int status;
		// rc_pid = waitpid(pid_arr[i], &status, 0);
		// assert(rc_pid == pid_arr[i]);
		rc_pid = wait(&status);   /* return the pid of finished process */

		printf("Reach here\n");
		if (rc_pid > 0)
		{
		    if (WIFEXITED(status)) {
			printf("Child exited with RC=%d\n",WEXITSTATUS(status));
		    }else if (WIFSIGNALED(status)) {
			printf("Child exited via signal %d\n",WTERMSIG(status));
		    }

		    /* Find the command name according to the return pid */
		    int j;
		    for (j = 0; j < process_num; j++) {
			if (rc_pid == process_arr[j].pid) {
			    int c;
			    for (c = 0; c < process_arr[j].size; c++) {
				printf("%s ", process_arr[j].cmd_name[c]);
			    }
			    break;
			}
		    }
		    printf("\n");
		}
	    }
	    if (profile) {
		printf("Normal: \n");
		print_cpu_time(startingUserTime, startingSystemTime);
	    }
	    if (profile) {
		printf("Child: \n");
		print_cpu_time(startingUserTime, startingSystemTime);
	    }
	    break;

	/* -------------------close------------------- */
	case 'O':
	    if (verbose) printf("%s\n",optarg);
	    if (profile) {
		get_starting_time(&startingUserTime, &startingSystemTime);
	    }
	    close_fd = atoi(argv[optind++]);
	    close(fd_list[close_fd]);
	    fd_list[close_fd] = -1;
	    if (profile) {
		print_cpu_time(startingUserTime, startingSystemTime);
	    }
	    break;

	/* -------------------abort------------------- */
	case 'a':
	    if (verbose) printf("%s\n",optarg);
	    if (profile) {
		get_starting_time(&startingUserTime, &startingSystemTime);
	    }
	    fflush(stdout);
	    abort();
	    if(profile) {
		print_cpu_time(startingUserTime, startingSystemTime);
	    }
	    break;

	/* -------------------catch------------------- */
	case 't':
	    if (verbose) printf("%s\n", optarg);
	    if (profile) {
		get_starting_time(&startingUserTime, &startingSystemTime);
	    }
	    int sig = atoi(optarg);
	    if (signal(sig, sig_handler) == SIG_ERR) {
		printf("Cannot catch signal %d \n", sig);
	    }
	    if (profile) {
		print_cpu_time(startingUserTime, startingSystemTime);
	    }
	    break;

	/* -------------------ignore------------------- */
	case 'I':
	    if (verbose) printf("%s\n", optarg);
	    if (profile) {
		get_starting_time(&startingUserTime, &startingSystemTime);
	    }
	    sig = atoi(optarg);
	    if (signal(sig, SIG_IGN) == SIG_ERR) {
		printf("Cannot ignore signal %d \n", sig);
	    }
	    if (profile) {
		print_cpu_time(startingUserTime, startingSystemTime);
	    }
	    break;

	/* -------------------default------------------- */
	case 'e':
	    if (profile) {
		get_starting_time(&startingUserTime, &startingSystemTime);
	    }
	    sig = atoi(optarg);
	    if (signal(sig, SIG_DFL) == SIG_ERR) {
		printf("Cannot catch signal %d \n", sig);
	    }
	    if (profile) {
		print_cpu_time(startingUserTime, startingSystemTime);
	    }
	    break;
	/* -------------------pause------------------- */
	case 'p':
	    if (verbose) printf("%s\n",optarg);
	    if (profile) {
		get_starting_time(&startingUserTime, &startingSystemTime);
	    }
	    pause();
	    if (profile) {
		print_cpu_time(startingUserTime, startingSystemTime);
	    }
	    break;

	/* -------------------profile------------------- */
	case 'o':
	    if (verbose) printf("\n");
	    profile = 1;
	    break;

	}
    }
    fflush(stdout);
    free(fd_list);
    free(pid_arr);

    exit(0);
}
