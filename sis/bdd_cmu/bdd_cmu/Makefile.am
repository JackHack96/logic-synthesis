AM_CPPFLAGS = -I$(srcdir)/../mem

libbdd_cmu_a_SOURCES_local = bdd.c bddapply.c bddassoc.c bddblk.c bddcache.c \
    bddcmp.c bddcomp.c bddcproject.c bdddump.c bddhash.c bddint.h bddmisc.c \
    bddprimes.c bddprint.c bddprprofile.c bddqnt.c bddreduce.c bddrelprod.c \
    bddreorder.c bddsat.c bddsize.c bddsupport.c bddswap.c bddunique.c \
    bdduser.h bddwarn.c mtbdd.c

if SIS_COND_CMUBDD
noinst_LIBRARIES = libbdd_cmu.a
libbdd_cmu_a_SOURCES = $(libbdd_cmu_a_SOURCES_local)
dist_man3_MANS = bdd.3
endif

EXTRA_DIST = $(libbdd_cmu_a_SOURCES_local) bdd.3 testbdd.c
