#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

int main (int argc, char *argv[])
{
  /* array to store file descriptors */
  int *fd_list = (int *)malloc(100 * sizeof(int));
  int fd_index = 0;

  /* for verbose */
  int verbose = 0;

  /* for parsing option */
  int long_index = 0;
  int opt = 0;

  /* exit code */
  int exit_code = 0;


  static struct option long_options[] = {
    /* Lab 1A */
    {"rdonly",     required_argument, 0,  'R' },
    {"wronly",     required_argument, 0,  'W' },
    {"command",    required_argument, 0,  'C' },
    {"verbose",    no_argument,       0,  'V' },
    {0,            0,                 0,   0  }
  };

  while ((opt = getopt_long(argc, argv, "", long_options, &long_index)) != -1) {

    if (verbose) {
      printf("--%s ",long_options[long_index].name);
    }
    switch (opt) {
      /* -------------------rdonly------------------- */
    case 'R':
      if (verbose) printf("%s\n",optarg);
      if ((fd_list[fd_index++] = open(optarg,O_RDONLY)) == -1) {
	fprintf(stderr, "Cannot open file %s with read only.\n", optarg);
	exit_code = 1;
      }
      fd_list[fd_index] = -1; /* Indicate it's the end of fd_list */
      break;

      /* -------------------wronly------------------- */
    case 'W':
      if (verbose) printf("%s\n",optarg);
      if ((fd_list[fd_index++] = open(optarg, O_WRONLY)) == -1) {
	fprintf(stderr, "Cannot open file %s with write only.\n", optarg);
	exit_code = 1;
      }
      fd_list[fd_index] = -1;
      break;

      /* -------------------command------------------- */
    case 'C':
      ;
      /* Get the full command name */
      optind--;
      int count = 0;
      char *cmd_name[256];
      cmd_name[count++] = strdup(long_options[long_index].name);

      while (optind < argc) {
	if (argv[optind][0] == '-' && argv[optind][1] == '-') {
	  break;
	}
	cmd_name[count++] = argv[optind++];
      }
      cmd_name[count] = NULL;
     

      /* Store the file descriptors for input, output, and error */
      int in = atoi(cmd_name[1]);
      int out = atoi(cmd_name[2]);
      int err = atoi(cmd_name[3]);

      /* Get the arguments from command name */
      char *args[128];
      int index = 0;
      int start = 4;

      while (start < count) {
	/* strdup returns a pointer to a null-terminated byte string */
	args[index++] = strdup(cmd_name[start++]);
      }
      args[index] = NULL;

      if (verbose) {
	printf("%d %d %d ", in, out, err);
	int j;
	for (j = 0; j < index; j++){
	  printf("%s ",args[j]);
	}
	printf("\n");
      }

      /* Check file descriptor validation */
      if (in >= fd_index || out >= fd_index || err >= fd_index){
	exit_code = 1;
	fprintf(stderr, "Invalid file descriptor.\n");
	break;
      }

      if (fd_list[in] < 0 || fd_list[out] < 0 || fd_list[err] < 0){
	exit_code = 1;
	fprintf(stderr, "Invalid file descriptor.\n");
	break;
      }
      
      /* create child process */
      int rc = fork();

      if (rc < 0) {
	exit_code = 1;
	fprintf(stderr, "fork failed\n");
      }else if (rc == 0) {
	dup2(fd_list[in], 0);
	close(fd_list[in]); /* close unused file descriptor */

	dup2(fd_list[out], 1);
	close(fd_list[out]);

	dup2(fd_list[err], 2);
	close(fd_list[err]);
	
	/* Execute the command */
	if (execvp (args[0], args) < 0) {
	  fprintf(stderr, "Cannot execute command.\n");
	  exit_code = 1;
	}
      }else {
	/* Parent return here */
	break;
      }
      break;
      /* -------------------verbose------------------- */
    case 'V':
      verbose = 1;
      break;
    case '?':
      fprintf(stderr, "Invalid option\n");
      exit_code = 1;
    default:
      break;
    }
  }

  free(fd_list);
  exit(exit_code);
}
