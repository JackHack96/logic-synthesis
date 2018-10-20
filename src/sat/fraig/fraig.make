CSRC +=  fraigApi.c fraigBalance.c fraigCanon.c fraigFanout.c fraigSupp.c \
	fraigFeed.c fraigGc.c fraigMan.c fraigMem.c fraigNode.c fraigOper.c \
	fraigPrime.c fraigRetime.c fraigSat.c fraigTable.c fraigUtil.c \
	fraigVec.c fraigCutSop.c fraigImp.c fraigMinCut.c fraigShow.c \
	fraigSym.c fraigSymSim.c fraigSymStr.c fraigSymSat.c \
	graCreate.c graList.c graMinCut.c graVec.c

HEADERS += fraig.h fraigInt.h graInt.h

DEPENDENCYFILES = $(CSRC)
