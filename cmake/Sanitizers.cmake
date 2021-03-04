find_program(CLANG_APPLY_EXE NAMES "clang-apply-replacements" DOC "Path to clang-apply-replacements executable")
find_program(CLANG_TIDY_EXE NAMES "clang-tidy" DOC "Path to clang-tidy executable")

cmake_dependent_option(USE_CLANG_TIDY "Allow the codebase to use the clang-tidy sanitizer" ON "CLANG_TIDY_EXE" OFF)

if(CMAKE_GENERATOR MATCHES "[Mm][Aa][Kk][Ee][Ff][Ii][Ll][Ee]|[Nn][Ii][Nn][Jj][Aa]")
    set(CAN_RUN_FIXERS TRUE)
endif()

if(USE_CLANG_TIDY)
    set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY_EXE})
    if(CAN_RUN_FIXERS)
        set(_TIDY_REPLACEMENTS_DIR "${CMAKE_BINARY_DIR}/clang-tidy-replacements")
        add_custom_target(tidy_init
            COMMENT "Preparing to tidy..."
            COMMAND ${CMAKE_COMMAND} -E remove_directory "${_TIDY_REPLACEMENTS_DIR}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${_TIDY_REPLACEMENTS_DIR}"
        )
        add_custom_target(tidy DEPENDS tidy_init)
        if(CLANG_APPLY_EXE)
            add_custom_target(fix
                DEPENDS tidy
                COMMENT "Applying fixes..."
                COMMAND ${CLANG_APPLY_EXE} "${_TIDY_REPLACEMENTS_DIR}"
                WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
            )
        endif()

        # If we were on CMake 3.20, we could do this per-target...
        set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    endif()
endif()

function(plasma_sanitize_target TARGET)
    if(USE_CLANG_TIDY)
        # CMake only natively supports CXX_CLANG_TIDY with Unix Makefiles and Ninja generator as of
        # CMake 3.19. But most everyone is using Visual Studio, so we'll reset the code analysis
        # type to clang-tidy. This is a huge win given how terrible MS's code analysis is.
        if(MSVC)
            set_target_properties(
                ${TARGET} PROPERTIES
                VS_GLOBAL_ClangTidyToolExe "${CLANG_TIDY_EXE}"
                VS_GLOBAL_EnableClangTidyCodeAnalysis TRUE
                VS_GLOBAL_EnableMicrosoftCodeAnalysis FALSE
            )
        endif()

        if(CAN_RUN_FIXERS)
            get_target_property(_TARGET_SOURCES ${TARGET} SOURCES)
            get_target_property(_TARGET_SOURCE_DIR ${TARGET} SOURCE_DIR)
            foreach(_SOURCE_FILE IN LISTS _TARGET_SOURCES)
                list(APPEND _TIDY_SOURCES "${_TARGET_SOURCE_DIR}/${_SOURCE_FILE}")
            endforeach()

            set(_TARGET_FIXES_FILE "${_TIDY_REPLACEMENTS_DIR}/${TARGET}.yaml")
            add_custom_target(tidy_${TARGET}
                DEPENDS tidy_init
                COMMENT "Running clang-tidy on ${TARGET}..."
                COMMAND_EXPAND_LISTS
                COMMAND ${CLANG_TIDY_EXE} -p "${CMAKE_BINARY_DIR}" "-export-fixes=${_TARGET_FIXES_FILE}" "${_TIDY_SOURCES}"
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                BYPRODUCTS "${_TARGET_FIXES_FILE}"
            )
            add_dependencies(tidy tidy_${TARGET})
        endif()
    endif()
endfunction()
