find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(bootstrap_dir "${CMAKE_CURRENT_BINARY_DIR}/bootstrap")
add_custom_command(OUTPUT "${bootstrap_dir}/bootstrap.exe" "${bootstrap_dir}/bootstrap_bytes.hpp"
  COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/bootstrap.py" "${bootstrap_dir}"
  DEPENDS bootstrap.py VERBATIM)
add_executable(prism_execution_probe probe.cpp "${bootstrap_dir}/bootstrap_bytes.hpp")
target_include_directories(prism_execution_probe PRIVATE "${bootstrap_dir}")
target_link_libraries(prism_execution_probe PRIVATE prism::abi)
if(BUILD_TESTING)
  add_test(NAME prism.execution.bootstrap COMMAND prism_execution_probe "${bootstrap_dir}/bootstrap.exe")
  set_tests_properties(prism.execution.bootstrap PROPERTIES LABELS P01 TIMEOUT 10)
endif()
