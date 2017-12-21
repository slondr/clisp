.PHONY: clean

CC ?= gcc
CFLAGS += -Wall -pedantic -O2 -g

all: clean clisp

clisp: clisp.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	@rm -fv clisp