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

add_library(clang::clang UNKNOWN IMPORTED)
set_target_properties(clang::clang PROPERTIES
    IMPORTED_LOCATION ${CLANG_LIB}
    INTERFACE_INCLUDE_DIRECTORIES ${CLANG_INCLUDE_DIR}
)
