#!/bin/bash
nohup sshpass -p "rplonglab" ssh pi@192.168.2.143 "rm ~/Desktop/403_DRHT/test_results/*"
 
nohup sshpass -p "rplonglab" ssh pi@192.168.2.179 "rm ~/Desktop/403_DRHT/test_results/*"

nohup sshpass -p "rplonglab" ssh pi@192.168.2.203 "rm ~/Desktop/403_DRHT/test_results/*"

nohup sshpass -p "rplonglab" ssh pi@192.168.2.106 "rm ~/Desktop/403_DRHT/test_results/*"

nohup sshpass -p "rplonglab" ssh pi@192.168.2.115 "rm ~/Desktop/403_DRHT/test_results/*"

# nohup sshpass -p "rplonglab" ssh pi@192.168.1.105 "rm ~/Desktop/data_out/*"

# nohup sshpass -p "rplonglab" ssh pi@192.168.1.109 "rm ~/Desktop/data_out/*"

# nohup sshpass -p "rplonglab" ssh pi@192.168.1.139 "rm ~/Desktop/data_out/*"

# nohup sshpass -p "rplonglab" ssh pi@192.168.1.121 "rm ~/Desktop/data_out/*"

# nohup sshpass -p "rplonglab" ssh pi@192.168.1.105 "iperf -s > listener_result.txt"

# nohup sshpass -p "rplonglab" ssh pi@192.168.1.109 "iperf -s > listener_result.txt"

# remember that iperf -s is never killed -- you may need to be able to get back in here and kill the thing

# nohup sshpass -p "rplonglab" ssh -o StrictHostKeyChecking=no pi@192.168.1.109 "iperf -c 10.0.0.16 -t 20 -i 2"
