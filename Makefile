CFLAGS=-Wall -Wextra -Os -march=native
CLIBS=-lX11 -lImlib2

all: config.h dwmwall 

dwmwall: dwmwall.c
	$(CC) $(CFLAGS) $< $(CLIBS) -o $@

config.h:
	cp config.def.h config.h

install: all
	cp -f dwmwall /usr/local/bin/dwmwall

uninstall:
	rm /usr/local/bin/dwmwall


clean:
	rm dwmwall
