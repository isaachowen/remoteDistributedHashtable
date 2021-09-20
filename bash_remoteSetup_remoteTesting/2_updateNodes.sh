#!/bin/bash

timestamp=$(date +%H%M)

declare -a StringArray=("192.168.2.143" "192.168.2.179" "192.168.2.203" "192.168.2.106" "192.168.2.115")
 
# Iterate the string array using for loop

for x in ${StringArray[@]};
do

	#copy the new listener.cpp to the directory
	echo "hi"
	echo  ${x}
	sshpass -p 'rplonglab' scp ~/Desktop/CSE_403/Version_control/V8_attempt_3/403_listener_V8/listener.cpp pi@${x}:/home/pi/Desktop/403_DRHT
	#compile the listener program
	sshpass -p 'rplonglab' ssh pi@${x} "cd /home/pi/Desktop/403_DRHT ; g++ -pthread -std=c++11 -lboost_system -lboost_date_time -lboost_thread listener.cpp -o listener"
	
	# copy the new talker.cpp to the directory
	sshpass -p 'rplonglab' scp ~/Desktop/CSE_403/Version_control/V8_attempt_3/403_talker_V8/talker.cpp pi@${x}:/home/pi/Desktop/403_DRHT
	# compile the talker program
	sshpass -p 'rplonglab' ssh pi@${x} "cd /home/pi/Desktop/403_DRHT/ ; g++ -pthread -std=c++11 talker.cpp -o talker"

	#copy the IP_local and IP_global files to the directory
	sshpass -p 'rplonglab' scp ~/Desktop/CSE_403/Version_control/V8_attempt_3/403_talker_V8/IP_local.txt pi@${x}:/home/pi/Desktop/403_DRHT
	sshpass -p 'rplonglab' scp ~/Desktop/CSE_403/Version_control/V8_attempt_3/403_talker_V8/IP_global.txt pi@${x}:/home/pi/Desktop/403_DRHT
done

