find_package(GTest)

add_executable(libFuzzyPCTest
  TestRunner.cpp
)

target_compile_options(libFuzzyPCTest PRIVATE
  -march=native -O2 -std=c++2a -lm -maes
)

target_include_directories(libFuzzyPCTest PUBLIC
  ${GTEST_INCLUDE_DIR}
)

target_link_libraries(libFuzzyPCTest PUBLIC
  libFuzzyPC
  ${GTEST_LIBRARIES}
)

target_sources(libFuzzyPCTest
  PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/gtest_BenesNetwork.cpp
  ${CMAKE_CURRENT_LIST_DIR}/gtest_Pns.cpp
  ${CMAKE_CURRENT_LIST_DIR}/gtest_Okvs.cpp
  ${CMAKE_CURRENT_LIST_DIR}/gtest_EqualityCheck.cpp
  ${CMAKE_CURRENT_LIST_DIR}/gtest_Cpsi.cpp
  ${CMAKE_CURRENT_LIST_DIR}/gtest_SecretSharedOt.cpp
)