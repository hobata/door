#!/bin/sh

while true
do
nc -l -p 12346
aplay -D hw:0,0 /home/volumio/door_svr/Doorbell-Melody01-1.wav
aplay -D hw:0,0 /home/volumio/door_svr/Doorbell-Melody01-1.wav
aplay -D hw:0,0 /home/volumio/door_svr/Doorbell-Melody01-1.wav
done
