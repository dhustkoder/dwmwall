CFLAGS=-Wall -Wextra -Os 
CLIBS=-lX11 -lImlib2

all: dwmwall 

dwmwall: dwmwall.c config.h
	$(CC) $(CFLAGS) $^ $(CLIBS) -o $@

config.h:
	cp config.def.h config.h

clean:
	rm dwmwall
