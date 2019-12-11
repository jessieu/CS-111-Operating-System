#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h> //O_RSYNC only works under the Linux environment
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/resource.h>

/* collect process information - initialize in command and retrieve in wait */
struct process_info
{
  char *cmd_name[256];
  pid_t pid;
  int size;
} process_info;

void sig_handler(int signo)
{
  /* catch and exit with the signo */
  fprintf(stderr, "%d caught.\n", signo);
  exit(signo);
}

int get_starting_time (struct timeval *startingUTime, struct timeval *startingSTime )
{
  struct rusage usage;
  if (getrusage(RUSAGE_SELF, &usage) == -1){
    return -1;
  }

  *startingUTime = usage.ru_utime;
  startingUTime->tv_sec = usage.ru_utime.tv_sec;
  startingUTime->tv_usec = usage.ru_utime.tv_usec;

  *startingSTime = usage.ru_stime;
  startingSTime->tv_sec = usage.ru_stime.tv_sec;
  startingSTime->tv_usec = usage.ru_stime.tv_usec;

  return 0;
}

// for resources used by the current process
int print_cpu_time(struct timeval startingUTime, struct timeval startingSTime)
{
  struct rusage usage;
  if (getrusage(RUSAGE_SELF, &usage) == -1){
    return -1;
  }
  struct timeval endingUTime = usage.ru_utime;
  struct timeval endingSTime = usage.ru_stime;
  
  double uS = ((double)(startingUTime.tv_usec/1000000.0)) + (startingUTime.tv_sec);
  double uE = ((double)endingUTime.tv_usec/1000000.0) + (endingUTime.tv_sec);
  double userTime = uE - uS;
  
  double sS = ((double)startingSTime.tv_usec/1000000.0) + (startingSTime.tv_sec);
  double sE = ((double)endingSTime.tv_usec/1000000.0) + (endingSTime.tv_sec);
  double sysTime = sE -sS;

  printf("CPU time: %f sec user, %f sec system\n", userTime, sysTime);
  fflush(stdout);

  return 0;
}

// for resources used by all the terminated children of the current process
int print_child_cpu_time(struct timeval startingUTime, struct timeval startingSTime)
{
  struct rusage usage;
  if (getrusage(RUSAGE_CHILDREN, &usage) == -1){
    return -1;
  }
  struct timeval endingUTime = usage.ru_utime;
  struct timeval endingSTime = usage.ru_stime;
  
  double uS = ((double)(startingUTime.tv_usec/1000000.0)) + (startingUTime.tv_sec);
  double uE = ((double)endingUTime.tv_usec/1000000.0) + (endingUTime.tv_sec);
  double userTime = uE - uS;

  double sS = ((double)startingSTime.tv_usec/1000000.0) + (startingSTime.tv_sec);
  double sE = ((double)endingSTime.tv_usec/1000000.0) + (endingSTime.tv_sec);
  double sysTime = sE -sS;

  printf("Child CPU time: %f sec user, %f sec system\n", userTime, sysTime);
  fflush(stdout);

  return 0;
}

