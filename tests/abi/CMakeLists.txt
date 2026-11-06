enable_language(ASM)
add_executable(prism_abi_test abi.cpp registers.S)
target_link_libraries(prism_abi_test PRIVATE prism::abi)
if(BUILD_TESTING)
  find_package(Python3 REQUIRED COMPONENTS Interpreter)
  add_test(NAME prism.abi.boundaries COMMAND prism_abi_test)
  add_test(NAME prism.execution.reject COMMAND ${Python3_EXECUTABLE}
    "${CMAKE_CURRENT_SOURCE_DIR}/reject.py" $<TARGET_FILE:prism_execution_probe>
    "${PROJECT_BINARY_DIR}/components/experiments/execution/bootstrap/bootstrap.exe")
  set_tests_properties(prism.abi.boundaries prism.execution.reject PROPERTIES LABELS P01 TIMEOUT 15)
endif()
prism_add_component_check(P01 prism_abi_test prism_execution_probe)
