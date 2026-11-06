add_library(prism_contracts INTERFACE)
add_library(prism::contracts ALIAS prism_contracts)
target_include_directories(prism_contracts INTERFACE "${PROJECT_SOURCE_DIR}/include")
target_compile_features(prism_contracts INTERFACE cxx_std_20)
