cmake_minimum_required(VERSION 3.24)
project(ServerStudy CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Google Test 설정 추가 ---
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/heads/main.zip
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
# MSVC 사용자라면 런타임 라이브러리 일치를 위해 필요함
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
# ----------------------------

if(MSVC)
    add_compile_options(/W4)
    add_compile_options(/fsanitize=address)
    add_link_options(/fsanitize=address)

    # 디버그 정보(PDB)가 온전하게 남도록 증분 링크 비활성화 (ASan 필수 권장사항)
    add_link_options(/INCREMENTAL:NO)
else()
    add_compile_options(-Wall -Wextra)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

file(GLOB_RECURSE COMMON_SRC "Network/Common/*.cpp")
file(GLOB_RECURSE SERVER_ONLY_SRC "Network/Server/*.cpp")
file(GLOB_RECURSE CLIENT_ONLY_SRC "Network/Client/*.cpp")
file(GLOB_RECURSE TEST_SRC "Tests/*.cpp")

add_executable(server ${SERVER_ONLY_SRC} ${COMMON_SRC})
add_executable(client ${CLIENT_ONLY_SRC} ${COMMON_SRC})
add_executable(tests ${TEST_SRC} ${COMMON_SRC})

if(MSVC)
    # /wd4189: 지역 변수가 초기화되었으나 참조되지 않음 경고 끄기
    target_compile_options(tests PRIVATE /wd4189)
else()
    # GCC/Clang에서 안 쓰는 변수 경고 끄기
    target_compile_options(tests PRIVATE -Wno-unused-variable)
endif()

if(WIN32)
    target_link_libraries(server PRIVATE ws2_32)
    target_link_libraries(client PRIVATE ws2_32)
    target_link_libraries(tests PRIVATE ws2_32 gtest_main)
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
add_custom_target(run-tests
    COMMAND tests
    DEPENDS tests
    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    COMMENT "Building and Running Tests..."
)
