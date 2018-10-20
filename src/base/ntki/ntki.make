CSRC += ntki.c ntkiCollapse.c ntkiEliminate.c ntkiMerge.c ntkiPrint.c \
	ntkiSat.c ntkiSweep.c ntkiGlobal.c ntkiVerify.c ntkiOrder.c \
	ntkiPower.c ntkiFxu.c ntkiLxu.c ntkiResub.c ntkiStrash.c ntkiDecomp.c \
	ntkiDefault.c ntkiEncode.c ntkiCollapseNew.c ntkiDsd.c ntkiBalance.c \
	ntkiUnreach.c ntkiFrames.c ntkiShow.c ntkiMap.c ntkiFpga.c \
	ntkiFraig.c ntkiFraigChoice.c ntkiFraigSweep.c ntkiFraigVerify.c \
	ntkiAttach.c ntkiResm.c ntkiMntk.c ntkiSymms.c ntkiRetime.c \
	ntkiFraigRetime.c

HEADERS += ntki.h ntkiInt.h

DEPENDENCYFILES = $(CSRC)
