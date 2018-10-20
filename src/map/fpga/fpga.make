CSRC += fpga.c fpgaCore.c fpgaCreate.c fpgaCut.c fpgaLib.c fpgaMatch.c \
	fpgaShow.c fpgaTruth.c fpgaUtils.c fpgaVec.c fpgaFraig.c \
	fpgaFanout.c fpgaTime.c fpgaCutUtils.c

HEADERS += fpga.h fpgaInt.h 

DEPENDENCYFILES = $(CSRC)
