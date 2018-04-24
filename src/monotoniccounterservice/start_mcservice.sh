#!/bin/bash


ret=0
while [ $ret -ne 1 ]; do
	./stop_mcservice.sh &>/dev/null
	sleep 1s

	netstat -tnlp 2>/dev/null | grep mcserver &>/dev/null
	ret=$?
done

./mcserver 8000 &
