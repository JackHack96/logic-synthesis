/**CFile****************************************************************

  FileName    [dualInt.h]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Automata synthesis from L language specifications. 
  The theory supporting this package is developed by Anatoly Chebotarev,
  Kiev, Ukraine, 1990 - 2003.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - August 18, 2003.]

  Revision    [$Id: dualInt.h,v 1.4 2004/02/19 03:06:53 alanmi Exp $]

***********************************************************************/

#ifndef __DUAL_INT_H__
#define __DUAL_INT_H__

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "mv.h"
#include "dual.h"
#include "auInt.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

typedef struct DualSpecStruct       Dual_Spec_t;       // the operation stack

struct DualSpecStruct
{
    Fnc_Manager_t * pMan;
    char *        pName;        // the model name
    DdManager *   dd;           // the DD manager
    DdNode    *   bSpec;        // the formula 
    int           nVars;        // the total number of variables
    int           nVarsIn;      // the number of inputs
    int           nVarsPar;     // the number of parameters
    int           nVarsOut;     // the number of outputs
    char **       ppVarNames;   // the variable names
    int           nVarsAlloc;   // the number of allocated names
    int           nRank;        // the rank of the formula
    DdNode **     pbVars;       // 0-rank variables
    // synthesis info
    int           nStates;      // the number of states
    int           nStatesAlloc; // the allocated number
    DdNode **     pbMarksP;     // otmetka sostoyaniya
    DdNode **     pbMarks;      // otmetka sostoyaniya
    DdNode **     pbTrans;      // oblast perehoda 
    // initial states
    DdNode *      bCondInit;    // the initial condition
    int           nStatesInit;  // the number of initial states
    int *         pStatesInit;  // the array of initial states
    // temporary data
    int           FileSize;
    char *        pBuffer;
    char *        pDotModel;
    char *        pDotIns;
    char *        pDotOuts;
    char *        pDotPars;
    char *        pDotInit;
    char *        pDotStart;
    FILE *        pOutput;
};

////////////////////////////////////////////////////////////////////////
///                       GLOBAL VARIABLES                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFITIONS                            ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DECLARATIONS                        ///
////////////////////////////////////////////////////////////////////////

/*=== dualSpec.c =============================================================*/
extern Dual_Spec_t *      Dual_SpecAlloc();
extern void               Dual_SpecFree( Dual_Spec_t * p );
/*=== dualRead.c =============================================================*/
extern Dual_Spec_t *      Dual_SpecRead( Mv_Frame_t * pMvsis, char * FileName );
/*=== dualNormal.c ============================================================*/
extern int                Dual_SpecNormal( Dual_Spec_t * p );
/*=== dualSynth.c ============================================================*/
extern Au_Auto_t *       Dual_AutoSynthesize( Dual_Spec_t * p );
/*=== dualForm.c =============================================================*/
extern int                Dual_ConvertAut2Formula( Au_Auto_t * pAut, char * pFileName, int Rank );
/*=== dualNet.c =============================================================*/
extern Dual_Spec_t *      Dual_DeriveSpecFromNetwork( Ntk_Network_t * pNet, int Rank, bool fOrder );

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
#endif
