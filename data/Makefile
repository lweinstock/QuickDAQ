# General compiler settings/flags
CC=g++
LDFLAGS=-lpthread
CFLAGS=-Wall -std=c++20 -fPIC
# Debugging
CFLAGS+=-g

# labdev flags
CFLAGS+=$(shell pkg-config liblabdev --cflags)
LDFLAGS+=$(shell pkg-config liblabdev --libs)

# CERN root stuff
CFLAGS+=$(shell root-config --cflags)
LDFLAGS+=$(shell root-config --libs)

BIN=SpectrumAnalysis
OBJ=

.PHONY: all clean $(BIN)

all: $(BIN)

%.o: %.cpp
	$(CC) -c $^ $(CFLAGS)

$(BIN): $(BIN).o $(OBJ)
	$(CC) $^ -o $@ $(LDFLAGS)

clean:
	rm -f *.o
	rm -f $(BIN)