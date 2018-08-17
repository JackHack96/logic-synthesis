project(stamina)
set(project "stamina")

set(${project}, "")
list(APPEND ${project}_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/bcover.c
        ${CMAKE_CURRENT_SOURCE_DIR}/class.c
        ${CMAKE_CURRENT_SOURCE_DIR}/conf.c
        ${CMAKE_CURRENT_SOURCE_DIR}/disjoint.c
        ${CMAKE_CURRENT_SOURCE_DIR}/global.h
        ${CMAKE_CURRENT_SOURCE_DIR}/install_state.c
        ${CMAKE_CURRENT_SOURCE_DIR}/iso.c
        ${CMAKE_CURRENT_SOURCE_DIR}/main.c
        ${CMAKE_CURRENT_SOURCE_DIR}/map.c
        ${CMAKE_CURRENT_SOURCE_DIR}/max.c
        ${CMAKE_CURRENT_SOURCE_DIR}/max.h
        ${CMAKE_CURRENT_SOURCE_DIR}/merge.c
        ${CMAKE_CURRENT_SOURCE_DIR}/merge.h
        ${CMAKE_CURRENT_SOURCE_DIR}/misc.c
        ${CMAKE_CURRENT_SOURCE_DIR}/prime.c
        ${CMAKE_CURRENT_SOURCE_DIR}/read_fsm.c
        ${CMAKE_CURRENT_SOURCE_DIR}/stack.c
        ${CMAKE_CURRENT_SOURCE_DIR}/stack.h
        ${CMAKE_CURRENT_SOURCE_DIR}/struct.h
        ${CMAKE_CURRENT_SOURCE_DIR}/user.h
        )
add_executable(stamina ${${project}_SOURCES})