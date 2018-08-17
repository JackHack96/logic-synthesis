# Generating the library libmincov.a
set(project "mincov")

set(${project}, "")
list(APPEND ${project}_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/bin_mincov.c
        ${CMAKE_CURRENT_SOURCE_DIR}/bin_sol.c
        ${CMAKE_CURRENT_SOURCE_DIR}/dominate.c
        ${CMAKE_CURRENT_SOURCE_DIR}/gimpel.c
        ${CMAKE_CURRENT_SOURCE_DIR}/indep.c
        ${CMAKE_CURRENT_SOURCE_DIR}/main.c
        ${CMAKE_CURRENT_SOURCE_DIR}/mincov.c
        ${CMAKE_CURRENT_SOURCE_DIR}/mincov_int.h
        ${CMAKE_CURRENT_SOURCE_DIR}/part.c
        ${CMAKE_CURRENT_SOURCE_DIR}/solution.c
        )
add_library ( mincov STATIC ${${project}_SOURCES} )