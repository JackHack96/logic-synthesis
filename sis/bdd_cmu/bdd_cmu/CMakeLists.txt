# Generating the library libbdd_cmu.a
set(project "bdd_cmu")

set(${project}, "")
list(APPEND ${project}_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/bdd.c
        ${CMAKE_CURRENT_SOURCE_DIR}/bddapply.c
        ${CMAKE_CURRENT_SOURCE_DIR}/bddassoc.c
        ${CMAKE_CURRENT_SOURCE_DIR}/bddblk.c
        ${CMAKE_CURRENT_SOURCE_DIR}/bddcache.c
        ${CMAKE_CURRENT_SOURCE_DIR}/bddcmp.c
        ${CMAKE_CURRENT_SOURCE_DIR}/bddcomp.c
        ${CMAKE_CURRENT_SOURCE_DIR}/bddcproject.c
        ${CMAKE_CURRENT_SOURCE_DIR}/bdddump.c
        ${CMAKE_CURRENT_SOURCE_DIR}/bddhash.c
        ${CMAKE_CURRENT_SOURCE_DIR}/bddint.h
        ${CMAKE_CURRENT_SOURCE_DIR}/bddmisc.c
        ${CMAKE_CURRENT_SOURCE_DIR}/bddprimes.c
        ${CMAKE_CURRENT_SOURCE_DIR}/bddprint.c
        ${CMAKE_CURRENT_SOURCE_DIR}/bddprprofile.c
        ${CMAKE_CURRENT_SOURCE_DIR}/bddqnt.c
        ${CMAKE_CURRENT_SOURCE_DIR}/bddreduce.c
        ${CMAKE_CURRENT_SOURCE_DIR}/bddrelprod.c
        ${CMAKE_CURRENT_SOURCE_DIR}/bddreorder.c
        ${CMAKE_CURRENT_SOURCE_DIR}/bddsat.c
        ${CMAKE_CURRENT_SOURCE_DIR}/bddsize.c
        ${CMAKE_CURRENT_SOURCE_DIR}/bddsupport.c
        ${CMAKE_CURRENT_SOURCE_DIR}/bddswap.c
        ${CMAKE_CURRENT_SOURCE_DIR}/bddunique.c
        ${CMAKE_CURRENT_SOURCE_DIR}/bdduser.h
        ${CMAKE_CURRENT_SOURCE_DIR}/bddwarn.c
        ${CMAKE_CURRENT_SOURCE_DIR}/mtbdd.c
        )

add_library(bdd_cmu STATIC ${${project}_SOURCES})