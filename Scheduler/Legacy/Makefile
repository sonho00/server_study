CXX = g++
CFLAGS = -std=c++20 -O3 -pthread
LDFLAGS = -lws2_32

SRCS = $(wildcard *.cpp)
TARGET = Main.exe

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	del *.exe
