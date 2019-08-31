# NAME: Luke Jung
# EMAIL: lukejung@ucla.edu
# ID: 904982644

default: 
	gcc -std=c99 -Wall -Wextra -lm -lmraa -g lab4c_tcp.c -o lab4c_tcp
	gcc -std=c99 -Wall -Wextra -lm -lmraa -g -lssl -lcrypto lab4c_tls.c -o lab4c_tls

clean:
	rm -f lab4c-904982644.tar.gz lab4c_tcp lab4c_tls

dist: default
	tar -czvf lab4c-904982644.tar.gz lab4c_tcp.c lab4c_tls.c Makefile README