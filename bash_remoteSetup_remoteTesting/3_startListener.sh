#!/bin/bash

nohup sshpass -p "rplonglab" ssh pi@192.168.2.143 'cd ./Desktop/403_DRHT ; ./listener -k 1' &
nohup sshpass -p "rplonglab" ssh pi@192.168.2.203 'cd ./Desktop/403_DRHT ; ./listener -k 1' &
nohup sshpass -p "rplonglab" ssh pi@192.168.2.106 'cd ./Desktop/403_DRHT ; ./listener -k 1' &
nohup sshpass -p "rplonglab" ssh pi@192.168.2.115 'cd ./Desktop/403_DRHT ; ./listener -k 1' &
nohup sshpass ssh pi@192.168.2.179 "cd Desktop/403_DRHT ; ./listener -k 1" &







