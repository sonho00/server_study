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

add_executable(Server ${SERVER_ONLY_SRC} ${COMMON_SRC})
add_executable(Client ${CLIENT_ONLY_SRC} ${COMMON_SRC})

if(WIN32)
    target_link_libraries(Server PRIVATE ws2_32)
    target_link_libraries(Client PRIVATE ws2_32)
endif()

# --- 테스트 자동화 로직 ---
file(GLOB TEST_DIRS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/Tests "Tests/[0-9][0-9]_*")

foreach(T_DIR ${TEST_DIRS})
    string(REGEX MATCH "[0-9][0-9]" TEST_NUM ${T_DIR})
    file(GLOB TEST_SOURCES "Tests/${T_DIR}/*.cpp")

    if(TEST_SOURCES)
        set(TEST_TARGET_NAME test_${TEST_NUM})
        add_executable(${TEST_TARGET_NAME} ${TEST_SOURCES} ${COMMON_SRC} Tests/Client.cpp)

        set_target_properties(${TEST_TARGET_NAME} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/Tests"
            OUTPUT_NAME ${T_DIR}
        )

        if(WIN32)
            target_link_libraries(${TEST_TARGET_NAME} PRIVATE ws2_32)
        endif()
    endif()
endforeach()

# 실행 편의 기능 (make run-server / make run-client)

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
add_custom_target(run-${TEST_TARGET_NAME}
    COMMAND ${TEST_TARGET_NAME}
    DEPENDS ${TEST_TARGET_NAME}
    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    COMMENT "Running ${TEST_TARGET_NAME}..."
)
