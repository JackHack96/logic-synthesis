project(jedi)

add_executable(jedi
        cluster.c
        expand.c
        getopt.c
        hill.c
        hot.c
        jedi.c


        options.c
        random.c
        read.c
        rp.c


        util.c

        weights.c
        write.c
        )
target_link_libraries(jedi utility m)