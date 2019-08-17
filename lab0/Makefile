all: lab0.c
		 	gcc -g -Wall -Wextra -std=c99 -o lab0 lab0.c

check:
			./P0_check.sh 005119471

clean:
			rm -f lab0

dist:
			tar -cvzf lab0-005119471.tar.gz ./Makefile ./lab0.c ./backtrace.png ./breakpoint.png ./README
