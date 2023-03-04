CC      = gcc
CFLAGS  = -Wall
.PHONY: all
all: oss worker
oss: oss.o oss.h
	$(CC) -o $@ $^
worker: worker.o oss.h
	$(CC) -o $@ $^
%.o: %.c
	$(CC) -c $(CFLAGS) $*.c

clean: 
	rm *.o oss worker