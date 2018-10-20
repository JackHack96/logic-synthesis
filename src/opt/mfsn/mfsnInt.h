/**CFile****************************************************************

  FileName    [mfsnInt.h]

  PackageName [Simplification of networks using complete flexiblity.]

  Synopsis    [Internal declarations of the simplification package]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - July 13, 2003.]

  Revision    [$Id: mfsnInt.h,v 1.3 2003/11/18 18:55:15 alanmi Exp $]

***********************************************************************/

#ifndef __MFSN_INT_H__
#define __MFSN_INT_H__

#include "mv.h"
#include "ntkInt.h"
#include "fmn.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFITIONS                            ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

typedef struct 
{
    // general info
	int nIters;             // "i" the number of iterations
    int AcceptType;
    int fGlobal;            // computes the global functions only
    // parameters of DCM
    int TypeSpec;           // the flexibility type to compute
    int TypeFlex;           // the spec type to use
    int nBddSizeMax;        // "b" the largest BDD size
	int fSweep;             // "s" removes equivalent nets
    int fUseExdc;           // set to 1 if the EXDC is to be used
    int fDynEnable;         // enables dynmamic variable reordering
	int fVerbose;           // "v" defines verbosity level
	int fUseProduct;        // "p" defines verbosity level
    // parameters of the network
	dcmn_dir_type_t fDir;   // "d" direction to consider nodes
    int fOneNode;           // enable simplification for one node only
	int fResub;             // "r" defines whether resubstion is used
	int fPhase;             // "p" performs phase assignment
	int fEspresso;          // "e" using the reduced offset
	int fPrint;             // "g" output don't-cares without minimizing
} Mfsn_Params_t;

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DECLARATIONS                        ///
////////////////////////////////////////////////////////////////////////
/*=== mfsnSimp.c =======================================================*/
extern int Mfsn_NetworkMfs( Mfsn_Params_t * p, Ntk_Network_t * pNet );
extern int Mfsn_NetworkMfsOne( Mfsn_Params_t * p, Ntk_Node_t * pNode );
/*=== mfsnUtils.c =======================================================*/

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
