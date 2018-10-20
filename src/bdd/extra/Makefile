# Makefile for EXTRA library of DD-based procedures - October 11, 2003

# comment out the following line to see what's happening
.SILENT:

# modify these macros to point to your include and lib directory
INCDIR = /u/alanmi/local/include/
LIBDIR = /u/alanmi/local/lib/

# modify this line to compile with compilers other than g++
CC       = gcc

# Flags :
#      -O    = optimize
#      -Wall = display all warnings
#      -c    = compile or assemble source but don't link 
#      -g    = produce debugging information
#      -a    = generate extra code to write profile information;
#              this records the number of times each block is executed

#CFLAGS = -O3 -pedantic -Wall -ansi
#CFLAGS = -c -g -Wall -D_UNIX

# these are compiler flags for HP
#CFLAGS  = -c -O -D_UNIX -D_HP_

# these are compiler flags for SunOS
CFLAGS  = -c -O -D_UNIX

#LFLAGS = -pg -Wall
LFLAGS  =

NAME = extra

CFILES = extraAddMisc.c extraAddSpectra.c extraBddAuto.c \
	extraBddBoundSet.c extraBddDistance.c extraBddKmap.c \
	extraBddMisc.c extraBddPermute.c extraBddSupp.c \
	extraBddSymm.c extraBddUnate.c extraBddWidth.c \
	extraDdMinterm.c extraDdMisc.c extraDdNodePath.c \
	extraDdPrint.c extraDdShift.c extraDdSigma.c extraDdTimed.c \
	extraDdTransfer.c extraUtilFile.c extraUtilMemory.c \
	extraUtilMisc.c extraUtilProgress.c extraZddCover.c \
	extraZddExor.c extraZddFactor.c extraZddGraph.c \
	extraZddIsop.c extraZddLitCount.c extraZddMaxMin.c \
	extraZddMisc.c extraZddPermute.c extraZddSubSup.c

HEADER = extra.h

OBJS	= $(CFILES:.c=.o)

.SUFFIXES: .o .c

.c.o:
	echo Compiling file $< ...
	$(CC) $(CFLAGS) -I$(INCDIR) $<

all: lib$(NAME).a

lib$(NAME).a: $(OBJS)
	echo Creating library lib$(NAME).a ...
	ar -r lib$(NAME).a $(OBJS)
	ranlib lib$(NAME).a
	cp lib$(NAME).a $(LIBDIR)/lib$(NAME).a
	cp $(HEADER)    $(INCDIR)/$(HEADER)

clean:
	rm *.o                


