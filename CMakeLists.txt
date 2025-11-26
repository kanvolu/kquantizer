cmake_minimum_required(VERSION 3.28)
project(KQuantizer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ------------------ Source Outside of Main.cpp  -------------------
add_library(KQ_Obj OBJECT
    src/blur.cpp
    src/palette-parsing.cpp
    src/reshaping.cpp
)

target_include_directories(KQ_Obj PUBLIC ${CMAKE_SOURCE_DIR}/include)

# --------------------- Executable -----------------------------
add_executable(KQuantizer main.cpp)

target_link_libraries(KQuantizer PRIVATE KQ_Obj)


# --------------------- Compiler options ------------------------
target_compile_options(KQuantizer PRIVATE -Wall -Werror)

target_compile_options(KQuantizer PRIVATE
    $<$<CONFIG:Debug>:-g -O0>
    $<$<CONFIG:Release>:-O3 -march=native>
)
