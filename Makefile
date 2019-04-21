CXX=g++
CXXFLAGS=-std=c++11 -m64 -g -D__STDC_FORMAT_MACROS -O3 -c -Wall
LDFLAGS=-m64 -g -ljansson -lreadline -lcityhash
SOURCES=main.cc ruby_heap_obj.cc parser.cc graph.cc dominator_tree.cc progress.cc
OBJECTS=$(SOURCES:.cc=.o)
EXECUTABLE=harb

.PHONY: clean

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(OBJECTS) $(LDFLAGS) -o $@

.cc.o:
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	rm -f $(EXECUTABLE) $(OBJECTS)

