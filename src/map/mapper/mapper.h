/**CFile****************************************************************

  FileName    [mapper.h]
 
  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Generic technology mapping engine.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 8, 2003.]

  Revision    [$Id: mapper.h,v 1.12 2005/07/08 01:01:24 alanmi Exp $]

***********************************************************************/

#ifndef __MAPPER_H__
#define __MAPPER_H__

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

typedef struct Map_ManStruct_t_         Map_Man_t;
typedef struct Map_NodeStruct_t_        Map_Node_t;
typedef struct Map_NodeVecStruct_t_     Map_NodeVec_t;
typedef struct Map_CutStruct_t_         Map_Cut_t;
typedef struct Map_MatchStruct_t_       Map_Match_t;
typedef struct Map_SuperStruct_t_       Map_Super_t;
typedef struct Map_SuperLibStruct_t_    Map_SuperLib_t;
typedef struct Map_HashTableStruct_t_   Map_HashTable_t;
typedef struct Map_HashEntryStruct_t_   Map_HashEntry_t;
typedef struct Map_TimeStruct_t_        Map_Time_t;

// the pair of rise/fall time parameters
struct Map_TimeStruct_t_
{
    float              Rise;
    float              Fall;
    float              Worst;
};

////////////////////////////////////////////////////////////////////////
///                       GLOBAL VARIABLES                           ///
////////////////////////////////////////////////////////////////////////

extern Map_SuperLib_t * s_pSuperLib;

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFITIONS                            ///
////////////////////////////////////////////////////////////////////////
 
#define Map_IsComplement(p)    (((int)((long) (p) & 01)))
#define Map_Regular(p)         ((Map_Node_t *)((unsigned)(p) & ~01)) 
#define Map_Not(p)             ((Map_Node_t *)((long)(p) ^ 01)) 
#define Map_NotCond(p,c)       ((Map_Node_t *)((long)(p) ^ (c)))

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/*=== mapperCreate.c =============================================================*/
extern Map_Man_t *     Map_ManCreate( int nInputs, int nOutputs, int fVerbose );
extern Map_Node_t *    Map_NodeCreate( Map_Man_t * p, Map_Node_t * p1, Map_Node_t * p2 );
extern void            Map_ManFree( Map_Man_t * pMan );
extern void            Map_ManPrintTimeStats( Map_Man_t * p );
extern void            Map_ManPrintStatsToFile( char * pName, float Area, float Delay, int Time );
extern int             Map_ManReadInputNum( Map_Man_t * p );
extern int             Map_ManReadOutputNum( Map_Man_t * p );
extern Map_Node_t **   Map_ManReadInputs ( Map_Man_t * p );
extern Map_Node_t **   Map_ManReadOutputs( Map_Man_t * p );
extern Map_Node_t *    Map_ManReadConst1 ( Map_Man_t * p );
extern Map_Time_t *    Map_ManReadInputArrivals( Map_Man_t * p );
extern Mio_Library_t * Map_ManReadGenLib ( Map_Man_t * p );
extern bool            Map_ManReadVerbose( Map_Man_t * p );
extern void            Map_ManSetTimeToMap( Map_Man_t * p, int Time );
extern void            Map_ManSetTimeToNet( Map_Man_t * p, int Time );
extern void            Map_ManSetTimeSweep( Map_Man_t * p, int Time );
extern void            Map_ManSetTimeTotal( Map_Man_t * p, int Time );
extern void            Map_ManSetOutputNames( Map_Man_t * p, char ** ppNames );
extern void            Map_ManSetAreaRecovery( Map_Man_t * p, int fAreaRecovery );
extern void            Map_ManSetDelayTarget( Map_Man_t * p, float DelayTarget );
extern void            Map_ManSetObeyFanoutLimits( Map_Man_t * p, bool fObeyFanoutLimits );              
extern void            Map_ManSetNumIterations( Map_Man_t * p, int nNumIterations );
extern int             Map_ManReadPass( Map_Man_t * p );
extern void            Map_ManSetPass( Map_Man_t * p, int nPass );
extern int             Map_ManReadFanoutViolations( Map_Man_t * p );
extern void            Map_ManSetFanoutViolations( Map_Man_t * p, int nVio );
extern void            Map_ManSetChoiceNodeNum( Map_Man_t * p, int nChoiceNodes );
extern void            Map_ManSetChoiceNum( Map_Man_t * p, int nChoices );
extern void            Map_ManSetVerbose( Map_Man_t * p, int fVerbose );

extern char *          Map_NodeReadData0( Map_Node_t * p );
extern char *          Map_NodeReadData1( Map_Node_t * p );
extern int             Map_NodeReadNum( Map_Node_t * p );
extern int             Map_NodeReadLevel( Map_Node_t * p );
extern Map_Cut_t *     Map_NodeReadCuts( Map_Node_t * p );
extern Map_Cut_t *     Map_NodeReadCut0( Map_Node_t * p );
extern Map_Cut_t *     Map_NodeReadCut1( Map_Node_t * p );
extern Map_Node_t *    Map_NodeReadOne( Map_Node_t * p );
extern Map_Node_t *    Map_NodeReadTwo( Map_Node_t * p );
extern void            Map_NodeSetData0( Map_Node_t * p, char * pData );
extern void            Map_NodeSetData1( Map_Node_t * p, char * pData );
extern void            Map_NodeSetNextE( Map_Node_t * p, Map_Node_t * pNextE );
extern void            Map_NodeSetRepr( Map_Node_t * p, Map_Node_t * pRepr );

