CC=gcc
CFLAGS= -lpthread -lrt -I.
DEPS =
OBJ = main.o 

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

Sum: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)
