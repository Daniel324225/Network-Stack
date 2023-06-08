cmake_minimum_required(VERSION 3.20)

project("TCP IP")

add_library(compiler_options INTERFACE)
target_compile_features(compiler_options INTERFACE cxx_std_23)
target_compile_options(compiler_options INTERFACE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wconversion -pedantic -Werror>
  $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
)

add_executable(TCP_IP main.cpp packet.h utils.h)
target_link_libraries(TCP_IP PRIVATE compiler_options)

enable_testing()
add_subdirectory(tests)
