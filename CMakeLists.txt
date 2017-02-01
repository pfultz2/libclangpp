cmake_minimum_required (VERSION 2.8)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(BCMTest)

find_library(
    CLANG_LIB clang 
    PATH_SUFFIXES 
        llvm-4.0/lib
        llvm-3.9/lib
        llvm-3.8/lib
        llvm-3.7/lib
        llvm-3.6/lib
        llvm-3.5/lib
)

get_filename_component(CLANG_LIB_DIR ${CLANG_LIB} PATH)
get_filename_component(CLANG_ROOT ${CLANG_LIB_DIR}/.. ABSOLUTE)

message(STATUS "Clang found: ${CLANG_ROOT}")

find_path(CLANG_INCLUDE_DIR clang-c/Index.h PATHS ${CLANG_ROOT}/include)

# find_package(LLVM CONFIG)
# message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")

include_directories(include)
bcm_test_header(NAME clangpp-header HEADER clangpp.hpp STATIC)
target_include_directories(clangpp-header PUBLIC include ${CLANG_INCLUDE_DIR})
target_link_libraries(clangpp-header ${CLANG_LIB})

bcm_add_test(NAME test-basic SOURCES test/basic.cpp)
target_include_directories(test-basic PUBLIC include ${CLANG_INCLUDE_DIR})
target_link_libraries(test-basic ${CLANG_LIB})
