include_directories("../../CoreLib")
include_directories("../../FeatureLib")
include_directories("../../NucleusLib/inc")
include_directories("../../NucleusLib")
include_directories("../../PubUtilLib")

set(plVault_SOURCES
    plDniCoordinateInfo.cpp
    plVaultConstants.cpp
)

set(plVault_HEADERS
    plDniCoordinateInfo.h
    plVault.h
    plVaultConstants.h
    plVaultCreatable.h
)

add_library(plVault STATIC ${plVault_SOURCES} ${plVault_HEADERS})

source_group("Source Files" FILES ${plVault_SOURCES})
source_group("Header Files" FILES ${plVault_HEADERS})
