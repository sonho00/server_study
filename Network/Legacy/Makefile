CXX = g++
LDFLAGS = -lws2_32

SRCS = $(wildcard *.cpp)
TARGETS = $(SRCS:.cpp=.exe)

all: $(TARGETS)

%.exe: %.cpp
	$(CXX) $< -o $@ $(LDFLAGS)

clean:
	del *.exe
