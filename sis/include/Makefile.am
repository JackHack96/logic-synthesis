BUILT_SOURCES = array.h astg.h atpg.h sat.h avl.h bdd.h clock.h command.h \
    decomp.h delay.h enc.h error.h espresso.h extract.h factor.h gcd.h \
    graph.h graph_static.h graphics.h io.h latch.h library.h spMatrix.h \
    spConfig.h list.h lsort.h luniq.h map.h maxflow.h mincov.h minimize.h \
    network.h node.h nodeindex.h ntbdd.h order.h phase.h pld.h \
    power.h resub.h retime.h seqbdd.h simplify.h sis.h sparse.h speed.h \
    st.h stg.h util.h ansi.h var_set.h

install: $(BUILT_SOURCES)
CLEANFILES = $(BUILT_SOURCES)

if SIS_COND_CMUBDD
bdd.h: $(top_srcdir)/sis/bdd_cmu/bdd.h
	$(RM) $@
	$(LN_S) $< $@
endif

if SIS_COND_UCBBDD
bdd.h: $(top_srcdir)/sis/bdd_ucb/bdd.h
	$(RM) $@
	$(LN_S) $< $@
endif

if SIS_COND_CUDD
bdd.h: ../bdd_cudd/bdd.h
	$(RM) $@
	$(LN_S) $< $@

../bdd_cudd/bdd.h:
	cd ../bdd_cudd && $(MAKE) bdd.h

st.h: ../bdd_cudd/st.h
	$(RM) $@
	$(LN_S) $< $@

../bdd_cudd/st.h:
	cd ../bdd_cudd && $(MAKE) st.h
else
st.h: $(top_srcdir)/sis/st/st.h
	$(RM) $@
	$(LN_S) $< $@
endif

array.h: $(top_srcdir)/sis/array/array.h
	$(RM) $@
	$(LN_S) $< $@

astg.h: $(top_srcdir)/sis/astg/astg.h
	$(RM) $@
	$(LN_S) $< $@

atpg.h: $(top_srcdir)/sis/atpg/atpg.h
	$(RM) $@
	$(LN_S) $< $@

sat.h: $(top_srcdir)/sis/atpg/sat.h
	$(RM) $@
	$(LN_S) $< $@

avl.h: $(top_srcdir)/sis/avl/avl.h
	$(RM) $@
	$(LN_S) $< $@

clock.h: $(top_srcdir)/sis/clock/clock.h
	$(RM) $@
	$(LN_S) $< $@

command.h: $(top_srcdir)/sis/command/command.h
	$(RM) $@
	$(LN_S) $< $@

decomp.h: $(top_srcdir)/sis/decomp/decomp.h
	$(RM) $@
	$(LN_S) $< $@

delay.h: $(top_srcdir)/sis/delay/delay.h
	$(RM) $@
	$(LN_S) $< $@

enc.h: $(top_srcdir)/sis/enc/enc.h
	$(RM) $@
	$(LN_S) $< $@

error.h: $(top_srcdir)/sis/error/error.h
	$(RM) $@
	$(LN_S) $< $@

espresso.h: $(top_srcdir)/sis/espresso/espresso.h
	$(RM) $@
	$(LN_S) $< $@

extract.h: $(top_srcdir)/sis/extract/extract.h
	$(RM) $@
	$(LN_S) $< $@

factor.h: $(top_srcdir)/sis/factor/factor.h
	$(RM) $@
	$(LN_S) $< $@

gcd.h: $(top_srcdir)/sis/gcd/gcd.h
	$(RM) $@
	$(LN_S) $< $@

graph.h: $(top_srcdir)/sis/graph/graph.h
	$(RM) $@
	$(LN_S) $< $@

graph_static.h: $(top_srcdir)/sis/graph/graph_static.h
	$(RM) $@
	$(LN_S) $< $@

graphics.h: $(top_srcdir)/sis/graphics/graphics.h
	$(RM) $@
	$(LN_S) $< $@

