cmake_minimum_required(VERSION 3.20)

project("TCP IP")

set(EXE_NAME a.out)

set(WARNINGS
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wconversion -pedantic -Werror>
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
)

add_executable(${EXE_NAME} tap.cpp main.cpp bitformat.h tap.h)

target_compile_features(${EXE_NAME} PUBLIC cxx_std_23)
target_compile_options(${EXE_NAME} PUBLIC ${WARNINGS})
