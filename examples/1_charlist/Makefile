CC = gcc
CFLAGS = -g -pipe -O2 -Wall -arch ppc
LDFLAGS = -L/usr/local/lib ../../.libs/libaochat.a -lcrypto -arch ppc
LIBS = main.o
BIN = charlist

all: $(BIN)

$(BIN): $(LIBS)
	$(CC) -o $(BIN) $(LIBS) $(LDFLAGS)

.c.o:
	$(CC) $(CFLAGS) -c $<

clean:
	@rm -f *~ $(LIBS) $(BIN)

distclean: clean

# depend
*.o: Makefile
