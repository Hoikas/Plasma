include_directories(../../CoreLib)
include_directories(../../NucleusLib)
include_directories(../../NucleusLib/inc)
include_directories(../../PubUtilLib)
include_directories(../../FeatureLib)

set(pfVoIP_SOURCES
    pfMumbleLink.cpp
)

set(pfVoIP_HEADERS
    pfMumbleLink.h
    pfVoIPCreatable.h
)

add_library(pfVoIP STATIC ${pfVoIP_SOURCES} ${pfVoIP_HEADERS})

source_group("Source Files" FILES ${pfVoIP_SOURCES})
source_group("Header Files" FILES ${pfVoIP_HEADERS})
