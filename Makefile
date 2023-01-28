CC=gcc
CCOPTS=--std=gnu99 -g -Wall 
AR=ar


BINS=PseudoFAT

OBJS=fat.c

HEADERS=fat.h utils.h linked_list.h

LIBS=

%.o:	%.c $(HEADERS)
	$(CC) $(CCOPTS) -c -o $@  $<

phony: clean all


all:	$(BINS) $(LIBS)

fat: fat.c $(OBJS)
	$(CC) $(CCOPTS) -o $@ $^

clean:
	rm -rf *.o *~ $(LIBS) $(BINS)