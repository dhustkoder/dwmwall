CFLAGS=-Wall -Wextra -Os 
CLIBS=-lX11 -lImlib2

all: dwmwall 

dwmwall: dwmwall.c
	$(CC) $(CFLAGS) $^ $(CLIBS) -o $@

clean:
	rm dwmwall
