/**CFile****************************************************************

  FileName    [resm.h]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Generic resynthesis engine.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - August 18, 2004.]

  Revision    [$Id: resm.h,v 1.2 2005/02/28 05:34:33 alanmi Exp $]

***********************************************************************/

#ifndef __RESM_H__
#define __RESM_H__

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

#define RESM_MAX_FANINS  6

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

typedef struct Resm_ManStruct_t_         Resm_Man_t;
typedef struct Resm_NodeStruct_t_        Resm_Node_t;
typedef struct Resm_NodeVecStruct_t_     Resm_NodeVec_t;
typedef struct Resm_NodeStruct_t_        Resm_Cut_t;
typedef struct Map_TimeStruct_t_         Resm_Time_t;

////////////////////////////////////////////////////////////////////////
///                       GLOBAL VARIABLES                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFITIONS                            ///
////////////////////////////////////////////////////////////////////////
 
#define Resm_IsComplement(p)    (((int)((long) (p) & 01)))
#define Resm_Regular(p)         ((Resm_Node_t *)((unsigned)(p) & ~01)) 
#define Resm_Not(p)             ((Resm_Node_t *)((long)(p) ^ 01)) 
#define Resm_NotCond(p,c)       ((Resm_Node_t *)((long)(p) ^ (c)))

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/*=== resmCreate.c ===============================================================*/
extern Resm_Node_t *   Resm_ManReadConst1( Resm_Man_t * p );
extern Resm_Node_t **  Resm_ManReadInputs( Resm_Man_t * p );
extern Resm_Node_t **  Resm_ManReadOutputs( Resm_Man_t * p );
extern Mio_Library_t * Resm_ManReadGenLib( Resm_Man_t * p );
extern Map_Time_t *    Resm_ManReadInputArrivals( Resm_Man_t * p );
extern void            Resm_ManSetTimeToMap( Resm_Man_t * p, int Time );
extern void            Resm_ManSetTimeToNet( Resm_Man_t * p, int Time );
extern void            Resm_ManSetTimeTotal( Resm_Man_t * p, int Time );
extern void            Resm_ManSetOutputNames( Resm_Man_t * p, char ** ppOutputNames );
extern void            Resm_ManSetGlobalCones( Resm_Man_t * p, int fGlobalCones );
extern void            Resm_ManSetUseThree( Resm_Man_t * p, int fUseThree );
extern void            Resm_ManSetCritWindow( Resm_Man_t * p, int CritWindow );
extern void            Resm_ManSetConeDepth( Resm_Man_t * p, int ConeDepth );
extern void            Resm_ManSetConeSize( Resm_Man_t * p, int ConeSize );
extern void            Resm_ManSetDelayLimit( Resm_Man_t * p, float DelayLimit );
extern void            Resm_ManSetAreaLimit( Resm_Man_t * p, float AreaLimit );
extern void            Resm_ManSetTimeLimit( Resm_Man_t * p, float TimeLimit );

extern Mio_Gate_t *    Resm_NodeReadGate( Resm_Node_t * pNode );
extern int             Resm_NodeReadLeavesNum( Resm_Node_t * pNode );
extern Resm_Node_t **  Resm_NodeReadLeaves( Resm_Node_t * pNode );
extern char *          Resm_NodeReadData0( Resm_Node_t * pNode );
extern char *          Resm_NodeReadData1( Resm_Node_t * pNode );
extern int             Resm_NodeIsConst( Resm_Node_t * pNode );
extern int             Resm_NodeIsVar( Resm_Node_t * pNode );
extern void            Resm_NodeSetData0( Resm_Node_t * pNode, char * pData );
extern void            Resm_NodeSetData1( Resm_Node_t * pNode, char * pData );

extern Resm_Man_t *    Resm_ManStart( int nInputs, int nOutputs, int nNodesMax, int fVerbose );
extern void            Resm_ManFree( Resm_Man_t * p );
extern void            Resm_ManPrintTimeStats( Resm_Man_t * p );
extern void            Resm_ManCleanData( Resm_Man_t * p );

/*=== resmNet.c ===============================================================*/
extern Resm_Node_t *   Resm_NodeAlloc( Resm_Man_t * p, Resm_Node_t ** ppLeaves, int nLeaves, Mio_Gate_t * pGate );
extern void            Resm_NodeRegister( Resm_Node_t * pNode );
extern void            Resm_NodeFree( Resm_Node_t * pNode );
/*=== resmCore.c =============================================================*/
extern int             Resm_Resynthesize( Resm_Man_t * p );
/*=== resmTime.c =============================================================*/
extern Resm_Time_t *   Resm_NodeReadTimeArrival( Resm_Node_t * pNode, int fPhase );

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
#endif
