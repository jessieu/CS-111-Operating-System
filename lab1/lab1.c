#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>


int main (int argc, char *argv[])
{
	int fd;
	int opt = 0;
	int index;
	int verbose = 0;
	int long_index = 0;
	static struct option long_options[] = {
            {"rdonly",     required_argument, 0,  'R' },
            {"wronly",     required_argument, 0,  'W' },
            {"command",    required_argument, 0,  'C' },
            {"verbose",    no_argument,       0,  'V' },
            {0,           0,                 0,   0  }
  };

  while ((opt = getopt_long(argc, argv, "R:W:C:V", long_options, &long_index)) != -1) {

  	if (verbose){
  		printf("%s ",long_options[long_index].name);
    	
    }
    switch (opt) {
	//--rdonly
    case 'R':
    	if (verbose) printf("%s\n",optarg);
    	if ((fd = open(optarg, O_RDONLY, 0)) == -1){
          fprintf(stderr, "Cannot open file %s with read only.\n", optarg);
          exit(1);
        }
    	break;


	//--wronly
    case 'W':
    	if (verbose) printf("%s\n",optarg);
    	if ((fd = open(optarg, O_WRONLY, 0)) == -1){
          fprintf(stderr, "Cannot open file %s with write only.\n", optarg);
          exit(1);
        }
    	break;
	//--command
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
	//--verbose	
    case 'V':
    	verbose = 1;
    	break;
    }
   }	
}
