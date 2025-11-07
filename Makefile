# General compiler settings/flags
CC=g++
LDFLAGS=-lpthread
CFLAGS=-Wall -std=c++17 -fPIC
# Debugging
CFLAGS+=-g

# labdev flags
CFLAGS+=$(shell pkg-config --cflags labkit)
LDFLAGS+=$(shell pkg-config --libs labkit)

# wxWidget flags
CFLAGS+=$(shell wx-config --cxxflags)
LDFLAGS+=$(shell wx-config --libs)
LDFLAGS+=$(shell wx-config --optional-libs propgrid)

# CERN root stuff
CFLAGS+=$(shell root-config --cflags)
LDFLAGS+=$(shell root-config --libs)

BIN=QuickDAQ
OBJ=plot.o

.PHONY: all clean $(BIN)

all: $(BIN)

%.o: %.cpp
	$(CC) -c $^ $(CFLAGS)

$(BIN): $(BIN).o $(OBJ)
	$(CC) $^ -o $@ $(LDFLAGS)

clean:
	rm -f *.o
	rm -f $(BIN) $(BIN2)