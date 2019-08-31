#!/bin/sh
echo "Smoke tests for Lab1a"
echo "Test 1: Test --rdonly"
echo "luke" > rdinput.txt
./simpsh --rdonly rdinput.txt
if [[ $? -ne 0 ]]
then
	echo "rdonly failed, exit code: $$"
	exit 1
fi
rm -f rdinput.txt
echo "Test 2: Test --wronly"
echo "luke" > wrinput.txt
./simpsh --wronly wrinput.txt
if [[ $? -ne 0 ]]
then
	echo "wronly failed, exit code: $$"
	exit 1
fi
rm -f wrinput.txt
echo "Test 3: Test --command"
echo "luke" > a.txt
cat a.txt > b.txt
cat a.txt > c.txt
./simpsh \
  --rdonly a.txt \
  --rdonly b.txt \
  --rdonly c.txt \
  --command 0 1 2 sleep 61
  
if [[ $? -ne 0 ]]
then
	echo "command failed, exit code: $$"
	exit 1
fi

echo "Test 4: Test --verbose"
echo "luke" > wrinput.txt
./simpsh --verbose --wronly wrinput.txt
if [[ $? -ne 0 ]]
then
  echo "wronly failed, exit code: $$"
  exit 1
fi