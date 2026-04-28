cmake_minimum_required(VERSION 3.10)
project(VibeCoder)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define directories
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(DEPS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps)

# Include directories
include_directories(
    ${SRC_DIR}/shared
    ${DEPS_DIR}/json/include
    ${DEPS_DIR}/luajit/src
    ${DEPS_DIR}/zdraw/zdraw/zdraw
)

# Shared Files
set(SHARED_SOURCES
    ${SRC_DIR}/shared/pipe_comm.cpp
)

# Injector
add_executable(injector ${SRC_DIR}/injector/main.cpp ${SRC_DIR}/injector/injector.cpp)
target_link_libraries(injector)

# DLL
add_library(vibecoder SHARED ${SRC_DIR}/dll/main.cpp ${SRC_DIR}/dll/lua_engine.cpp ${SHARED_SOURCES})
target_link_libraries(vibecoder winmm psapi)

# Editor
add_executable(editor ${SRC_DIR}/editor/main.cpp ${SRC_DIR}/editor/pipe_client.cpp ${SRC_DIR}/editor/ai_backend.cpp ${SHARED_SOURCES})
target_link_libraries(editor wininet user32 gdi32 d3d11)
