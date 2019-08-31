OPTIMIZE = -O2
SOURCE = simpsh.c
CC = gcc
OUTPUT = simpsh

default:
	$(CC) $(OPTIMIZE) $(SOURCE) -o $(OUTPUT)

clean:
	rm -f simpsh *.tar.gz

check:
	@chmod +x test.sh; ./test.sh

fix:
	$(CC) $(OPTIMIZE) -g $(SOURCE) -o $(OUTPUT)

dist: default
	tar -czf lab1-904982644.tar.gz simpsh.c Makefile test.sh README report.pdf
