#!/bin/bash

# NAME: Luke Jung
# EMAIL: lukejung@ucla.edu
# ID: 904982644

default:
	gcc -Wall -Wextra -std=c99 -g -lmraa -lm lab4b.c -o lab4b 

check: default
	./lab4b --log=log.txt <<-EOF
	START
	SCALE=C
	OFF
	EOF

	if [ $? -ne 0 ]
	then
		echo "Failed: Did not exit 0"
	else
		echo "Success"
	fi

	for c in START SCALE=C OFF SHUTDOWN
		do
			grep "$c" log.txt > /dev/null
			if [ $? -ne 0 ]
			then
				echo "log $c didn't show up"
			else
				echo "log $c succeeded"
			fi
		done

	rm -f log.txt


	./lab4b --log=log.txt <<-EOF
	START
	SCALE=F
	OFF
	EOF

	if [ $? -ne 0 ]
	then
		echo "Failed: Did not exit 0"
	else
		echo "Success"
	fi

	for c in START SCALE=F OFF SHUTDOWN
		do
			grep "$c" log.txt > /dev/null
			if [ $? -ne 0 ]
			then
				echo "log $c didn't show up"
			else
				echo "log $c succeeded"
			fi
		done
	rm -f log.txt

	./lab4b --log=log.txt <<-EOF
	START
	LOG
	PERIOD=2
	OFF
	EOF

	if [ $? -ne 0 ]
	then
		echo "Failed: Did not exit 0"
	else
		echo "Success"
	fi

	for c in START LOG PERIOD=2 OFF SHUTDOWN
		do
			grep "$c" log.txt > /dev/null
			if [ $? -ne 0 ]
			then
				echo "log $c didn't show up"
			else
				echo "log $c succeeded"
			fi
		done
	rm -f log.txt

clean: 
	rm -f lab4b-904982644.tar.gz lab4b

dist: default
	tar -czvf lab4b-904982644.tar.gz lab4b.c Makefile README