/**CFile****************************************************************

  FileName    [shHash.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Hashing the two-input AND gates.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: shHash.c,v 1.7 2004/06/01 02:11:53 alanmi Exp $]

***********************************************************************/

#include "shInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

struct ShTableStruct
{
    Sh_Node_t ** pBins;       // the table of entries
    int          nSizeInit;   // the requested size of the hash table
    int          nSize;       // the actual size of the hash table
    int          nNodes;      // the total number of AND nodes in the table
    int          nNodesId;    // the next node ID to assign
    Sh_Node_t *  pListUnique; // the list of unique nodes
    int          nListUnique; // the number of unique nodes
};

// iterators through the entries in one bin of the hash table
#define Sh_TableBinForEachEntry( pBin, pEnt )              \
    for ( pEnt = pBin;                                     \
          pEnt;                                            \
          pEnt = pEnt->pNext )
#define Sh_TableBinForEachEntrySafe( pBin, pEnt, pEnt2 )   \
    for ( pEnt = pBin,                                     \
          pEnt2 = pEnt? pEnt->pNext: NULL;                 \
          pEnt;                                            \
          pEnt = pEnt2,                                    \
          pEnt2 = pEnt? pEnt->pNext: NULL )

// static procedures
static void Sh_TableReclaimNode( Sh_Manager_t * pMan, Sh_Node_t * gNode );
static void Sh_TableResize( Sh_Manager_t * pMan );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Allocates the hash table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sh_Table_t * Sh_TableAlloc( int nSizeInit )
{
    Sh_Table_t * p;
    p = ALLOC( Sh_Table_t, 1 );
    memset( p, 0, sizeof(Sh_Table_t) );
    p->nSizeInit = nSizeInit;
    p->nSize = Cudd_Prime(p->nSizeInit);
    p->pBins = ALLOC( Sh_Node_t *, p->nSize );
    memset( p->pBins, 0, sizeof(Sh_Node_t *) * p->nSize );
    p->nNodes     = 0;
    p->nNodesId   = SH_CONST_INDEX + 1;
    return p;
}