extern int             Map_NodeIsConst( Map_Node_t * p );
extern int             Map_NodeIsVar( Map_Node_t * p );
extern int             Map_NodeIsAnd( Map_Node_t * p );
extern int             Map_NodeComparePhase( Map_Node_t * p1, Map_Node_t * p2 );

extern Map_Super_t *   Map_CutReadSuper0( Map_Cut_t * p );
extern Map_Super_t *   Map_CutReadSuper1( Map_Cut_t * p );
extern int             Map_CutReadLeavesNum( Map_Cut_t * p );
extern Map_Node_t **   Map_CutReadLeaves( Map_Cut_t * p );
extern unsigned        Map_CutReadPhase0( Map_Cut_t * p );
extern unsigned        Map_CutReadPhase1( Map_Cut_t * p );

extern char *          Map_SuperReadFormula( Map_Super_t * p );
extern Mio_Gate_t *    Map_SuperReadRoot( Map_Super_t * p );
extern int             Map_SuperReadNum( Map_Super_t * p );
extern Map_Super_t **  Map_SuperReadFanins( Map_Super_t * p );
extern int             Map_SuperReadFaninNum( Map_Super_t * p );
extern Map_Super_t *   Map_SuperReadNext( Map_Super_t * p );
extern int             Map_SuperReadNumPhases( Map_Super_t * p );
extern unsigned char * Map_SuperReadPhases( Map_Super_t * p );
extern int             Map_SuperReadFanoutLimit( Map_Super_t * p );

extern Mio_Library_t * Map_SuperLibReadGenLib( Map_SuperLib_t * p );
extern float           Map_SuperLibReadAreaInv( Map_SuperLib_t * p );
extern Map_Time_t      Map_SuperLibReadDelayInv( Map_SuperLib_t * p );
extern int             Map_SuperLibReadVarsMax( Map_SuperLib_t * p );

extern Map_Node_t *    Map_NodeAnd( Map_Man_t * p, Map_Node_t * p1, Map_Node_t * p2 );
extern Map_Node_t *    Map_NodeOr( Map_Man_t * p, Map_Node_t * p1, Map_Node_t * p2 );
extern Map_Node_t *    Map_NodeExor( Map_Man_t * p, Map_Node_t * p1, Map_Node_t * p2 );
extern Map_Node_t *    Map_NodeMux( Map_Man_t * p, Map_Node_t * pNode, Map_Node_t * pNodeT, Map_Node_t * pNodeE );
extern void            Map_NodeSetChoice( Map_Man_t * pMan, Map_Node_t * pNodeOld, Map_Node_t * pNodeNew );

/*=== resmCanon.c =============================================================*/
extern int             Map_CanonComputeSlow( unsigned uTruths[][2], int nVarsMax, int nVarsReal, unsigned uTruth[], unsigned char * puPhases, unsigned uTruthRes[] );
/*=== mapperCut.c =============================================================*/
extern void            Map_MappingCreatePiCuts( Map_Man_t * p );
extern Map_Cut_t *     Map_CutAlloc( Map_Man_t * p );
/*=== mapperCutUtils.c =============================================================*/
extern void            Map_CutCreateFromNode( Map_Man_t * p, Map_Super_t * pSuper, int iRoot, unsigned uPhaseRoot, 
                           int * pLeaves, int nLeaves, unsigned uPhaseLeaves );
/*=== mapperCore.c =============================================================*/
extern int             Map_Mapping( Map_Man_t * p );
/*=== mapperFraig.c =============================================================*/
extern Map_Man_t *     Map_ManDupFraig( Fraig_Man_t * pManFraig );
extern Map_Man_t *     Map_ManBalanceFraig( Fraig_Man_t * pManFraig, int * pInputArrivals );
/*=== mapperLib.c =============================================================*/
extern int             Map_SuperLibDeriveFromGenlib( Mio_Library_t * pLib );
/*=== mapperMntk.c =============================================================*/
//extern Mntk_Man_t *    Map_ConvertMappingToMntk( Map_Man_t * pMan );
/*=== mapperSuper.c =============================================================*/
extern char *          Map_LibraryReadFormulaStep( char * pFormula, char * pStrings[], int * pnStrings );
/*=== mapperSweep.c =============================================================*/
extern void            Map_NetworkSweep( Ntk_Network_t * pNet );
/*=== mapperTable.c =============================================================*/
extern Map_Super_t *   Map_SuperTableLookupC( Map_SuperLib_t * pLib, unsigned uTruth[] );
/*=== mapperTime.c =============================================================*/
/*=== mapperUtil.c =============================================================*/
extern int             Map_ManCheckConsistency( Map_Man_t * p );
extern st_table *      Map_CreateTableGate2Super( Map_Man_t * p );
extern void            Map_ManCleanData( Map_Man_t * p );
extern void            Map_MappingSetupTruthTables( unsigned uTruths[][2] );
extern void            Map_MappingSetupTruthTablesLarge( unsigned uTruths[][32] );

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
#endif
