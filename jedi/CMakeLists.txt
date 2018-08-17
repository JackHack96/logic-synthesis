project(jedi)

add_executable(jedi
        cluster.c
        expand.c
        getopt.c
        hill.c
        hot.c
        jedi.c
        inc/jedi.h
        inc/jedi_int.h
        options.c
        random.c
        read.c
        rp.c
        inc/rp.h
        inc/rp_int.h
        util.c
        inc/util.h
        weights.c
        write.c
        )
target_link_libraries(jedi utility m)