add_library(prism_kernel32_core core.cpp)
add_library(prism::kernel32_core ALIAS prism_kernel32_core)
target_link_libraries(prism_kernel32_core PUBLIC prism::runtime)
target_compile_options(prism_kernel32_core PRIVATE -Wall -Wextra -Wpedantic)
