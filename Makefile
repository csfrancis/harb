CXX=g++
CXXFLAGS=-std=c++11 -m64 -g -Ivendor -D__STDC_FORMAT_MACROS -DNDEBUG -O3 -c -Wall
ifdef DEBUG
  CXXFLAGS += -O0 -UNDEBUG
endif
LDFLAGS=-m64 -g -lreadline
SOURCES=main.cc ruby_heap_obj.cc parser.cc graph.cc dominator_tree.cc progress.cc output.cc
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

