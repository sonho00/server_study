CXX = g++
# -MMD -MP 옵션이 헤더 파일 변경을 감지해줍니다.
CXXFLAGS = -I. -std=c++20 -O3 -Wall -Wextra -MMD -MP
LDFLAGS = -lws2_32

BIN_DIR = bin
OBJ_DIR = obj
TEST_OUT_DIR = bin\Tests

# 1. 소스 및 오브젝트 파일 설정
SERVER_SRC = $(wildcard Network/Common/*.cpp) $(wildcard Network/Server/*.cpp)
CLIENT_SRC = $(wildcard Network/Common/*.cpp) $(wildcard Network/Client/*.cpp)

SERVER_OBJS = $(patsubst %.cpp, $(OBJ_DIR)/%.o, $(SERVER_SRC))
CLIENT_OBJS = $(patsubst %.cpp, $(OBJ_DIR)/%.o, $(CLIENT_SRC))

# 2. 테스트 이름 추출 로직
TEST_NAMES = $(foreach dir,$(wildcard Tests/[0-9][0-9]_*),$(firstword $(subst _, ,$(notdir $(dir)))))
TEST_TARGETS = $(addprefix test-,$(TEST_NAMES))

all: server client

# --- 서버 & 클라이언트 빌드 ---
server: $(BIN_DIR)\Server.exe

$(BIN_DIR)\Server.exe: $(SERVER_OBJS)
	@if not exist $(@D) mkdir $(@D)
	$(CXX) $(SERVER_OBJS) -o $@ $(LDFLAGS)

client: $(BIN_DIR)\Client.exe

$(BIN_DIR)\Client.exe: $(CLIENT_OBJS)
	@if not exist $(@D) mkdir $(@D)
	$(CXX) $(CLIENT_OBJS) -o $@ $(LDFLAGS)

# --- 실행 명령 ---
run-server: server
	@.\bin\Server.exe

run-client: client
	@.\bin\Client.exe

# --- 테스트 빌드 및 실행 ---
# 테스트는 매번 새로운 환경에서 실행하는 경우가 많아 .PHONY 성격을 유지합니다.
$(TEST_TARGETS): test-%:
	$(eval DIR := $(wildcard Tests/$*_*))
	$(eval NAME := $(notdir $(DIR)))
	@if not exist $(TEST_OUT_DIR) mkdir $(TEST_OUT_DIR)
	@echo [Compiling Test] $(NAME)
	$(CXX) $(CXXFLAGS) $(DIR)/*.cpp Network/Common/*.cpp Tests/Client.cpp -o $(TEST_OUT_DIR)\$(NAME).exe $(LDFLAGS)
	@echo [Running] $(NAME).exe
	@.\$(TEST_OUT_DIR)\$(NAME).exe

# --- 개별 .cpp 파일을 .o로 만드는 규칙 ---
$(OBJ_DIR)/%.o: %.cpp
	@if not exist $(subst /,\,$(dir $@)) mkdir $(subst /,\,$(dir $@))
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 생성된 .d 파일(헤더 의존성)을 읽어들임
-include $(SERVER_OBJS:.o=.d) $(CLIENT_OBJS:.o=.d)

# --- 정리 ---
clean:
	@if exist $(BIN_DIR) rmdir /s /q $(BIN_DIR)
	@if exist $(OBJ_DIR) rmdir /s /q $(OBJ_DIR)

.PHONY: all clean run-server run-client $(TEST_TARGETS)
