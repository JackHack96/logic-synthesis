/**CFile****************************************************************

  FileName    [aigTable.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [AND-INV graph package with built in sat_sweep.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 8, 2003.]

  Revision    [$Id: aigTable.c,v 1.1 2004/07/15 02:50:16 alanmi Exp $]

***********************************************************************/

#include "aigInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void Aig_TableResizeS( Aig_HashTable_t * p );
static void Aig_TableResizeF( Aig_HashTable_t * p );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Allocates the hash table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_HashTable_t * Aig_HashTableCreate()
{
    Aig_HashTable_t * p;
    // allocate the table
    p = ALLOC( Aig_HashTable_t, 1 );
    memset( p, 0, sizeof(Aig_HashTable_t) );
    // allocate and clean the bins
    p->nBins = Cudd_Prime(10000);
    p->pBins = ALLOC( Aig_Node_t *, p->nBins );
    memset( p->pBins, 0, sizeof(Aig_Node_t *) * p->nBins );
    return p;
}

/**Function*************************************************************

  Synopsis    [Deallocates the supergate hash table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_HashTableFree( Aig_HashTable_t * p )
{
    FREE( p->pBins );
    FREE( p );
}

/**Function*************************************************************

  Synopsis    [Looks up an entry in the structural hash table.]

  Description [If the entry with the same children does not exists, 
  creates it, inserts it into the table, and returns 0. If the entry 
  with the same children exists, finds it, and return 1. In both cases, 
  the new/old entry is returned in ppNodeRes.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_HashTableLookupS( Aig_Man_t * pMan, Aig_HashTable_t * p, Aig_Node_t * p1, Aig_Node_t * p2, Aig_Node_t ** ppNodeRes )
{
    Aig_Node_t * pEnt;
    unsigned Key;

    if ( p1 > p2 )
        pEnt = p1, p1 = p2, p2 = pEnt;

    Key = hashKey2( p1, p2, p->nBins );
    for ( pEnt = p->pBins[Key]; pEnt; pEnt = pEnt->pNextS )
        if ( pEnt->p1 == p1 && pEnt->p2 == p2 )
        {
            *ppNodeRes = pEnt;
            return 1;
        }
    // resize the table
    if ( p->nEntries >= 2 * p->nBins )
    {
        Aig_TableResizeS( p );
        Key = hashKey2( p1, p2, p->nBins );
    }
    // create the new node
    pEnt = Aig_NodeCreate( pMan, p1, p2 );
    // add the node to the corresponding linked list in the table
    pEnt->pNextS = p->pBins[Key];
    p->pBins[Key] = pEnt;
    *ppNodeRes = pEnt;
    p->nEntries++;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Insert the entry in the functional hash table.]

  Description [If the entry with the same key exists, return it right away.  
  If the entry with the same key does not exists, inserts it and returns NULL. ]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Node_t * Aig_HashTableLookupF( Aig_HashTable_t * p, Aig_Node_t * pNode )
{
    Aig_Node_t * pEnt;
    unsigned Key;
    int s;

    Key = pNode->pSims->uHash % p->nBins;
    for ( pEnt = p->pBins[Key]; pEnt; pEnt = pEnt->pNextF )
    {
        for ( s = 0; s < NSIMS; s++ )
            if ( pEnt->pSims->uTests[s] != pNode->pSims->uTests[s] )
                break;
        if ( s == NSIMS ) // equivalent up to the complement
            return pEnt; 
    }
    // resize the table
    if ( p->nEntries >= 2 * p->nBins )
    {
        Aig_TableResizeF( p );
        Key = pNode->pSims->uHash % p->nBins;
    }
    // add the node to the corresponding linked list in the table
    pNode->pNextF = p->pBins[Key];
    p->pBins[Key] = pNode;
    p->nEntries++;
    // return NULL, because there is no functional equivalence in this case
    return NULL;
}


/**Function*************************************************************

  Synopsis    [Resizes the table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_TableResizeS( Aig_HashTable_t * p )
{
    Aig_Node_t ** pBinsNew;
    Aig_Node_t * pEnt, * pEnt2;
    int nBinsNew, Counter, i, clk;
    unsigned Key;

clk = clock();
    // get the new table size
    nBinsNew = Cudd_Prime(2 * p->nBins); 
    // allocate a new array
    pBinsNew = ALLOC( Aig_Node_t *, nBinsNew );
    memset( pBinsNew, 0, sizeof(Aig_Node_t *) * nBinsNew );
    // rehash the entries from the old table
    Counter = 0;
    for ( i = 0; i < p->nBins; i++ )
        for ( pEnt = p->pBins[i], pEnt2 = pEnt? pEnt->pNextS: NULL; pEnt; 
              pEnt = pEnt2, pEnt2 = pEnt? pEnt->pNextS: NULL )
        {
            Key = hashKey2( pEnt->p1, pEnt->p2, nBinsNew );
            pEnt->pNextS = pBinsNew[Key];
            pBinsNew[Key] = pEnt;
            Counter++;
        }
    assert( Counter == p->nEntries );
//    printf( "Increasing the unique table size from %6d to %6d. ", p->nBins, nBinsNew );
//    PRT( "Time", clock() - clk );
    // replace the table and the parameters
    free( p->pBins );
    p->pBins = pBinsNew;
    p->nBins = nBinsNew;
}

/**Function*************************************************************

  Synopsis    [Resizes the table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_TableResizeF( Aig_HashTable_t * p )
{
    Aig_Node_t ** pBinsNew;
    Aig_Node_t * pEnt, * pEnt2;
    int nBinsNew, Counter, i, clk;
    unsigned Key;

clk = clock();
    // get the new table size
    nBinsNew = Cudd_Prime(2 * p->nBins); 
    // allocate a new array
    pBinsNew = ALLOC( Aig_Node_t *, nBinsNew );
    memset( pBinsNew, 0, sizeof(Aig_Node_t *) * nBinsNew );
    // rehash the entries from the old table
    Counter = 0;
    for ( i = 0; i < p->nBins; i++ )
        for ( pEnt = p->pBins[i], pEnt2 = pEnt? pEnt->pNextF: NULL; pEnt; 
              pEnt = pEnt2, pEnt2 = pEnt? pEnt->pNextF: NULL )
        {
            Key = pEnt->pSims->uHash % nBinsNew;
            pEnt->pNextF = pBinsNew[Key];
            pBinsNew[Key] = pEnt;
            Counter++;
        }
    assert( Counter == p->nEntries );
//    printf( "Increasing the unique table size from %6d to %6d. ", p->nBins, nBinsNew );
//    PRT( "Time", clock() - clk );
    // replace the table and the parameters
    free( p->pBins );
    p->pBins = pBinsNew;
    p->nBins = nBinsNew;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


