cmake_minimum_required(VERSION 3.28)
project(KQuantizer)


# set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(KQuantizer
    main.cpp
    src/kdtree.cpp
)

target_compile_features(KQuantizer PRIVATE cxx_std_17)

target_include_directories(KQuantizer PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_compile_options(-Wall -g -O0)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()