io.h: $(top_srcdir)/sis/io/io.h
	$(RM) $@
	$(LN_S) $< $@

latch.h: $(top_srcdir)/sis/latch/latch.h
	$(RM) $@
	$(LN_S) $< $@

spMatrix.h: $(top_srcdir)/sis/linsolv/spMatrix.h
	$(RM) $@
	$(LN_S) $< $@

spConfig.h: $(top_srcdir)/sis/linsolv/spConfig.h
	$(RM) $@
	$(LN_S) $< $@

list.h: $(top_srcdir)/sis/list/list.h
	$(RM) $@
	$(LN_S) $< $@

lsort.h: $(top_srcdir)/sis/lsort/lsort.h
	$(RM) $@
	$(LN_S) $< $@

luniq.h: $(top_srcdir)/sis/lsort/luniq.h
	$(RM) $@
	$(LN_S) $< $@

library.h: $(top_srcdir)/sis/map/library.h
	$(RM) $@
	$(LN_S) $< $@

map.h: $(top_srcdir)/sis/map/map.h
	$(RM) $@
	$(LN_S) $< $@

maxflow.h: $(top_srcdir)/sis/maxflow/maxflow.h
	$(RM) $@
	$(LN_S) $< $@

mincov.h: $(top_srcdir)/sis/mincov/mincov.h
	$(RM) $@
	$(LN_S) $< $@

minimize.h: $(top_srcdir)/sis/minimize/minimize.h
	$(RM) $@
	$(LN_S) $< $@

network.h: $(top_srcdir)/sis/network/network.h
	$(RM) $@
	$(LN_S) $< $@

node.h: $(top_srcdir)/sis/node/node.h
	$(RM) $@
	$(LN_S) $< $@

nodeindex.h: $(top_srcdir)/sis/node/nodeindex.h
	$(RM) $@
	$(LN_S) $< $@

ntbdd.h: $(top_srcdir)/sis/ntbdd/ntbdd.h
	$(RM) $@
	$(LN_S) $< $@

order.h: $(top_srcdir)/sis/order/order.h
	$(RM) $@
	$(LN_S) $< $@

phase.h: $(top_srcdir)/sis/phase/phase.h
	$(RM) $@
	$(LN_S) $< $@

pld.h: $(top_srcdir)/sis/pld/pld.h
	$(RM) $@
	$(LN_S) $< $@

power.h: $(top_srcdir)/sis/power/power.h
	$(RM) $@
	$(LN_S) $< $@

resub.h: $(top_srcdir)/sis/resub/resub.h
	$(RM) $@
	$(LN_S) $< $@

retime.h: $(top_srcdir)/sis/retime/retime.h
	$(RM) $@
	$(LN_S) $< $@

seqbdd.h: $(top_srcdir)/sis/seqbdd/seqbdd.h
	$(RM) $@
	$(LN_S) $< $@

simplify.h: $(top_srcdir)/sis/simplify/simplify.h
	$(RM) $@
	$(LN_S) $< $@

sis.h: $(top_srcdir)/sis/main/sis.h
	$(RM) $@
	$(LN_S) $< $@

sparse.h: $(top_srcdir)/sis/sparse/sparse.h
	$(RM) $@
	$(LN_S) $< $@

speed.h: $(top_srcdir)/sis/speed/speed.h
	$(RM) $@
	$(LN_S) $< $@

stg.h: $(top_srcdir)/sis/stg/stg.h
	$(RM) $@
	$(LN_S) $< $@

ansi.h: $(top_srcdir)/sis/util/ansi.h
	$(RM) $@
	$(LN_S) $< $@

util.h: $(top_srcdir)/sis/util/util.h
	$(RM) $@
	$(LN_S) $< $@

var_set.h: $(top_srcdir)/sis/var_set/var_set.h
	$(RM) $@
	$(LN_S) $< $@
