# Lightweight in-house test harness: no external framework dependency.
add_executable(vision_tests
  test_main.cc
  test_data.cc
)

# Performance regression gate; loose thresholds, skip with: ctest -E perf_gate
add_executable(perf_gate perf_gate.cc test_data.cc)
target_link_libraries(perf_gate PRIVATE simple_vision)
target_include_directories(perf_gate PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)
target_link_libraries(vision_tests PRIVATE simple_vision)
target_include_directories(vision_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)
add_test(NAME vision_tests COMMAND vision_tests)

add_test(NAME perf_gate COMMAND perf_gate)
