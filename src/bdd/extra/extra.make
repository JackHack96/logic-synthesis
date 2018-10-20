CSRC += extraAddMisc.c extraAddSpectra.c extraBddAuto.c \
	extraBddBoundSet.c extraBddDistance.c extraBddKmap.c \
	extraBddMisc.c extraBddPermute.c extraBddSupp.c \
	extraBddSymm.c extraBddUnate.c extraBddWidth.c \
	extraDdMinterm.c extraDdMisc.c extraDdNodePath.c \
	extraDdPrint.c extraDdShift.c extraDdSigma.c extraDdTimed.c \
	extraDdTransfer.c extraUtilFile.c extraUtilMemory.c \
	extraUtilMisc.c extraUtilProgress.c extraZddCover.c \
	extraZddExor.c extraZddFactor.c extraZddGraph.c \
	extraZddIsop.c extraZddLitCount.c extraZddMaxMin.c \
	extraZddMisc.c extraZddPermute.c extraZddSubSup.c \
	extraBddImage.c extraUtilGraph.c extraDdCache.c \
	extraUtilBitMatrix.c extraUtilIntVec.c extraUtilQueue.c

HEADERS += extra.h

DEPENDENCYFILES = $(CSRC)
