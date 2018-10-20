CSRC += resmCore.c resmCreate.c resmFanin.c resmFil.c resmFilSat.c \
	resmFilSim.c resmFilTop.c resmImage.c resmLogic.c resmMatch.c \
	resmNet.c resmNode.c resmTime.c resmUpdate.c resmUtils.c resmVec.c
	                    
HEADERS += resm.h resmInt.h 

DEPENDENCYFILES = $(CSRC)
