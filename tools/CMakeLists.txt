add_subdirectory(isa_dump)
add_subdirectory(identify_opcode)

set(MAVIS_TOOLS_INSTALL_TARGETS
    isa_dump
    identify_opcode
)

include(mavis_extra_tools.cmake OPTIONAL)

if(MAVIS_TOOLS_INSTALL_DIR)
    install(TARGETS ${MAVIS_TOOLS_INSTALL_TARGETS}
            DESTINATION ${MAVIS_TOOLS_INSTALL_DIR})
endif()
