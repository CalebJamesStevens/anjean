cmake_minimum_required(VERSION 3.20)

get_filename_component(PROJECT_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

if(NOT DEFINED RUN_CONFIG)
    set(RUN_CONFIG "dev")
endif()

if(NOT DEFINED RUN_ARGS)
    set(RUN_ARGS "")
endif()

set(EXE_CANDIDATES
    "${PROJECT_ROOT}/build/anjean"
    "${PROJECT_ROOT}/build/anjean.exe"
    "${PROJECT_ROOT}/build/Debug/anjean.exe"
    "${PROJECT_ROOT}/build/Release/anjean.exe"
)

set(EXE_PATH "")

foreach(candidate IN LISTS EXE_CANDIDATES)
    if(EXISTS "${candidate}")
        set(EXE_PATH "${candidate}")
        break()
    endif()
endforeach()

if(EXE_PATH STREQUAL "")
    message(FATAL_ERROR "Could not find built executable. Build first with: cmake --build --preset ${RUN_CONFIG}")
endif()

execute_process(
    COMMAND "${EXE_PATH}" ${RUN_ARGS}
    WORKING_DIRECTORY "${PROJECT_ROOT}"
    RESULT_VARIABLE EXIT_CODE
)

if(NOT EXIT_CODE EQUAL 0)
    message(FATAL_ERROR "Program exited with code ${EXIT_CODE}")
endif()