int main(int argc, char *argv[])
{
  /* array to store file descriptors */
  int fd_list[256];
  int fd_index = 0;

  /* for verbose */
  int verbose = 0;

  /* for profile */
  int profile = 0;
  struct timeval startingUserTime, startingSystemTime;

  /* for parsing option */
  int long_index = 0;
  int opt = 0;

  /* exit code */
  int exit_code = 0;
  int max_exit = 0;
  int max_sig = 0;

  /* flags for open file */
  int flags;

  /* array to store process information */
  struct process_info process_arr[256];
  int process_num = 0;

    static struct option long_options[] =
      {
        /* Lab 1A */
        {"rdonly", required_argument, 0, 'R'},
        {"wronly", required_argument, 0, 'W'},
        {"command", required_argument, 0, 'C'},
        {"verbose", no_argument, 0, 'V'},
        /* Lab 1B */
        {"append", no_argument, 0, 'A'},
        {"cloexec", no_argument, 0, 'L'},
        {"creat", no_argument, 0, 'c'},
        {"directory", no_argument, 0, 'D'},
        {"dsync", no_argument, 0, 'd'},
        {"excl", no_argument, 0, 'E'},
        {"nofollow", no_argument, 0, 'F'},
        {"nonblock", no_argument, 0, 'B'},
        {"rsync", no_argument, 0, 'S'},
        {"sync", no_argument, 0, 's'},
        {"trunc", no_argument, 0, 'T'},
        {"rdwr", required_argument, 0, 'r'},
        {"pipe", no_argument, 0, 'P'},
        {"wait", no_argument, 0, 'w'},
        {"close", required_argument, 0, 'O'},
        {"abort", no_argument, 0, 'a'},
        {"catch", required_argument, 0, 't'},
        {"ignore", required_argument, 0, 'I'},
        {"default", required_argument, 0, 'e'},
        {"pause", no_argument, 0, 'p'},
        {"chdir", required_argument, 0, 'h'},
        /* lab1c */
        {"profile", no_argument, 0, 'o'},
        {0, 0, 0, 0}
      };

    while ((opt = getopt_long(argc, argv, "", long_options, &long_index)) != -1)
      {

        if (verbose)
	  {
            printf("--%s ", long_options[long_index].name);
            fflush(stdout);
	  }
        switch (opt)
	  {
	    /* -------------------rdonly------------------- */
	  case 'R':
            if (verbose)
	      {
                printf("%s\n", optarg);
                fflush(stdout);
	      }
            if (profile)
	      {
                if(get_starting_time(&startingUserTime, &startingSystemTime) < 0){
		  fprintf(stderr, "getrusage error.\n");
		  exit_code = 1;
                }
	      }
            flags |= O_RDONLY;
            if ((fd_list[fd_index++] = open(optarg, flags, 0666)) == -1)
	      {
                fprintf(stderr, "Cannot open file %s with read only.\n", optarg);
                exit_code = 1;
	      }
            flags = 0;
            if (profile)
	      {
                if(print_cpu_time(startingUserTime, startingSystemTime) < 0){
		  fprintf(stderr, "getrusage error.\n");
		  exit_code = 1;
                }
	      }
            break;

	    /* -------------------wronly------------------- */
	  case 'W':
            if (verbose)
	      {
                printf("%s\n", optarg);
                fflush(stdout);
	      }
            if (profile)
	      {
                if(get_starting_time(&startingUserTime, &startingSystemTime) < 0){
		  fprintf(stderr, "getrusage error.\n");
		  exit_code = 1;
                }
	      }
            flags |= O_WRONLY;
            if ((fd_list[fd_index++] = open(optarg, flags, 0666)) == -1)
	      {
                fprintf(stderr, "Cannot open file %s with write only.\n", optarg);
                exit_code = 1;
	      }
            flags = 0;
            if (profile)
	      {
                if(print_cpu_time(startingUserTime, startingSystemTime) < 0){
		  fprintf(stderr, "getrusage error.\n");
		  exit_code = 1;
                }
	      }
            break;

	    /* -------------------command------------------- */
	  case 'C':
	    if (profile)
	      {
                if(get_starting_time(&startingUserTime, &startingSystemTime) < 0){
		  fprintf(stderr, "getrusage error.\n");
		  exit_code = 1;
                }
	      }
            optind--;

            char *args[256];

            int in = atoi(argv[optind++]);
            int out = atoi(argv[optind++]);
            int err = atoi(argv[optind++]);

            int index = 0;

            while (optind < argc)
	      {
                if (argv[optind][0] == '-' && argv[optind][1] == '-')
		  break;
                args[index++] = argv[optind++];
	      }
            args[index] = NULL;

            if (args[0] == NULL)
	      {
                fprintf(stderr, "%s\n", "Missing command");
                exit_code = 1;
                break;
	      }

            if (verbose)
	      {
                printf("%d %d %d ", in, out, err);
                fflush(stdout);
                int i;
                for (i = 0; i < index; i++)
		  {
                    printf("%s ", args[i]);
                    fflush(stdout);
		  }
                printf("\n");
                fflush(stdout);
	      }

            /* Check file descriptor validation */
            if (in >= fd_index || out >= fd_index || err >= fd_index)
	      {
                fprintf(stderr, "Invalid file descriptor.\n");
                exit_code = 1;
                break;
	      }

            if (fd_list[in] < 0 || fd_list[out] < 0 || fd_list[err] < 0)
	      {
                fprintf(stderr, "Invalid file descriptor.\n");
                exit_code = 1;
                break;
	      }

            /* create child process */
            int rc = fork();

            /* Store current process info and retrieve it in wait */
            process_arr[process_num].pid = rc;
            int j = 0;
            for (j = 0; j < index; j++)
	      {
                process_arr[process_num].cmd_name[j] = args[j];
	      }
            process_arr[process_num].size = index;
            process_num++;

            if (rc < 0)
	      {
                fprintf(stderr, "fork failed\n");

	      }
            else if (rc == 0)
	      {
                dup2(fd_list[in], 0);
                dup2(fd_list[out], 1);
                dup2(fd_list[err], 2);

                //for pipe: close file descriptors to force output from pipe
                int k;
                for (k = 0; k < fd_index; k++)
		  {
                    // skip closed file
                    if (fd_list[k] == -1)
		      {
                        continue;
		      }
                    close(fd_list[k]);
		  }

                /* Execute the command */
                if (execvp (args[0], args) < 0)
		  {
                    fprintf(stderr, "Cannot execute command.\n");
                    exit_code = 1;
		  }

	      }
            else
	      {
                /* Parent return here */
                // break;
	      }
            if (profile)
	      {
                if(print_cpu_time(startingUserTime, startingSystemTime) < 0){
		  fprintf(stderr, "getrusage error.\n");
		  exit_code = 1;
                }
	      }
            break;
	    /* -------------------verbose------------------- */
	  case 'V':
            if (profile)
	      {
                if(get_starting_time(&startingUserTime, &startingSystemTime) < 0){
		  fprintf(stderr, "getrusage error.\n");
		  exit_code = 1;
                }
	      }
            verbose = 1;
            if (profile)
	      {
                if(print_cpu_time(startingUserTime, startingSystemTime) < 0){
		  fprintf(stderr, "getrusage error.\n");
		  exit_code = 1;
                }
	      }
            break;

	    /* -------------------append------------------- */
	  case 'A':
            if (verbose)
	      {
                printf("\n");
                fflush(stdout);
	      }
            if (profile)
	      {
                if(get_starting_time(&startingUserTime, &startingSystemTime) < 0){
		  fprintf(stderr, "getrusage error.\n");
		  exit_code = 1;
                }
	      }
            flags |= O_APPEND;
            if (profile)
	      {
                if(print_cpu_time(startingUserTime, startingSystemTime) < 0){
		  fprintf(stderr, "getrusage error.\n");
		  exit_code = 1;
                }
	      }
            break;
	    /* -------------------cloexec------------------- */
	  case 'L':
            if (verbose)
	      {
                printf("\n");
                fflush(stdout);
	      }
            if (profile)
	      {
                if(get_starting_time(&startingUserTime, &startingSystemTime) < 0){
		  fprintf(stderr, "getrusage error.\n");
		  exit_code = 1;
                }
	      }
            flags |= O_CLOEXEC;
            if (profile)
	      {
                if(print_cpu_time(startingUserTime, startingSystemTime) < 0){
		  fprintf(stderr, "getrusage error.\n");
		  exit_code = 1;
                }
	      }
            break;

	    /* -------------------creat------------------- */
	  case 'c':
            if (verbose)
	      {
                printf("\n");
                fflush(stdout);
	      }
            if (profile)
	      {
                if(get_starting_time(&startingUserTime, &startingSystemTime) < 0){
		  fprintf(stderr, "getrusage error.\n");
		  exit_code = 1;
                }
	      }
            flags |= O_CREAT;
            if (profile)
	      {
                if(print_cpu_time(startingUserTime, startingSystemTime) < 0){
		  fprintf(stderr, "getrusage error.\n");
		  exit_code = 1;
                }
	      }
            break;

	    /* -------------------directory------------------- */
	  case 'D':
            if (verbose)
	      {
                printf("\n");
                fflush(stdout);
	      }
            if (profile)
	      {
                if(get_starting_time(&startingUserTime, &startingSystemTime) < 0){
		  fprintf(stderr, "getrusage error.\n");
		  exit_code = 1;
                }
	      }
            flags |= O_DIRECTORY;
            if (profile)
	      {
                if(print_cpu_time(startingUserTime, startingSystemTime) < 0){
		  fprintf(stderr, "getrusage error.\n");
		  exit_code = 1;
                }
	      }
            break;

	    /* -------------------dsync------------------- */
	  case 'd':
            if (verbose)
	      {
                printf("\n");
                fflush(stdout);
	      }
            if (profile)
	      {
                if(get_starting_time(&startingUserTime, &startingSystemTime) < 0){
		  fprintf(stderr, "getrusage error.\n");
		  exit_code = 1;
                }
	      }
            flags |= O_DSYNC;
            if (profile)
	      {
                if(print_cpu_time(startingUserTime, startingSystemTime) < 0){
		  fprintf(stderr, "getrusage error.\n");
		  exit_code = 1;
                }
	      }
            break;

	    /* -------------------excl------------------- */
	  case 'E': // When it is used together with creat, if the file exists, it would fail
            if (verbose)
	      {
                printf("\n");
                fflush(stdout);
	      }
            if (profile)
	      {
                if(get_starting_time(&startingUserTime, &startingSystemTime) < 0){
		  fprintf(stderr, "getrusage error.\n");
		  exit_code = 1;
                }
	      }
            flags |= O_EXCL;
            if (profile)
	      {
                if(print_cpu_time(startingUserTime, startingSystemTime) < 0){
		  fprintf(stderr, "getrusage error.\n");
		  exit_code = 1;
                }
	      }
            break;

	    /* -------------------nofollow------------------- */
	  case 'F':
            if (verbose)
	      {
                printf("\n");
                fflush(stdout);
	      }
            if (profile)
	      {
                if(get_starting_time(&startingUserTime, &startingSystemTime) < 0){
		  fprintf(stderr, "getrusage error.\n");
		  exit_code = 1;
                }
	      }
            flags |= O_NOFOLLOW;
            if (profile)
	      {
                if(print_cpu_time(startingUserTime, startingSystemTime) < 0){
		  fprintf(stderr, "getrusage error.\n");
		  exit_code = 1;
                }
	      }
            break;

	    /* -------------------nonblock------------------- */
	  case 'B':
            if (verbose)
	      {
                printf("\n");
                fflush(stdout);
	      }
            if (profile)
	      {
                if(get_starting_time(&startingUserTime, &startingSystemTime) < 0){
		  fprintf(stderr, "getrusage error.\n");
		  exit_code = 1;
                }
	      }
            flags |= O_NONBLOCK;
            if (profile)
	      {
                if(print_cpu_time(startingUserTime, startingSystemTime) < 0){
		  fprintf(stderr, "getrusage error.\n");
		  exit_code = 1;
                }
	      }
            break;

	    /* -------------------rsync------------------- */
	  case 'S': // this flag only works in linux
            if (verbose)
	      {
                printf("\n");
                fflush(stdout);
	      }
            if (profile)
	      {
                if(get_starting_time(&startingUserTime, &startingSystemTime) < 0){
		  fprintf(stderr, "getrusage error.\n");
		  exit_code = 1;
                }
	      }
            flags |= O_RSYNC;
            if (profile)
	      {
                if(print_cpu_time(startingUserTime, startingSystemTime) < 0){
		  fprintf(stderr, "getrusage error.\n");
		  exit_code = 1;
                }
	      }
            break;

	    /* -------------------sync------------------- */
	  case 's':
            if (verbose)
	      {
                printf("\n");
                fflush(stdout);
	      }
            if (profile)
	      {
                if(get_starting_time(&startingUserTime, &startingSystemTime) < 0){
		  fprintf(stderr, "getrusage error.\n");
		  exit_code = 1;
                }
	      }
            flags |= O_SYNC;
            if (profile)
	      {
                if(print_cpu_time(startingUserTime, startingSystemTime) < 0){
		  fprintf(stderr, "getrusage error.\n");
		  exit_code = 1;
                }
	      }
            break;

	    /* -------------------trunc------------------- */
	  case 'T':
            if (verbose)
	      {
                printf("\n");
                fflush(stdout);
	      }
            if (profile)
	      {
                if(get_starting_time(&startingUserTime, &startingSystemTime) < 0){
		  fprintf(stderr, "getrusage error.\n");
		  exit_code = 1;
                }
	      }
            flags |= O_TRUNC;
            if (profile)
	      {
                if(print_cpu_time(startingUserTime, startingSystemTime) < 0){
		  fprintf(stderr, "getrusage error.\n");
		  exit_code = 1;
                }
	      }
            break;

	    /* -------------------rdwr------------------- */
	  case 'r':
            if (verbose)
	      {
                printf("%s\n", optarg);
                fflush(stdout);
	      }
            if (profile)
	      {
                if(get_starting_time(&startingUserTime, &startingSystemTime) < 0){
		  fprintf(stderr, "getrusage error.\n");
		  exit_code = 1;
                }
	      }
            flags |= O_RDWR;

            if ((fd_list[fd_index++] = open(optarg, flags, 0666)) == -1)
	      {
                fprintf(stderr, "Cannot open file %s.\n", optarg);
                exit_code = 1;
	      }
            flags = 0; /* The file does not open with other flags */
            if (profile)
	      {
                if(print_cpu_time(startingUserTime, startingSystemTime) < 0){
		  fprintf(stderr, "getrusage error.\n");
		  exit_code = 1;
                }
	      }
            break;

	    /* -------------------pipe------------------- */
	  case 'P':
            if (verbose)
	      {
                printf("\n");
                fflush(stdout);
	      }
            if (profile)
	      {
                if(get_starting_time(&startingUserTime, &startingSystemTime) < 0){
		  fprintf(stderr, "getrusage error.\n");
		  exit_code = 1;
                }
	      }

            /* open a pipe */
            int fd[2];
            if (pipe(fd) == -1)
	      {
                fprintf(stderr, "Pipe failed.");
                exit_code = 1;
	      }
            fd_list[fd_index++] = fd[0];
            fd_list[fd_index++] = fd[1];
            if (profile)
	      {
                if(print_cpu_time(startingUserTime, startingSystemTime) < 0){
		  fprintf(stderr, "getrusage error.\n");
		  exit_code = 1;
                }
	      }
            break;
	    /* -------------------wait------------------- */
	  case 'w':
            if (verbose)
	      {
                printf("\n");
                fflush(stdout);
	      }
            if (profile)
	      {
                if(get_starting_time(&startingUserTime, &startingSystemTime) < 0){
		  fprintf(stderr, "getrusage error.\n");
		  exit_code = 1;
                }
	      }

            pid_t rc_pid;
            int status, w_status;

            while ((rc_pid = waitpid(-1, &status, 0)) > 0)
	      {
                /* exit after finished */
                if (WIFEXITED(status))
		  {
                    w_status = WEXITSTATUS(status);;
                    if (w_status > max_exit)
		      {
                        max_exit = w_status;
		      }
                    printf("exit %d ", w_status);
                    fflush(stdout);
		  }
                /* exit by signaled */
                else if (WIFSIGNALED(status))
		  {
                    w_status = WTERMSIG(status);

                    if (w_status > max_sig)
		      {
                        max_sig = w_status;
		      }
                    printf("signal %d ", w_status);
                    fflush(stdout);
		  }

                /* Find the command name according to the return pid */
                int i;
                for (i = 0; i < process_num; i++)
		  {
                    if (rc_pid == process_arr[i].pid)
		      {
                        int j;
                        for (j = 0; j < process_arr[i].size; j++)
			  {
                            printf("%s ", process_arr[i].cmd_name[j]);
			  }
                        printf("\n");
                        fflush(stdout);
                        break;
		      }
		  }
	      }
            if (profile)
	      {
                if(print_cpu_time(startingUserTime, startingSystemTime) < 0){
		  fprintf(stderr, "getrusage error.\n");
		  exit_code = 1;
                }

                if(print_child_cpu_time(startingUserTime, startingSystemTime) < 0){
		  fprintf(stderr, "getrusage error.\n");
		  exit_code = 1;
                }
	      }
            break;

	    /* -------------------close------------------- */
	  case 'O':
            if (verbose)
	      {
                printf("%s\n", optarg);
                fflush(stdout);
	      }
            if (profile)
	      {
                if(get_starting_time(&startingUserTime, &startingSystemTime) < 0){
		  fprintf(stderr, "getrusage error.\n");
		  exit_code = 1;
                }
	      }
            int close_fd = atoi(optarg);
            close(fd_list[close_fd]);
            fd_list[close_fd] = -1; /* Not allowed to be reused */
            if (profile)
	      {
                if(print_cpu_time(startingUserTime, startingSystemTime) < 0){
		  fprintf(stderr, "getrusage error.\n");
		  exit_code = 1;
                }
	      }
            break;

	    /* -------------------abort------------------- */
	  case 'a':
            if (verbose)
	      {
                printf("\n");
                fflush(stdout);
	      }
            if (profile)
	      {
                if(get_starting_time(&startingUserTime, &startingSystemTime) < 0){
		  fprintf(stderr, "getrusage error.\n");
		  exit_code = 1;
                }
	      }
            raise(SIGSEGV); /* Dump a core */
            if (profile)
	      {
                if(print_cpu_time(startingUserTime, startingSystemTime) < 0){
		  fprintf(stderr, "getrusage error.\n");
		  exit_code = 1;
                }
	      }
            break;

	    /* -------------------catch------------------- */
	  case 't':
            if (verbose)
	      {
                printf("%s\n", optarg);
                fflush(stdout);
	      }
            if (profile)
	      {
                if(get_starting_time(&startingUserTime, &startingSystemTime) < 0){
		  fprintf(stderr, "getrusage error.\n");
		  exit_code = 1;
                }
	      }
            int sig_c = atoi(optarg);
            if (signal(sig_c, sig_handler) == SIG_ERR)
	      {
                fprintf(stderr, "Cannot catch signal %d \n", sig_c);
                exit_code = 1;
	      }
            if (profile)
	      {
                if(print_cpu_time(startingUserTime, startingSystemTime) < 0){
		  fprintf(stderr, "getrusage error.\n");
		  exit_code = 1;
                }
	      }
            break;

	    /* -------------------ignore------------------- */
	  case 'I':
            if (verbose)
	      {
                printf("%s\n", optarg);
                fflush(stdout);
	      }
            if (profile)
	      {
                if(get_starting_time(&startingUserTime, &startingSystemTime) < 0){
		  fprintf(stderr, "getrusage error.\n");
		  exit_code = 1;
                }
	      }
            int sig_i = atoi(optarg);
            if (signal(sig_i, SIG_IGN) == SIG_ERR)
	      {
                fprintf(stderr, "Cannot ignore signal %d \n", sig_i);
                exit_code = 1;
	      }
            if (profile)
	      {
                if(print_cpu_time(startingUserTime, startingSystemTime) < 0){
		  fprintf(stderr, "getrusage error.\n");
		  exit_code = 1;
                }
	      }
            break;

	    /* -------------------default------------------- */
	  case 'e':
            if (verbose)
	      {
                printf("\n");
                fflush(stdout);
	      }
            if (profile)
	      {
                if(get_starting_time(&startingUserTime, &startingSystemTime) < 0){
		  fprintf(stderr, "getrusage error.\n");
		  exit_code = 1;
                }
	      }
            int sig_d = atoi(optarg);
            if (signal(sig_d, SIG_DFL) == SIG_ERR)
	      {
                fprintf(stderr, "Cannot catch signal %d \n", sig_d);
	      }
            if (profile)
	      {
                if(print_cpu_time(startingUserTime, startingSystemTime) < 0){
		  fprintf(stderr, "getrusage error.\n");
		  exit_code = 1;
                }
	      }
            break;
	    /* -------------------pause------------------- */
	  case 'p':
            if (verbose)
	      {
                printf("\n");
                fflush(stdout);
	      }
            if (profile)
	      {
                if(get_starting_time(&startingUserTime, &startingSystemTime) < 0){
		  fprintf(stderr, "getrusage error.\n");
		  exit_code = 1;
                }
	      }
            pause(); //Suspends execution until any signal is caught
            if (profile)
	      {
                if(print_cpu_time(startingUserTime, startingSystemTime) < 0){
		  fprintf(stderr, "getrusage error.\n");
		  exit_code = 1;
                }
	      }
            break;
	    /* -------------------chdir------------------- */
	  case 'h':
            if (verbose)
	      {
                printf("%s\n", optarg);
                fflush(stdout);
	      }
            if (profile)
	      {
                if(get_starting_time(&startingUserTime, &startingSystemTime) < 0){
		  fprintf(stderr, "getrusage error.\n");
		  exit_code = 1;
                }
	      }
            if (chdir(optarg) == -1)
	      {
                fprintf(stderr, "Cannot open directory %s\n", optarg);
                exit_code = 1;
	      }
            if (profile)
	      {
                if(print_cpu_time(startingUserTime, startingSystemTime) < 0){
		  fprintf(stderr, "getrusage error.\n");
		  exit_code = 1;
                }
	      }
            break;
	    /* -------------------profile------------------- */
	  case 'o':
            if (verbose)
	      {
                printf("\n");
                fflush(stdout);
	      }
            profile = 1;
	    break;
	  case '?':
            fprintf(stderr, "Invalid option\n");
            exit_code = 1;
	  default:
            break;
	  }
      }

    // terminate with signal if child receieved signal
    if (max_sig)
      {
        signal(max_sig, SIG_DFL); //terminate the process upon receipt of max_sig
        kill(getpid(), max_sig);  //terminate the process with signal number(same as raise)
      }
    else if (max_exit)
      {
        exit(max_exit);
      }

    exit(exit_code);
}
