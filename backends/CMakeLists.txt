file(GLOB BACKEND_DIRS LIST_DIRECTORIES true *)

foreach(dir ${BACKEND_DIRS})
    if(IS_DIRECTORY ${dir})
        add_subdirectory(${dir})
    endif()
endforeach()
