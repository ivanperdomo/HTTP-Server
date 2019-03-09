
#
# Makefile for lab 6, part 1
#

CC  = gcc
CXX = g++

INCLUDES = 
CFLAGS   = -g -Wall $(INCLUDES)
CXXFLAGS = -g -Wall $(INCLUDES)

LDFLAGS = -g 
LDLIBS = 

http-server: http-server.o

http-server.o:

.PHONY: clean
clean:
	rm -f *.o *~ a.out core http-server

.PHONY: all
all: clean http-server

