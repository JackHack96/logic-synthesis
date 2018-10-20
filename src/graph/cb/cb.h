/**CFile****************************************************************

  FileName    [cb.h]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [External declarations of graph/club structure.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: cb.h,v 1.2 2003/05/27 23:14:43 alanmi Exp $]

***********************************************************************/

#ifndef __CB_H__
#define __CB_H__

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////
#include "mv.h"
#include "simpArray.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

typedef struct CbGraphStruct   Cb_Graph_t;
typedef struct CbVertexStruct  Cb_Vertex_t;
typedef struct CbOptionStruct  Cb_Option_t;


// directed graph
struct CbGraphStruct
{
    int               iTravs;           // traversal I.D.
    int               nLevels;          // number of levels
    int               nVertices;        // total number of vertices
    // references of special vertices (may overlap)
    sarray_t         *pRoots;           // sarray of roots (out-degree == 0)
    sarray_t         *pLeaves;          // sarray of leaves (in-degree == 0)
    // unique linked list of all vertices
    Cb_Vertex_t      *pVertexHead;      // list head
    Cb_Vertex_t      *pVertexTail;      // list tail
    Cb_Vertex_t      *pVertexSpec;      // special list (DFS, etc.)
};

struct CbVertexStruct 
{
    unsigned          Id;               // unique I.D.
    int               iTravs;           // traversal I.D.
    int               iLevel;           // level info after levelization
    int               nWeigh;           // integer weight of the vertex
    // information of parents and children 
    int               nIn;              // in-degree
    Cb_Vertex_t    ** pIn;              // parent vertices
    int               nOut;             // out-degree
    Cb_Vertex_t    ** pOut;             // child vertices
    // pointer in single linked lists
    Cb_Vertex_t     * pNext;            // linked list unique
    Cb_Vertex_t     * pNextSpec;        // linked list for special traversal
    Cb_Vertex_t     * pNextClub;        // linked list inside a club
    // customized data field
    Cb_Vertex_t     * pDomin;           // pointer to its dominator
    void            * pData1;           // user data #1
    void            * pData2;           // user data #2
};


// clubbing options
struct CbOptionStruct
{
    bool              fVerb;            // verbose mode
    int               nMaxOut;          // maximum # binary-bit output per club
    int               nMaxIn;           // maximum # binary-bit input per club
    int               nCost;            // maximum cost/(# F.F. lits) per club
    int               iMethod;          // which algorithm to use
};



////////////////////////////////////////////////////////////////////////
///                       MACRO DEFITIONS                            ///
////////////////////////////////////////////////////////////////////////

// linked list of vertices
#define Cb_ListForEachVertex( pHead, pV )                   \
        for ( pV = pHead; pV; pV = pV->pNext )

#define Cb_ListForEachVertexSafe( pHead, pV, pV2 )          \
        for ( pV = pHead,                                   \
              pV2 = ( pV ? pV->pNext : NULL);               \
              pV;                                           \
              pV = pV2,                                     \
              pV2 = ( pV ? pV->pNext : NULL) )

// the unique linked list in a graph
#define Cb_GraphForEachVertex( pG, pV )                     \
        for ( pV = pG->pVertexHead; pV; pV = pV->pNext )

#define Cb_GraphForEachVertexSafe( pG, pV, pV2 )            \
        for ( pV = pG->pVertexHead,                         \
              pV2 = ( pV ? pV->pNext : NULL);               \
              pV;                                           \
              pV = pV2,                                     \
              pV2 = ( pV ? pV->pNext : NULL) )

// the special linked list in a graph
#define Cb_GraphForEachVertexSpecial( pG, pV )              \
        for ( pV = pG->pVertexSpec; pV; pV = pV->pNextSpec )

#define Cb_GraphForEachVertexSpecialSafe( pG, pV, pV2 )     \
        for ( pV = pG->pVertexSpec,                         \
              pV2 = ( pV ? pV->pNextSpec : NULL);           \
              pV;                                           \
              pV = pV2,                                     \
              pV2 = ( pV ? pV->pNextSpec : NULL) )

// the club linked list in a sub-graph
#define Cb_GraphForEachVertexClub( pG, pV )                 \
        for ( pV = pG->pVertexHead; pV; pV = pV->pNextClub )

#define Cb_GraphForEachVertexClubSafe( pG, pV, pV2 )        \
        for ( pV = pG->pVertexHead,                         \
              pV2 = ( pV ? pV->pNextClub : NULL);           \
              pV;                                           \
              pV = pV2,                                     \
              pV2 = ( pV ? pV->pNextClub : NULL) )


// use the first 2 bit of the Id as flags
#define Cb_VertexSetFlag1( pV )        ( (pV)->Id |= 0x80000000 )
#define Cb_VertexSetFlag2( pV )        ( (pV)->Id |= 0x40000000 )
#define Cb_VertexResetFlag1( pV )      ( (pV)->Id &= 0x7FFFFFFF )
#define Cb_VertexResetFlag2( pV )      ( (pV)->Id &= 0xBFFFFFFF )
#define Cb_VertexTestFlag1( pV )       ( (pV)->Id &  0x80000000 )
#define Cb_VertexTestFlag2( pV )       ( (pV)->Id &  0x40000000 )


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/*===== cbApi.c ==========================================================*/
extern Cb_Graph_t *      Cb_GraphAlloc( int nRoots, int nLeaves );
extern void              Cb_GraphFree( Cb_Graph_t *pG );
extern void              Cb_GraphFreeVertices( Cb_Graph_t *pG );
extern int               Cb_GraphReadVertexNum( Cb_Graph_t *pG ) ;
extern int               Cb_GraphReadRootsNum( Cb_Graph_t *pG ) ;
extern sarray_t *        Cb_GraphReadRoots( Cb_Graph_t *pG ) ;
extern int               Cb_GraphReadLeavesNum( Cb_Graph_t *pG ) ;
extern sarray_t *        Cb_GraphReadLeaves( Cb_Graph_t *pG ) ;
extern void              Cb_GraphAddRoot( Cb_Graph_t *pG, Cb_Vertex_t *pV ) ;
extern void              Cb_GraphAddLeaf( Cb_Graph_t *pG, Cb_Vertex_t *pV ) ;
extern void              Cb_GraphAddVertexToTail( Cb_Graph_t *pG, Cb_Vertex_t *pV );

