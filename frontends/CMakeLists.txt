# find each frontends
file(GLOB FRONTEND_DIRS LIST_DIRECTORIES true *)

foreach(dir ${FRONTEND_DIRS})
    if(IS_DIRECTORY ${dir})
        add_subdirectory(${dir})
    endif()
endforeach()
