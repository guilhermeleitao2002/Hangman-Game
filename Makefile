CC = gcc
CFLAGS = -g

all: player GS

player: player.o
	$(CC) $(CFLAGS) -o player player.o

GS: GS.o
	$(CC) $(CFLAGS) -o GS GS.o

player.o: player.c
	$(CC) $(CFLAGS) -c player.c

GS.o: GS.c
	$(CC) $(CFLAGS) -c GS.c

clean:
	rm -f *.o player GS

kill: clean
	rm -f player GS