include_directories("../../CoreLib")
include_directories("../../NucleusLib/inc")
include_directories("../../NucleusLib")
include_directories("../../PubUtilLib")

set(plBullet_SOURCES
    plBTPhysical.cpp
    plBTPhysicalControllerCore.cpp
    plLOSDispatch.cpp
    plSimulationMgr.cpp
)

set(plBullet_HEADERS
    plBTPhysical.h
    plBTPhysicalControllerCore.h
    plBulletCreatable.h
    plLOSDispatch.h
    plSimulationMgr.h
)

add_library(plBullet STATIC ${plBullet_SOURCES} ${plBullet_HEADERS})

source_group("Source Files" FILES ${plBullet_SOURCES})
source_group("Header Files" FILES ${plBullet_HEADERS})