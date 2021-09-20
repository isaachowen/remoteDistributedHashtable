#!/bin/bash

declare -a StringArray=("192.168.2.143" "192.168.2.179" "192.168.2.203" "192.168.2.106" "192.168.2.115")
 
# Iterate the string array using for loop
time=$1

for x in ${StringArray[@]};
do 
	for i in `seq 1 8`
	do
		tail -6 ./experiment_results/${time}${x}_data_out/${i}.txt | awk 'FNR == 5 {print $2}' >> assembled_results/${time}_avgThruput.txt
	done
	echo "one loop done"
done

# rm *
# echo "remove successful"
