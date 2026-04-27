CXX = g++
CXXFLAGS = -I. -std=c++20 -O3 -Wall -Wextra
LDFLAGS = -lws2_32

BIN_DIR = bin
TEST_OUT_DIR = bin\Tests

# 1. 소스 자동 인식
SERVER_SRC = $(wildcard Network/Common/*.cpp) $(wildcard Network/Server/*.cpp)
CLIENT_SRC = $(wildcard Network/Common/*.cpp) $(wildcard Network/Client/*.cpp)

# 2. 테스트 폴더에서 숫자만 추출 (01, 02...)
TEST_NAMES = $(foreach dir,$(wildcard Tests/[0-9][0-9]_*),$(firstword $(subst _, ,$(notdir $(dir)))))
TEST_TARGETS = $(addprefix test-,$(TEST_NAMES))

all: server client $(TEST_TARGETS)

# --- 서버 & 클라이언트 ---
server:
	@if not exist $(BIN_DIR) mkdir $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(SERVER_SRC) -o $(BIN_DIR)\Server.exe $(LDFLAGS)

client:
	@if not exist $(BIN_DIR) mkdir $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(CLIENT_SRC) -o $(BIN_DIR)\Client.exe $(LDFLAGS)

run-server: server
	@.\bin\Server.exe

run-client: client
	@.\bin\Client.exe

# --- 테스트 (make test-01 하나로 빌드+실행) ---
$(TEST_TARGETS): test-%:
	$(eval DIR := $(wildcard Tests/$*_*))
	$(eval NAME := $(notdir $(DIR)))
	@if not exist $(TEST_OUT_DIR) mkdir $(TEST_OUT_DIR)
	$(CXX) $(CXXFLAGS) $(DIR)/*.cpp Tests/Client.cpp -o $(TEST_OUT_DIR)\$(NAME).exe $(LDFLAGS)
	@echo [Running] $(NAME).exe
	@.\$(TEST_OUT_DIR)\$(NAME).exe

# --- 정리 (bin 폴더 통째로 삭제) ---
clean:
	@if exist $(BIN_DIR) rmdir /s /q $(BIN_DIR)

.PHONY: all server client run-server run-client clean $(TEST_TARGETS)