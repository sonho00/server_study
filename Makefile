CXX = g++
CXXFLAGS= -I. -std=c++20 -O3 -Wall -Wextra
LDFLAGS = -lws2_32

SERVER = $(wildcard Network/Common/*.cpp) $(wildcard Network/Server/*.cpp)
CLIENT = $(wildcard Network/Common/*.cpp) $(wildcard Network/Client/*.cpp)
TESTS = $(wildcard Network/Common/*.cpp) $(wildcard Network/Tests/*.cpp)
TARGETS = bin/Server.exe bin/Client.exe

all: $(TARGETS)

bin/Server.exe: $(SERVER)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

bin/Client.exe: $(CLIENT)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	
tests: bin/Tests.exe
bin/Tests.exe: $(TESTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	del bin\*.exe
