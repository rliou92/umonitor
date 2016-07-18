 .PHONY: display

all: display.o display
	gcc display.o -o display -lX11 -lXrandr -lconfig
i2c.o:
	gcc -o i2c.o -c i2c.c
display.o:
	gcc -o display.o -c display.c
udev: 
	gcc -oumonitor -ludev umonitor.c

clean:
	rm -f *.o umonitor display
