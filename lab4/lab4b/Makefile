default:
	gcc -Wall -Wextra -g -lmraa -lm lab4b.c -o lab4b

check:default
	chmod +x test.sh
	./test.sh

dist:
	tar -cvzf lab4b-005119471.tar.gz lab4b.c test.sh README Makefile

clean:
	rm -f lab4b-005119471.tar.gz lab4b 
