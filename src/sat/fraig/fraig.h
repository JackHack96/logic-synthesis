/**CFile****************************************************************

  FileName    [fraig.h]

  PackageName [FRAIG: Functionally reduced AND-INV graphs.]

  Synopsis    [External declarations of the FRAIG package.]

  Author      [Alan Mishchenko <alanmi@eecs.berkeley.edu>]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.1. Started - October 1, 2004]

  Revision    [$Id: fraig.h,v 1.18 2005/07/08 01:01:30 alanmi Exp $]

***********************************************************************/
 
#ifndef __FRAIG_H__
#define __FRAIG_H__

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

typedef struct Fraig_ManStruct_t_         Fraig_Man_t;
typedef struct Fraig_NodeStruct_t_        Fraig_Node_t;
typedef struct Fraig_NodeVecStruct_t_     Fraig_NodeVec_t;
typedef struct Fraig_SimsStruct_t_        Fraig_Sims_t;
typedef struct Fraig_HashTableStruct_t_   Fraig_HashTable_t;

////////////////////////////////////////////////////////////////////////
///                       GLOBAL VARIABLES                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFITIONS                            ///
////////////////////////////////////////////////////////////////////////

// this macro enables reference counting
//#define FRAIG_USE_REF_COUNTING

#ifdef FRAIG_USE_REF_COUNTING
//#define Fraig_Ref(p)               ((Fraig_Regular(p))->nRefs++)
//#define Fraig_Deref(p)             ((Fraig_Regular(p))->nRefs--)
//#define Fraig_RecursiveDeref(p,c)  if ((p)->fRefCount) Fraig_RecursiveDerefInternal((p),(c))
#define Fraig_Ref(p)               Fraig_RefInternal(p)
#define Fraig_Deref(p)             Fraig_DerefInternal(p)
#define Fraig_RecursiveDeref(p,c)  Fraig_RecursiveDerefInternal((p),(c))
#else
#define Fraig_Ref(p)              
#define Fraig_Deref(p)            
#define Fraig_RecursiveDeref(p,c)
#endif
 
// macros working with complemented attributes of the nodes
#define Fraig_IsComplement(p)    (((int)((long) (p) & 01)))
#define Fraig_Regular(p)         ((Fraig_Node_t *)((unsigned)(p) & ~01)) 
#define Fraig_Not(p)             ((Fraig_Node_t *)((long)(p) ^ 01)) 
#define Fraig_NotCond(p,c)       ((Fraig_Node_t *)((long)(p) ^ (c)))

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/*=== fraigApi.c =============================================================*/
extern Fraig_NodeVec_t *   Fraig_ManReadVecInputs( Fraig_Man_t * p );
extern Fraig_NodeVec_t *   Fraig_ManReadVecOutputs( Fraig_Man_t * p );    
extern Fraig_NodeVec_t *   Fraig_ManReadVecNodes( Fraig_Man_t * p );      
extern Fraig_Node_t **     Fraig_ManReadInputs ( Fraig_Man_t * p );       
extern Fraig_Node_t **     Fraig_ManReadOutputs( Fraig_Man_t * p );       
extern Fraig_Node_t **     Fraig_ManReadNodes( Fraig_Man_t * p );         
extern int                 Fraig_ManReadInputNum ( Fraig_Man_t * p );     
extern int                 Fraig_ManReadOutputNum( Fraig_Man_t * p );     
extern int                 Fraig_ManReadNodeNum( Fraig_Man_t * p );       
extern Fraig_Node_t *      Fraig_ManReadConst1 ( Fraig_Man_t * p );       
extern Fraig_Node_t *      Fraig_ManReadIthVar( Fraig_Man_t * p, int i ); 
extern Fraig_Node_t *      Fraig_ManReadIthNode( Fraig_Man_t * p, int i );
extern char **             Fraig_ManReadInputNames( Fraig_Man_t * p );    
extern char **             Fraig_ManReadOutputNames( Fraig_Man_t * p );   
extern char *              Fraig_ManReadVarsInt( Fraig_Man_t * p );       
extern char *              Fraig_ManReadSat( Fraig_Man_t * p );           
extern int                 Fraig_ManReadFuncRed( Fraig_Man_t * p );       
extern int                 Fraig_ManReadFeedBack( Fraig_Man_t * p );      
extern int                 Fraig_ManReadDoSparse( Fraig_Man_t * p );      
extern int                 Fraig_ManReadRefCount( Fraig_Man_t * p );      
extern int                 Fraig_ManReadChoicing( Fraig_Man_t * p );      
extern int                 Fraig_ManReadVerbose( Fraig_Man_t * p );       
extern int                 Fraig_ManReadSimRounds( Fraig_Man_t * p );     

