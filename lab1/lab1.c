#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h> //note: O_RSYNC only works under the Linux environment
#include <unistd.h>
#include <string.h> //for strdup
#include <sys/stat.h>
#include <errno.h>
#include <sys/wait.h>

/* collect process information - initialize in command and retrieve in wait */
struct process_info
{
	char **cmd_name;
	pid_t pid;
}process_info;


int main (int argc, char *argv[])
{
	/* array to store file descriptors */
	int *fd_list = (int *)malloc(25 * sizeof(int));
  int fd_index = 0;
  int flag = 0;

	int close_fd = 0;

	/* array to store process information */
	struct process_info *process_arr = malloc(25 * sizeof(process_info));
	int process_num = 0;

	/* array to store process id */
	pid_t *pid_arr = malloc(25 * sizeof(pid_t));
	int pid_num = 0;

	int opt = 0;
	int verbose = 0;
	int profile = 0;
	int long_index = 0;

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

  while ((opt = getopt_long(argc, argv, "R:W:C:VALcDdEFBSsTrPwO:at:I:e:po", long_options, &long_index)) != -1) {

  	if (verbose){
  		printf("%s ",long_options[long_index].name);
    }
    switch (opt) {
	/* -------------------rdonly------------------- */
    case 'R':
    	if (verbose) printf("%s\n",optarg);
        flag |= O_RDONLY;
    	if ((fd_list[fd_index++] = open(optarg, flag, 0)) == -1){
          fprintf(stderr, "Cannot open file %s with read only.\n", optarg);
          exit(1);
        }

        flag = 0;
    	break;


	/* -------------------wronly------------------- */
    case 'W':
    	if (verbose) printf("%s\n",optarg);
        flag |= O_WRONLY;
    	if ((fd_list[fd_index++] = open(optarg, flag, 0)) == -1){
          fprintf(stderr, "Cannot open file %s with write only.\n", optarg);
          exit(1);
        }
        flag = 0;
    	break;
	/* -------------------command------------------- */
    case 'C':
			optind--;
		  int count = 0;
			char *cmd_name[256];
			cmd_name[count++] = strdup (long_options[long_index].name);

			while (optind < argc) {
				if (strdup(argv[optind])[0] == '-' && strdup(argv[optind])[1] == '-' )  break;
				cmd_name[count++] = strdup(argv[optind++]);
			}
			cmd_name[count] = "\0";

    	int in = atoi(cmd_name[1]);
    	int out = atoi(cmd_name[2]);
    	int err = atoi(cmd_name[3]);
    	char *args[128];
			int index = 0;
			int start = 4;
    	while (start < count){
    		/* strdup returns a pointer to a null-terminated byte string */
    		args[index++] = cmd_name[start++];
    	}
    	args[index] = "\0";

    	if (verbose){
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
			process_num++;

    	if (rc < 0){
    		fprintf(stderr, "fork failed\n");
    		exit(1);
    	}else if (rc == 0){
				dup2(fd_list[in], 0);
				dup2(fd_list[out], 1);
				dup2(fd_list[err], 2);

				int k;
				for (k = 0 ; k < fd_index; k++){
					close (fd_list[k]);
					fd_list[k] = -1;
				}
    		if (execvp (args[0], args) < 0) {
					int l;
					for (l = 0; l < index; l++) {
						printf("%s\n", args[l]);
					}
					fprintf(stderr, "Cannot execute command.\n");
					exit(1);
				}
    	}else {
    		// int rc_wait = wait(NULL);
    		// printf("Exit with code %d\n", rc_wait);
				printf("Parent Exit\n");
    	}
			fflush(stdout);
    	break;
	/* -------------------verbose------------------- */
    case 'V':
    	verbose = 1;
    	break;
    /* -------------------append------------------- */
    case 'A':
        if (verbose) printf("%s\n",optarg);
        flag |= O_APPEND;
        break;
    /* -------------------cloexec------------------- */
    case 'L':
        if (verbose) printf("%s\n",optarg);
        flag |= O_CLOEXEC;
        break;

    /* -------------------creat------------------- */
    case 'c':
        if (verbose) printf("%s\n",optarg);
        flag |= O_CREAT;
        break;

    /* -------------------directory------------------- */
    case 'D':
        if (verbose) printf("%s\n",optarg);
        flag |= O_DIRECTORY;
        break;

    /* -------------------dsync------------------- */
    case 'd':
        if (verbose) printf("%s\n",optarg);
        flag |= O_DSYNC;
        break;

    /* -------------------excl------------------- */
    case 'E':
        if (verbose) printf("%s\n",optarg);
        flag |= O_EXCL;
        break;

    /* -------------------nofollow------------------- */
    case 'F':
        if (verbose) printf("%s\n",optarg);
        flag |= O_NOFOLLOW;
        break;

    /* -------------------nonblock------------------- */
    case 'B':
        if (verbose) printf("%s\n",optarg);
        flag |= O_NONBLOCK;
        break;

    /* -------------------rsync------------------- */
    case 'S':
        if (verbose) printf("%s\n",optarg);
        flag |= O_RSYNC;
        break;

    /* -------------------sync------------------- */
    case 's':
        if (verbose) printf("%s\n",optarg);
        flag |= O_SYNC;
        break;

    /* -------------------trunc------------------- */
    case 'T':
        if (verbose) printf("%s\n",optarg);
        flag |= O_TRUNC;
        break;

    /* -------------------rdwr------------------- */
    case 'r':
        if (verbose) printf("%s\n",optarg);
        flag |= O_RDWR;
        if ((fd_list[fd_index++] = open(optarg, flag, 0)) == -1){
          fprintf(stderr, "RSYNC file %s failed.\n", optarg);
          exit(1);
        }
        flag = 0;
        break;

    /* -------------------pipe------------------- */

    /* -------------------wait------------------- */
		case 'w':
			if (verbose) printf("true\n");

			int i, status;
			pid_t rc_pid;
			for (i = 0; i < pid_num; i++){
				rc_pid = wait(&status); /* return the pid of finished process */

				if (rc_pid > 0)
				{
				  if (WIFEXITED(status)) {
				    printf("Child exited with RC=%d\n",WEXITSTATUS(status));
				  }else if (WIFSIGNALED(status)) {
				    printf("Child exited via signal %d\n",WTERMSIG(status));
				  }

					/* Find the command name according to the return pid */
					int j;
					for (j = 0; i < process_num; j++) {
						if (rc_pid == process_arr[j].pid) {
							int c = 0;
							while (process_arr[j].cmd_name[c]){
								printf("%s\n", process_arr[j].cmd_name[c++]);
							}
							printf("\n");
							break;
						}
					}
			}
		}
			break;

    /* -------------------close------------------- */
		case 'O':
			if (verbose) printf("%s\n",optarg);
			close_fd = atoi(argv[optind++]);
			close(fd_list[close_fd]);
			fd_list[close_fd] = -1;
			break;

    /* -------------------abort------------------- */
		case 'a':
			if (verbose) printf("%s\n",optarg);
			abort();
			break;

    /* -------------------catch------------------- */

    /* -------------------ignore------------------- */

    /* -------------------default------------------- */

    /* -------------------pause------------------- */
		case 'p':
			if (verbose) printf("%s\n",optarg);
			pause();
			break;

    /* -------------------profile------------------- */
		case 'o':
			if (verbose) printf("%s\n",optarg);
			profile = 1;
			break;

    }
   }

   free(fd_list);
	 free(pid_arr);
}
