# DA Project II — register allocation tool
CXX      ?= g++
INCLUDES := -Iinclude -I.
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O2 $(INCLUDES)
TARGET    = myProg

SRCS = main.cpp \
	src/io/input_parser.cpp \
	src/allocation/web_builder.cpp \
	src/allocation/interference_graph_builder.cpp \
	src/allocation/allocation_output.cpp \
	src/allocation/run_allocation.cpp

OBJS = $(SRCS:.cpp=.o)

HDRS = $(wildcard include/da/*.h) $(wildcard include/da/*.hpp) $(wildcard include/graph/*.hpp)

.PHONY: all clean docs

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp $(HDRS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	$(RM) $(TARGET) $(TARGET).exe $(OBJS)

docs:
	doxygen Doxyfile
