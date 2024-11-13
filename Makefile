# General compiler settings/flags
CC=g++
LDFLAGS=-lpthread
CFLAGS=-Wall -std=c++20 -fPIC
# Debugging
CFLAGS+=-g

# labdaq stuff

# labdev flags
CFLAGS+=$(shell pkg-config --cflags liblabdev)
LDFLAGS+=$(shell pkg-config --libs liblabdev)

# wxWidget flags
CFLAGS+=$(shell wx-config --cxxflags)
LDFLAGS+=$(shell wx-config --libs)
LDFLAGS+=$(shell wx-config --optional-libs propgrid)

# CERN root stuff
CFLAGS+=$(shell root-config --cflags)
LDFLAGS+=$(shell root-config --libs)

BIN=QuickDAQ
OBJ=
#OBJ+=OsciSettings.o
#OBJ+=FGenSettings.o

.PHONY: all clean

all: $(BIN)

%.o: %.cpp
	$(CC) -c $^ $(CFLAGS)

$(BIN): $(BIN).o $(OBJ)
	$(CC) $^ -o $@ $(LDFLAGS)

clean:
	rm -f *.o
	rm -f $(BINS)