#!/bin/bash
nohup sshpass -p "rplonglab" ssh pi@192.168.2.143 "killall listener" & nohup sshpass -p "rplonglab" ssh pi@192.168.2.179 "killall listener" & nohup sshpass -p "rplonglab" ssh pi@192.168.2.203 "killall listener" & nohup sshpass -p "rplonglab" ssh pi@192.168.2.106 "killall listener" & nohup sshpass -p "rplonglab" ssh pi@192.168.2.115 "killall listener"

nohup sshpass -p "rplonglab" ssh pi@192.168.2.143 "killall talker" & nohup sshpass -p "rplonglab" ssh pi@192.168.2.179 "killall talker" & nohup sshpass -p "rplonglab" ssh pi@192.168.2.203 "killall talker" & nohup sshpass -p "rplonglab" ssh pi@192.168.2.106 "killall talker" & nohup sshpass -p "rplonglab" ssh pi@192.168.2.115 "killall talker"
