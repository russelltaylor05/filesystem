CC 	= gcc

CFLAGS  = -Wall -g

LD 	= gcc

LDFLAGS  = -Wall -g

PROGS	= minls minget

SRCS	= minls.c minget.c


EXTRACLEAN = core $(PROGS)

all: 	$(PROGS)

minls: minls.o min.o libmin.a
	$(LD) $(LDFLAGS) -o minls minls.o  -L. -lmin

minget: minget.o min.o libmin.a
	$(LD) $(LDFLAGS) -o minget minget.o -L. -lmin

libmin.a: min.o
	ar rcs libmin.a min.o

min.o: min.c
	$(LD) $(LDFLAGS) -c min.c

allclean: clean
	@rm -f $(EXTRACLEAN)

clean:	
	rm -f  *.o *~ TAGS minls minget output/*.out *.a

