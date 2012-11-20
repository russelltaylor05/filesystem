CC 	= gcc

CFLAGS  = -Wall -g

LD 	= gcc

LDFLAGS  = -Wall -g

PUBFILES =  README  hungrymain.c  libPLN.a  libsnakes.a  lwp.h\
	    numbersmain.c  snakemain.c  snakes.h

PROGS	= minls minget

SNAKEOBJS  = snakemain.o 

HUNGRYOBJS = hungrymain.o 

NUMOBJS    = numbersmain.o

OBJS	= $(SNAKEOBJS) $(HUNGRYOBJS) $(NUMOBJS) 

SRCS	= minls.c minget.c


EXTRACLEAN = core $(PROGS)

all: 	$(PROGS)

minls: minls.o
	$(LD) $(LDFLAGS) -o minls minls.o

minget: minget.o
	$(LD) $(LDFLAGS) -o minget minget.o


allclean: clean
	@rm -f $(EXTRACLEAN)

clean:	
	rm -f  *.o *~ TAGS minls minget

