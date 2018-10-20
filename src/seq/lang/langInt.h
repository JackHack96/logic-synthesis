/**CFile****************************************************************

  FileName    [langInt.h]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Implicit language solving flow.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: langInt.h,v 1.5 2004/02/19 03:06:54 alanmi Exp $]

***********************************************************************/

#ifndef __LANG_INT_H__
#define __LANG_INT_H__
 
////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "mv.h"
#include "lang.h"
#include "au.h"
 
////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

typedef struct LangAutoStruct    Lang_Auto_t;    // the automaton
struct LangAutoStruct
{
    // general information
    char *           pName;           // the name of the automaton
    char *           pHistory;        // info about the genesys of the automaton
    int              nInputs;         // the number of IO variables
    int              nOutputs;        // currently, always 0
    int              nLatches;        // the number of MV latches 
    int              nStates;         // the number of states
    int              nAccepting;      // the number of reachable states
    int              nStateBits;      // the number of state bits
    // variable/state names
    char **          ppNamesInput;    // input names
    char **          ppNamesOutput;   // output names
    st_table *       tCode2Name;      // the mapping of CS minterms into state names
    // functionality
    Fnc_Manager_t *  pMan;            // the functionality manager
    DdManager *      dd;              // the BDD manager
    DdNode *         bRel;            // the transition relation
    DdNode *         bInit;           // the initial state
    DdNode *         bStates;         // the reachable states
    DdNode *         bAccepting;      // the accepting states
    // variable ordering
    Vmx_VarMap_t *   pVmx;            // IO vars + CS vars + NS vars
};

