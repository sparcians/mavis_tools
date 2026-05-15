project(isa_dump)
add_executable(isa_dump main.cpp)
target_link_libraries (isa_dump mavis Boost::program_options)
