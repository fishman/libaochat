CC = gcc
CFLAGS = -g -pipe -O2 -Wall
LDFLAGS = -L/usr/local/lib -laochat
LIBS = main.o
BIN = echobot

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
