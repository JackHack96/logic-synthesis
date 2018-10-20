CSRC += aio.c aioRead.c aioReadAut.c aioReadBlifMv.c \
	aioReadFile.c aioReadLines.c aioReadNet.c aioReadNode.c \
	aioWrite.c aioWriteAut.c aioWriteBlifMv.c
HEADERS += aio.h aioInt.h

DEPENDENCYFILES = $(CSRC)
