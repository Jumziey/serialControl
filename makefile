CC = gcc
CFLAGS = -g -O0
LOADLIBES = -lSDL -lconfuse


sc: sc.o arduino-serial-lib.o
