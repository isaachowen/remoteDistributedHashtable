#!/bin/bash

timestamp=$(date +%H%M)

declare -a StringArray=("192.168.2.143" "192.168.2.179" "192.168.2.203" "192.168.2.106" "192.168.2.115")
 
# Iterate the string array using for loop

for x in ${StringArray[@]};
do
	echo "hello"
	sshpass -p 'rplonglab' scp -r pi@${x}:/home/pi/Desktop/403_DRHT/test_results ~/Desktop/CSE_403/Bash_scripting_testing/experiment_results/data_out
	mv ~/Desktop/CSE_403/Bash_scripting_testing/experiment_results/data_out ~/Desktop/CSE_403/Bash_scripting_testing/experiment_results/${timestamp}_${x}_data_out
done

