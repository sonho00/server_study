CXX = g++
CXXFLAGS= -I. -std=c++20 -O3 -Wall -Wextra
LDFLAGS = -lws2_32

SRCS = $(wildcard Network/IOCP/*.cpp) $(wildcard Network/Server/*.cpp)
TARGETS = bin/Server.exe bin/Client.exe

all: $(TARGETS)

bin/Server.exe: $(SRCS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

bin/Client.exe: Network/Client/Client.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	del bin\*.exe
