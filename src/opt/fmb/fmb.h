/**CFile****************************************************************

  FileName    [fmb.h]

  PackageName [Binary flexibility manager.]

  Synopsis    [External declarations of the flexibility manager.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 3.0. Started - September 27, 2003.]

  Revision    [$Id: fmb.h,v 1.3 2004/05/12 04:30:11 alanmi Exp $]

***********************************************************************/

#ifndef __FMB_H__
#define __FMB_H__

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

#define FMB_MAX_FANIN_GROWTH             10    // the largest allowed increase in the MAX fanin count of the network
#define FMB_ADD_BITS_NUM                  1    // the number of additional bits used for Z vars of the cut network
#define FMB_BDD_DFS_LIMIT           5000000    // the largest tolerated global BDD size
#define FMB_VERY_LAGRE_BDD_SIZE    10000000    // very large BDD size

////////////////////////////////////////////////////////////////////////
///                         TYPEDEFS                                 ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                      STRUCTURE DEFITIONS                         ///
////////////////////////////////////////////////////////////////////////

typedef struct Fmb_Manager_t_   Fmb_Manager_t;  // the flexibility manager
typedef struct Fmb_Params_t_    Fmb_Params_t;   // the manager parameters

struct Fmb_Params_t_ 
{
	int          fUseExdc;    // set to 1 if the EXDC is to be used
	int          fDynEnable;  // enables dynmamic variable reordering
	int          fVerbose;    // to enable the output of puzzling messages
    int          nLevelTfi;   // the number of TFI logic levels
    int          nLevelTfo;   // the number of TFO logic levels
    int          fUseSat;     // use the SAT-based DC computation
    int          nNodesMax;
};

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFITIONS                            ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                      EXPORTED FUNCTIONS                          ///
////////////////////////////////////////////////////////////////////////

/*=== fmmApiFunc.c =========================================================*/
extern DdNode *         Fmb_ManagerComputeGlobalDcNode( Fmb_Manager_t * p, Ntk_Node_t * pPivot );
extern Mvr_Relation_t * Fmb_ManagerComputeLocalDcNode( Fmb_Manager_t * p, Ntk_Node_t * pPivot, Ntk_Node_t * ppFanins[], int nFanins );
/*=== fmmFlexFunc.c ============================================================*/
extern DdNode *         Fmb_FlexComputeGlobal( Fmb_Manager_t * p );
extern Mvr_Relation_t * Fmb_FlexComputeLocal( Fmb_Manager_t * pMan, Ntk_Node_t * pNode, DdNode * bGlobal, Ntk_Node_t ** ppFanins, int nFanins );
/*=== fmmApiRel.c =========================================================*/
extern DdNode *         Fmb_ManagerComputeGlobalDcNodeRel( Fmb_Manager_t * p, Ntk_Node_t * pPivots[], int nPivots );
extern Mvr_Relation_t * Fmb_ManagerComputeLocalDcNodeRel( Fmb_Manager_t * p, Ntk_Node_t * pPivots[], int nPivots, Ntk_Node_t * ppFanins[], int nFanins );
/*=== fmmFlexRel.c ============================================================*/
extern DdNode *         Fmb_FlexComputeGlobalRel( Fmb_Manager_t * p );
extern Mvr_Relation_t * Fmb_FlexComputeLocalRel( Fmb_Manager_t * pMan, DdNode * bGlobal, Ntk_Node_t ** ppFanins, int nFanins );
/*=== fmmMan.c =========================================================*/
extern Fmb_Manager_t *  Fmb_ManagerStart( Ntk_Network_t * pNet, Fmb_Params_t * pPars );
extern void             Fmb_ManagerResize( Fmb_Manager_t * p, int nGlo, int nLoc, int nExt );
extern void             Fmb_ManagerStop( Fmb_Manager_t * p );
extern void             Fmb_ManagerPrintTimeStats( Fmb_Manager_t * pMan );
extern void             Fmb_ManagerCleanCurrentWindow( Fmb_Manager_t * p );
extern void             Fmb_ManagerSetParameters( Fmb_Manager_t * p, Fmb_Params_t * pPars );
extern DdManager *      Fmb_ManagerReadDdGlo( Fmb_Manager_t * p );
extern int              Fmb_ManagerReadVerbose( Fmb_Manager_t * p );
extern void             Fmb_ManagerAddToTimeSopMin( Fmb_Manager_t * p, int Time );
extern void             Fmb_ManagerAddToTimeResub( Fmb_Manager_t * p, int Time );
extern void             Fmb_ManagerAddToTimeCdc( Fmb_Manager_t * p, int Time );
/*=== fmmSweep.c ============================================================*/
extern int              Fmb_NetworkSweep( Ntk_Network_t * pNet );
extern int              Fmb_NetworkMergeEquivalentNets( Fmb_Manager_t * p );
/*=== fmbImage.c =======================================================*/
extern DdNode *         Fmb_ImageCompute( DdManager * dd, DdNode ** pbFuncs, DdNode ** pbVars, DdNode * bCareSet, int nFuncs, int nBddSize );
extern void             Fmb_ImageTimeoutSet( int Timeout );
extern void             Fmb_ImageTimeoutReset();

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
