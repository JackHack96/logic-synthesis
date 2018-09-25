string(TIMESTAMP TODAY "\"%d/%m/%Y\"")
set(SIS_VERSION "\"SIS 1.4.0\"")
set(CMAKE_C_FLAGS "-DSIS -DCUR_DATE='${TODAY}' -DLIBRARY='\"${CMAKE_SOURCE_DIR}/sis/sis_lib\"' -DPACKAGE_STRING='${SIS_VERSION}'")

add_library(main STATIC
        sis.c
        sis_glbs.c
        sis_init.c
        version.c
        )