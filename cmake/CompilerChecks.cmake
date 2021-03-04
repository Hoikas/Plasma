# Detect Clang compiler
if("${CMAKE_CXX_COMPILER_ID}" MATCHES ".*Clang")
    set(CMAKE_COMPILER_IS_CLANGXX 1)
endif()

# MSVC automatically defines -D_DEBUG when /MTd or /MDd is set, so we
# need to make sure it gets added for other compilers too
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANGXX)
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -D_DEBUG")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")
endif()

# Use LTCG if available.
cmake_policy(SET CMP0069 NEW)   # gtest projects use old cmake compatibility...
if(NOT DEFINED CMAKE_INTERPROCEDURAL_OPTIMIZATION)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT _IPO_SUPPORTED LANGUAGES CXX OUTPUT _IPO_OUTPUT)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ${_IPO_SUPPORTED} CACHE BOOL "Use link-time optimizations")
    if(_IPO_SUPPORTED)
        message(STATUS "IPO supported: using link-time optimizations")
    else()
        message(STATUS "IPO not supported: ${_IPO_OUTPUT}")
    endif()
endif()

# Check for SIMD headers
include(CheckIncludeFile)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    CHECK_INCLUDE_FILE("cpuid.h" HAVE_CPUID)
else()
    CHECK_INCLUDE_FILE("intrin.h" HAVE_CPUID)
endif()

CHECK_INCLUDE_FILE("immintrin.h" HS_AVX2)
CHECK_INCLUDE_FILE("immintrin.h" HS_AVX)
CHECK_INCLUDE_FILE("nmmintrin.h" HS_SSE42)
CHECK_INCLUDE_FILE("smmintrin.h" HS_SSE41)
CHECK_INCLUDE_FILE("tmmintrin.h" HS_SSE4)
CHECK_INCLUDE_FILE("pmmintrin.h" HS_SSE3)
CHECK_INCLUDE_FILE("emmintrin.h" HS_SSE2)
CHECK_INCLUDE_FILE("xmmintrin.h" HS_SSE1)

# GCC requires us to set the -m<instructionset> flag for the source file using the intrinsics.
# We can't do that project-wide or we'll just crash on launch with an illegal instruction on some
# systems. So, we have another helper method...
function(plasma_target_simd_sources TARGET)
    set(_INSTRUCTION_SETS "SSE1;SSE2;SSE3;SSE4;SSE41;SSE42;AVX;AVX2")
    set(_GCC_ARGS "-msse;-msse2;-msse3;-msse4;-msse4.1;-msse4.2;-mavx;-mavx2")
    cmake_parse_arguments(_passf "" "SOURCE_GROUP" "${_INSTRUCTION_SETS}" ${ARGN})

    # Hack: if we ever bump to CMake 3.17, use ZIP_LISTS. Currently, Ubuntu 20.04 is on CMake 3.15.
    list(LENGTH _INSTRUCTION_SETS _LENGTH)
    math(EXPR _LENGTH "${_LENGTH} - 1")

    foreach(i RANGE ${_LENGTH})
        # foreach(i IN ZIP_LISTS _INSTRUCTION_SETS _GCC_ARGS)
        list(GET _INSTRUCTION_SETS ${i} i_0)
        list(GET _GCC_ARGS ${i} i_1)

        set(_CURRENT_SOURCE_FILES ${_passf_${i_0}})
        if(_CURRENT_SOURCE_FILES AND HS_${i_0})
            target_sources(${TARGET} PRIVATE ${_CURRENT_SOURCE_FILES})
            if(_passf_SOURCE_GROUP)
                source_group(${_passf_SOURCE_GROUP} FILES ${_CURRENT_SOURCE_FILES})
            endif()
            if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
                set_source_files_properties(
                    ${_CURRENT_SOURCE_FILES} PROPERTIES
                    COMPILE_OPTIONS ${i_1}
                )
            endif()
        endif()
    endforeach()
endfunction()
