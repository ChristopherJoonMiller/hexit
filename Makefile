OBJS = main.o hexit.o
CC = g++
DEBUG = -g
CFLAGS = -Wall -c
LFLAGS = -Wall
LDLIBS = -lcurses -ltermkey
EXE = hexit

SRCS = \
main.cpp \
hexit.cpp \

hexit: $(OBJS)

	$(CC) $(DEBUG) $(LFLAGS) $(LDLIBS) -o $(EXE) $(OBJS)

hexit.objs: $(SRCS) hexit.h
	$(CC) $(DEBUG) $(CFLAGS) $(SRCS)

all: clean hexit.objs hexit
	
clean:
	rm *.o hexit