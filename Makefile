CC=g++
CFLAGS=-c -g -O0 -Wall
LDFLAGS=-lpthread -lrt
SOURCES=main.cpp ThreadConfig.cpp LockFreeQ.cpp JobSwarm.cpp sgif.cpp
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=JobSwarm

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

clean:
	rm -f $(OBJECTS) $(EXECUTABLE)
	
.cpp.o:
	$(CC) $(CFLAGS) $< -o $@


