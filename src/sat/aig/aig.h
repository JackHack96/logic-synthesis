/**CFile****************************************************************

  FileName    [aig.h]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [AND-INV graph package with built in sat_sweep.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - July 10, 2004.]

  Revision    [$Id: aig.h,v 1.2 2004/07/29 04:54:47 alanmi Exp $]

***********************************************************************/

#ifndef __AIG_H__
#define __AIG_H__

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

typedef struct Aig_ManStruct_t_         Aig_Man_t;
typedef struct Aig_NodeStruct_t_        Aig_Node_t;
typedef struct Aig_NodeVecStruct_t_     Aig_NodeVec_t;
typedef struct Aig_SimsStruct_t_        Aig_Sims_t;
typedef struct Aig_HashTableStruct_t_   Aig_HashTable_t;

////////////////////////////////////////////////////////////////////////
///                       GLOBAL VARIABLES                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFITIONS                            ///
////////////////////////////////////////////////////////////////////////
 
#define Aig_IsComplement(p)    (((int)((long) (p) & 01)))
#define Aig_Regular(p)         ((Aig_Node_t *)((unsigned)(p) & ~01)) 
#define Aig_Not(p)             ((Aig_Node_t *)((long)(p) ^ 01)) 
#define Aig_NotCond(p,c)       ((Aig_Node_t *)((long)(p) ^ (c)))

#define Aig_Ref(p)   
#define Aig_Deref(p)
#define Aig_RecursiveDeref(p,c)

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/*=== aigAnd.c =============================================================*/
extern Aig_Node_t *      Aig_NodeAnd( Aig_Man_t * p, Aig_Node_t * p1, Aig_Node_t * p2 );
extern Aig_Node_t *      Aig_NodeOr( Aig_Man_t * p, Aig_Node_t * p1, Aig_Node_t * p2 );
extern Aig_Node_t *      Aig_NodeExor( Aig_Man_t * p, Aig_Node_t * p1, Aig_Node_t * p2 );
extern Aig_Node_t *      Aig_NodeMux( Aig_Man_t * p, Aig_Node_t * pNode, Aig_Node_t * pNodeT, Aig_Node_t * pNodeE );

/*=== aigMan.c =============================================================*/
extern Aig_Man_t *       Aig_ManCreate( int nInputs, int nOutputs, int fVerbose );
extern void              Aig_ManFree( Aig_Man_t * pMan );
extern void              Aig_ManPrintTimeStats( Aig_Man_t * p );

extern Aig_Node_t **     Aig_ManReadInputs ( Aig_Man_t * p );
extern Aig_Node_t **     Aig_ManReadOutputs( Aig_Man_t * p );
extern int               Aig_ManReadInputNum( Aig_Man_t * p );
extern int               Aig_ManReadOutputNum( Aig_Man_t * p );
extern Aig_Node_t *      Aig_ManReadConst1 ( Aig_Man_t * p );
extern bool              Aig_ManReadVerbose( Aig_Man_t * p );
extern char *            Aig_ManReadVarsInt( Aig_Man_t * p );
extern char *            Aig_ManReadSat( Aig_Man_t * p );
extern void              Aig_ManSetTimeToAig( Aig_Man_t * p, int Time );
extern void              Aig_ManSetTimeToNet( Aig_Man_t * p, int Time );
extern void              Aig_ManSetTimeTotal( Aig_Man_t * p, int Time );
extern void              Aig_ManSetOutputNames( Aig_Man_t * p, char ** ppNames );
extern void              Aig_ManSetOneLevel( Aig_Man_t * p, int fOneLevel );

extern char *            Aig_NodeReadData0( Aig_Node_t * p );
extern char *            Aig_NodeReadData1( Aig_Node_t * p );
extern int               Aig_NodeReadNum( Aig_Node_t * p );
extern Aig_Node_t *      Aig_NodeReadOne( Aig_Node_t * p );
extern Aig_Node_t *      Aig_NodeReadTwo( Aig_Node_t * p );
extern void              Aig_NodeSetData0( Aig_Node_t * p, char * pData );
extern void              Aig_NodeSetData1( Aig_Node_t * p, char * pData );

/*=== aigSat.c =============================================================*/
extern int               Aig_NodeIsEquivalent( Aig_Man_t * p, Aig_Node_t * p1, Aig_Node_t * p2, int fComp, int fSkipZero, int fVerbose );

/*=== aigTable.c =============================================================*/
extern Aig_HashTable_t * Aig_HashTableCreate();
extern void              Aig_HashTableFree( Aig_HashTable_t * p );
extern int               Aig_HashTableLookupS( Aig_Man_t * pMan, Aig_HashTable_t * p, Aig_Node_t * p1, Aig_Node_t * p2, Aig_Node_t ** ppNodeRes );
extern Aig_Node_t *      Aig_HashTableLookupF( Aig_HashTable_t * p, Aig_Node_t * pNode );

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
#endif
