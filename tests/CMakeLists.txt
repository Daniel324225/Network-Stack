include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/001f281926c74a819e9266bb4ec525515ff39418.zip
  DOWNLOAD_EXTRACT_TIMESTAMP True
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

macro(add_test TESTNAME)
    add_executable(${TESTNAME} ${ARGN})
    target_link_libraries(${TESTNAME} GTest::gtest_main compiler_options)
    target_include_directories(${TESTNAME} PRIVATE ${CMAKE_SOURCE_DIR})
    gtest_discover_tests(${TESTNAME})
endmacro()

add_test(packet packet_test.cpp)