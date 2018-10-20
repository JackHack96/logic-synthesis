/**CFile****************************************************************

  FileName    [fpga.h]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Technology mapping for variable-size-LUT FPGAs.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - August 18, 2004.]

  Revision    [$Id: fpga.h,v 1.8 2005/07/08 01:01:22 alanmi Exp $]

***********************************************************************/

#ifndef __FPGA_H__
#define __FPGA_H__

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

// the maximum size of LUTs used for mapping
#define FPGA_MAX_LUTSIZE   10

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

typedef struct Fpga_ManStruct_t_         Fpga_Man_t;
typedef struct Fpga_NodeStruct_t_        Fpga_Node_t;
typedef struct Fpga_NodeVecStruct_t_     Fpga_NodeVec_t;
typedef struct Fpga_CutStruct_t_         Fpga_Cut_t;
typedef struct Fpga_LutLibStruct_t_      Fpga_LutLib_t;

////////////////////////////////////////////////////////////////////////
///                       GLOBAL VARIABLES                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFITIONS                            ///
////////////////////////////////////////////////////////////////////////
 
#define Fpga_IsComplement(p)    (((int)((long) (p) & 01)))
#define Fpga_Regular(p)         ((Fpga_Node_t *)((unsigned)(p) & ~01)) 
#define Fpga_Not(p)             ((Fpga_Node_t *)((long)(p) ^ 01)) 
#define Fpga_NotCond(p,c)       ((Fpga_Node_t *)((long)(p) ^ (c)))

#define Fpga_Ref(p)   
#define Fpga_Deref(p)
#define Fpga_RecursiveDeref(p,c)

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/*=== fpgaCreate.c =============================================================*/
extern Fpga_Man_t *    Fpga_ManCreate( int nInputs, int nOutputs, int fVerbose );
extern Fpga_Node_t *   Fpga_NodeCreate( Fpga_Man_t * p, Fpga_Node_t * p1, Fpga_Node_t * p2 );
extern void            Fpga_ManFree( Fpga_Man_t * pMan );
extern void            Fpga_ManPrintTimeStats( Fpga_Man_t * p );

extern int             Fpga_ManReadInputNum( Fpga_Man_t * p );
extern int             Fpga_ManReadOutputNum( Fpga_Man_t * p );
extern Fpga_Node_t **  Fpga_ManReadInputs ( Fpga_Man_t * p );
extern Fpga_Node_t **  Fpga_ManReadOutputs( Fpga_Man_t * p );
extern Fpga_Node_t *   Fpga_ManReadConst1 ( Fpga_Man_t * p );
extern float *         Fpga_ManReadInputArrivals( Fpga_Man_t * p );
extern int             Fpga_ManReadVerbose( Fpga_Man_t * p );
extern float *         Fpga_ManReadLutAreas( Fpga_Man_t * p );
extern void            Fpga_ManSetTimeToMap( Fpga_Man_t * p, int Time );
extern void            Fpga_ManSetTimeToNet( Fpga_Man_t * p, int Time );
extern void            Fpga_ManSetTimeTotal( Fpga_Man_t * p, int Time );
extern void            Fpga_ManSetOutputNames( Fpga_Man_t * p, char ** ppNames );
extern void            Fpga_ManSetTree( Fpga_Man_t * p, int fTree );
extern void            Fpga_ManSetPower( Fpga_Man_t * p, int fPower );
extern void            Fpga_ManSetAreaRecovery( Fpga_Man_t * p, int fAreaRecovery );
extern void            Fpga_ManSetResyn( Fpga_Man_t * p, int fResynthesis );
extern void            Fpga_ManSetDelayLimit( Fpga_Man_t * p, float DelayLimit );
extern void            Fpga_ManSetAreaLimit( Fpga_Man_t * p, float AreaLimit );
extern void            Fpga_ManSetTimeLimit( Fpga_Man_t * p, float TimeLimit );
extern void            Fpga_ManSetObeyFanoutLimits( Fpga_Man_t * p, int fObeyFanoutLimits );              
extern void            Fpga_ManSetNumIterations( Fpga_Man_t * p, int nNumIterations );
extern int             Fpga_ManReadFanoutViolations( Fpga_Man_t * p );
extern void            Fpga_ManSetFanoutViolations( Fpga_Man_t * p, int nVio );
extern void            Fpga_ManSetChoiceNodeNum( Fpga_Man_t * p, int nChoiceNodes );
extern void            Fpga_ManSetChoiceNum( Fpga_Man_t * p, int nChoices );
extern void            Fpga_ManSetVerbose( Fpga_Man_t * p, int fVerbose );
extern void            Fpga_ManSetLatchNum( Fpga_Man_t * p, int nLatches );
extern void            Fpga_ManSetSequential( Fpga_Man_t * p, int fSequential );
extern void            Fpga_ManSetName( Fpga_Man_t * p, char * pFileName );

