cmake_minimum_required (VERSION 2.6)
project (document)

set(SOURCES
    src/Parser.cpp src/Parser.h src/json.cpp
    src/Iterator.cpp src/Iterator.h)

set(INCLUDES
    include/json/json.h)

add_library(${PROJECT_NAME} SHARED ${SOURCES} ${INCLUDES})
include_directories(. ./include ./include/json)

set(CMAKE_CXX_FLAGS "--std=c++17 -Wall -pedantic-errors -fPIC -Winit-self -Wold-style-cast -Woverloaded-virtual -Wuninitialized -Wmissing-declarations -Wunused-result -O3 -Wdouble-promotion")
set(CMAKE_CXX_FLAGS_DEBUG "-DDEBUG -g -O0")

install(TARGETS ${PROJECT_NAME}
        LIBRARY DESTINATION lib
        PERMISSIONS WORLD_READ WORLD_EXECUTE)

install(DIRECTORY include/json DESTINATION include)
