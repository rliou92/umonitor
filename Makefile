 .PHONY: display

all: display.o display
	gcc display.o -o display -lX11 -lXrandr -lconfig
display.o:
	gcc -o display.o -c display.c
udev: 
	gcc -oumonitor -ludev umonitor.c

clean:
	rm -f *.o umonitor display
