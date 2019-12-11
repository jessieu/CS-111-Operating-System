default: simpsh.c
	gcc -g -Wall -Wextra simpsh.c -o simpsh
check:default
	chmod +x test.sh
	./test.sh
clean:
	rm -f *.o simpsh  *.gz
dist: default
	tar -czvf lab1-005119471.tar.gz simpsh.c README Makefile test.sh Report.pdf
