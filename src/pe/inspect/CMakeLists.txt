add_library(prism_pe_inspect STATIC inspect.cpp)
add_library(prism::pe_inspect ALIAS prism_pe_inspect)
target_link_libraries(prism_pe_inspect PUBLIC prism::contracts)
target_compile_options(prism_pe_inspect PRIVATE -Wall -Wextra -Wpedantic)
