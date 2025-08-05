cmake_minimum_required(VERSION 3.28)
project(KQuantizer)

set(CMAKE_CXX_STANDARD 17)

add_executable(KQuantizer
    main.cpp
    src/lodepng.cpp
)

target_include_directories(KQuantizer PRIVATE ${CMAKE_SOURCE_DIR}/include)
