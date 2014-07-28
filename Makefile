CXX=g++
CXXFLAGS=-m64 -std=c++0x -g -D__STDC_FORMAT_MACROS -O3 -c -Wall
LDFLAGS=-m64 -g -ltcmalloc_minimal -ljansson -lreadline -lcityhash
GENERATED_SOURCES=lexer.cc parser.cc
GENERATED_HEADERS=$(GENERATED_SOURCES:.cc=.h)
SOURCES=harb.cc node/print.cc node/binary_op.cc main.cc fields.cc nodes.cc $(GENERATED_SOURCES)
OBJECTS=$(SOURCES:.cc=.o)
EXECUTABLE=harb

.PHONY: clean

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(OBJECTS) $(LDFLAGS) -o $@

lexer.cc: lexer.l
	flex --header-file=lexer.h -o lexer.cc lexer.l

parser.cc: parser.y
	bison --defines=parser.h -o parser.cc parser.y

.cc.o:
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	rm -f $(EXECUTABLE) $(OBJECTS) $(GENERATED_SOURCES) $(GENERATED_HEADERS)