typedef struct LangStackStruct    Lang_Stack_t;    // the automaton stack
struct LangStackStruct
{
    Lang_Auto_t **   pData;           // the array of elements
    int              Top;             // the index
    int              Size;            // the stack size
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

/*=== langRead.c ============================================================*/
extern Lang_Auto_t *  Lang_AutoRead( Mv_Frame_t * pMvsis, char * FileName );
extern Lang_Auto_t *  Lang_AutomatonConvert( Au_Auto_t * pAut, DdManager * ddUser );
/*=== langWrite.c ============================================================*/
extern void           Lang_AutoWrite( Lang_Auto_t * pAut, char * FileName );
/*=== langStg.c ============================================================*/
extern Lang_Auto_t *  Lang_ExtractStg( Ntk_Network_t * pNet, int fLongNames );
/*=== langSupport.c ============================================================*/
extern int            Lang_AutoSupport( Lang_Auto_t * pAut, char * pInputString );
/*=== langDet.c ============================================================*/
extern Lang_Auto_t *  Lang_AutoDeterminize( Lang_Auto_t * pAut, int fAllAccepting, int fLongNames );
/*=== langCheck.c ============================================================*/
extern void           Lang_AutoCheck( FILE * pOut, Lang_Auto_t * pAut1, Lang_Auto_t * pAut2, bool fError );
/*=== langMinim.c ============================================================*/
extern Lang_Auto_t *  Lang_AutoMinimize( Lang_Auto_t * pAut );
/*=== langReenc.c ============================================================*/
typedef struct Lang_Reenc_t_ Lang_Reenc_t;
extern int            Lang_AutoReencode( Lang_Auto_t * pAut );
extern Lang_Reenc_t * Lang_AutoReencodePrepare( Lang_Auto_t * pAut );
extern Vmx_VarMap_t * Lang_AutoReencodeReadVmxNew( Lang_Reenc_t * p );
extern void           Lang_AutoReencodeCleanup( Lang_Reenc_t * p );
extern DdNode *       Lang_AutoReencodeOne( Lang_Reenc_t * p, DdNode * bFunc );
extern DdNode *       Lang_AutoReencodeTwo( Lang_Reenc_t * p, DdNode * bFunc );
/*=== langReach.c ============================================================*/
extern void           Lang_Reachability( Lang_Auto_t * pAut, bool fReorder, bool fVerbose );
/*=== langProd.c ============================================================*/
extern Lang_Auto_t *  Lang_AutoProduct( Lang_Auto_t * pAut1, Lang_Auto_t * pAut2, bool fLongNames, bool fSimple );
/*=== langFilter.c ============================================================*/
extern int            Lang_AutoPrefixClose( Lang_Auto_t * pAut );
extern int            Lang_AutoProgressive( Lang_Auto_t * pAut, int nInputs, bool fVerbose );
extern Lang_Auto_t *  Lang_AutoReduce( Lang_Auto_t * pAut, int nInputs, bool fVerbose );
/*=== langOper.c ============================================================*/
extern void           Lang_AutoPrintStats( FILE * pOut, Lang_Auto_t * pAut, int nInputs );
extern int            Lang_AutoComplete( Lang_Auto_t * pLang, int nInputs, int fAccepting, int fCheckOnly );
extern DdNode *       Lang_AutoIncompleteDomain( Lang_Auto_t * pAut, DdNode * bRelation, DdNode * bReach );
extern DdNode *       Lang_AutoIncompleteStates( Lang_Auto_t * pAut, DdNode * bRelation, DdNode * bReach );
extern int            Lang_AutoCheckNd( FILE * pOut, Lang_Auto_t * pAut, int nInputs, bool fVerbose );
extern int            Lang_AutoComplement( Lang_Auto_t * pAut );
/*=== langUtils.c ============================================================*/
extern Lang_Auto_t *  Lang_AutoAlloc();
extern Lang_Auto_t *  Lang_AutoClone( Lang_Auto_t * pAut );
extern Lang_Auto_t *  Lang_AutoDup( Lang_Auto_t * pAut );
extern void           Lang_AutoFree( Lang_Auto_t * pAut );
extern void           Lang_AutoFreeStateNames( DdManager * dd, st_table * tCode2Name );
extern int            Lang_AutoCountTransitions( Lang_Auto_t * pAut );
extern void           Lang_AutoExtendManager( Lang_Auto_t * pAut, int nAddInputBits );
extern DdNode *       Lang_AutoGetStateCode( Lang_Auto_t * pAut, int iState, int fUseNsVars );
extern DdNode *       Lang_AutoGetUnusedCodeMinterm( Lang_Auto_t * pAut );
extern int            Lang_AutoVerifySupport( Lang_Auto_t * pAut );
extern void           Lang_AutoAssignHistory( Lang_Auto_t * pLang, int argc, char * argv[] );
extern char *         Lang_AutoGetDcStateName( Lang_Auto_t * pLang );
extern st_table *     Lang_AutoTransferNamesTable( Lang_Auto_t * pLang, DdManager * dd, int * pPermute );
extern void           Lang_AutoPrintNameTable( Lang_Auto_t * pLang );
extern DdNode *       Lang_AutoCharCubeIO( DdManager * dd, Lang_Auto_t * pAut );
extern DdNode *       Lang_AutoCharCubeCs( DdManager * dd, Lang_Auto_t * pAut );
extern DdNode *       Lang_AutoCharCubeNs( DdManager * dd, Lang_Auto_t * pAut );
/*=== langStack.c ============================================================*/
extern Lang_Stack_t * Lang_StackStart  ( int nDepth );
extern bool           Lang_StackIsEmpty( Lang_Stack_t * p );
extern void           Lang_StackPush   ( Lang_Stack_t * p, Lang_Auto_t * pAut );
extern Lang_Auto_t *  Lang_StackPop    ( Lang_Stack_t * p );
extern void           Lang_StackFree   ( Lang_Stack_t * p );
/*=== langVarMap.c ============================================================*/
extern Vmx_VarMap_t * Lang_VarMapStg( Ntk_Network_t * pNet );
extern Vmx_VarMap_t * Lang_VarMapDupLatch( Lang_Auto_t * pAut );
extern Vmx_VarMap_t * Lang_VarMapProduct( Lang_Auto_t * pAut, Lang_Auto_t * pAut1, Lang_Auto_t * pAut2, int ** ppPermute );
extern Vmx_VarMap_t * Lang_VarMapReencodeNew( Lang_Auto_t * pAut, int nStatesNew );
extern Vmx_VarMap_t * Lang_VarMapReencodeExt( Lang_Auto_t * pAut, int nStatesNew );

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
#endif
