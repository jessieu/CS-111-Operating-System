#! /bin/bash

threads=(1 2 4 8 12)
none_iterations=(100 1000 10000 100000)
yield_iterations=(10 20 40 80 100 1000 10000 100000)
sync_opt=('m' 's' 'c')

if [ -e lab2_add.csv ] ; then
    rm lab2_add.csv
fi

touch lab2_add.csv

# No synchronization & no yield
for i in "${threads[@]}"
do 
    for j in "${none_iterations[@]}"
    do
	./lab2_add --threads=$i --iterations=$j >> lab2_add.csv
    done
done

# No synchronization & with yield
for i in "${threads[@]}"
do 
    for j in "${yield_iterations[@]}"
    do
	./lab2_add --threads=$i --iterations=$j --yield >> lab2_add.csv
    done
done

# With synchronizaiton & no yield
for i in "${threads[@]}"
do
    for j in "${none_iterations[@]}"
    do
	for k in "${sync_opt[@]}"
	do
	    ./lab2_add --threads=$i --iterations=$j --sync=$k >> lab2_add.csv
	done
    done
done

# With synchronizaiton & yield                                                   
for i in "${threads[@]}"
do
    for j in "${none_iterations[@]}"
    do
	for k in "${sync_opt[@]}"
	do
            ./lab2_add --threads=$i --iterations=$j --sync=$k --yield >> lab2_add.csv
        done
    done
done

if [ -e lab2_list.csv ] ; then
    rm lab2_list.csv
fi

touch lab2_list.csv


# For lab2_list
#
#
one_thread_iteration=(10 100 1000 10000 20000)
no_yield_iteration=(1 10 100 1000)
yield_iter=(1 2 4 8 16 32)
sync_threads=(1 2 4 8 12 16 24) 
yield_opt=("i" "d" "l" "id" "il" "dl" "idl")
sync_list=('m' 's')

# Single thread with no synchronization & no yield                                  
for i in "${one_thread_iteration[@]}"
do
    ./lab2_list --threads=1 --iterations=$i >> lab2_list.csv
done

# No synchronization & No yield
for i in "${threads[@]}"
do
    for j in "${no_yield_iteration[@]}"
    do
	./lab2_list --threads=$i --iterations=$j >> lab2_list.csv
    done
done

# No synchronization & with yield combination
for i in "${threads[@]}"
do
    for j in "${yield_iter[@]}"
    do
	for k in "${yield_opt[@]}"
	do
	    #echo $k >> lab2_list.csv
	    ./lab2_list --threads=$i --iterations=$j --yield=$k >> lab2_list.csv
	done
    done
done

# Yield with synchronization
for j in "${sync_list[@]}"
do
    for k in "${yield_opt[@]}"
    do
        ./lab2_list --threads=12 --iterations=32 --yield=$k --sync=$j >> lab2_list.csv
    done
done




# No yield with synchronization 
for i in "${sync_threads[@]}"
do
    for j in "${sync_list[@]}"
    do
	./lab2_list --threads=$i --iterations=1000 --sync=$j >> lab2_list.csv
    done
done
