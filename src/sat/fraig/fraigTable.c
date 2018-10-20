/**CFile****************************************************************

  FileName    [fraigTable.c]

  PackageName [FRAIG: Functionally reduced AND-INV graphs.]

  Synopsis    [Structural and functional hash tables.]

  Author      [Alan Mishchenko <alanmi@eecs.berkeley.edu>]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.1. Started - October 1, 2004]

  Revision    [$Id: fraigTable.c,v 1.7 2005/07/08 01:01:34 alanmi Exp $]

***********************************************************************/

#include "fraigInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void Fraig_TableResizeS( Fraig_HashTable_t * p );
static void Fraig_TableResizeF( Fraig_HashTable_t * p, int fUseSimR );

// copied from CUDD for stand-aloneness
static unsigned int Cudd_Prime( unsigned int  p );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Allocates the hash table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fraig_HashTable_t * Fraig_HashTableCreate( int nSize )
{
    Fraig_HashTable_t * p;
    // allocate the table
    p = ALLOC( Fraig_HashTable_t, 1 );
    memset( p, 0, sizeof(Fraig_HashTable_t) );
    // allocate and clean the bins
    p->nBins = Cudd_Prime(nSize);
    p->pBins = ALLOC( Fraig_Node_t *, p->nBins );
    memset( p->pBins, 0, sizeof(Fraig_Node_t *) * p->nBins );
    return p;
}

