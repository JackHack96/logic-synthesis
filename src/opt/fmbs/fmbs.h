/**CFile****************************************************************

  FileName    [fmbs.h]

  PackageName [Binary flexibility manager.]

  Synopsis    [External declarations of the flexibility manager.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 3.0. Started - September 27, 2003.]

  Revision    [$Id: fmbs.h,v 1.2 2004/10/18 01:39:22 alanmi Exp $]

***********************************************************************/

#ifndef __FMBS_H__
#define __FMBS_H__

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                         TYPEDEFS                                 ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                      STRUCTURE DEFITIONS                         ///
////////////////////////////////////////////////////////////////////////

typedef struct Fmbs_Manager_t_   Fmbs_Manager_t;  // the flexibility manager
typedef struct Fmbs_Params_t_    Fmbs_Params_t;   // the manager parameters

struct Fmbs_Params_t_ 
{
	int          fUseExdc;    // set to 1 if the EXDC is to be used
	int          fDynEnable;  // enables dynmamic variable reordering
	int          fVerbose;    // to enable the output of puzzling messages
    int          nLevelTfi;   // the number of TFI logic levels
    int          nLevelTfo;   // the number of TFO logic levels
    int          fUseSat;     // use the SAT-based DC computation
    int          nNodesMax;   // the max number of AND-gates
};

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFITIONS                            ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                      EXPORTED FUNCTIONS                          ///
////////////////////////////////////////////////////////////////////////

/*=== fmmMan.c =========================================================*/
extern Fmbs_Manager_t *  Fmbs_ManagerStart( Ntk_Network_t * pNet, Fmbs_Params_t * pPars );
extern void              Fmbs_ManagerStop( Fmbs_Manager_t * p );
extern void              Fmbs_ManagerPrintTimeStats( Fmbs_Manager_t * pMan );
extern void              Fmbs_ManagerSetParameters( Fmbs_Manager_t * p, Fmbs_Params_t * pPars );
extern void              Fmbs_ManagerSetNetwork( Fmbs_Manager_t * p, Ntk_Network_t * pNet );
extern int               Fmbs_ManagerReadVerbose( Fmbs_Manager_t * p );
extern int               Fmbs_ManagerReadNodesMax( Fmbs_Manager_t * p );
extern void              Fmbs_ManagerAddToTimeSopMin( Fmbs_Manager_t * p, int Time );
extern void              Fmbs_ManagerAddToTimeResub( Fmbs_Manager_t * p, int Time );
extern void              Fmbs_ManagerAddToTimeCdc( Fmbs_Manager_t * p, int Time );
/*=== fmmApi.c =========================================================*/
extern Mvr_Relation_t *  Fmbs_ManagerComputeLocalDcNode( Fmbs_Manager_t * p, Ntk_Node_t * pPivot, Ntk_Node_t * ppFanins[], int nFanins );

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
