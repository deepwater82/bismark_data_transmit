

all: ptf.c
	gcc -o ptf ptf.c -Wall -lcurl

clean:
	rm ptf
