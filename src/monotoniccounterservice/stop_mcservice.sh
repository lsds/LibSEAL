#!/bin/bash

PID=0
while [[ ! -z $PID ]]; do
	PID=$(ps aux | grep '/[m]cserver' | awk '{print $2}') 
	if [[ -z $PID ]]; then
		echo "No more mcserver processes"
		break
	fi

	pkill mcserver
	for pid in $PID; do
		echo "gonna kill $pid"
		kill $pid
		kill -9 $pid
	done
	sleep 1s;
done

