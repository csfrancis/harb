CXX=g++
CXXFLAGS:=-std=c++11 -m64 -g -Ivendor -D__STDC_FORMAT_MACROS -DNDEBUG -O3 -c -Wall $(CXXFLAGS)
ifdef DEBUG
  CXXFLAGS += -O0 -UNDEBUG
endif
LDLIBS:=-lreadline $(LDLIBS)
LDFLAGS:=-m64 -g $(LDFLAGS)
SOURCES=main.cc ruby_heap_obj.cc parser.cc graph.cc dominator_tree.cc progress.cc output.cc
OBJECTS=$(SOURCES:.cc=.o)
EXECUTABLE=harb

.PHONY: clean

.PHONY: all
all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(OBJECTS) $(LDFLAGS) $(LDLIBS) -o $@

.cc.o:
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	rm -f $(EXECUTABLE) $(OBJECTS)

