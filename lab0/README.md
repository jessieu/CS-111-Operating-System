getopt

- What need to add into the function?
  - `extern char *optarg`
    - take the parsing option name as parameter
  - `extern int optind`
    - current index in the main function's argument list
    - used to find argument after all the option processing is done
- 3 parameters in getopt function
  - argc
  - argv
  - Any option that requires a parameter alongside it is suffixed by a colon (`:`)

