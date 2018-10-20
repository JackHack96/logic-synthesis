/**CFile****************************************************************

  FileName    [verInt.h]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Internal declarations of the verification package.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: verInt.h,v 1.6 2004/07/08 23:24:21 satrajit Exp $]

***********************************************************************/

/* 
The code of this package is inspired by the following paper:
F. Lu, L. Wang, K. Cheng, R. Huang, "A circuit SAT solver with
signal correlation guided learning", DATE 2003, pp. 892-897. 
*/

#ifndef __VER_INT_H__
#define __VER_INT_H__

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "mv.h"
#include "util.h"
#include "extra.h"
#include "shInt.h"
#include "sat.h"
#include "ver.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

typedef struct VerManagerStruct      Ver_Manager_t;   

struct VerManagerStruct
{
    // strashing
    Sh_Manager_t *     pShMan;
    Sh_Network_t *     pShNet;
    // equivalence classes
    Sh_Node_t **       ppClasses;
    int                nClasses;
    int                nClassesAlloc;
    // various flags
    bool               fMiter;         // this flag is set to 1 if the network is a miter
    // various stats
    int                nRounds;        // the number of rounds of simulation performed
};

////////////////////////////////////////////////////////////////////////
///                       GLOBAL VARIABLES                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFITIONS                            ///
////////////////////////////////////////////////////////////////////////

#define VER_STRASH_READ(pNode)      ((Sh_Node_t *)pNode->pCopy)
#define VER_STRASH_WRITE(pNode,p)   (pNode->pCopy = (Ntk_Node_t *)p)

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/*=== verSim.c =============================================================*/
extern int            Ver_VerificationSimulate( Ver_Manager_t * p, int nRounds );
/*=== verSolve.c =============================================================*/
extern void           Ver_NetworkSweepSat( Ver_Manager_t * p, int iSplitLimit );
extern int            Ver_NetworkEquivCheckSat( Ver_Manager_t * p );
extern void           Vec_NetworkAddClauses( Sat_Solver_t * pSat, Sh_Network_t * pNet );
extern void           Ver_NetworkAddOutputOne( Sat_Solver_t * pSat, Sh_Network_t * pShNet );
/*=== verStrash.c =============================================================*/
extern Sh_Network_t * Ver_NetworkStrash( Sh_Manager_t * pMan, Ntk_Network_t * pNet, bool fUseBdds );
extern Sh_Network_t * Ver_NetworkStrashMiter( Sh_Manager_t * pMan, Ntk_Network_t * pNet1, Ntk_Network_t * pNet2, int fPoOnly );
extern Sh_Node_t *    Ver_NetworkStrashTransRelation( Sh_Manager_t * pMan, Ntk_Network_t * pNet );
extern Sh_Node_t *    Ver_NetworkStrashInitState( Sh_Manager_t * pMan, Ntk_Network_t * pNet );
extern Sh_Node_t *    Ver_NetworkStrashBdd( Sh_Manager_t * pMan, DdManager * dd, DdNode * bFunc );
/*=== verFuncDep.c =============================================================*/
extern void           Ver_FunctionalDependency( Ntk_Network_t * pNet );

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
#endif
