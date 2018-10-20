/**CFile****************************************************************

  FileName    [auPart.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Various procedures working with automata and FSMs.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: auPart.c,v 1.1 2004/02/19 03:06:47 alanmi Exp $]

***********************************************************************/

#include "auInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// this structure stores information about state partitions
typedef struct Au_SPInfoInt_t_ Au_SPInfoInt_t;
struct Au_SPInfoInt_t_
{
    DdManager * dd;         // the BDD manager
    int       nParts;       // the number of state partitions
    int       nPartsAlloc;  // the number of state partitions allocated
    int *     pSizes;       // the sizes of each partition
    int *     pSizesAlloc;  // the sizes of each partition allocated
    int **    ppParts;      // the number of states in each partition
    DdNode ** pbFuncs;      // the functions for each partition
};


static Au_SPInfoInt_t * Au_AutoInfoIntAlloc( DdManager * dd, int nPartsAlloc, int nSizeAlloc );
static void           Au_AutoInfoIntAddReset( Au_SPInfoInt_t * p );
static void           Au_AutoInfoIntAddTrans( Au_SPInfoInt_t * p, DdNode * bFunc, int iState );
static int            Au_AutoInfoIntAddPart( Au_SPInfoInt_t * p, DdNode * bFunc, int iPrev );
static void           Au_AutoInfoIntAddEntry( Au_SPInfoInt_t * p, int iPart, int Entry );
static void           Au_AutoInfoIntFree( Au_SPInfoInt_t * p );
static Au_SPInfo_t * Au_AutoInfoDup( Au_SPInfoInt_t * p );


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Computes the state partitions of the automaton.]

  Description [Computes state partitions for each state.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Au_AutoStatePartitions( DdManager * dd, Au_Auto_t * pAut )
{
    Au_State_t * pState;
    Au_Trans_t * pTrans;
    Au_SPInfoInt_t * p;
    Au_SPInfo_t * pReal;
    int s;
    p = Au_AutoInfoIntAlloc( dd, 50, 50 );
    for ( s = 0; s < pAut->nStates; s++ )
    {
        pState = pAut->pStates[s];
        Au_StateForEachTransition( pState, pTrans )
        {
            Au_AutoInfoIntAddTrans( p, pTrans->bCond, pTrans->StateNext );
        }
        pReal = Au_AutoInfoDup( p );
        Au_AutoInfoSet( pState, pReal );
        Au_AutoInfoIntAddReset( p );
    }
    Au_AutoInfoIntFree( p );
}

/**Function*************************************************************

  Synopsis    [Computes the state partitions of the automaton.]

  Description [Computes state partitions for each state.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Au_AutoStatePartitionsPrint( Au_Auto_t * pAut )
{
    Au_SPInfo_t * p;
//    Au_Trans_t * pTrans;
    Au_State_t * pState;

    int s, i, k;
    for ( s = 0; s < pAut->nStates; s++ )
    {
        pState = pAut->pStates[s];
//        printf( "State %2d ---> ", s );
//        Au_StateForEachTransition( pState, pTrans )
//            printf( " %2d", pTrans->StateNext );
//        printf( "\n" );
        p = Au_AutoInfoGet( pState );
        for ( i = 0; i < p->nParts; i++ )
        {
            // skip self referencing clause
            printf( "%3d =>", s );
            for ( k = 0; k < p->pSizes[i]; k++ )
                printf( "%s%d", (k? " + ": " "), p->ppParts[i][k] );
            printf( "\n" );
        }
    }
}

/**Function*************************************************************

  Synopsis    [Computes the state partitions of the automaton.]

  Description [Computes state partitions for each state.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Au_AutoStatePartitionsFree( Au_Auto_t * pAut )
{
    Au_SPInfo_t * p;
    int s;
    for ( s = 0; s < pAut->nStates; s++ )
    {
        p = Au_AutoInfoGet( pAut->pStates[s] );
        Au_AutoInfoFree( p );
        pAut->pStates[s]->pSubs = NULL;
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Au_SPInfo_t * Au_AutoInfoGet( Au_State_t * pState )
{
    return (Au_SPInfo_t *)pState->pSubs;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Au_AutoInfoSet( Au_State_t * pState, Au_SPInfo_t * p )
{
    assert( pState->pSubs == NULL );
    pState->pSubs = (unsigned short *)p;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Au_AutoInfoFree( Au_SPInfo_t * p )
{
    FREE( p->ppParts[0] );
    FREE( p->ppParts );
    FREE( p->pSizes );
    FREE( p );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Au_SPInfo_t * Au_AutoInfoDup( Au_SPInfoInt_t * p )
{
    Au_SPInfo_t * pNew;
    int nEntries, i;
    // count the number of elements in the info
    nEntries = 0;
    for ( i = 0; i < p->nParts; i++ )
        nEntries += p->pSizes[i];
    // start the new part info
    pNew = ALLOC( Au_SPInfo_t, 1 );
    pNew->nParts = p->nParts;
    pNew->pSizes = ALLOC( int, p->nParts );
    memcpy( pNew->pSizes, p->pSizes, sizeof(int) * p->nParts );
    pNew->ppParts = ALLOC( int *, p->nParts );
    pNew->ppParts[0] = ALLOC( int, nEntries );
    for ( i = 1; i < p->nParts; i++ )
        pNew->ppParts[i] = pNew->ppParts[i-1] + pNew->pSizes[i-1];
    // copy the contents
    for ( i = 0; i < p->nParts; i++ )
        memcpy( pNew->ppParts[i], p->ppParts[i], sizeof(int) * pNew->pSizes[i] );
    return pNew;
}





/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Au_SPInfoInt_t * Au_AutoInfoIntAlloc( DdManager * dd, int nPartsAlloc, int nSizeAlloc )
{
    Au_SPInfoInt_t * pNew;
    int i;
    // start the new part info
    pNew = ALLOC( Au_SPInfoInt_t, 1 );
    memset( pNew, 0, sizeof(Au_SPInfoInt_t) );
    pNew->dd          = dd;
    pNew->nParts      = 0;
    pNew->nPartsAlloc = nPartsAlloc;
    pNew->pSizes      = ALLOC( int, nPartsAlloc );
    pNew->pSizesAlloc = ALLOC( int, nPartsAlloc );
    pNew->ppParts     = ALLOC( int *, nPartsAlloc );
    pNew->pbFuncs     = ALLOC( DdNode *, nPartsAlloc );
    for ( i = 0; i < nPartsAlloc; i++ )
    {
        pNew->pSizes[i]      = 0;
        pNew->pSizesAlloc[i] = nSizeAlloc;
        pNew->ppParts[i]     = ALLOC( int, nSizeAlloc );
    }
    return pNew;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Au_AutoInfoIntAddReset( Au_SPInfoInt_t * p )
{
    int i;
    for ( i = 0; i < p->nParts; i++ )
        Cudd_RecursiveDeref( p->dd, p->pbFuncs[i] );
    p->nParts = 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Au_AutoInfoIntAddTrans( Au_SPInfoInt_t * p, DdNode * bFunc, int iState )
{
    DdManager * dd = p->dd;
    DdNode * bFunc1, * bFunc2;
    int i, iNew, nPartsOld;
    if ( p->nParts == 0 )
    {
        assert( bFunc != b0 );
        if ( bFunc == b1 )
        {
            iNew = Au_AutoInfoIntAddPart( p, bFunc, -1 );
            Au_AutoInfoIntAddEntry( p, iNew, iState );
            assert( p->nParts == 1 );
            return;
        }
        // create two partitions, with and without this state
        Au_AutoInfoIntAddPart( p, Cudd_Not(bFunc), -1 );
        iNew = Au_AutoInfoIntAddPart( p, bFunc, -1 );
        Au_AutoInfoIntAddEntry( p, iNew, iState );
        assert( p->nParts == 2 );
        return;
    }
    // go through the available partitions
    nPartsOld = p->nParts;
    for ( i = 0; i < nPartsOld; i++ )
    {
        if ( Cudd_bddLeq( dd, bFunc, Cudd_Not(p->pbFuncs[i]) ) ) // no overlap
            continue;
        if ( Cudd_bddLeq( dd, p->pbFuncs[i], bFunc ) ) 
        {  // bFunc completely contains p->pbFuncs[i]
            Au_AutoInfoIntAddEntry( p, i, iState );
            continue;
        }
        // the case when they have non-empty intersection
        bFunc1 = Cudd_bddAnd( dd, p->pbFuncs[i], Cudd_Not(bFunc) ); Cudd_Ref( bFunc1 );
        bFunc2 = Cudd_bddAnd( dd, p->pbFuncs[i],          bFunc  ); Cudd_Ref( bFunc2 );
        assert( bFunc1 != b0 );
        assert( bFunc2 != b0 );
        // update the first part (nothing to add to it)
        Cudd_RecursiveDeref( dd, p->pbFuncs[i] );
        p->pbFuncs[i] = bFunc1;   // takes ref
        // create the second part (add this state to the second part)
        iNew = Au_AutoInfoIntAddPart( p, bFunc2, i );
        Au_AutoInfoIntAddEntry( p, iNew, iState );
        Cudd_RecursiveDeref( dd, bFunc2 );
    }
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Au_AutoInfoIntAddPart( Au_SPInfoInt_t * p, DdNode * bFunc, int iPrev )
{
    int Res, i;
    assert( iPrev < p->nParts );
    if ( p->nParts == p->nPartsAlloc )
    {
        p->pSizes      = REALLOC( int,      p->pSizes,       2 * p->nPartsAlloc );
        p->pSizesAlloc = REALLOC( int,      p->pSizesAlloc,  2 * p->nPartsAlloc );
        p->pbFuncs     = REALLOC( DdNode *, p->pbFuncs,      2 * p->nPartsAlloc );
        for ( i = p->nPartsAlloc; i < 2 * p->nPartsAlloc; i++ )
        {
            p->pSizesAlloc[i] = p->pSizesAlloc[0];
            p->ppParts[i]     = ALLOC( int, p->pSizesAlloc[0] );
        }
        p->nPartsAlloc *= 2;
    }
    // set the function
    p->pbFuncs[p->nParts]  = bFunc;   Cudd_Ref( bFunc );
    // copy the contents of the column
    if ( iPrev >= 0 )
    {
        if ( p->pSizesAlloc[p->nParts] < p->pSizes[iPrev] )
        {
            p->ppParts[p->nParts] = REALLOC( int, p->ppParts[p->nParts], 2 * p->pSizes[iPrev] );
            p->pSizesAlloc[p->nParts] = 2 * p->pSizes[iPrev];
        }
        memcpy( p->ppParts[p->nParts], p->ppParts[iPrev], sizeof(int) * p->pSizes[iPrev] );
        p->pSizes[p->nParts] = p->pSizes[iPrev];
    }
    else
        p->pSizes[p->nParts] = 0;    
    Res = p->nParts++;
    return Res;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Au_AutoInfoIntAddEntry( Au_SPInfoInt_t * p, int iPart, int Entry )
{
    assert( iPart >= 0 && iPart < p->nParts );
    if ( p->pSizes[iPart] == p->pSizesAlloc[iPart] )
    {
        p->ppParts[iPart] = REALLOC( int, p->ppParts[iPart],  2 * p->pSizesAlloc[iPart] );
        p->pSizesAlloc[iPart] *= 2;
    }
    p->ppParts[iPart][ p->pSizes[iPart]++ ] = Entry;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Au_AutoInfoIntFree( Au_SPInfoInt_t * p )
{
    int i;
    for ( i = 0; i < p->nPartsAlloc; i++ )
        FREE( p->ppParts[i] );
    FREE( p->pbFuncs );
    FREE( p->ppParts );
    FREE( p->pSizesAlloc );
    FREE( p->pSizes );
    FREE( p );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


