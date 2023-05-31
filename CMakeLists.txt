cmake_minimum_required(VERSION 3.10)
project(misbitfont_assembler VERSION 0.2 LANGUAGES C CXX)

find_package(fmt REQUIRED)

add_executable(misbitfont_assembler src/main.cpp)
target_include_directories(misbitfont_assembler PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_compile_features(misbitfont_assembler PRIVATE cxx_std_20)
target_link_libraries(misbitfont_assembler fmt::fmt msbtfont)
