#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#define BUFFER_SIZE 1024

void signal_handler (int sig)
{
  if (sig == SIGSEGV){
    fprintf(stderr, "Segmentation Fault Caught\n");
    exit(4);
  }
}

void segfault_creator (void)
{
  char *str = NULL;
  *str = 'a';
  return;
}

int main (int argc, char *argv[])
{
  int fd;
  char buffer[BUFFER_SIZE];

  int opt = 0;
  static struct option long_options[] = {
    {"input",     required_argument, 0,  'i' },
    {"output",    required_argument, 0,  'o' },
    {"segfault",  no_argument,       0,  's' },
    {"catch",     no_argument,       0,  'c' },
    {"dump-core", no_argument,       0,  'd' },
    {0,           0,                 0,   0  }
  };

  static char usage[] = "usage: %s [--input=filename] [--output=filename]\
                       [--segfault] [--catch] [dump-core]\n";

  int long_index = 0;

  while ((opt = getopt_long(argc, argv, "i:o:scd", long_options, &long_index)) != -1) {
    switch (opt) {
    case 'i':
      if ((fd = open(optarg, O_RDONLY, 0)) == -1){
	fprintf(stderr, "Error on argument --input. Unable to open input file %s.\n", optarg);
	exit(2);
      }
      dup2(fd, 0); /* direct stdin to input file */
      close(fd);
      break;
    case 'o':
      if ((fd = open (optarg, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR | S_IWUSR)) == -1) {
	fprintf(stderr, "Error on argument --output. Unable to open output file %s.\n", optarg);
	exit(3);
      }
      dup2 (fd,1);
      close(fd);
      break;
    case 's':
      segfault_creator();
      break;
    case 'c': /* catch sigsegv and handle immediately */
      signal (SIGSEGV, signal_handler);
      break;
    case 'd': 
      /* Revert to default handler, will exit on next SIGSEGV with core dump */
      signal(SIGSEGV, SIG_DFL);
      break;
    default:
      fprintf(stderr, "Unrecoginzed argument.\n%s\n", strerror(errno));
      printf(usage);
      exit(1);
    }
  }
  
  /* Read input and then write to output */
  int n;
  while ((n = read(0, buffer, BUFFER_SIZE)) > 0) {
    if (write (1, buffer, n) < 0) {
      fprintf(stderr, "Cannot write to file: %s\n", strerror (errno));
      exit (3);
    }
  }
  if (n < 0) {
    fprintf (stderr, "Error on reading file: %s\n", strerror (errno));
    exit(2);
  }

  exit(0);
}
