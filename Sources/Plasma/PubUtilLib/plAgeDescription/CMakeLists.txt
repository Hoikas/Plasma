set(plAgeDescription_SOURCES
    plAgeDescription.cpp
)

set(plAgeDescription_HEADERS
    plAgeDescription.h
    plAgeManifest.h
)

add_library(plAgeDescription STATIC ${plAgeDescription_SOURCES} ${plAgeDescription_HEADERS})
target_link_libraries(
    plAgeDescription
    PUBLIC
        CoreLib
        plFile
        plUnifiedTime
    PRIVATE
        pnKeyedObject
)

plasma_sanitize_target(plAgeDescription)

source_group("Source Files" FILES ${plAgeDescription_SOURCES})
source_group("Header Files" FILES ${plAgeDescription_HEADERS})
