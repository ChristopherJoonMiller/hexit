OBJS = hexit.o
CC = g++
DEBUG = -g
CFLAGS = -Wall -c
LFLAGS = -Wall
LDLIBS = -lcurses
EXE = hexit

hexit: $(OBJS)
	$(CC) $(LFLAGS) $(LDLIBS) -o $(EXE) $(OBJS)

hexit.o: hexit.cpp hexit.h
	$(CC) $(CFLAGS) hexit.cpp

clean:
	rm *.o hexit