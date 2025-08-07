cmake_minimum_required(VERSION 3.28)
project(KQuantizer)


# set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(KQuantizer
    main.cpp
    src/lodepng.cpp
)

target_compile_features(KQuantizer PRIVATE cxx_std_17)


target_include_directories(KQuantizer PRIVATE ${CMAKE_SOURCE_DIR}/include)
