/**CFile****************************************************************

  FileName    [auInt.h]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Internal declarations of the STG-based automata manipulation.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: auInt.h,v 1.1 2004/02/19 03:06:46 alanmi Exp $]

***********************************************************************/

#ifndef __AU_INT_H__
#define __AU_INT_H__

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "mv.h"
#include "au.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

#define UNREACHABLE   10000

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

struct AuRelStruct
{
    Au_Auto_t *     pAut;            // the parent automaton
    // functionality
    DdManager *      dd;              // the manager
    DdNode *         bRel;            // the BDD for the transition relation
    // variable maps
    Vm_VarMap_t *    pVm;             // the variable map
    Vmx_VarMap_t *   pVmx;            // the extended variable map

    // quantification cubes
    DdNode *         bCubeCs;         // the cube of current state variables
    DdNode *         bCubeNs;         // the cube of next state variables
    DdNode *         bCubeIn;         // the cube of input variables
    // state codes
    DdNode **        pbStatesCs;      // the codes of the current states
    DdNode **        pbStatesNs;      // the codes of the next states
    int              nStates;         // the number of states
};


// this structure stores information about state partitions
typedef struct Au_SPInfo_t_ Au_SPInfo_t;
struct Au_SPInfo_t_
{
    int       nParts;       // the number of state partitions
    int *     pSizes;       // the sizes of each partition
    int **    ppParts;      // the number of states in each partition
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

/*=== autCreate.c =============================================================*/
extern Au_Auto_t *      Au_AutoClone( Au_Auto_t * pAut );
extern Au_Auto_t *      Au_AutoDup( Au_Auto_t * pAut );
extern Au_Auto_t *      Au_AutoReverse( Au_Auto_t * pAut );
extern Au_Auto_t *      Au_AutoExtract( Au_Auto_t * pAut, int fUseTransitions );
extern Au_Auto_t *      Au_AutoSingleState( Au_Auto_t * pAut, DdManager * dd, DdNode * bCombFunc, Vmx_VarMap_t * pVmx );
extern Au_Rel_t *       Au_AutoGetTR( DdManager * dd, Au_Auto_t * pAut, bool fReorder );
extern void              Au_AutoFreeTR( Au_Auto_t * pAut );
extern void              Au_AutoStateFree( Au_State_t * pState );
extern Mvc_Cover_t *     Au_AutoStateSumConds( Au_Auto_t * pAut, Au_State_t * pState );
extern DdNode *          Au_AutoStateSumAccepting( Au_Auto_t * pAut, DdManager * dd, DdNode * pbCodes[] );
extern void              Au_AutoTransFree( Au_Trans_t * pTrans );
/*=== autRel.c =============================================================*/
extern Au_Rel_t *       Au_AutoRelCreate( DdManager * dd, Au_Auto_t * pAut, bool fReorder );
extern void              Au_AutoRelFree( Au_Rel_t * pTR );
extern Au_Auto_t *      Au_AutoRelCreateAuto( Au_Rel_t * pTR );
/*=== fsmUtils.c =============================================================*/
extern int               Au_ReadNamePairs( FILE * pOutput, Ntk_Network_t * pNet1, Ntk_Network_t * pNet2, 
                             char * argv[], int argc, char * ppNames1[], char * ppNames2[] );
extern char *            Au_UtilsCombineNames( char * pName1, char * pName2 );
extern Vmx_VarMap_t *    Au_UtilsCreateVmx( Fnc_Manager_t * pMan, int nInputs, int nStates, int fUseNs );
extern int *             Au_UtilsPermuteNs2Cs( Au_Rel_t * pTR );
extern void              Au_UtilsSetVarMap( Au_Rel_t * pTR );
 
/*=== autReadPara.c =============================================================*/
extern int               Au_AutoReadPara( Mv_Frame_t * pMvsis, char * pInputString, char * FileNameIn, char * FileNameOut );
/*=== autWrite.c =============================================================*/
/*=== autDot.c =============================================================*/
extern void              Au_AutoWriteDot( Au_Auto_t * pAut, char * pFileName, int nStatesMax, int fShowCond );
/*=== autEncode.c =============================================================*/
extern int               Au_AutoEncodeSymbolicFsm( Mv_Frame_t * pMvsis, char * pFileNameIn, char * pFileNameOut );
/*=== autLang.c ===========================================================*/
extern void              Au_AutoLanguage( FILE * pOut, Au_Auto_t * pAut, int Length );
/*=== autBench.c ===========================================================*/
extern Au_Auto_t *      Auto_AutoBenchmarkGenerate( Au_Auto_t * pAutIn, int nInsRemove, int nOutsRemove );

/*=== autCompl.c ===========================================================*/
extern void              Au_AutoComplement( Au_Auto_t * pAut );
extern bool              Au_AutoStateIsCcmplete( Au_Auto_t * pAut, Au_State_t * pState, 
                            DdNode * bCube, Vm_VarMap_t * pVm, DdManager * dd, DdNode ** pbCodes, bool fUseMark, int fMoore );
/*=== autDetExp.c =============================================================*/
extern Au_Auto_t *      Au_AutoDeterminizeExp( Au_Auto_t * pAut, int fAllAccepting, int fLongNames );
/*=== autDetFull.c =============================================================*/
extern Au_Auto_t *      Au_AutoDeterminizeFull( Au_Auto_t * pAut, int fAllAccepting, int fLongNames );
/*=== autDetPart.c =============================================================*/
extern Au_Auto_t *      Au_AutoDeterminizePart( Au_Auto_t * pAut, int fAllAccepting, int fLongNames );

/*=== autProd.c =============================================================*/
extern Au_Auto_t *      Au_AutoProduct( Au_Auto_t * pAut1, Au_Auto_t * pAut2, int fLongNames );
extern int               Au_AutoProductCheckInputs( Au_Auto_t * pAut1, Au_Auto_t * pAut2 );
/*=== autSupp.c ===========================================================*/
extern void              Au_AutoSupport( Au_Auto_t * pAut, char * pInputOrder );
extern void              Au_AutoRaise( Au_Auto_t * pAut, char * pInputOrder );
extern void              Au_AutoLower( Au_Auto_t * pAut, char * pInputOrder );
/*=== autFilter.c ===========================================================*/
extern Au_Auto_t *      Au_AutoPrefixClose( Au_Auto_t * pAut );
extern Au_Auto_t *      Au_AutoFilter( Au_Auto_t * pAut );
extern Au_Auto_t *      Au_AutoFilterMarked( Au_Auto_t * pAut );
extern int               Au_AutoReachability( Au_Auto_t * pAut, int fUseAccepting );
/*=== autReduce.c ===========================================================*/
extern Au_Auto_t *      Au_AutoReduction( Au_Auto_t * pAut, int nInputs, bool fVerbose );
/*=== autRedSat.c ===========================================================*/
extern Au_Auto_t *      Au_AutoReductionSat( Au_Auto_t * pAut, int nInputs, bool fVerbose );
/*=== autComb.c ===========================================================*/
extern Au_Auto_t *      Au_AutoCombinationalSat( Au_Auto_t * pAut, int nInputs, bool fVerbose );

/*=== fsmMinim.c =============================================================*/
extern Au_Auto_t *      Au_AutoStateMinimize( Au_Auto_t * pAut, int fUseTable );
/*=== fsmDcMin.c =============================================================*/
extern Au_Auto_t *      Au_AutoStateDcMinimize( Au_Auto_t * pAut, int fVerbose );
/*=== autCheck.c ===========================================================*/
extern void              Au_AutoCheck( FILE * pOutput, Au_Auto_t * pAut1, Au_Auto_t * pAut2, bool fError );
extern bool              Au_AutoCheckProduct12( Au_Auto_t * pAut1, Au_Auto_t * pAut2 );
extern int               Au_AutoCheckNd( FILE * pOut, Au_Auto_t * pAut, int nInputs, int fVerbose );

/*=== autPart.c ===========================================================*/
extern void              Au_AutoStatePartitions( DdManager * dd, Au_Auto_t * pAut );
extern void              Au_AutoStatePartitionsPrint( Au_Auto_t * pAut );
extern void              Au_AutoStatePartitionsFree( Au_Auto_t * pAut );
extern Au_SPInfo_t *    Au_AutoInfoGet( Au_State_t * pState );
extern void              Au_AutoInfoSet( Au_State_t * pState, Au_SPInfo_t * p );
extern void              Au_AutoInfoFree( Au_SPInfo_t * p );
extern void              Au_ComputeConditions( DdManager * dd, Au_Auto_t * pAut, Vmx_VarMap_t * pVmx );
extern void              Au_DeleteConditions( DdManager * dd, Au_Auto_t * pAut );
extern int               Au_FindDcState( Au_Auto_t * pAut );

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
#endif