extern void                Fraig_ManSetFuncRed( Fraig_Man_t * p, int fFuncRed );        
extern void                Fraig_ManSetFeedBack( Fraig_Man_t * p, int fFeedBack );      
extern void                Fraig_ManSetDoSparse( Fraig_Man_t * p, int fDoSparse );      
extern void                Fraig_ManSetRefCount( Fraig_Man_t * p, int fRefCount );      
extern void                Fraig_ManSetChoicing( Fraig_Man_t * p, int fChoicing ); 
extern void                Fraig_ManSetTryProve( Fraig_Man_t * p, int fTryProve );
extern void                Fraig_ManSetVerbose( Fraig_Man_t * p, int fVerbose );        
extern void                Fraig_ManSetTimeToGraph( Fraig_Man_t * p, int Time );        
extern void                Fraig_ManSetTimeToNet( Fraig_Man_t * p, int Time );          
extern void                Fraig_ManSetTimeTotal( Fraig_Man_t * p, int Time );          
extern void                Fraig_ManSetOutputNames( Fraig_Man_t * p, char ** ppNames ); 
extern void                Fraig_ManSetInputNames( Fraig_Man_t * p, char ** ppNames );  
extern void                Fraig_ManSetPo( Fraig_Man_t * p, Fraig_Node_t * pNode );

extern Fraig_Node_t *      Fraig_NodeReadData0( Fraig_Node_t * p );                     
extern Fraig_Node_t *      Fraig_NodeReadData1( Fraig_Node_t * p );                     
extern int                 Fraig_NodeReadNum( Fraig_Node_t * p );                       
extern Fraig_Node_t *      Fraig_NodeReadOne( Fraig_Node_t * p );                       
extern Fraig_Node_t *      Fraig_NodeReadTwo( Fraig_Node_t * p );                       
extern Fraig_Node_t *      Fraig_NodeReadNextE( Fraig_Node_t * p );                     
extern Fraig_Node_t *      Fraig_NodeReadRepr( Fraig_Node_t * p );                      
extern int                 Fraig_NodeReadNumRefs( Fraig_Node_t * p );                   
extern int                 Fraig_NodeReadNumFanouts( Fraig_Node_t * p );                
extern int                 Fraig_NodeReadSimInv( Fraig_Node_t * p );                    
extern unsigned *          Fraig_NodeReadSimInfo( Fraig_Node_t * p );                   
extern int                 Fraig_NodeReadNumOnes( Fraig_Node_t * p );

extern void                Fraig_NodeSetData0( Fraig_Node_t * p, Fraig_Node_t * pData );
extern void                Fraig_NodeSetData1( Fraig_Node_t * p, Fraig_Node_t * pData );

extern int                 Fraig_NodeIsConst( Fraig_Node_t * p );
extern int                 Fraig_NodeIsVar( Fraig_Node_t * p );
extern int                 Fraig_NodeIsAnd( Fraig_Node_t * p );
extern int                 Fraig_NodeComparePhase( Fraig_Node_t * p1, Fraig_Node_t * p2 );

extern Fraig_Node_t *      Fraig_NodeOr( Fraig_Man_t * p, Fraig_Node_t * p1, Fraig_Node_t * p2 );
extern Fraig_Node_t *      Fraig_NodeAnd( Fraig_Man_t * p, Fraig_Node_t * p1, Fraig_Node_t * p2 );
extern Fraig_Node_t *      Fraig_NodeOr( Fraig_Man_t * p, Fraig_Node_t * p1, Fraig_Node_t * p2 );
extern Fraig_Node_t *      Fraig_NodeExor( Fraig_Man_t * p, Fraig_Node_t * p1, Fraig_Node_t * p2 );
extern Fraig_Node_t *      Fraig_NodeMux( Fraig_Man_t * p, Fraig_Node_t * pNode, Fraig_Node_t * pNodeT, Fraig_Node_t * pNodeE );
extern void                Fraig_NodeSetChoice( Fraig_Man_t * pMan, Fraig_Node_t * pNodeOld, Fraig_Node_t * pNodeNew );

/*=== fraigBalance.c =============================================================*/
extern Fraig_Man_t *       Fraig_ManBalance( Fraig_Man_t * pMan, int fAreaDup, int * pInputArrivals );

/*=== fraigMan.c =============================================================*/
extern Fraig_Man_t *       Fraig_ManCreate();
extern Fraig_Man_t *       Fraig_ManClone( Fraig_Man_t * pMan );
extern void                Fraig_ManFree( Fraig_Man_t * pMan );
extern void                Fraig_ManPrintTimeStats( Fraig_Man_t * p );

