cmake_minimum_required(VERSION 3.21)

project(mks_util)
message(">> Module ${PROJECT_NAME}")

file(GLOB_RECURSE SOURCE_FILES FILES_MATCHING PATTERN "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
add_library(${PROJECT_NAME} STATIC ${SOURCE_FILES})

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-march=native" COMPILER_SUPPORTS_MARCH_NATIVE)
if(COMPILER_SUPPORTS_MARCH_NATIVE)
    message(STATUS "Adding -march=native compilation flag ")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
    set_source_files_properties(string_compare.cpp PROPERTIES COMPILE_FLAGS -march=native)
else()
    message(WARNING Compiler doesn't support SIMD instructions)
endif()

set(LBS)
if(MSVC)
    # demangle
    set(LIBS dbghelp)
endif(MSVC)

add_subdirectory(lib/date)
add_subdirectory(lib/log)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(${PROJECT_NAME} mks_log date::date)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 17)