extern Cb_Vertex_t *     Cb_VertexAlloc( int nIn, int nOut );
extern void              Cb_VertexFree( Cb_Vertex_t *pV );
extern Cb_Vertex_t *     Cb_VertexReadInput( Cb_Vertex_t *pV, int i ) ;
extern Cb_Vertex_t *     Cb_VertexReadOutput( Cb_Vertex_t *pV, int i ) ;
extern void              Cb_VertexSetInput( Cb_Vertex_t *pV, int i, Cb_Vertex_t *pInput );
extern void              Cb_VertexSetOutput( Cb_Vertex_t *pV, int i, Cb_Vertex_t *pOutput );
extern bool              Cb_VertexIsGraphRoot( Cb_Vertex_t *pV );
extern bool              Cb_VertexIsGraphRootDriver( Cb_Vertex_t *pV );
extern bool              Cb_VertexIsGraphRootFanin( Cb_Vertex_t *pV );


/*===== cbClub.c ==========================================================*/
extern void              Cb_ClubAddVertexToTail( Cb_Graph_t *pClub, Cb_Vertex_t *pV );
extern void              Cb_ClubRemoveVertexFromList( Cb_Graph_t *pClub, Cb_Vertex_t *pV );
extern bool              Cb_ClubCheckVertexRoot( Cb_Graph_t *pClub, Cb_Vertex_t *pSpecial );
extern bool              Cb_ClubCheckVertexLeaf( Cb_Graph_t *pClub, Cb_Vertex_t *pSpecial );
extern bool              Cb_ClubCheckVertexCyclic( Cb_Graph_t *pClub, Cb_Vertex_t *pSpecial );
extern bool              Cb_ClubCheckInclusion( Cb_Option_t *pOpt, Cb_Graph_t *pClub,
                                                Cb_Vertex_t *pSpecial);
extern bool              Cb_ClubCheckInclusionGraphRoot( Cb_Graph_t *pClub,Cb_Vertex_t *pSpecial );
extern bool              Cb_ClubCheckGraphRootMerged( Cb_Graph_t *pClub );

extern void              Cb_ClubInclude( Cb_Graph_t *pClub, Cb_Vertex_t *pV, bool fAdjustBound );
extern void              Cb_ClubExclude( Cb_Graph_t *pClub, Cb_Vertex_t *pV, bool fAdjustBound );
extern void              Cb_ClubExpandFanout( Cb_Option_t *pOpt, Cb_Graph_t *pG, 
                                              Cb_Graph_t *pClub, int iLevel);
extern void              Cb_ClubImposeBoundary( Cb_Graph_t *pClub );
extern void              Cb_ClubExposeBoundary( Cb_Graph_t *pClub );
extern void              Cb_ClubListFree( sarray_t *listClubs );
extern void              Cb_ClubListPrint( sarray_t *listClubs );


/*===== cbCmd.c ==========================================================*/
extern Cb_Option_t *     Cb_OptionInit( Mv_Frame_t *pMvsis );


/*===== cbDfs.c ==========================================================*/
extern int               Cb_GraphLevelize( Cb_Graph_t *pG, Cb_Vertex_t **ppLevels, int fFromRoot );
extern int               Cb_GraphGetLevelNum( Cb_Graph_t *pG );
extern void              Cb_GraphDfs( Cb_Graph_t *pG, bool fFromRoot, bool fPreTrav );


/*===== cbDom.c ==========================================================*/
extern void              Cb_GraphDominators( Cb_Graph_t *pG );
extern sarray_t *        Cb_GraphClubDominator( Cb_Option_t *pOpt, Cb_Graph_t *pG );


/*===== cbGreedy.c ==========================================================*/
extern sarray_t *        Cb_GraphClubGreedy( Cb_Option_t *pOpt, Cb_Graph_t *pG );


/*===== cbNtk.c ==========================================================*/
extern void              Cb_NetworkClub( Cb_Option_t *pOpt, Ntk_Network_t *pNet );
extern void              Cb_NetworkClusterFromClubs( Ntk_Network_t *pNet, Cb_Graph_t *pG,
                                                     sarray_t *listClubs );
extern Cb_Graph_t *      Cb_GraphCreateFromNetwork( Ntk_Network_t *pNet );
extern Cb_Vertex_t *     Cb_VertexCreateFromNode( Ntk_Node_t  *pNode );


/*===== cbPrint.c ==========================================================*/
extern void              Cb_NetworkPrintDominators( FILE *pOut, Ntk_Network_t *pNet, int nNodes );
extern void              Cb_NetworkPrintDot( FILE *pOut, Ntk_Network_t *pNet );


/*===== cbUtils.c ==========================================================*/
extern void              Cb_GraphPrintDot( FILE *pFile, Cb_Graph_t *pG );
extern Cb_Vertex_t *     Cb_ListSpecialRemoveByIndex( Cb_Vertex_t **ppHead, int iSpecial );


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

#endif

