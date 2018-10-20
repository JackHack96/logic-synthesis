/**CFile****************************************************************

  FileName    [mtnk.h]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Generic mapped network.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - August 18, 2004.]

  Revision    [$Id: mntk.h,v 1.1 2005/02/28 05:34:28 alanmi Exp $]

***********************************************************************/

#ifndef __MNTK_H__
#define __MNTK_H__

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

#define MNTK_MAX_FANINS  6

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

typedef struct Mntk_ManStruct_t_         Mntk_Man_t;
typedef struct Mntk_NodeStruct_t_        Mntk_Node_t;
typedef struct Mntk_NodeVecStruct_t_     Mntk_NodeVec_t;
typedef struct Mntk_NodeStruct_t_        Mntk_Cut_t;
typedef struct Map_TimeStruct_t_         Mntk_Time_t;

////////////////////////////////////////////////////////////////////////
///                       GLOBAL VARIABLES                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFITIONS                            ///
////////////////////////////////////////////////////////////////////////
 
#define Mntk_IsComplement(p)    (((int)((long) (p) & 01)))
#define Mntk_Regular(p)         ((Mntk_Node_t *)((unsigned)(p) & ~01)) 
#define Mntk_Not(p)             ((Mntk_Node_t *)((long)(p) ^ 01)) 
#define Mntk_NotCond(p,c)       ((Mntk_Node_t *)((long)(p) ^ (c)))

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/*=== resmCreate.c ===============================================================*/
extern Mntk_Node_t *   Mntk_ManReadConst1( Mntk_Man_t * p );
extern Mntk_Node_t **  Mntk_ManReadInputs( Mntk_Man_t * p );
extern Mntk_Node_t **  Mntk_ManReadOutputs( Mntk_Man_t * p );
extern Mio_Library_t * Mntk_ManReadGenLib( Mntk_Man_t * p );
extern Map_Time_t *    Mntk_ManReadInputArrivals( Mntk_Man_t * p );
extern void            Mntk_ManSetTimeToMap( Mntk_Man_t * p, int Time );
extern void            Mntk_ManSetTimeToNet( Mntk_Man_t * p, int Time );
extern void            Mntk_ManSetTimeTotal( Mntk_Man_t * p, int Time );
extern void            Mntk_ManSetOutputNames( Mntk_Man_t * p, char ** ppOutputNames );
extern void            Mntk_ManSetGlobalCones( Mntk_Man_t * p, int fGlobalCones );
extern void            Mntk_ManSetUseThree( Mntk_Man_t * p, int fUseThree );
extern void            Mntk_ManSetCritWindow( Mntk_Man_t * p, int CritWindow );
extern void            Mntk_ManSetConeDepth( Mntk_Man_t * p, int ConeDepth );
extern void            Mntk_ManSetConeSize( Mntk_Man_t * p, int ConeSize );
extern void            Mntk_ManSetDelayLimit( Mntk_Man_t * p, float DelayLimit );
extern void            Mntk_ManSetAreaLimit( Mntk_Man_t * p, float AreaLimit );
extern void            Mntk_ManSetTimeLimit( Mntk_Man_t * p, float TimeLimit );

extern Mio_Gate_t *    Mntk_NodeReadGate( Mntk_Node_t * pNode );
extern int             Mntk_NodeReadLeavesNum( Mntk_Node_t * pNode );
extern Mntk_Node_t **  Mntk_NodeReadLeaves( Mntk_Node_t * pNode );
extern char *          Mntk_NodeReadData0( Mntk_Node_t * pNode );
extern char *          Mntk_NodeReadData1( Mntk_Node_t * pNode );
extern int             Mntk_NodeIsConst( Mntk_Node_t * pNode );
extern int             Mntk_NodeIsVar( Mntk_Node_t * pNode );
extern void            Mntk_NodeSetData0( Mntk_Node_t * pNode, char * pData );
extern void            Mntk_NodeSetData1( Mntk_Node_t * pNode, char * pData );

extern Mntk_Man_t *    Mntk_ManStart( int nInputs, int nOutputs, int nNodesMax, int fVerbose );
extern void            Mntk_ManFree( Mntk_Man_t * p );
extern void            Mntk_ManPrintTimeStats( Mntk_Man_t * p );
extern void            Mntk_ManCleanData( Mntk_Man_t * p );

/*=== resmNet.c ===============================================================*/
extern Mntk_Node_t *   Mntk_NodeAlloc( Mntk_Man_t * p, Mntk_Node_t ** ppLeaves, int nLeaves, Mio_Gate_t * pGate );
extern void            Mntk_NodeRegister( Mntk_Node_t * pNode );
extern void            Mntk_NodeFree( Mntk_Node_t * pNode );
/*=== resmCore.c =============================================================*/
extern int             Mntk_Resynthesize( Mntk_Man_t * p );
/*=== resmTime.c =============================================================*/
extern Mntk_Time_t *   Mntk_NodeReadTimeArrival( Mntk_Node_t * pNode, int fPhase );

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
#endif