/**Function*************************************************************

  Synopsis    [Deallocates the supergate hash table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_HashTableFree( Fraig_HashTable_t * p )
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
int Fraig_HashTableLookupS( Fraig_Man_t * pMan, Fraig_Node_t * p1, Fraig_Node_t * p2, Fraig_Node_t ** ppNodeRes )
{
    Fraig_HashTable_t * p = pMan->pTableS;
    Fraig_Node_t * pEnt;
    unsigned Key;

    // order the arguments
    if ( Fraig_Regular(p1)->Num > Fraig_Regular(p2)->Num )
        pEnt = p1, p1 = p2, p2 = pEnt;

    Key = Fraig_HashKey2( p1, p2, p->nBins );
    Fraig_TableBinForEachEntryS( p->pBins[Key], pEnt )
        if ( pEnt->p1 == p1 && pEnt->p2 == p2 )
        {
            *ppNodeRes = pEnt;
            // if the node was dead, bring to life its children
            // but do not increment the reference counter of the node itself
#ifdef FRAIG_USE_REF_COUNTING
            if ( pMan->fRefCount )  
            {
                if ( pEnt->pRepr == NULL && pEnt->nRefs == 0 )
                    Fraig_TableReclaimNode(pMan, pEnt);
                else if ( pEnt->pRepr && pEnt->pRepr->nRefs == 0 )
                    Fraig_TableReclaimNode(pMan, pEnt->pRepr);
            }
#endif
            return 1;
        }

#ifdef FRAIG_USE_REF_COUNTING
    // check if it is a good time for a garbage collection  
    if ( pMan->fRefCount && p->nEntries >= 3 * p->nBins )
    {
        Fraig_ManagerGarbageCollect( pMan );
        // check if it is a good time for table resizing
        if ( p->nEntries >= 2 * p->nBins )
        {
            Fraig_TableResizeS( p );
            Key = Fraig_HashKey2( p1, p2, p->nBins );
        }
    }
#else
    // check if it is a good time for table resizing
    if ( !pMan->fRefCount && p->nEntries >= 2 * p->nBins )
    {
        Fraig_TableResizeS( p );
        Key = Fraig_HashKey2( p1, p2, p->nBins );
    }
#endif

    // create the new node
    pEnt = Fraig_NodeCreate( pMan, p1, p2 );
    // add the node to the corresponding linked list in the table
    pEnt->pNextS = p->pBins[Key];
    p->pBins[Key] = pEnt;
    *ppNodeRes = pEnt;
    p->nEntries++;
//printf( "S: adding node %d\n", pEnt->Num );
    return 0;
}


/**Function*************************************************************

  Synopsis    [Insert the entry in the functional hash table.]

  Description [If the entry with the same key exists, return it right away.  
  If the entry with the same key does not exists, inserts it and returns NULL. ]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fraig_Node_t * Fraig_HashTableLookupF( Fraig_Man_t * pMan, Fraig_Node_t * pNode )
{
    Fraig_HashTable_t * p = pMan->pTableF;
    Fraig_Node_t * pEnt, * pEntD;
    unsigned Key;

    // go through the hash table entries
    Key = pNode->pSimsR->uHash % p->nBins;
    Fraig_TableBinForEachEntryF( p->pBins[Key], pEnt )
    {
        // if their simulation info differs, skip
        if ( !Fraig_CompareSimInfo( pNode, pEnt, FRAIG_SIM_ROUNDS, 1 ) )
            continue;
        // equivalent up to the complement
        Fraig_TableBinForEachEntryD( pEnt, pEntD )
        {
            // if their simulation info differs, skip
            if ( !Fraig_CompareSimInfo( pNode, pEntD, pMan->iWordStart, 0 ) )
                continue;
            // found a simulation-equivalent node
            return pEntD; 
        }
        // did not find a simulation equivalent node
        // add the node to the corresponding linked list
        pNode->pNextD = pEnt->pNextD;
        pEnt->pNextD  = pNode;
        // return NULL, because there is no functional equivalence in this case
        return NULL;
    }

    // check if it is a good time for table resizing
    if ( p->nEntries >= 2 * p->nBins )
    {
        Fraig_TableResizeF( p, 1 );
        Key = pNode->pSimsD->uHash % p->nBins;
    }

    // add the node to the corresponding linked list in the table
    pNode->pNextF = p->pBins[Key];
    p->pBins[Key] = pNode;
    p->nEntries++;
    // return NULL, because there is no functional equivalence in this case
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Insert the entry in the functional hash table.]

  Description [If the entry with the same key exists, return it right away.  
  If the entry with the same key does not exists, inserts it and returns NULL. ]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fraig_Node_t * Fraig_HashTableLookupF0( Fraig_Man_t * pMan, Fraig_Node_t * pNode )
{
    Fraig_HashTable_t * p = pMan->pTableF0;
    Fraig_Node_t * pEnt;
    unsigned Key;

    // go through the hash table entries
    Key = pNode->pSimsD->uHash % p->nBins;
    Fraig_TableBinForEachEntryF( p->pBins[Key], pEnt )
    {
        // if their simulation info differs, skip
        if ( !Fraig_CompareSimInfo( pNode, pEnt, pMan->iWordStart, 0 ) )
            continue;
        // found a simulation-equivalent node
        return pEnt; 
    }

    // check if it is a good time for table resizing
    if ( p->nEntries >= 2 * p->nBins )
    {
        Fraig_TableResizeF( p, 0 );
        Key = pNode->pSimsD->uHash % p->nBins;
    }

    // add the node to the corresponding linked list in the table
    pNode->pNextF = p->pBins[Key];
    p->pBins[Key] = pNode;
    p->nEntries++;
    // return NULL, because there is no functional equivalence in this case
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Insert the entry in the functional hash table.]

  Description [Unconditionally add the node to the corresponding 
  linked list in the table.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_HashTableInsertF0( Fraig_Man_t * pMan, Fraig_Node_t * pNode )
{
    Fraig_HashTable_t * p = pMan->pTableF0;
    unsigned Key = pNode->pSimsD->uHash % p->nBins;

    pNode->pNextF = p->pBins[Key];
    p->pBins[Key] = pNode;
    p->nEntries++;
}


/**Function*************************************************************

  Synopsis    [Resizes the table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_TableResizeS( Fraig_HashTable_t * p )
{
    Fraig_Node_t ** pBinsNew;
    Fraig_Node_t * pEnt, * pEnt2;
    int nBinsNew, Counter, i, clk;
    unsigned Key;

clk = clock();
    // get the new table size
    nBinsNew = Cudd_Prime(2 * p->nBins); 
    // allocate a new array
    pBinsNew = ALLOC( Fraig_Node_t *, nBinsNew );
    memset( pBinsNew, 0, sizeof(Fraig_Node_t *) * nBinsNew );
    // rehash the entries from the old table
    Counter = 0;
    for ( i = 0; i < p->nBins; i++ )
        Fraig_TableBinForEachEntrySafeS( p->pBins[i], pEnt, pEnt2 )
        {
            Key = Fraig_HashKey2( pEnt->p1, pEnt->p2, nBinsNew );
            pEnt->pNextS = pBinsNew[Key];
            pBinsNew[Key] = pEnt;
            Counter++;
        }
    assert( Counter == p->nEntries );
//    printf( "Increasing the structural table size from %6d to %6d. ", p->nBins, nBinsNew );
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
void Fraig_TableResizeF( Fraig_HashTable_t * p, int fUseSimR )
{
    Fraig_Node_t ** pBinsNew;
    Fraig_Node_t * pEnt, * pEnt2;
    int nBinsNew, Counter, i, clk;
    unsigned Key;

clk = clock();
    // get the new table size
    nBinsNew = Cudd_Prime(2 * p->nBins); 
    // allocate a new array
    pBinsNew = ALLOC( Fraig_Node_t *, nBinsNew );
    memset( pBinsNew, 0, sizeof(Fraig_Node_t *) * nBinsNew );
    // rehash the entries from the old table
    Counter = 0;
    for ( i = 0; i < p->nBins; i++ )
        Fraig_TableBinForEachEntrySafeF( p->pBins[i], pEnt, pEnt2 )
        {
            if ( fUseSimR )
                Key = pEnt->pSimsR->uHash % nBinsNew;
            else
                Key = pEnt->pSimsD->uHash % nBinsNew;
            pEnt->pNextF = pBinsNew[Key];
            pBinsNew[Key] = pEnt;
            Counter++;
        }
    assert( Counter == p->nEntries );
//    printf( "Increasing the functional table size from %6d to %6d. ", p->nBins, nBinsNew );
//    PRT( "Time", clock() - clk );
    // replace the table and the parameters
    free( p->pBins );
    p->pBins = pBinsNew;
    p->nBins = nBinsNew;
}

/**Function********************************************************************

  Synopsis    [Returns the next prime &gt;= p.]

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
unsigned int Cudd_Prime( unsigned int  p)
{
    int i,pn;

    p--;
    do {
        p++;
        if (p&1) {
	    pn = 1;
	    i = 3;
	    while ((unsigned) (i * i) <= p) {
		if (p % i == 0) {
		    pn = 0;
		    break;
		}
		i += 2;
	    }
	} else {
	    pn = 0;
	}
    } while (!pn);
    return(p);

} /* end of Cudd_Prime */


