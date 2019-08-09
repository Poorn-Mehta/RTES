CC=gcc
CFLAGS= -Wall -O0 -lpthread -lrt -I.
DEPS =
OBJ = main.o 

all:EX2_Q1

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

EX2_Q1: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)
