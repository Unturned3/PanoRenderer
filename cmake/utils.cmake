
function(append var value)
    set(${var} "${${var}} ${value}" PARENT_SCOPE)
endfunction()

function(prevent_in_src_build)
    if(CMAKE_HOME_DIRECTORY STREQUAL CMAKE_BINARY_DIR)
        message(FATAL_ERROR
            "In-source builds not allowed. Please run "
            "CMake from a separate build directory.")
    endif()
endfunction()

function(get_os_name var)
    set(${var} ${CMAKE_SYSTEM_NAME} PARENT_SCOPE)
endfunction()

function(get_linux_distro_name var)
    execute_process(
        COMMAND bash -c
        "cat /etc/os-arelease 2> /dev/null | grep '^ID=' | cut -d'=' -f 2"
        OUTPUT_VARIABLE tmp)
    if("${tmp}" STREQUAL "")
        set(tmp n/a)
    endif()
    set(${var} ${tmp} PARENT_SCOPE)
endfunction()
