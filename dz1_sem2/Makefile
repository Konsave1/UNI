CXX ?= g++
CXXFLAGS ?= -std=c++11 -Wall -Wextra -Wpedantic -O2

.PHONY: all test clean

all: build/dz1_sem2

build:
	mkdir -p build

build/dz1_sem2: main.cpp sequence.cpp sequence.h personnel.cpp personnel.h person.h | build
	$(CXX) $(CXXFLAGS) main.cpp sequence.cpp personnel.cpp -o $@

build/tests: tests.cpp sequence.cpp sequence.h personnel.cpp personnel.h person.h | build
	$(CXX) $(CXXFLAGS) tests.cpp sequence.cpp personnel.cpp -o $@

test: build/tests
	mkdir -p build
	@dir=$$(mktemp -d ./build/tests.XXXXXX); ./build/tests "$$dir"

clean:
	rm -rf build
