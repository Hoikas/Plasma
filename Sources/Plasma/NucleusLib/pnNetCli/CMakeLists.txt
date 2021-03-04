set(pnNetCli_HEADERS
    Intern.h
    Pch.h
    pnNetCli.h
)

set(pnNetCli_SOURCES
    pnNcChannel.cpp
    pnNcCli.cpp
    pnNcEncrypt.cpp
    pnNcUtils.cpp
)

add_library(pnNetCli STATIC ${pnNetCli_HEADERS} ${pnNetCli_SOURCES})
target_link_libraries(
    pnNetCli
    PUBLIC
        CoreLib
        pnEncryption
    PRIVATE
        pnAsyncCore
        pnNetBase
        pnUtils
        pnUUID
)

plasma_sanitize_target(pnNetCli)

source_group("Header Files" FILES ${pnNetCli_HEADERS})
source_group("Source Files" FILES ${pnNetCli_SOURCES})
