/**CFile****************************************************************

  FileName    [autInt.h]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Internal declarations of the STG-based automata manipulation.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: autiInt.h,v 1.4 2005/06/02 03:34:20 alanmi Exp $]

***********************************************************************/

#ifndef __AUTI_INT_H__
#define __AUTI_INT_H__

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "mv.h"
#include "autInt.h"
#include "aio.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

#define UNREACHABLE   10000

#ifndef WIN32
#define _unlink unlink
#endif

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

// this structure stores information about state partitions
typedef struct Aut_SPInfo_t_ Aut_SPInfo_t;
struct Aut_SPInfo_t_
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

/*=== autiCompl.c =============================================================*/
extern int               Auti_AutoComplete( Aut_Auto_t * pAut, int nInputs, int fAccepting );
extern void              Auti_AutoComplement( Aut_Auto_t * pAut );
extern Aut_State_t *     Auti_AutoFindDcState( Aut_Auto_t * pAut );
extern Aut_State_t *     Auti_AutoFindDcStateAccepting( Aut_Auto_t * pAut );
extern Aut_State_t *     Auti_AutoFindDcStateNonAccepting( Aut_Auto_t * pAut );
extern void              Auti_AutoPrintStats( FILE * pFile, Aut_Auto_t * pAut, int nInputs, int fVerbose );
/*=== autiFilter.c =============================================================*/
extern void              Auti_AutoPrefixClose( Aut_Auto_t * pAut );
extern void              Auti_AutoProgressive( Aut_Auto_t * pAut, int nInputs, int fVerbose, int fPrefix );
extern void              Auti_AutoMoore( Aut_Auto_t * pAut, int nInputs, int fVerbose );
extern int               Auti_AutoCheckIsComplete( Aut_Auto_t * pAut, int nInputs );
extern int               Auti_AutoCheckIsMoore( Aut_Auto_t * pAut, int nInputs );
/*=== autiCheck.c =============================================================*/
extern void              Auti_AutoCheck( FILE * pOut, Aut_Auto_t * pAut1, Aut_Auto_t * pAut2, bool fError );
extern int               Auti_AutoCheckIsNd( FILE * pOut, Aut_Auto_t * pAut, int nInputs, int fVerbose );
/*=== autiDetPart.c =============================================================*/
extern Aut_Auto_t *      Auti_AutoDeterminizePart( Aut_Auto_t * pAut, int fAllAccepting, int fLongNames );
/*=== autiDot.c =============================================================*/
extern void              Auti_AutoWriteDot( Aut_Auto_t * pAut, char * pFileName, int nStatesMax, int fShowCond );
/*=== autiProd.c =============================================================*/
extern Aut_Auto_t *      Auti_AutoProduct( Aut_Auto_t * pAut1, Aut_Auto_t * pAut2, int fLongNames );
extern int               Auti_AutoProductCheckInputs( Aut_Auto_t * pAut1, Aut_Auto_t * pAut2 );
extern int               Auti_AutoProductCommonSupp( Aut_Auto_t * pAut1, Aut_Auto_t * pAut2, char * pFileName1, char * pFileName2, Aut_Auto_t ** ppAut1, Aut_Auto_t ** ppAut2 );
/*=== autiSupp.c ===========================================================*/
extern int               Auti_AutoSupport( Aut_Auto_t * pAut, char * pInputOrder );
/*=== autiDcMin.c =============================================================*/
extern Aut_Auto_t *      Auti_AutoStateDcMin( Aut_Auto_t * pAut, int fVerbose );
/*=== autiDcMin.c =============================================================*/
extern double            Auti_AutoComputeVolume( Aut_Auto_t * pAut, int Length );
/*=== autiMinim.c =============================================================*/
extern Aut_Auto_t *      Auti_AutoStateMinimize( Aut_Auto_t * pAut );

// the command below are still not ported


/*=== autiLang.c ===========================================================*/
extern void              Auti_AutoLanguage( FILE * pOut, Aut_Auto_t * pAut, int Length );
/*=== autiDet.c =============================================================*/
extern Aut_Auto_t *      Auti_AutoDeterminize( Aut_Auto_t * pAut, int fAllAccepting, int fLongNames );
/*=== autiRedSat.c ===========================================================*/
extern Aut_Auto_t *      Auti_AutoReductionSat( Aut_Auto_t * pAut, int nInputs, bool fVerbose );
/*=== autiReduce.c ===========================================================*/
extern Aut_Auto_t *      Auti_AutoReduction( Aut_Auto_t * pAut, int nInputs, bool fVerbose );
/*=== autiComb.c ===========================================================*/
extern Aut_Auto_t *      Auti_AutoCombinationalSat( Aut_Auto_t * pAut, int nInputs, bool fVerbose );

/*=== autiPart.c ===========================================================*/
extern void              Auti_AutoStatePartitions( DdManager * dd, Aut_Auto_t * pAut );
extern void              Auti_AutoStatePartitionsPrint( Aut_Auto_t * pAut );
extern void              Auti_AutoStatePartitionsFree( Aut_Auto_t * pAut );
extern Aut_SPInfo_t *    Auti_AutoInfoGet( Aut_State_t * pState );
extern void              Auti_AutoInfoSet( Aut_State_t * pState, Aut_SPInfo_t * p );
extern void              Auti_AutoInfoFree( Aut_SPInfo_t * p );
extern void              Aut_ComputeConditions( DdManager * dd, Aut_Auto_t * pAut, Vmx_VarMap_t * pVmx );
extern void              Aut_DeleteConditions( DdManager * dd, Aut_Auto_t * pAut );
extern int               Aut_FindDcState( Aut_Auto_t * pAut );

/*=== autiTrim.c ===========================================================*/
extern Aut_Auto_t *      Auti_AutoTrim( Aut_Auto_t * pAutX, Aut_Auto_t * pAutA );

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
#endif