/**Function*************************************************************

  Synopsis    [Frees the hash table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sh_TableFree( Sh_Manager_t * pMan, bool fCheckZeroRefs )
{
    Sh_Table_t * p = pMan->pTable;
    int nRefNodes;
    if ( fCheckZeroRefs )
        if ( nRefNodes = Sh_ManagerCheckZeroRefs( pMan ) )
        {
            printf( "\n" );
            printf( "The deleted AI-graph manager has %d referenced nodes.\n", nRefNodes );
            printf( "Reference leaks may adversely impact performance on large problems.\n" );
            printf( "\n" );
        }

#ifdef USE_SYSTEM_MEMORY_MANAGEMENT
    Sh_Node_t * pEnt, * pEnt2;
    int i;
    // delete var maps
    for ( i = 0; i < p->nSize; i++ )
        Sh_TableBinForEachEntrySafe( p->pBins[i], pEnt, pEnt2 )
        {
            assert( pEnt->nRefs == 0 );
            FREE( pEnt );
        }
    // delete the unique nodes
    Sh_TableBinForEachEntrySafe( pTable->pListUnique, pEnt, pEnt2 )
    {
        assert( pEnt->nRefs == 0 );
        FREE( pEnt );
    }
#endif
    FREE( p->pBins );
    FREE( p );
}

/**Function*************************************************************

  Synopsis    [Returns the number of node in the table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sh_TableReadNodes( Sh_Manager_t * pMan )
{
    return pMan->pTable->nNodes;
}

/**Function*************************************************************

  Synopsis    [Returns the number of unhashed node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sh_TableReadNodeUniqueNum( Sh_Manager_t * pMan )
{
    return pMan->pTable->nListUnique;
}

/**Function*************************************************************

  Synopsis    [Looks up the node in the table.]

  Description [This procedure implements one-level hashing. All the nodes
  are hashed by their children. If the node with the same children was already
  created, it is returned by the call to this funtion. If it does not exist,
  this procedure creates a new node with these children. The reference counter
  of the returned node is not incremented. In the rare case, when a node is 
  found dead in the table (existing but with reference count equal to zero), 
  this procedure reclaims the dead node by increasing the number of references
  in the node's predecessors, but the node itself is returned unreferenced.
  This procedure may invoke garbage collection and table resizing if the number 
  of existing nodes exceeds specified limits.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sh_Node_t * Sh_TableLookup( Sh_Manager_t * pMan, Sh_Node_t * gNode1, Sh_Node_t * gNode2 )
{
    Sh_Table_t * p = pMan->pTable;
    Sh_Node_t *  gNode;
    unsigned Key;

    if ( gNode1 == gNode2 )
        return gNode1;

    assert( Sh_Regular(gNode1)->index != Sh_Regular(gNode2)->index );
    if ( gNode1 > gNode2 )
        gNode = gNode1, gNode1 = gNode2, gNode2 = gNode;

    Key = hashKey2( gNode1, gNode2, p->nSize );
    Sh_TableBinForEachEntry( p->pBins[Key], gNode )
        if ( gNode->pOne == gNode1 && gNode->pTwo == gNode2 )
        {
            // if the node is dead, bring to life its children
            // but do not increment the reference counter of the node itself
            if ( gNode->nRefs == 0 )
                Sh_TableReclaimNode(pMan, gNode);
            return gNode;
        }

    // check if it is a good time for a garbage collection  
    if ( pMan->fEnableGC && p->nNodes >= 3 * p->nSizeInit )
    {
        Sh_ManagerGarbageCollect( pMan );
        // check if it is a good time for tahle resizing
        if ( p->nNodes >= 2 * p->nSizeInit )
        {
            Sh_TableResize( pMan );
            Key = hashKey2( gNode1, gNode2, p->nSize );
        }
    }

    // create the new node
#ifdef USE_SYSTEM_MEMORY_MANAGEMENT
    gNode = ALLOC( Sh_Node_t, 1 );
#else
    gNode = (Sh_Node_t *)Extra_MmFixedEntryFetch( pMan->pMem );
#endif
    memset( gNode, 0, sizeof(Sh_Node_t) );
    gNode->pOne  = gNode1;         shRef( gNode1 );
    gNode->pTwo  = gNode2;         shRef( gNode2 );
    gNode->index = p->nNodesId++;
    // increment the number of nodes in the table
    p->nNodes++;
    // add the node to the corresponding linked list in the table
    gNode->pNext = p->pBins[Key];
    p->pBins[Key] = gNode;
    // report progress to the user
    if ( pMan->fVerbose && p->nNodesId % 1000000 == 0 )
        printf( "Total nodes created = %8d. Nodes in the table = %8d.\n", p->nNodesId, p->nNodes );
    return gNode;
}

/**Function*************************************************************

  Synopsis    [Recursively reclaims the node from the dead.]

  Description [The reference counter of the returned node is not changed,
  but the reference counters of all its children and their children is
  incremented by 1.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sh_TableReclaimNode( Sh_Manager_t * pMan, Sh_Node_t * gNode )
{
    Sh_Node_t * gNodeR;
    assert( !Sh_IsComplement(gNode) );
    assert( gNode->nRefs == 0 );
    if ( shNodeIsConst(gNode) )
        return;
    if ( shNodeIsVar(gNode) )
        return;
    // if the left son is dead, bring him to life
    gNodeR = Sh_Regular(gNode->pOne);
    if ( gNodeR->nRefs == 0 )
        Sh_TableReclaimNode( pMan, gNodeR );
    // in any case, increment the reference counter
    gNodeR->nRefs++;
    // if the right son is dead, bring him to life
    gNodeR = Sh_Regular(gNode->pTwo);
    if ( gNodeR->nRefs == 0 )
        Sh_TableReclaimNode( pMan, gNodeR );
    // in any case, increment the reference counter
    gNodeR->nRefs++;
}

/**Function*************************************************************

  Synopsis    [Creates a unique node equivalent to the given node.]

  Description [A unique node is used to represent the funtionality of the
  given node without overlapping with functionalities that hash into the
  same node. The unique nodes are essentially duplicates of some nodes
  in the table. The unique nodes are not in the table but in a special 
  linked list.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sh_Node_t * Sh_TableInsertUnique( Sh_Manager_t * pMan, Sh_Node_t * gNode )
{
    Sh_Node_t * gNodeR, * gNodeUnique;
    bool fCompl;

    gNodeR = Sh_Regular(gNode);
    fCompl = Sh_IsComplement(gNode);

    // create the new node
#ifdef USE_SYSTEM_MEMORY_MANAGEMENT
    gNodeUnique = ALLOC( Sh_Node_t, 1 );
#else
    gNodeUnique = (Sh_Node_t *)Extra_MmFixedEntryFetch( pMan->pMem );
#endif
    memset( gNodeUnique, 0, sizeof(Sh_Node_t) );
    gNodeUnique->pOne  = gNodeR->pOne;   shRef( gNodeR->pOne );
    gNodeUnique->pTwo  = gNodeR->pTwo;   shRef( gNodeR->pTwo );
    gNodeUnique->index = - pMan->pTable->nListUnique - 1;

    // add the unique node to the list of unique nodes
    gNodeUnique->pNext = pMan->pTable->pListUnique;
    pMan->pTable->pListUnique = gNodeUnique;
    pMan->pTable->nListUnique++;
    return Sh_NotCond( gNodeUnique, fCompl );
}

/**Function*************************************************************

  Synopsis    [Performs garbage collection in the table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sh_ManagerGarbageCollect( Sh_Manager_t * pMan )
{
    Sh_Table_t * pTable = pMan->pTable;
    Sh_Node_t * pEnt, * pEnt2;
    Sh_Node_t ** ppList;
    int nNodesNew, i, clk;
clk = clock();
    // start the counter of remaining nodes
    nNodesNew = 0;
    // go through the nodes in the table
    for ( i = 0; i < pTable->nSize; i++ )
    {
        ppList = pTable->pBins + i;
        Sh_TableBinForEachEntrySafe( pTable->pBins[i], pEnt, pEnt2 )
        {
            if ( pEnt->nRefs == 0 )
            {
#ifdef USE_SYSTEM_MEMORY_MANAGEMENT
                free( pEnt );
#else
                Extra_MmFixedEntryRecycle( pMan->pMem, (char *)pEnt );
#endif
            }
            else
            {
                *ppList = pEnt;
                ppList = &pEnt->pNext;
                nNodesNew++;
            }
        }
        *ppList = NULL;
    }
    // go through the nodes in the unique list
    ppList = &pTable->pListUnique;
    Sh_TableBinForEachEntrySafe( pTable->pListUnique, pEnt, pEnt2 )
        if ( pEnt->nRefs == 0 )
        {
#ifdef USE_SYSTEM_MEMORY_MANAGEMENT
            free( pEnt );
#else
            Extra_MmFixedEntryRecycle( pMan->pMem, (char *)pEnt );
#endif
        }
        else
        {
            *ppList = pEnt;
            ppList = &pEnt->pNext;
            nNodesNew++;
        }
    *ppList = NULL;
    if ( pMan->fVerbose )
    {
        printf( "Garbage collection reduced nodes from %8d to %8d. ", pTable->nNodes, nNodesNew );
        PRT( "Time", clock() - clk );
    }
    pTable->nNodes = nNodesNew;    
}

/**Function*************************************************************

  Synopsis    [Resizes the table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sh_TableResize( Sh_Manager_t * pMan )
{
    Sh_Table_t * p = pMan->pTable;
    Sh_Node_t ** pBinsNew;
    Sh_Node_t * pEnt, * pEnt2;
    unsigned Key;
    int nSizeInit, nSize;
    int Counter, i, clk;
clk = clock();
    // get the new table size
    nSizeInit = 2 * p->nSizeInit;
    nSize     = Cudd_Prime(nSizeInit); 
    // allocate a new array
    pBinsNew = ALLOC( Sh_Node_t *, nSize );
    memset( pBinsNew, 0, sizeof(Sh_Node_t *) * nSize );
    // rehash the entries from the old table
    Counter = 0;
    for ( i = 0; i < p->nSize; i++ )
        Sh_TableBinForEachEntrySafe( p->pBins[i], pEnt, pEnt2 )
        {
            Key = hashKey2( pEnt->pOne, pEnt->pTwo, nSize );
            pEnt->pNext = pBinsNew[Key];
            pBinsNew[Key] = pEnt;
            Counter++;
        }
    assert( Counter == p->nNodes );
    if ( pMan->fVerbose )
    {
        printf( "Increasing the unique table size from %8d to %8d. ", p->nSize, nSize );
        PRT( "Time", clock() - clk );
    }
    // replace the table and the parameters
    free( p->pBins );
    p->pBins = pBinsNew;
    p->nSizeInit = nSizeInit;
    p->nSize = nSize;
}

/**Function*************************************************************

  Synopsis    [Returns the number of nodes with non-zero reference count.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sh_ManagerCheckZeroRefs( Sh_Manager_t * pMan )
{
    Sh_Table_t * pTable = pMan->pTable;
    Sh_Node_t * pEnt;
    int i, Counter;
    // clean normal nodes
    Counter = 0;
    for ( i = 0; i < pTable->nSize; i++ )
        Sh_TableBinForEachEntry( pTable->pBins[i], pEnt )
            Counter += (pEnt->nRefs > 0);
    // clean normal nodes
    Sh_TableBinForEachEntry( pTable->pListUnique, pEnt )
        Counter += (pEnt->nRefs > 0);
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Cleans the data fields of all nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sh_ManagerCleanData( Sh_Manager_t * pMan )
{
    Sh_Table_t * pTable = pMan->pTable;
    Sh_Node_t * pEnt;
    int i;
    // clean normal nodes
    for ( i = 0; i < pTable->nSize; i++ )
        Sh_TableBinForEachEntry( pTable->pBins[i], pEnt )
            pEnt->pData = 0, pEnt->pData2 = 0;
    // clean normal nodes
    Sh_TableBinForEachEntry( pTable->pListUnique, pEnt )
        pEnt->pData = 0, pEnt->pData2 = 0;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


