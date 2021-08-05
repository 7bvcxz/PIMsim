CXX=g++
CXXFLAGS=-Wall -O3 -std=c++11

EXE_NAME=PimSimulator

SRCS = utils.cc PimUnit.cc PimSimulator.cc

OBJECTS = $(addsuffix .o, $(basename $(SRCS)))

all: $(EXE_NAME)

$(EXE_NAME): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o : %.cc
	$(CXX) $(CXXFLAGS) -o $@ -c $<

clean:
	rm -f $(OBJECTS) $(EXE_NAME)
