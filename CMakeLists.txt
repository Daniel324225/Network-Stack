cmake_minimum_required(VERSION 3.20)

project("Network Stack")

add_library(compiler_options INTERFACE)
target_compile_features(compiler_options INTERFACE cxx_std_23)
target_compile_options(compiler_options INTERFACE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wconversion -pedantic -Werror>
  $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
)

add_executable(
  network_stack 
  src/main.cpp 
  src/packet.h 
  src/utils.h 
  src/tap.cpp src/tap.h 
  src/ipv4.cpp src/ipv4.h 
  src/internet_layer.h
  src/link_layer.h
  src/arp.h 
  src/ethernet.h
  src/channel.h
)

target_link_libraries(network_stack PRIVATE compiler_options)

if (${CMAKE_HOST_LINUX})
  configure_file(
    setup.sh setup.sh
    COPYONLY
    FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ WORLD_READ
  )
  configure_file(
    setup_dev.sh setup_dev.sh
    COPYONLY
    FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ WORLD_READ
  )
  configure_file(
    setup_ip_tables.sh setup_ip_tables.sh
    COPYONLY
    FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ WORLD_READ
  )
endif()

enable_testing()
add_subdirectory(tests)
