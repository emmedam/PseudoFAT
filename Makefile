CC=gcc
CCOPTS=--std=gnu99 -g -Wall 
AR=ar

OBJS=fat.o

HEADERS=fat.h utils.h linked_list.h

LIBS=libpseudofat.a

BINS=PseudoFAT

phony: clean all


all:	$(BINS) $(LIBS)

%.o:	%.c $(HEADERS)
		$(CC) $(CCOPTS) -c -o $@  $<

libpseudofat.a:	$(OBJS) 
				$(AR) -rcs $@ $^
				$(RM) $(OBJS)

PseudoFAT: PseudoFAT.o $(LIBS)
			$(CC) $(CCOPTS) -o $@ $^ -lm

clean:
	rm -rf *.o *~ $(LIBS) $(BINS)