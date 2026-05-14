project(identify_opcode)
add_executable(identify_opcode main.cpp)
target_link_libraries (identify_opcode mavis Boost::program_options Boost::json)
