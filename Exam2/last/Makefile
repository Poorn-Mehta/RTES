CC=gcc
CFLAGS= -Wall -O0 -lpthread -lrt -I.
DEPS =
OBJ = main.o 

all:EX2_Q2

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

EX2_Q2: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)
