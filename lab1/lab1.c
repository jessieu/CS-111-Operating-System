#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
// #include <sys/wait.h>


int main (int argc, char *argv[])
{
	int *fd_list = (int *)malloc(25 * sizeof(int));
    int fd_index = 0;
    int flag = 0;

	int opt = 0;
	int index;
	int verbose = 0;
    int create = 0;
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
    	int i = atoi(argv[optind++]);
    	int o = atoi(argv[optind++]);
    	int e = atoi(argv[optind++]);
    	//char *cmd = strdup(argv[++optind]);
    	int count = 0;
    	char *args[128];
    	while (optind < argc){
    		if (strdup(argv[optind])[0] == '-') break;
    		/* strdup returns a pointer to a null-terminated byte string */
    		args[count++] = strdup(argv[optind++]);
    	}
    	args[count] = "\0";

    	if (verbose){
    		printf("%d %d %d ", i, o, e);
    		int j;
    		for (j = 0; j < count; j++)
    			printf("%s ",args[j]);
    	}
    	printf("\n");

    	int rc = fork();
    	if (rc < 0){
    		fprintf(stderr, "fork failed\n");
    		exit(1);
    	}else if (rc == 0){
    		execvp (args[0], args);
    	}else {
    		int rc_wait = wait(NULL);
    		printf("Exit with code %d\n", rc_wait);
    	}
    	
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
        create = 1;
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

    /* -------------------close------------------- */

    /* -------------------abort------------------- */

    /* -------------------catch------------------- */

    /* -------------------ignore------------------- */

    /* -------------------deafult------------------- */

    /* -------------------pause------------------- */

    /* -------------------profile------------------- */  
            
    }
   }

   free(fd_list);	
}
