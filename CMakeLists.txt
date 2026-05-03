cmake_minimum_required(VERSION 3.10)
project(ServerStudy CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra -O3)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

file(GLOB COMMON_SRC "Network/Common/*.cpp")
file(GLOB SERVER_ONLY_SRC "Network/Server/*.cpp")
file(GLOB CLIENT_ONLY_SRC "Network/Client/*.cpp")
file(GLOB TEST_SRC "Tests/*.cpp")

add_executable(Server ${SERVER_ONLY_SRC} ${COMMON_SRC})
add_executable(Client ${CLIENT_ONLY_SRC} ${COMMON_SRC})
add_executable(Tests ${TEST_SRC} ${COMMON_SRC})

if(WIN32)
    target_link_libraries(Server PRIVATE ws2_32)
    target_link_libraries(Client PRIVATE ws2_32)
    target_link_libraries(Tests PRIVATE ws2_32)
endif()

# 서버 실행 타겟
add_custom_target(run-server
    COMMAND Server
    DEPENDS Server
    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    COMMENT "Building and Running Server..."
)

# 클라이언트 실행 타겟
add_custom_target(run-client
    COMMAND Client
    DEPENDS Client
    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    COMMENT "Building and Running Client..."
)

# 테스트 실행 타겟
add_custom_target(run-test
    COMMAND Tests
    DEPENDS Tests
    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    COMMENT "Building and Running Tests..."
)
