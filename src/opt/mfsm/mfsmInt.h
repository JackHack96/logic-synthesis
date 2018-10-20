/**CFile****************************************************************

  FileName    [mfsnInt.h]

  PackageName [Simplification of networks using complete flexiblity.]

  Synopsis    [Internal declarations of the simplification package]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 3.0. Started - September 27, 2003.]

  Revision    [$Id: mfsmInt.h,v 1.4 2004/05/12 04:30:14 alanmi Exp $]

***********************************************************************/

#ifndef __MFSN_INT_H__
#define __MFSN_INT_H__

#include "mv.h"
#include "ntkInt.h"
#include "fmm.h"
#include "fmb.h"
#include "fmbs.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

typedef enum fmm_dir_type_enum fmm_dir_type_t;
enum fmm_dir_type_enum {
    MFSM_DIR_FROM_INPUTS, MFSM_DIR_FROM_OUTPUTS, MFSM_DIR_RANDOM, MFSM_DIR_UNASSIGNED
};

typedef enum fmm_bdd_type_enum fmm_bdd_type_t;
enum fmm_bdd_type_enum {
    MFSM_BDD_CONSTRAIN, MFSM_BDD_RESTRICT, MFSM_BDD_SQUEEZE, MFSM_BDD_ISOP, MFSM_BDD_UNASSIGNED
};

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFITIONS                            ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

typedef struct 
{
    // parameters of MFSM
	int nIters;             // "-i" the number of iterations
    int nCands;             // "-c" the number of resubstitution candidates to try
    int nNodesMax;          // "-n" the number of AND-gates in the SAT computation
    int nFaninMax;          // "-f" the limit on the fanin count
	fmm_dir_type_t fDir;    // "-d" direction to consider nodes
	int fSweep;             // "-q" removes equivalent nets
	int fResub;             // "-r" defines whether resubstion is used
	int fEspresso;          // "-e" using the reduced offset
	int fPrint;             // "-k" output don't-cares without minimizing
    int AcceptType;         // selects the accepting condition
    // parameters of DCM
    int fUseExdc;           // "-x" set to 1 if the EXDC is to be used
    int fDynEnable;         // "-y" enables dynmamic variable reordering
	int fVerbose;           // "-v" defines verbosity level
    int nLevelTfi;          // "-w" the number of TFI logic levels
    int nLevelTfo;          // "-w" the number of TFO logic levels
    int fBinary;            // the network is pure binary (used to simplify computation)
    int fUseSat;            // "-s" uses sat to compute don't-cares
} Mfsm_Params_t;

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DECLARATIONS                        ///
////////////////////////////////////////////////////////////////////////
/*=== mfsnSimp.c =======================================================*/
extern int Mfsm_NetworkMfs( Mfsm_Params_t * p, Ntk_Network_t * pNet, Ntk_Node_t * ppNodes[], int nNodes );
extern int Mfsb_NetworkMfs( Mfsm_Params_t * p, Ntk_Network_t * pNet, Ntk_Node_t * ppNodes[], int nNodes );
/*=== mfsnUtils.c =======================================================*/

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
