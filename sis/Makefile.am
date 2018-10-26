SUBDIRS = include \
    array astg atpg avl bdd_cmu bdd_cudd bdd_ucb clock command decomp \
    delay doc enc error espresso extract factor gcd genlib graph graphics \
    io latch linsolv list lsort main map maxflow mincov minimize network \
    node ntbdd octio order phase pld power resub retime seqbdd sim \
    simplify sis_lib sparse speed st stg test timing util var_set

if SIS_COND_CMUBDD
SIS_BDD_LIBS= bdd_cmu/bdd_cmu/libbdd_cmu.a \
    bdd_cmu/bdd_port/libbdd_port.a \
    bdd_cmu/mem/libmem.a \
    st/libst.a
endif
if SIS_COND_UCBBDD
SIS_BDD_LIBS = bdd_ucb/libbdd_ucb.a \
    st/libst.a
endif
if SIS_COND_CUDD
SIS_BDD_LIBS = bdd_cudd/libbdd_cudd.a \
    bdd_cudd/libcudd_mtr.a \
    bdd_cudd/libcudd_st.a
endif

lib_LIBRARIES = libsis.a
libsis_a_SOURCES =
libsis_a_DEPENDENCIES = array/libarray.a \
    astg/libastg.a \
    atpg/libatpg.a \
    avl/libavl.a \
    clock/libclock.a \
    command/libcommand.a \
    decomp/libdecomp.a \
    delay/libdelay.a \
    enc/libenc.a \
    error/liberror.a \
    espresso/libespresso.a \
    extract/libextract.a \
    factor/libfactor.a \
    gcd/libgcd.a \
    genlib/libgenlib.a \
    graph/libgraph.a \
    graphics/libgraphics.a \
    io/libio.a \
    latch/liblatch.a \
    linsolv/liblinsolv.a \
    list/liblist.a \
    main/libmain.a \
    map/libmap.a \
    maxflow/libmaxflow.a \
    mincov/libmincov.a \
    minimize/libminimize.a \
    network/libnetwork.a \
    node/libnode.a \
    ntbdd/libntbdd.a \
    order/liborder.a \
    phase/libphase.a \
    pld/libpld.a \
    power/libpower.a \
    resub/libresub.a \
    retime/libretime.a \
    seqbdd/libseqbdd.a \
    sim/libsim.a \
    simplify/libsimplify.a \
    sparse/libsparse.a \
    speed/libspeed.a \
    stg/libstg.a \
    test/libtest.a \
    timing/libtiming.a \
    util/libutil.a \
    var_set/libvar_set.a \
    $(SIS_BDD_LIBS)

libsis.a: $(libsis_a_DEPENDENCIES)
	$(RM) $@
	for I in $^ ; do \
	  $(RM) -r workdir && mkdir workdir && cd workdir \
	  && $(AR) x ../$$I && $(AR) r ../$@ * \
	  && cd .. && $(RM) -r workdir ; \
	done
	$(RANLIB) $@

bin_PROGRAMS = sis
sis_SOURCES =
sis_LDADD = libsis.a -lm -lreadline
