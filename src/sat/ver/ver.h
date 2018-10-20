/**CFile****************************************************************

  FileName    [ver.h]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [External declarations of the verification package.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ver.h,v 1.7 2004/07/08 23:24:21 satrajit Exp $]

***********************************************************************/

#ifndef __VER_H__
#define __VER_H__

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "sh.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                       GLOBAL VARIABLES                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFITIONS                            ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/*=== verSweep.c =============================================================*/
extern void           Ver_NetworkSweep( Ntk_Network_t * pNet, int fVerbose, int fMergeInvertedNodes, int fSplitLimit );
extern st_table *     Ver_NetworkDetectEquiv( Ntk_Network_t * pNet, Sh_Manager_t * pShMan, Sh_Network_t * pShNet,
                                                                                       int fVerbose, int fSplitLimit );
extern void Ver_NetworkMergeEquiv( Ntk_Network_t * pNet, st_table * tEquiv, int fVerbose, int fMergeInvertedNodes );
/*=== verReach.c =============================================================*/
extern void           Ver_NetworkReachability( Ntk_Network_t * pNet, int nItersLimit, int nSquar, int fAlgorithm );
/*=== verVerify.c =============================================================*/
extern void           Ver_NetworkVerifySatComb( Ntk_Network_t * pNet1, Ntk_Network_t * pNet2, int fVerbose );
extern void           Ver_NetworkVerifySatSeq( Ntk_Network_t * pNet1, Ntk_Network_t * pNet2, int nFrames, int fVerbose );
/*=== verStrash.c =============================================================*/
extern Sh_Network_t * Ver_NetworkStrashInt( Sh_Manager_t * pMan, Ntk_Network_t * pNet );

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

#endif
