OBJS = hexit.o
CC = g++
DEBUG = -g
CFLAGS = -Wall -c
LFLAGS = -Wall
LDLIBS = -lcurses -ltermkey
EXE = hexit

hexit: $(OBJS)
	$(CC) $(DEBUG) $(LFLAGS) $(LDLIBS) -o $(EXE) $(OBJS)

hexit.o: hexit.cpp hexit.h
	$(CC) $(DEBUG) $(CFLAGS) hexit.cpp

clean:
	rm *.o hexit