default: 
	gcc -g -Wall -Wextra -pthread lab2_add.c -o lab2_add
	gcc -g -Wall -Wextra -pthread lab2_list.c SortedList.c -o lab2_list
tests:default
	chmod +x test.sh 
	./test.sh
graphs:tests
	chmod +x lab2_add.gp
	gnuplot ./lab2_add.gp
	chmod +x lab2_list.gp
	gnuplot ./lab2_list.gp
clean:
	rm -f *.o lab2_add lab2_list *.gz
dist: graphs
	tar -czvf lab2a-005119471.tar.gz Makefile lab2_add.c lab2_list.c SortedList.c SortedList.h README test.sh *.csv *.png *.gp
