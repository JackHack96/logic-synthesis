AM_CPPFLAGS = -I../include

libbdd_cudd_a_SOURCES_local = \
    cuddAPI.c \
    cuddAddAbs.c \
    cuddAddApply.c \
    cuddAddFind.c \
    cuddAddInv.c \
    cuddAddIte.c \
    cuddAddNeg.c \
    cuddAddWalsh.c \
    cuddAndAbs.c \
    cuddAnneal.c \
    cuddApa.c \
    cuddApprox.c \
    cuddBddAbs.c \
    cuddBddCorr.c \
    cuddBddIte.c \
    cuddBridge.c \
    cuddCache.c \
    cuddCheck.c \
    cuddClip.c \
    cuddCof.c \
    cuddCompose.c \
    cuddDecomp.c \
    cuddEssent.c \
    cuddExact.c \
    cuddExport.c \
    cuddGenCof.c \
    cuddGenetic.c \
    cuddGroup.c \
    cuddHarwell.c \
    cuddInit.c \
    cuddInteract.c \
    cuddLCache.c \
    cuddLevelQ.c \
    cuddLinear.c \
    cuddLiteral.c \
    cuddMatMult.c \
    cuddPriority.c \
    cuddRead.c \
    cuddRef.c \
    cuddReorder.c \
    cuddSat.c \
    cuddSign.c \
    cuddSolve.c \
    cuddSplit.c \
    cuddSubsetHB.c \
    cuddSubsetSP.c \
    cuddSymmetry.c \
    cuddTable.c \
    cuddUtil.c \
    cuddWindow.c \
    cuddZddCount.c \
    cuddZddFuncs.c \
    cuddZddGroup.c \
    cuddZddIsop.c \
    cuddZddLin.c \
    cuddZddMisc.c \
    cuddZddPort.c \
    cuddZddReord.c \
    cuddZddSetop.c \
    cuddZddSymm.c \
    cuddZddUtil.c \
    cudd.h \
    cuddInt.h \
    cuddBddPort.c \
    cuddPwPt.c \
    cuddBdd.h \
    epd.c \
    epd.h \
    datalimit.c 

libcudd_mtr_a_SOURCES_local = \
    mtrBasic.c \
    mtrGroup.c \
    mtr.h \
    mtrInt.h

libcudd_st_a_SOURCES_local = \
    st.c \
    st.h

if SIS_COND_CUDD

BUILT_SOURCES = $(libbdd_cudd_a_SOURCES_local) bdd.h \
		$(libcudd_mtr_a_SOURCES_local) \
		$(libcudd_st_a_SOURCES_local)

install: $(BUILT_SOURCES)
CLEANFILES = $(BUILT_SOURCES)

noinst_LIBRARIES = libbdd_cudd.a libcudd_mtr.a libcudd_st.a
nodist_libbdd_cudd_a_SOURCES = $(libbdd_cudd_a_SOURCES_local)
nodist_libcudd_mtr_a_SOURCES = $(libcudd_mtr_a_SOURCES_local)
nodist_libcudd_st_a_SOURCES = $(libcudd_st_a_SOURCES_local)
nodist_pkginclude_HEADERS = bdd.h mtr.h st.h

%.c: @SIS_CUDDDIR@/cudd/%.c
	$(RM) $@
	$(LN_S) $< $@

%.h: @SIS_CUDDDIR@/cudd/%.h
	$(RM) $@
	$(LN_S) $< $@

%.c: @SIS_CUDDDIR@/epd/%.c
	$(RM) $@
	$(LN_S) $< $@

%.h: @SIS_CUDDDIR@/epd/%.h
	$(RM) $@
	$(LN_S) $< $@

%.c: @SIS_CUDDDIR@/mtr/%.c
	$(RM) $@
	$(LN_S) $< $@

%.h: @SIS_CUDDDIR@/mtr/%.h
	$(RM) $@
	$(LN_S) $< $@

%.c: @SIS_CUDDDIR@/sis/%.c
	$(RM) $@
	$(LN_S) $< $@

%.h: @SIS_CUDDDIR@/sis/%.h
	$(RM) $@
	$(LN_S) $< $@

%.c: @SIS_CUDDDIR@/util/%.c
	$(RM) $@
	$(LN_S) $< $@

%.h: @SIS_CUDDDIR@/util/%.h
	$(RM) $@
	$(LN_S) $< $@

bdd.h: cuddBdd.h
	$(RM) $@
	$(LN_S) $< $@

endif
