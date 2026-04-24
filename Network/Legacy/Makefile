CXX = g++
LDFLAGS = -lws2_32
CXXFLAGS = -std=c++20 -Wall -Wextra -O3

SRCS = MultiThreadEcho.cpp
TARGETS = $(SRCS:.cpp=.exe)

all: $(TARGETS)

%.exe: %.cpp
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS)

clean:
	del *.exe
