cmake_minimum_required(VERSION 3.0)

if (POLICY CMP0079)
  cmake_policy(SET CMP0079 NEW)
endif (POLICY CMP0079)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(OS "unix")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY
        STRINGS "Release" "Debug" "MinSizeRel" "RelWithDebInfo")
endif()

project(FuzzyPC VERSION 0.0.1 LANGUAGES C CXX)

set(OC_THIRDPARTY_HINT "${CMAKE_CURRENT_LIST_DIR}/thirdparty")

find_package(libOTe REQUIRED PATHS "${CMAKE_CURRENT_LIST_DIR}/thirdparty")

find_package(Kuku REQUIRED PATHS "${CMAKE_CURRENT_LIST_DIR}/thirdparty")

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/src)

target_compile_options(libFuzzyPC PRIVATE
  -march=native -O2 -std=c++2a -lm -maes
)

target_include_directories(libFuzzyPC PUBLIC
  ${CMAKE_CURRENT_LIST_DIR}/src
  ${CMAKE_CURRENT_LIST_DIR}/thirdparty/include
)

target_link_libraries(libFuzzyPC PUBLIC
  oc::libOTe
  xxhash
  Kuku::kuku
)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/test)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/run)
