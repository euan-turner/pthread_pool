CC = gcc
CFLAGS = 

LIB = pool.a
LIBOBJS = pthread_pool.o queue.o

BUILD = $(LIB)

all: $(BUILD)

$(LIB): $(LIBOBJS)
	ar rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o $(BUILD)