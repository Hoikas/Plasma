set(pnInputCore_HEADERS
    plControlDefinition.h
    plControlEventCodes.h
    plInputMap.h
    plKeyDef.h
    plKeyMap.h
)

set(pnInputCore_SOURCES
    plInputMap.cpp
)

add_library(pnInputCore STATIC ${pnInputCore_HEADERS} ${pnInputCore_SOURCES})
target_link_libraries(
    pnInputCore
    PUBLIC
        CoreLib
    PRIVATE
        plResMgr # :(
)

target_include_directories(pnInputCore PRIVATE "${PLASMA_SOURCE_ROOT}/PubUtilLib")

plasma_sanitize_target(pnInputCore)

source_group("Header Files" FILES ${pnInputCore_HEADERS})
source_group("Source Files" FILES ${pnInputCore_SOURCES})
