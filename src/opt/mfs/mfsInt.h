/**CFile****************************************************************

  FileName    [mfsInt.h]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Declarations of the MFS package.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: mfsInt.h,v 1.5 2003/11/18 18:55:13 alanmi Exp $]

***********************************************************************/
 
#ifndef __MFS_INT_H__
#define __MFS_INT_H__

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "mv.h"
#include "ntkInt.h"
#include "sh.h"    // AND/INV graph package
#include "wn.h"    // windowing package
#include "fm.h"    // flaxibility manager

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

// the structure used to pass information to "mfs"
typedef struct MfsParamsStruct  Mfs_Params_t;
struct MfsParamsStruct
{
    // the general parameters
    bool           fUseSS;         // set to 1 to use set simulation
    // the window parameters
    bool           fUseWindow;     // set to 1 to use windowing
    int            nLevelsTfi;     // the number of trans fanin logic
    int            nLevelsTfo;     // the number of trans fanout logic
    int            nTermsLimit;    // the max number of leaves/roots to consider
    // various modifiable flags 
    bool           fUseMv;         // switch "-m"
    bool           fUseSpec;       // switch "-s"
    bool           fOrderItoO;     // switch "-o"
    bool           fVerbose;       // switch "-v"
    bool           fShowKMap;      // switch "-k"
    int            nCands;         // switch "-c": the number of resub cands
    // various non-modifiable flags 
    bool           fBinary;        // set to 1 for binary networks
    bool           fNonDet;        // set to 1 for non-det networks
    bool           fSetInputs;     // set to 1 to use set inputs (for ND and windowing)
    bool           fSetOutputs;    // set to 1 to use set outputs (for ND networks)
    int            AcceptType;     // the acceptable type of the nodes
    bool           fOneNode;       // the acceptable type of the nodes
};

////////////////////////////////////////////////////////////////////////
///                       GLOBAL VARIABLES                           ///
////////////////////////////////////////////////////////////////////////

extern int timeGlobalFirst;
extern int timeGlobal;
extern int timeContain;
extern int timeImage;
extern int timeImagePrep;
extern int timeStrash;
extern int timeOther;
extern int timeSopMin;
extern int timeResub;

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/*=== mfsSpec.c ============================================================*/
extern void             Mfs_NetworkMfsSpec( Mfs_Params_t * p, Ntk_Network_t * pNet );
extern void             Mfs_NetworkMarkNDCones( Ntk_Network_t * pNet );
/*=== mfsWnd.c ============================================================*/
extern void             Mfs_NetworkMfs( Mfs_Params_t * p, Ntk_Network_t * pNet );
extern void             Mfs_NetworkMfsOne( Mfs_Params_t * p, Ntk_Node_t * pNode );
/*=== mfsUtils.c ============================================================*/
extern void             Mfs_NetworkMfsPreprocess( Mfs_Params_t * p, Ntk_Network_t * pNet );
extern int              Mfs_NetworkMfsNodeMinimize( Mfs_Params_t * p, Ntk_Node_t * pNode, Mvr_Relation_t * pFlex );
extern Ntk_Node_t **    Mfs_NetworkOrderNodesByLevel( Ntk_Network_t * pNet, bool fFromOutputs );

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

#endif

