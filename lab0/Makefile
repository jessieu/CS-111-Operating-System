.SILENT:

default: lab0.c
	gcc -g -Wall -Wextra lab0.c -o lab0
check:default InToOut UnrecognizedArg BadInputTest BadOutputTest CatchTest DumpCoreTest DumpBfCatchTest 
clean:
	rm -f *.o lab0 *.txt *.gz
dist: default
	tar -czvf lab0-005119471.tar.gz lab0.c README Makefile backtrace.png breakpoint.png

InToOut:
	echo "Hello World" > input_test.txt; \
	./lab0 --input=input_test.txt --output=output_test.txt > /dev/null 2>&1; \
	cmp input_test.txt output_test.txt; \
	if [[ $$? -ne 0 ]];then \
		echo "Error: did not copy input to output successfully."; \
	else \
		echo "Pass copy from input file to output file test."; \
	fi; \
	rm -rf input_test.txt output_test.txt;

UnrecognizedArg:
	./lab0 --version > /dev/null 2>&1; \
	if [[ $$? -ne 1 ]];then \
		echo "Error: did not detect unrecognized argument successfully."; \
	else \
		echo "Pass unrecognized argument test."; \
	fi; \

BadInputTest:
	./lab0 --input=NotExist.txt > /dev/null 2>&1; \
	if [[ $$? -ne 2 ]];then\
		echo "Error: did not handle file not exist properly."; \
	else \
		echo "Pass BadInputTest."; \
	fi; \

BadOutputTest:
	echo "I am not allowed to write." > out.txt; \
	chmod u-w out.txt; 
	./lab0 --output=out.txt > /dev/null 2>&1; \
	if [[ $$? -ne 3 ]]; then \
		echo "Error: did not handle failed to write properly."; \
	else \
		echo "Pass BadOutputTest"; \
	fi; \
	rm -rf out.txt;

CatchTest:
	./lab0 --catch --segfault > /dev/null 2>&1;\
	if [[ $$? -ne 4 ]]; then\
		echo "Error: did not catch segfault successfully."; \
	else \
		echo "Pass CathTest"; \
	fi; \

DumpCoreTest:
	./lab0 --dump-core --segfault > /dev/null 2>&1;\
	if [[ $$? -ne 139 ]]; then \
		echo "Error: did not dump core successfully."; \
	else \
		echo "Pass DumpCoreTest"; \
	fi;

DumpBfCatchTest:
	./lab0 --dump-core --catch --segfault > /dev/null 2>&1;\
	if [[ $$? -ne 4 ]]; then\
		echo "Error: catch did not replace dump-core."; \
	else \
		echo "Pass DumpBfCatchTest"; \
	fi;
