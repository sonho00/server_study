CXX = g++
LDFLAGS = -lws2_32

SRCS = $(wildcard *.cpp)
TARGETS = IOCPEchoServer.exe

all: $(TARGETS)

IOCPEchoServer.exe: $(SRCS)
	$(CXX) -o $@ $^ $(LDFLAGS)

clean:
	del *.exe
