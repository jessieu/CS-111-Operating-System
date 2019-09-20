build:
	gcc -Wall -Wextra -pthread lab2_add.c -o lab2_add
	gcc -Wall -Wextra -pthread lab2_list.c -o lab2_list

tests: clean build
	chmod +x test.py
	python ./test.py

graphs: tests
	gnuplot lab2_add.gp
	gnuplot lab2_list.gp

dist: graphs
	tar -zvcf lab2a-005119471.tar.gz Makefile README *.c *.h *.png *.csv *.gp


clean:
	rm -rd lab2_add lab2_list lab2a-005119471.tar.gz
