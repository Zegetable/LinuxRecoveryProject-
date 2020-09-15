CC=gcc
CC_FLAGS= -Wall -lm

all: build

build:
	$(CC) $(CC_FLAGS) -o recovery src/recovery.c src/functions.c
clean:
	rm -f ./recovery