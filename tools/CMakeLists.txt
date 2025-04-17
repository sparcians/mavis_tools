add_subdirectory(isa_dump)

set(MAVIS_TOOLS_INSTALL_TARGETS
    isa_dump
)

include(mavis_extra_tools.cmake OPTIONAL)

if(MAVIS_TOOLS_INSTALL_DIR)
    install(TARGETS ${MAVIS_TOOLS_INSTALL_TARGETS}
            DESTINATION ${MAVIS_TOOLS_INSTALL_DIR})
endif()
