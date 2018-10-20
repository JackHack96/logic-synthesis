/**CFile****************************************************************

  FileName    [graInt.h]

  PackageName [FRAIG: Functionally reduced AND-INV graphs.]

  Synopsis    [Generic graph data struture.]

  Author      [Alan Mishchenko <alanmi@eecs.berkeley.edu>]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.1. Started - October 1, 2004]

  Revision    [$Id: graInt.h,v 1.2 2005/07/08 01:01:34 alanmi Exp $]

***********************************************************************/
 
#ifndef __GRA_INT_H__
#define __GRA_INT_H__

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include "util.h"
#include "st.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////
 
////////////////////////////////////////////////////////////////////////
///                       MACRO DEFITIONS                            ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

typedef struct Gra_NodeList_t_  Gra_NodeList_t;   // the list of nodes
typedef struct Gra_NodeVec_t_   Gra_NodeVec_t;    // the vector of nodes
typedef struct Gra_Node_t_      Gra_Node_t;       // the node
typedef struct Gra_Graph_t_     Gra_Graph_t;      // the graph

struct Gra_NodeList_t_
{
    Gra_Node_t *      pHead;           // the first node in the list
    Gra_Node_t *      pTail;           // the last node in the list
    int               nItems;          // the number of nodes in the list
};

struct Gra_NodeVec_t_
{
    Gra_Node_t **     pArray;          // the array of nodes
    int               nSize;           // the number of entries in the array
    int               nCap;            // the number of allocated entries
};

struct Gra_Node_t_
{
    Gra_Node_t *      pNext;           // the next node
    Gra_Node_t *      pPrev;           // the previous node
    Gra_NodeVec_t     vEdgesI;         // the set of predecessors
    Gra_NodeVec_t     vEdgesO;         // the set of successors
    unsigned          fMark1  :  1;    // the mark
    unsigned          fMark2  :  1;    // the mark
    unsigned          Num     : 30;    // the number of this node
    Gra_Graph_t *     pGraph;          // the graph
};

struct Gra_Graph_t_
{
    Gra_NodeList_t    lNodes;          // the list of nodes
    st_table *        tEdges;          // the hash table of edges
};

// iterator through the list of all nodes
#define Gra_NodeListForEachNode( List, Node )                      \
    for ( Node = (List)->pHead;                                    \
          Node;                                                    \
          Node = (Node)->pNext )
#define Gra_NodeListForEachNodeSafe( List, Node, Node2 )           \
    for ( Node = (List)->pHead, Node2 = Node? Node->pNext : NULL;  \
          Node;                                                    \
          Node = Node2, Node2 = Node? Node->pNext : NULL )

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DECLARATIONS                        ///
////////////////////////////////////////////////////////////////////////

/*=== graMinCut.c =============================================================*/
extern void                Gra_GraphMinCut( Gra_Graph_t * p, Gra_Node_t * pHost );

/*=== graCreate.c =============================================================*/
extern Gra_Graph_t *       Gra_GraphCreate();
extern void                Gra_GraphDelete( Gra_Graph_t * p );
extern Gra_Node_t *        Gra_NodeCreate( Gra_Graph_t * p, int Num );
extern void                Gra_NodeDelete( Gra_Node_t * pNode );
extern int                 Gra_EdgeCreate( Gra_Node_t * pNodeI, Gra_Node_t * pNodeO );
extern void                Gra_EdgeDelete( Gra_Node_t * pNodeI, Gra_Node_t * pNodeO );
extern void                Gra_NodeCollapse( Gra_Node_t * pNode );
extern int                 Gra_NodeHasSelfLoop( Gra_Node_t * pNode );
extern void                Gra_GraphPrint( Gra_Graph_t * p );

/*=== graList.c =============================================================*/
extern void                Gra_NodeListAddLast( Gra_NodeList_t * pList, Gra_Node_t * pNode );
extern void                Gra_NodeListDelete( Gra_NodeList_t * pList, Gra_Node_t * pNode );

/*=== graVec.c ===============================================================*/
extern Gra_NodeVec_t *     Gra_NodeVecAlloc( int nCap );
extern void                Gra_NodeVecFree( Gra_NodeVec_t * p );
extern Gra_NodeVec_t *     Gra_NodeVecDup( Gra_NodeVec_t * p );
extern Gra_Node_t **       Gra_NodeVecReadArray( Gra_NodeVec_t * p );
extern int                 Gra_NodeVecReadSize( Gra_NodeVec_t * p );
extern void                Gra_NodeVecGrow( Gra_NodeVec_t * p, int nCapMin );
extern void                Gra_NodeVecShrink( Gra_NodeVec_t * p, int nSizeNew );
extern void                Gra_NodeVecClear( Gra_NodeVec_t * p );
extern void                Gra_NodeVecPush( Gra_NodeVec_t * p, Gra_Node_t * Entry );
extern int                 Gra_NodeVecPushUnique( Gra_NodeVec_t * p, Gra_Node_t * Entry );
extern void                Gra_NodeVecPushOrder( Gra_NodeVec_t * p, Gra_Node_t * pNode );
extern int                 Gra_NodeVecPushUniqueOrder( Gra_NodeVec_t * p, Gra_Node_t * pNode );
extern Gra_Node_t *        Gra_NodeVecPop( Gra_NodeVec_t * p );
extern void                Gra_NodeVecRemove( Gra_NodeVec_t * p, Gra_Node_t * Entry );
extern void                Gra_NodeVecWriteEntry( Gra_NodeVec_t * p, int i, Gra_Node_t * Entry );
extern Gra_Node_t *        Gra_NodeVecReadEntry( Gra_NodeVec_t * p, int i );

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

#endif