/*=== fraigGc.c =============================================================*/
extern void                Fraig_RefInternal( Fraig_Node_t * p );
extern void                Fraig_DerefInternal( Fraig_Node_t * p );
extern void                Fraig_RecursiveDerefInternal( Fraig_Man_t * pMan, Fraig_Node_t * gNode );
extern int                 Fraig_ManagerCheckZeroRefs( Fraig_Man_t * pMan );

/*=== fraigDfs.c =============================================================*/
extern Fraig_NodeVec_t *   Fraig_Dfs( Fraig_Man_t * pMan, int fEquiv );
extern Fraig_NodeVec_t *   Fraig_DfsOne( Fraig_Man_t * pMan, Fraig_Node_t * pNode, int fEquiv );
extern Fraig_NodeVec_t *   Fraig_DfsNodes( Fraig_Man_t * pMan, Fraig_Node_t ** ppNodes, int nNodes, int fEquiv );
extern Fraig_NodeVec_t *   Fraig_DfsReverse( Fraig_Man_t * pMan );
extern int                 Fraig_CountNodes( Fraig_Man_t * pMan, int fEquiv );
extern int                 Fraig_CheckTfi( Fraig_Man_t * pMan, Fraig_Node_t * pOld, Fraig_Node_t * pNew );
extern int                 Fraig_CountLevels( Fraig_Man_t * pMan );

/*=== fraigSat.c =============================================================*/
extern int                 Fraig_NodesAreEqual( Fraig_Man_t * p, Fraig_Node_t * pNode1, Fraig_Node_t * pNode2, int nBTLimit );
extern int                 Fraig_NodeIsEquivalent( Fraig_Man_t * p, Fraig_Node_t * pOld, Fraig_Node_t * pNew, int nBTLimit );
extern Fraig_NodeVec_t *   Fraig_CollectSupergate( Fraig_Node_t * pNode, int fStopAtMux );

/*=== fraigShow.c =============================================================*/
extern void                Fraig_MappingShow( Fraig_Man_t * pMan, char * pFileName );
extern void                Fraig_MappingShowNodes( Fraig_Man_t * pMan, Fraig_Node_t ** ppRoots, int nRoots, char * pFileName );

/*=== fraigVec.c ===============================================================*/
extern Fraig_NodeVec_t *   Fraig_NodeVecAlloc( int nCap );
extern void                Fraig_NodeVecFree( Fraig_NodeVec_t * p );
extern Fraig_NodeVec_t *   Fraig_NodeVecDup( Fraig_NodeVec_t * p );
extern Fraig_Node_t **     Fraig_NodeVecReadArray( Fraig_NodeVec_t * p );
extern int                 Fraig_NodeVecReadSize( Fraig_NodeVec_t * p );
extern void                Fraig_NodeVecGrow( Fraig_NodeVec_t * p, int nCapMin );
extern void                Fraig_NodeVecShrink( Fraig_NodeVec_t * p, int nSizeNew );
extern void                Fraig_NodeVecClear( Fraig_NodeVec_t * p );
extern void                Fraig_NodeVecPush( Fraig_NodeVec_t * p, Fraig_Node_t * Entry );
extern int                 Fraig_NodeVecPushUnique( Fraig_NodeVec_t * p, Fraig_Node_t * Entry );
extern void                Fraig_NodeVecPushOrder( Fraig_NodeVec_t * p, Fraig_Node_t * pNode );
extern int                 Fraig_NodeVecPushUniqueOrder( Fraig_NodeVec_t * p, Fraig_Node_t * pNode );
extern void                Fraig_NodeVecPushOrderByLevel( Fraig_NodeVec_t * p, Fraig_Node_t * pNode );
extern int                 Fraig_NodeVecPushUniqueOrderByLevel( Fraig_NodeVec_t * p, Fraig_Node_t * pNode );
extern Fraig_Node_t *      Fraig_NodeVecPop( Fraig_NodeVec_t * p );
extern void                Fraig_NodeVecRemove( Fraig_NodeVec_t * p, Fraig_Node_t * Entry );
extern void                Fraig_NodeVecWriteEntry( Fraig_NodeVec_t * p, int i, Fraig_Node_t * Entry );
extern Fraig_Node_t *      Fraig_NodeVecReadEntry( Fraig_NodeVec_t * p, int i );
extern void                Fraig_NodeVecSortByLevel( Fraig_NodeVec_t * p );
extern void                Fraig_NodeVecSortByNumber( Fraig_NodeVec_t * p );

/*=== fraigUtil.c ===============================================================*/
extern void                Fraig_ManMarkRealFanouts( Fraig_Man_t * p );
extern int                 Fraig_ManCheckConsistency( Fraig_Man_t * p );
extern int                 Fraig_GetMaxLevel( Fraig_Man_t * pMan );
extern void                Fraig_ManReportChoices( Fraig_Man_t * pMan );

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
#endif