extern int             Fpga_LibReadLutMax( Fpga_LutLib_t * pLib );

extern char *          Fpga_NodeReadData0( Fpga_Node_t * p );
extern char *          Fpga_NodeReadData1( Fpga_Node_t * p );
extern int             Fpga_NodeReadNum( Fpga_Node_t * p );
extern int             Fpga_NodeReadLevel( Fpga_Node_t * p );
extern Fpga_Cut_t *    Fpga_NodeReadCuts( Fpga_Node_t * p );
extern Fpga_Cut_t *    Fpga_NodeReadCutBest( Fpga_Node_t * p );
extern Fpga_Node_t *   Fpga_NodeReadOne( Fpga_Node_t * p );
extern Fpga_Node_t *   Fpga_NodeReadTwo( Fpga_Node_t * p );
extern void            Fpga_NodeSetData0( Fpga_Node_t * p, char * pData );
extern void            Fpga_NodeSetData1( Fpga_Node_t * p, char * pData );
extern void            Fpga_NodeSetArrival( Fpga_Node_t * p, float Time );
extern void            Fpga_NodeSetNextE( Fpga_Node_t * p, Fpga_Node_t * pNextE );
extern void            Fpga_NodeSetRepr( Fpga_Node_t * p, Fpga_Node_t * pRepr );

extern int             Fpga_NodeIsConst( Fpga_Node_t * p );
extern int             Fpga_NodeIsVar( Fpga_Node_t * p );
extern int             Fpga_NodeIsAnd( Fpga_Node_t * p );
extern int             Fpga_NodeComparePhase( Fpga_Node_t * p1, Fpga_Node_t * p2 );

extern int             Fpga_CutReadLeavesNum( Fpga_Cut_t * p );
extern Fpga_Node_t **  Fpga_CutReadLeaves( Fpga_Cut_t * p );

extern Fpga_Node_t *   Fpga_NodeAnd( Fpga_Man_t * p, Fpga_Node_t * p1, Fpga_Node_t * p2 );
extern Fpga_Node_t *   Fpga_NodeOr( Fpga_Man_t * p, Fpga_Node_t * p1, Fpga_Node_t * p2 );
extern Fpga_Node_t *   Fpga_NodeExor( Fpga_Man_t * p, Fpga_Node_t * p1, Fpga_Node_t * p2 );
extern Fpga_Node_t *   Fpga_NodeMux( Fpga_Man_t * p, Fpga_Node_t * pNode, Fpga_Node_t * pNodeT, Fpga_Node_t * pNodeE );
extern void            Fpga_NodeSetChoice( Fpga_Man_t * pMan, Fpga_Node_t * pNodeOld, Fpga_Node_t * pNodeNew );

extern void            Fpga_ManStats( Fpga_Man_t * p );

/*=== fpgaCore.c =============================================================*/
extern int             Fpga_Mapping( Fpga_Man_t * p );
/*=== fpgaCut.c ===============================================================*/
extern void            Fpga_MappingCreatePiCuts( Fpga_Man_t * p );
/*=== fpgaCutUtils.c =============================================================*/
extern void            Fpga_CutCreateFromNode( Fpga_Man_t * p, int iRoot, int * pLeaves, int nLeaves );
extern void            Fpga_MappingSetUsedCuts( Fpga_Man_t * p );
/*=== fpgaFraig.c =============================================================*/
extern Fpga_Man_t *    Fpga_ManDupFraig( Fraig_Man_t * pManFraig );
extern Fpga_Man_t *    Fpga_ManBalanceFraig( Fraig_Man_t * pManFraig, int * pInputArrivals );
/*=== fpgaLib.c =============================================================*/
extern Fpga_LutLib_t * Fpga_LutLibDup( Fpga_LutLib_t * p );
/*=== fpgaTruth.c =============================================================*/
extern void            Fpga_TruthsCut( Fpga_Man_t * pMan, Fpga_Cut_t * pCut, unsigned * uTruth );
extern void            Fpga_TruthsCutTopo( Fpga_Man_t * pMan, Fpga_Node_t * pNode, Fpga_Cut_t * pCut, unsigned * uTruth );
extern int             Fpga_TruthsCutDontCare( Fpga_Man_t * pMan, Fpga_Cut_t * pCut, unsigned * uTruthDc );
/*=== fpgaUtil.c =============================================================*/
extern int             Fpga_ManCheckConsistency( Fpga_Man_t * p );
extern void            Fpga_ManCleanData0( Fpga_Man_t * pMan );
extern Fpga_NodeVec_t * Fpga_CollectNodeTfo( Fpga_Man_t * pMan, Fpga_Node_t * pNode );


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
#endif
