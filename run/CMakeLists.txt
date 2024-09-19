add_executable(Experiment1 Experiment1.cpp)

target_compile_options(Experiment1 PRIVATE
  -march=native -O2 -std=c++2a -lm
)

target_link_libraries(Experiment1 PUBLIC
  libFuzzyPC
)

add_executable(Experiment2 Experiment2.cpp)

target_compile_options(Experiment2 PRIVATE
  -march=native -O2 -std=c++2a -lm
)

target_link_libraries(Experiment2 PUBLIC
  libFuzzyPC
)

add_executable(Sender Sender.cpp)

target_compile_options(Sender PRIVATE
  -march=native -O2 -std=c++2a -lm
)

target_link_libraries(Sender PUBLIC
  libFuzzyPC
)

add_executable(Receiver Receiver.cpp)

target_compile_options(Receiver PRIVATE
  -march=native -O2 -std=c++2a -lm
)

target_link_libraries(Receiver PUBLIC
  libFuzzyPC
)