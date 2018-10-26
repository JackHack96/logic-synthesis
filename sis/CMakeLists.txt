project(sis)

include_directories(include)

add_subdirectory(array)
add_subdirectory(astg)
add_subdirectory(atpg)
add_subdirectory(avl)
add_subdirectory(bdd_cmu)
add_subdirectory(clock)
add_subdirectory(command)
add_subdirectory(decomp)
add_subdirectory(delay)
add_subdirectory(enc)
add_subdirectory(error)
add_subdirectory(espresso)
add_subdirectory(extract)
add_subdirectory(factor)
add_subdirectory(gcd)
add_subdirectory(genlib)
add_subdirectory(graph)
add_subdirectory(graphics)
add_subdirectory(io)
add_subdirectory(latch)
add_subdirectory(linsolv)
add_subdirectory(list)
add_subdirectory(main)
add_subdirectory(map)
add_subdirectory(maxflow)
add_subdirectory(mincov)
add_subdirectory(minimize)
add_subdirectory(network)
add_subdirectory(node)
add_subdirectory(ntbdd)
add_subdirectory(octio)
add_subdirectory(order)
add_subdirectory(phase)
add_subdirectory(pld)
add_subdirectory(power)
add_subdirectory(resub)
add_subdirectory(retime)
add_subdirectory(seqbdd)
add_subdirectory(sim)
add_subdirectory(simplify)
add_subdirectory(sparse)
add_subdirectory(speed)
add_subdirectory(st)
add_subdirectory(stg)
add_subdirectory(test)
add_subdirectory(timing)
add_subdirectory(util)
add_subdirectory(var_set)

set(LIBSIS_OUTPUT ${CMAKE_BINARY_DIR}/sis/libsis.a)

