if(BUILD_TESTING)
  # Each public header must compile independently (no hidden include order).
  foreach(header core abi pe backend)
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/${header}_header.cpp" "#include <prism/${header}.hpp>\n")
    add_library(prism_${header}_header OBJECT "${CMAKE_CURRENT_BINARY_DIR}/${header}_header.cpp")
    target_link_libraries(prism_${header}_header PRIVATE prism::contracts)
  endforeach()
  add_executable(prism_contract_check contracts.cpp)
  target_link_libraries(prism_contract_check PRIVATE prism::contracts)
  add_test(NAME prism.contracts COMMAND prism_contract_check)
  set_tests_properties(prism.contracts PROPERTIES LABELS P00)
  add_custom_target(check-p00
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --no-tests=error -L "^P00$"
    WORKING_DIRECTORY "${PROJECT_BINARY_DIR}" DEPENDS prism_contract_check
    prism_core_header prism_abi_header prism_pe_header prism_backend_header)
endif()
