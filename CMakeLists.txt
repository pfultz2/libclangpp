cmake_minimum_required (VERSION 2.8)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(BCMTest)
include(clang)

add_library(clangpp INTERFACE)
target_include_directories(clangpp INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(clangpp INTERFACE clang::clang)

# Installation
install(DIRECTORY include/ DESTINATION include)

install(TARGETS clangpp EXPORT clangpp-targets
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    INCLUDES DESTINATION include
)

install(EXPORT clangpp-targets
    FILE clangpp-targets.cmake
    NAMESPACE clang::
    DESTINATION lib/cmake/clangpp
)

install(FILES 
        cmake/clangpp-config.cmake
        cmake/clang.cmake
    DESTINATION lib/cmake/clangpp)

# Tests
bcm_test_header(NAME clangpp-header HEADER clangpp.hpp STATIC)
target_link_libraries(clangpp-header clangpp)

bcm_add_test(NAME test-basic SOURCES test/basic.cpp)
target_link_libraries(test-basic clangpp)
