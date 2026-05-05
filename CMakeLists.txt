cmake_minimum_required(VERSION 3.10)
project(ServerStudy CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

file(GLOB COMMON_SRC "Network/Common/*.cpp")
file(GLOB SERVER_ONLY_SRC "Network/Server/*.cpp")
file(GLOB CLIENT_ONLY_SRC "Network/Client/*.cpp")
file(GLOB TEST_SRC "Tests/*.cpp")

add_executable(server ${SERVER_ONLY_SRC} ${COMMON_SRC})
add_executable(client ${CLIENT_ONLY_SRC} ${COMMON_SRC})
add_executable(tests ${TEST_SRC} ${COMMON_SRC})

if(WIN32)
    target_link_libraries(server PRIVATE ws2_32)
    target_link_libraries(client PRIVATE ws2_32)
    target_link_libraries(tests PRIVATE ws2_32)
endif()

# 서버 실행 타겟
add_custom_target(run-server
    COMMAND server
    DEPENDS server
    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    COMMENT "Building and Running Server..."
)

# 클라이언트 실행 타겟
add_custom_target(run-client
    COMMAND client
    DEPENDS client
    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    COMMENT "Building and Running Client..."
)

# 테스트 실행 타겟
add_custom_target(run-test
    COMMAND tests
    DEPENDS tests
    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    COMMENT "Building and Running Tests..."
)
