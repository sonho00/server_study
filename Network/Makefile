CXX = g++
CXXFLAGS= -I./IOCP -I./Protocol -I./Server -std=c++20 -O3 -Wall -Wextra
LDFLAGS = -lws2_32

SRCS = $(wildcard IOCP/*.cpp) $(wildcard Server/*.cpp)
TARGETS = Server.exe Client.exe

all: $(TARGETS)

Server.exe: $(SRCS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

Client.exe: Client/Client.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	del *.exe
