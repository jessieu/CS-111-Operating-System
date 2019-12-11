#! /bin/bash

if [ -e lab2b_list.csv ] ; then
    rm lab2b_list.csv
fi

touch lab2b_list.csv


# For lab2_list
#
#threads=

# Synchronized list operations
for i in 1 2 4 8 12 16 24
do
    ./lab2_list --threads=$i --iterations=1000 --sync=m >> lab2b_list.csv
    ./lab2_list --threads=$i --iterations=1000 --sync=s >> lab2b_list.csv
done

# Unsynchronized yield
for i in 1 4 8 12 16
do
    for j in 1 2 4 8 16
    do 
	./lab2_list --threads=$i --iterations=$j --lists=4 --yield=id >> lab2b_list.csv
    done
done

# Synchronized yield
for i in 1 4 8 12 16
do
    for j in 10 20 40 80
    do
        ./lab2_list --threads=$i --iterations=$j --lists=4 --yield=id --sync=m >> lab2b_list.csv
	./lab2_list --threads=$i --iterations=$j --lists=4 --yield=id --sync=s >> lab2b_list.csv
	
    done
done

# Synchronized without yield                                                   
for i in 1 4 8 12
do
    for j in 1 4 8 16
    do
	./lab2_list --threads=$i --iterations=1000 --lists=$j --sync=m >> lab2b_list.csv
	./lab2_list --threads=$i --iterations=1000 --lists=$j --sync=s >> lab2b_list.csv

    done
done

