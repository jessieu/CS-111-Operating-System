default:
	gcc -Wall -Wextra -g -lmraa -lm lab4c_tcp.c -o lab4c_tcp
	gcc -Wall -Wextra -g -lmraa -lm -lssl -lcrypto -Wno-deprecated-declarations lab4c_tls.c -o lab4c_tls

dist:
	tar -cvzf lab4c-005119471.tar.gz lab4c_tcp.c lab4c_tls.c README Makefile

clean:
	rm -f lab4c-005119471.tar.gz lab4c_tcp lab4c_tls 