/**Function*************************************************************

  Synopsis    [Compares two pieces of simulation info.]

  Description [Returns 1 if they are equal.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_CompareSimInfo( Fraig_Node_t * pNode1, Fraig_Node_t * pNode2, int iWordLast, int fUseRand )
{
    Fraig_Sims_t * pSims1, * pSims2;
    int i;
    assert( !Fraig_IsComplement(pNode1) );
    assert( !Fraig_IsComplement(pNode2) );
    // get hold of simulation info
    pSims1 = fUseRand? pNode1->pSimsR : pNode1->pSimsD;
    pSims2 = fUseRand? pNode2->pSimsR : pNode2->pSimsD;    
    // if their signatures differ, skip
    if ( pSims1->uHash != pSims2->uHash )
        return 0;
    // check the simulation info
    for ( i = 0; i < iWordLast; i++ )
        if ( pSims1->uTests[i] != pSims2->uTests[i] )
            return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Compares two pieces of simulation info.]

  Description [Returns 1 if they are equal.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_CompareSimInfoUnderMask( Fraig_Node_t * pNode1, Fraig_Node_t * pNode2, int iWordLast, int fUseRand, unsigned * puMask )
{
    Fraig_Sims_t * pSims1, * pSims2;
    int i;
    assert( !Fraig_IsComplement(pNode1) );
    assert( !Fraig_IsComplement(pNode2) );
    // get hold of simulation info
    pSims1 = fUseRand? pNode1->pSimsR : pNode1->pSimsD;
    pSims2 = fUseRand? pNode2->pSimsR : pNode2->pSimsD;    
    // if their signatures differ, skip
//    if ( pSims1->uHash != pSims2->uHash )
//        return 0;
    // check the simulation info
    for ( i = 0; i < iWordLast; i++ )
        if ( (pSims1->uTests[i] & puMask[i]) != (pSims2->uTests[i] & puMask[i]) )
            return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Compares two pieces of simulation info.]

  Description [Returns 1 if they are equal.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_CollectXors( Fraig_Node_t * pNode1, Fraig_Node_t * pNode2, int iWordLast, int fUseRand, unsigned * puMask )
{
    Fraig_Sims_t * pSims1, * pSims2;
    int i;
    assert( !Fraig_IsComplement(pNode1) );
    assert( !Fraig_IsComplement(pNode2) );
    // get hold of simulation info
    pSims1 = fUseRand? pNode1->pSimsR : pNode1->pSimsD;
    pSims2 = fUseRand? pNode2->pSimsR : pNode2->pSimsD;    
    // check the simulation info
    for ( i = 0; i < iWordLast; i++ )
        puMask[i] = ( pSims1->uTests[i] ^ pSims2->uTests[i] );
}


/**Function*************************************************************

  Synopsis    [Prints stats of the structural table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_TablePrintStatsS( Fraig_Man_t * pMan )
{
    Fraig_HashTable_t * pT = pMan->pTableS;
    Fraig_Node_t * pNode;
    int i, Counter;

    printf( "Structural table. Table size = %d. Number of entries = %d.\n", pT->nBins, pT->nEntries );
    for ( i = 0; i < pT->nBins; i++ )
    {
        Counter = 0;
        Fraig_TableBinForEachEntryS( pT->pBins[i], pNode )
            Counter++;
        if ( Counter > 1 )
        {
            printf( "%d ", Counter );
            if ( Counter > 50 )
                printf( "{%d} ", i );
        }
    }
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    [Prints stats of the structural table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_TablePrintStatsF( Fraig_Man_t * pMan )
{
    Fraig_HashTable_t * pT = pMan->pTableF;
    Fraig_Node_t * pNode;
    int i, Counter;

    printf( "Functional table. Table size = %d. Number of entries = %d.\n", pT->nBins, pT->nEntries );
    for ( i = 0; i < pT->nBins; i++ )
    {
        Counter = 0;
        Fraig_TableBinForEachEntryF( pT->pBins[i], pNode )
            Counter++;
        if ( Counter > 1 )
            printf( "{%d} ", Counter );
    }
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    [Prints stats of the structural table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_TablePrintStatsF0( Fraig_Man_t * pMan )
{
    Fraig_HashTable_t * pT = pMan->pTableF0;
    Fraig_Node_t * pNode;
    int i, Counter;

    printf( "Zero-node table. Table size = %d. Number of entries = %d.\n", pT->nBins, pT->nEntries );
    for ( i = 0; i < pT->nBins; i++ )
    {
        Counter = 0;
        Fraig_TableBinForEachEntryF( pT->pBins[i], pNode )
            Counter++;
        if ( Counter == 0 )
            continue;
/*
        printf( "\nBin = %4d :  Number of entries = %4d\n", i, Counter );
        Fraig_TableBinForEachEntryF( pT->pBins[i], pNode )
            printf( "Node %5d. Hash = %10d.\n", pNode->Num, pNode->pSimsD->uHash );
*/
    }
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    [Rehashes the table after the simulation info has changed.]

  Description [Assumes that the hash values have been updated after performing
  additional simulation. Rehashes the table using the new hash values. 
  Uses pNextF to link the entries in the bins. Uses pNextD to link the entries 
  with identical hash values. Returns 1 if the identical entries have been found.
  Note that identical hash values may mean that the simulation data is different.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_TableRehashF0( Fraig_Man_t * pMan, int fLinkEquiv )
{
    Fraig_HashTable_t * pT = pMan->pTableF0;
    Fraig_Node_t ** pBinsNew;
    Fraig_Node_t * pEntF, * pEntF2, * pEnt, * pEntD2, * pEntN;
    int ReturnValue, Counter, i;
    unsigned Key;

    // allocate a new array of bins
    pBinsNew = ALLOC( Fraig_Node_t *, pT->nBins );
    memset( pBinsNew, 0, sizeof(Fraig_Node_t *) * pT->nBins );

    // rehash the entries in the table
    // go through all the nodes in the F-lists (and possible in D-lists, if used)
    Counter = 0;
    ReturnValue = 0;
    for ( i = 0; i < pT->nBins; i++ )
        Fraig_TableBinForEachEntrySafeF( pT->pBins[i], pEntF, pEntF2 )
        Fraig_TableBinForEachEntrySafeD( pEntF, pEnt, pEntD2 )
        {
            // decide where to put entry pEnt
            Key = pEnt->pSimsD->uHash % pT->nBins;
            if ( fLinkEquiv )
            {
                // go through the entries in the new bin
                Fraig_TableBinForEachEntryF( pBinsNew[Key], pEntN )
                {
                    // if they have different values skip
                    if ( pEnt->pSimsD->uHash != pEntN->pSimsD->uHash )
                        continue;
                    // they have the same hash value, add pEnt to the D-list pEnt3
                    pEnt->pNextD  = pEntN->pNextD;
                    pEntN->pNextD = pEnt;
                    ReturnValue = 1;
                    Counter++;
                    break;
                }
                if ( pEntN != NULL ) // already linked
                    continue;
                // we did not find equal entry
            }
            // link the new entry
            pEnt->pNextF  = pBinsNew[Key];
            pBinsNew[Key] = pEnt;
            pEnt->pNextD  = NULL;
            Counter++;
        }
    assert( Counter == pT->nEntries );
    // replace the table and the parameters
    free( pT->pBins );
    pT->pBins = pBinsNew;
    return ReturnValue;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


