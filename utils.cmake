
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
