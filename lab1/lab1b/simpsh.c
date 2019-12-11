#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h> //O_RSYNC only works under the Linux environment
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <signal.h>

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

int main(int argc, char *argv[])
{
    /* array to store file descriptors */
    int fd_list[256];  
    int fd_index = 0;

    /* for verbose */
    int verbose = 0;

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
            flags |= O_RDONLY;
            if ((fd_list[fd_index++] = open(optarg, flags, 0666)) == -1)
            {
                fprintf(stderr, "Cannot open file %s with read only.\n", optarg);
                exit_code = 1;
            }
            flags = 0;
            break;

        /* -------------------wronly------------------- */
        case 'W':
            if (verbose)
            {
                printf("%s\n", optarg);
                fflush(stdout);
            }
            flags |= O_WRONLY;
            if ((fd_list[fd_index++] = open(optarg, flags, 0666)) == -1)
            {
                fprintf(stderr, "Cannot open file %s with write only.\n", optarg);
                exit_code = 1;
            }
            flags = 0;
            break;

        /* -------------------command------------------- */
        case 'C':
            optind--;

            char *args[256];

            int in = atoi(argv[optind++]);
            int out = atoi(argv[optind++]);
            int err = atoi(argv[optind++]);

            int index = 0;

            while (optind < argc) {
                if (argv[optind][0] == '-' && argv[optind][1] == '-')
                    break;
                args[index++] = argv[optind++];
            }
            args[index] = NULL;
            
            if (args[0] == NULL){
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
                break;
            }
            break;
        /* -------------------verbose------------------- */
        case 'V':
            verbose = 1;
            break;

        /* -------------------append------------------- */
        case 'A':
            if (verbose)
            {
                printf("\n");
                fflush(stdout);
            }
            flags |= O_APPEND;
            break;
        /* -------------------cloexec------------------- */
        case 'L':
            if (verbose)
            {
                printf("\n");
                fflush(stdout);
            }
            flags |= O_CLOEXEC;
            break;

        /* -------------------creat------------------- */
        case 'c':
            if (verbose)
            {
                printf("\n");
                fflush(stdout);
            }
            flags |= O_CREAT;
            break;

        /* -------------------directory------------------- */
        case 'D':
            if (verbose)
            {
                printf("\n");
                fflush(stdout);
            }
            flags |= O_DIRECTORY;
            break;

        /* -------------------dsync------------------- */
        case 'd':
            if (verbose)
            {
                printf("\n");
                fflush(stdout);
            }
            flags |= O_DSYNC;
            break;

        /* -------------------excl------------------- */
        case 'E': // When it is used together with creat, if the file exists, it would fail
            if (verbose)
            {
                printf("\n");
                fflush(stdout);
            }
            flags |= O_EXCL;
            break;

        /* -------------------nofollow------------------- */
        case 'F':
            if (verbose)
            {
                printf("\n");
                fflush(stdout);
            }
            flags |= O_NOFOLLOW;
            break;

        /* -------------------nonblock------------------- */
        case 'B':
            if (verbose)
            {
                printf("\n");
                fflush(stdout);
            }
            flags |= O_NONBLOCK;
            break;

        /* -------------------rsync------------------- */
        case 'S': // this flag only works in linux 
            if (verbose)
            {
                printf("\n");
                fflush(stdout);
            }
            flags |= O_RSYNC;
            break;

        /* -------------------sync------------------- */
        case 's':
            if (verbose)
            {
                printf("\n");
                fflush(stdout);
            }
            flags |= O_SYNC;
            break;

        /* -------------------trunc------------------- */
        case 'T':
            if (verbose)
            {
                printf("\n");
                fflush(stdout);
            }
            flags |= O_TRUNC;
            break;

        /* -------------------rdwr------------------- */
        case 'r':
            if (verbose)
            {
                printf("%s\n", optarg);
                fflush(stdout);
            }
            flags |= O_RDWR;

            if ((fd_list[fd_index++] = open(optarg, flags, 0666)) == -1)
            {
                fprintf(stderr, "Cannot open file %s.\n", optarg);
                exit_code = 1;
            }
            flags = 0; /* The file does not open with other flags */
            break;

        /* -------------------pipe------------------- */
        case 'P':
            if (verbose)
            {
                printf("\n");
                fflush(stdout);
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
            break;
        /* -------------------wait------------------- */
        case 'w':
            if (verbose)
            {
                printf("\n");
                fflush(stdout);
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
            break;

        /* -------------------close------------------- */
        case 'O':
            if (verbose)
            {
                printf("%s\n", optarg);
                fflush(stdout);
            }
            int close_fd = atoi(optarg);
            close(fd_list[close_fd]);
            fd_list[close_fd] = -1; /* Not allowed to be reused */
            break;

        /* -------------------abort------------------- */
        case 'a':
            if (verbose)
            {
                printf("\n");
                fflush(stdout);
            }
            raise(SIGSEGV); /* Dump a core */
            break;

        /* -------------------catch------------------- */
        case 't':
            if (verbose)
            {
                printf("%s\n", optarg);
                fflush(stdout);
            }
            int sig_c = atoi(optarg);
            if (signal(sig_c, sig_handler) == SIG_ERR)
            {
                fprintf(stderr, "Cannot catch signal %d \n", sig_c);
                exit_code = 1;
            }
            break;

        /* -------------------ignore------------------- */
        case 'I':
            if (verbose)
            {
                printf("%s\n", optarg);
                fflush(stdout);
            }
            int sig_i = atoi(optarg);
            if (signal(sig_i, SIG_IGN) == SIG_ERR)
            {
                fprintf(stderr, "Cannot ignore signal %d \n", sig_i);
                exit_code = 1;
            }
            break;

        /* -------------------default------------------- */
        case 'e':
            if (verbose)
            {
                printf("\n");
                fflush(stdout);
            }
            int sig_d = atoi(optarg);
            if (signal(sig_d, SIG_DFL) == SIG_ERR)
            {
                fprintf(stderr, "Cannot catch signal %d \n", sig_d);
            }
            break;
        /* -------------------pause------------------- */
        case 'p':
            if (verbose)
            {
                printf("\n");
                fflush(stdout);
            }
            pause(); //Suspends execution until any signal is caught
            break;
        /* -------------------chdir------------------- */
        case 'h':
	    if (verbose)
	    {
	      printf("%s\n", optarg);
	      fflush(stdout);
	    }
            if (chdir(optarg) == -1)
            {
                fprintf(stderr, "Cannot open directory %s\n", optarg);
                exit_code = 1;
            }
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
