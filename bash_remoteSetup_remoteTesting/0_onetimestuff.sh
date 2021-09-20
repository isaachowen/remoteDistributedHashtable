#!/bin/bash

timestamp=$(date +%H%M)

# list of remote IP addresses
declare -a StringArray=("192.168.2.143" "192.168.2.179" "192.168.2.203" "192.168.2.106" "192.168.2.115")


for x in ${StringArray[@]};
do
	echo ${x}
	# sshpass -p 'rplonglab' ssh pi@${x} "sudo apt-get update ; sudo apt-get upgrade ; sudo apt-get install libboost-all-dev ; mkdir /home/pi/Desktop/403_DRHT"
	sshpass -p 'rplonglab' ssh pi@${x} "mkdir /home/pi/Desktop/403_DRHT/test_results"
done
