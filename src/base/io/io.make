CSRC += io.c ioFileOpen.c ioRead.c ioReadFile.c ioReadKiss.c ioReadLines.c \
	ioReadNet.c ioReadNode.c ioReadPla.c ioReadPlaMv.c ioWrite.c \
	ioWritePla.c ioWritePlaMv.c ioWriteSplit.c ioWriteBench.c \
	ioWriteMapped.c  ioReadBench.c
HEADERS += io.h ioInt.h

DEPENDENCYFILES = $(CSRC)
