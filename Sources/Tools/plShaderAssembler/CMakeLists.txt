set(plShaderAssembler_SOURCES
    main.cpp
)

add_executable(plShaderAssembler ${plShaderAssembler_SOURCES})
target_link_libraries(
    plShaderAssembler
    PRIVATE
        CoreLib
        ${DirectX_LIBRARIES}
)

plasma_sanitize_target(plShaderAssembler)

source_group("Source Files" FILES ${plShaderAssembler_SOURCES})
