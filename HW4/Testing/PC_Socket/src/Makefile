IDIR =../inc
CC=gcc
CFLAGS= -Wall -O0 -lpthread -lrt -I$(IDIR)

ODIR=obj
LDIR =

LIBS=-lm

_DEPS = main.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = main.o Aux_Func.o Socket.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

all: Project_1

$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

Project_1: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~ 
	rm -f Project_1
