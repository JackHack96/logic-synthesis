CSRC += satActivity.c satClause.c satClauseVec.c satMem.c satOrder.c \
	satQueue.c satSolverApi.c satSolverCore.c satSolverIo.c \
	satSolverProof.c satSolverSearch.c satSort.c satVec.c

HEADERS += sat.h satInt.h

DEPENDENCYFILES = $(CSRC)
