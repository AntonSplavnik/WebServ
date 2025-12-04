# Makefile for Parser_Test.cpp

# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra

# Targets
TARGET = parser_test
SOURCES = Parser_Test.cpp

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SOURCES)

clean:
	rm -f $(TARGET)

.PHONY: all clean
