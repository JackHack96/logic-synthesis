# Generating the library libbdd_cudd.a
set(project "bdd_cudd")

set(${project}, "")
list(APPEND ${project}_SOURCES

        )
add_library(bdd_cudd STATIC ${${project}_SOURCES})
# Generating the library libcudd_mtr.a
set(project "cudd_mtr")

set(${project}, "")
list(APPEND ${project}_SOURCES

        )
add_library(cudd_mtr STATIC ${${project}_SOURCES})
# Generating the library libcudd_st.a
set(project "cudd_st")

set(${project}, "")
list(APPEND ${project}_SOURCES

        )
add_library(cudd_st STATIC ${${project}_SOURCES})