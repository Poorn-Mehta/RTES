IDIR =../inc
CC=gcc
CFLAGS= -Wall -O0 -lpthread -lrt -I$(IDIR)

ODIR=obj
LDIR =

LIBS=-lm

_DEPS = main.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = main.o Aux_Func.o Cam_Monitor.o Cam_Func.o Cam_Brightness.o Cam_Grey.o Cam_Contrast.o Scheduler.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))


$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

Q5_2: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~ 
