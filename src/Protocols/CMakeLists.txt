file(GLOB_RECURSE SRCS *.cpp *.c)

add_library(libFuzzyPC STATIC ${SRCS})