IDIR =../inc
CC=gcc
CFLAGS= -Wall -O0 -lpthread -lrt -I$(IDIR)

ODIR=obj
LDIR =

LIBS=-lm

_DEPS = main.h Aux_Func.h Cam_Func.h Cam_Filter.h Scheduler.h Cam_Monitor.h Cam_RGB.h Storage.h Socket.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = main.o Aux_Func.o Cam_Func.o Cam_Filter.o Scheduler.o Cam_Monitor.o Cam_RGB.o Storage.o Socket.o
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
