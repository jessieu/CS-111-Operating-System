/* Reference: https://linux.die.net/man/3/getopt_long */
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
 #include <errno.h>

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
  char *ptr = NULL;
  *ptr = 1;
  return;
}

int main (int argc, char *argv[])
{
  int fd;
  int seg_flg;

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
          fprintf(stderr, "Cannot open file %s.\n", optarg);
          exit(1);
        }
        dup2(fd, 0); /* direct stdin to specific file descriptor */
        break;
      case 'o':
        if ((fd = open (optarg, O_WRONLY|O_CREAT|O_TRUNC, 0)) == -1) {
          fprintf(stderr, "Cannot create/truncate file %s.\n", optarg);
          exit(2);
        }
        dup2 (fd,1);
        // if ((fd = open(optarg, O_RDWR, 0)) == -1){
        //   if ((fd = create (optarg, PERMS)) == -1) {
        //     fprintf(stderr, "Cannot create file %s.\n", optarg);
        //     exit(2);
        //   }
        //   dup2(fd, 1);
        // }else {
        //   if (ftruncate (fd, 0) == -1){
        //     fprintf(stderr, "Cannot truncate file %s.\n", optarg);
        //     exit(2);
        //   }
        //   dup2(fd, 1);
        // }
        break;
      case 's':
        //memset(NULL, 1, 1);
        //raise(SIGSEGV);
        seg_flg = 1;
        break;
      case 'c':
        signal (SIGSEGV, signal_handler);
        break;
      case 'd': /* record the state of working memory when crashed */
        abort();
        break;
      default:
        fprintf(stderr, "Cannot identify option.\n%s\n", usage);
        exit(3);
    }

    if (seg_flg) segfault_creator();
  }
  int n;
  while ((n = read(0, buffer, BUFFER_SIZE)) > 0) {
    if (write (1, buffer, n) < 0) {
      fprintf(stderr, "Cannot write to file: %s\n", strerror (errno));
      exit (3);
    }
  }

  close (0);
  close (1);
  return 0;
}
