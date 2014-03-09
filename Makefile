CXX=g++
CXXFLAGS=-m64 -g -D__STDC_FORMAT_MACROS -O3 -c -Wall
LDFLAGS=-m64 -g -ltcmalloc_minimal -ljansson -lreadline -lcityhash
SOURCES=main.cc
OBJECTS=$(SOURCES:.cc=.o)
EXECUTABLE=rb_heapa

.PHONY: clean

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(OBJECTS) $(LDFLAGS) -o $@

.cc.o:
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	rm -f $(EXECUTABLE) $(OBJECTS)

