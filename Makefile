.PHONY: all clean format

CXX=g++
CXXFLAGS=-DNDEBUG -std=c++14
LDFLAGS= -lboost_program_options -lcrypto++ -lstdc++fs -lpthread
PROJECT = server_egorow
SOURCES := $(wildcard *.cpp)
HEADERS := $(wildcard *.h)
OBJECTS := $(SOURCES:%.cpp=%.o)

all : $(PROJECT)

$(PROJECT) : $(OBJECTS)
	$(CXX) $^ $(LDFLAGS) -o $@
	rm -f *.o *.orig

%.o : %.cpp
	$(CXX) -c $(CXXFLAGS) $< -o $@

$(OBJECTS) : $(HEADERS)

format:
	astyle *.cpp *.h
run:
	./$(PROJECT)
clean:
	rm -f $(PROJECT) *.o *.orig