set(OPREFIX object_)
add_custom_target(libsis_custom
        COMMAND mkdir ${OPREFIX}$<TARGET_NAME:array> || true && cd ${OPREFIX}$<TARGET_NAME:array> && ar -x $<TARGET_FILE:array>
        COMMAND mkdir ${OPREFIX}$<TARGET_NAME:astg> || true && cd ${OPREFIX}$<TARGET_NAME:astg> && ar -x $<TARGET_FILE:astg>
        COMMAND mkdir ${OPREFIX}$<TARGET_NAME:atpg> || true && cd ${OPREFIX}$<TARGET_NAME:atpg> && ar -x $<TARGET_FILE:atpg>
        COMMAND mkdir ${OPREFIX}$<TARGET_NAME:avl> || true && cd ${OPREFIX}$<TARGET_NAME:avl> && ar -x $<TARGET_FILE:avl>
        COMMAND mkdir ${OPREFIX}$<TARGET_NAME:bdd_cmu> || true && cd ${OPREFIX}$<TARGET_NAME:bdd_cmu> && ar -x $<TARGET_FILE:bdd_cmu>
        COMMAND mkdir ${OPREFIX}$<TARGET_NAME:clock> || true && cd ${OPREFIX}$<TARGET_NAME:clock> && ar -x $<TARGET_FILE:clock>
        COMMAND mkdir ${OPREFIX}$<TARGET_NAME:command> || true && cd ${OPREFIX}$<TARGET_NAME:command> && ar -x $<TARGET_FILE:command>
        COMMAND mkdir ${OPREFIX}$<TARGET_NAME:decomp> || true && cd ${OPREFIX}$<TARGET_NAME:decomp> && ar -x $<TARGET_FILE:decomp>
        COMMAND mkdir ${OPREFIX}$<TARGET_NAME:delay> || true && cd ${OPREFIX}$<TARGET_NAME:delay> && ar -x $<TARGET_FILE:delay>
        COMMAND mkdir ${OPREFIX}$<TARGET_NAME:enc> || true && cd ${OPREFIX}$<TARGET_NAME:enc> && ar -x $<TARGET_FILE:enc>
        COMMAND mkdir ${OPREFIX}$<TARGET_NAME:error> || true && cd ${OPREFIX}$<TARGET_NAME:error> && ar -x $<TARGET_FILE:error>
        COMMAND mkdir ${OPREFIX}$<TARGET_NAME:espresso_sis> || true && cd ${OPREFIX}$<TARGET_NAME:espresso_sis> && ar -x $<TARGET_FILE:espresso_sis>
        COMMAND mkdir ${OPREFIX}$<TARGET_NAME:extract> || true && cd ${OPREFIX}$<TARGET_NAME:extract> && ar -x $<TARGET_FILE:extract>
        COMMAND mkdir ${OPREFIX}$<TARGET_NAME:factor> || true && cd ${OPREFIX}$<TARGET_NAME:factor> && ar -x $<TARGET_FILE:factor>
        COMMAND mkdir ${OPREFIX}$<TARGET_NAME:gcd> || true && cd ${OPREFIX}$<TARGET_NAME:gcd> && ar -x $<TARGET_FILE:gcd>
        COMMAND mkdir ${OPREFIX}$<TARGET_NAME:genlib> || true && cd ${OPREFIX}$<TARGET_NAME:genlib> && ar -x $<TARGET_FILE:genlib>
        COMMAND mkdir ${OPREFIX}$<TARGET_NAME:graph> || true && cd ${OPREFIX}$<TARGET_NAME:graph> && ar -x $<TARGET_FILE:graph>
        COMMAND mkdir ${OPREFIX}$<TARGET_NAME:graphics> || true && cd ${OPREFIX}$<TARGET_NAME:graphics> && ar -x $<TARGET_FILE:graphics>
        COMMAND mkdir ${OPREFIX}$<TARGET_NAME:io> || true && cd ${OPREFIX}$<TARGET_NAME:io> && ar -x $<TARGET_FILE:io>
        COMMAND mkdir ${OPREFIX}$<TARGET_NAME:latch> || true && cd ${OPREFIX}$<TARGET_NAME:latch> && ar -x $<TARGET_FILE:latch>
        COMMAND mkdir ${OPREFIX}$<TARGET_NAME:linsolv> || true && cd ${OPREFIX}$<TARGET_NAME:linsolv> && ar -x $<TARGET_FILE:linsolv>
        COMMAND mkdir ${OPREFIX}$<TARGET_NAME:list> || true && cd ${OPREFIX}$<TARGET_NAME:list> && ar -x $<TARGET_FILE:list>
        COMMAND mkdir ${OPREFIX}$<TARGET_NAME:main> || true && cd ${OPREFIX}$<TARGET_NAME:main> && ar -x $<TARGET_FILE:main>
        COMMAND mkdir ${OPREFIX}$<TARGET_NAME:map> || true && cd ${OPREFIX}$<TARGET_NAME:map> && ar -x $<TARGET_FILE:map>
        COMMAND mkdir ${OPREFIX}$<TARGET_NAME:maxflow> || true && cd ${OPREFIX}$<TARGET_NAME:maxflow> && ar -x $<TARGET_FILE:maxflow>
        COMMAND mkdir ${OPREFIX}$<TARGET_NAME:mincov> || true && cd ${OPREFIX}$<TARGET_NAME:mincov> && ar -x $<TARGET_FILE:mincov>
        COMMAND mkdir ${OPREFIX}$<TARGET_NAME:minimize> || true && cd ${OPREFIX}$<TARGET_NAME:minimize> && ar -x $<TARGET_FILE:minimize>
        COMMAND mkdir ${OPREFIX}$<TARGET_NAME:network> || true && cd ${OPREFIX}$<TARGET_NAME:network> && ar -x $<TARGET_FILE:network>
        COMMAND mkdir ${OPREFIX}$<TARGET_NAME:node> || true && cd ${OPREFIX}$<TARGET_NAME:node> && ar -x $<TARGET_FILE:node>
        COMMAND mkdir ${OPREFIX}$<TARGET_NAME:ntbdd> || true && cd ${OPREFIX}$<TARGET_NAME:ntbdd> && ar -x $<TARGET_FILE:ntbdd>
        COMMAND mkdir ${OPREFIX}$<TARGET_NAME:octio> || true && cd ${OPREFIX}$<TARGET_NAME:octio> && ar -x $<TARGET_FILE:octio>
        COMMAND mkdir ${OPREFIX}$<TARGET_NAME:order> || true && cd ${OPREFIX}$<TARGET_NAME:order> && ar -x $<TARGET_FILE:order>
        COMMAND mkdir ${OPREFIX}$<TARGET_NAME:phase> || true && cd ${OPREFIX}$<TARGET_NAME:phase> && ar -x $<TARGET_FILE:phase>
        COMMAND mkdir ${OPREFIX}$<TARGET_NAME:pld> || true && cd ${OPREFIX}$<TARGET_NAME:pld> && ar -x $<TARGET_FILE:pld>
        COMMAND mkdir ${OPREFIX}$<TARGET_NAME:power> || true && cd ${OPREFIX}$<TARGET_NAME:power> && ar -x $<TARGET_FILE:power>
        COMMAND mkdir ${OPREFIX}$<TARGET_NAME:resub> || true && cd ${OPREFIX}$<TARGET_NAME:resub> && ar -x $<TARGET_FILE:resub>
        COMMAND mkdir ${OPREFIX}$<TARGET_NAME:retime> || true && cd ${OPREFIX}$<TARGET_NAME:retime> && ar -x $<TARGET_FILE:retime>
        COMMAND mkdir ${OPREFIX}$<TARGET_NAME:seqbdd> || true && cd ${OPREFIX}$<TARGET_NAME:seqbdd> && ar -x $<TARGET_FILE:seqbdd>
        COMMAND mkdir ${OPREFIX}$<TARGET_NAME:sim> || true && cd ${OPREFIX}$<TARGET_NAME:sim> && ar -x $<TARGET_FILE:sim>
        COMMAND mkdir ${OPREFIX}$<TARGET_NAME:simplify> || true && cd ${OPREFIX}$<TARGET_NAME:simplify> && ar -x $<TARGET_FILE:simplify>
        COMMAND mkdir ${OPREFIX}$<TARGET_NAME:sparse> || true && cd ${OPREFIX}$<TARGET_NAME:sparse> && ar -x $<TARGET_FILE:sparse>
        COMMAND mkdir ${OPREFIX}$<TARGET_NAME:speed> || true && cd ${OPREFIX}$<TARGET_NAME:speed> && ar -x $<TARGET_FILE:speed>
        COMMAND mkdir ${OPREFIX}$<TARGET_NAME:st> || true && cd ${OPREFIX}$<TARGET_NAME:st> && ar -x $<TARGET_FILE:st>
        COMMAND mkdir ${OPREFIX}$<TARGET_NAME:stg> || true && cd ${OPREFIX}$<TARGET_NAME:stg> && ar -x $<TARGET_FILE:stg>
        COMMAND mkdir ${OPREFIX}$<TARGET_NAME:test_sis> || true && cd ${OPREFIX}$<TARGET_NAME:test_sis> && ar -x $<TARGET_FILE:test_sis>
        COMMAND mkdir ${OPREFIX}$<TARGET_NAME:timing> || true && cd ${OPREFIX}$<TARGET_NAME:timing> && ar -x $<TARGET_FILE:timing>
        COMMAND mkdir ${OPREFIX}$<TARGET_NAME:util> || true && cd ${OPREFIX}$<TARGET_NAME:util> && ar -x $<TARGET_FILE:util>
        COMMAND mkdir ${OPREFIX}$<TARGET_NAME:var_set> || true && cd ${OPREFIX}$<TARGET_NAME:var_set> && ar -x $<TARGET_FILE:var_set>
        COMMAND ar -rs ${LIBSIS_OUTPUT} ${OPREFIX}*/*.o
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        DEPENDS array
        astg
        atpg
        avl
        bdd_cmu
        clock
        command
        decomp
        delay
        enc
        error
        espresso
        extract
        factor
        gcd
        genlib
        graph
        graphics
        io
        latch
        linsolv
        list
        main
        map
        maxflow
        mincov
        minimize
        network
        node
        ntbdd
        octio
        order
        phase
        pld
        power
        resub
        retime
        seqbdd
        sim
        simplify
        sparse
        speed
        st
        stg
        test
        timing
        util
        var_set
        )
add_library(libsis STATIC IMPORTED GLOBAL)
add_dependencies(libsis libsis_custom)
set_target_properties(libsis PROPERTIES IMPORTED_LOCATION ${LIBSIS_OUTPUT})

add_executable(sis main/sis.c)
target_link_libraries(sis libsis)