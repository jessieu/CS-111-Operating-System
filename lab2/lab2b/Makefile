default: 
	gcc -g -Wall -Wextra -pthread lab2_list.c SortedList.c -o lab2_list
tests:default
	chmod +x test.sh 
	./test.sh
profile:default		
	rm -f ./raw.gperf;
	LD_PRELOAD=/lib64/libprofiler.so CPUPROFILE=./raw.gperf ./lab2_list --threads=12 --iterations=1000 --sync=s
	/bin/pprof   --text ./lab2_list ./raw.gperf > profile.out
	/bin/pprof   --list=worker ./lab2_list ./raw.gperf >> profile.out
	rm -f ./raw.gperf
graphs:tests
	chmod +x lab2_list.gp
	gnuplot ./lab2_list.gp
clean:
	rm -f *.o lab2_list *.gz
dist: graphs profile
	tar -czvf lab2b-005119471.tar.gz Makefile lab2_list.c SortedList.c SortedList.h README test.sh profile.out lab2b_list.csv *.png lab2_list.gp
