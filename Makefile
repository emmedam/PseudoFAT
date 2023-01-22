CC=gcc
CCOPTS=--std=gnu99 -g -Wall 
AR=ar


BINS=main

OBJS=fat.c

HEADERS=fat.h

LIBS=

%.o:	%.c $(HEADERS)
	$(CC) $(CCOPTS) -c -o $@  $<

phony: clean all


all:	$(BINS) $(LIBS)

fat: fat.c $(OBJS)
	$(CC) $(CCOPTS) -o $@ $^

clean:
	rm -rf *.o *~ $(LIBS) $(BINS)