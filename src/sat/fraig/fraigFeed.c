/**CFile****************************************************************

  FileName    [fraigFeed.c]

  PackageName [FRAIG: Functionally reduced AND-INV graphs.]

  Synopsis    [Procedures to support the solver feedback.]

  Author      [Alan Mishchenko <alanmi@eecs.berkeley.edu>]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.1. Started - October 1, 2004]

  Revision    [$Id: fraigFeed.c,v 1.8 2005/07/08 01:01:31 alanmi Exp $]

***********************************************************************/

#include "fraigInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int   Fraig_FeedBackPrepare( Fraig_Man_t * p, int * pModel, Msat_IntVec_t * vVars );
static int   Fraig_FeedBackInsert( Fraig_Man_t * p, int nVarsPi );
static void  Fraig_FeedBackVerify( Fraig_Man_t * p, Fraig_Node_t * pOld, Fraig_Node_t * pNew );

static void Fraig_FeedBackCovering( Fraig_Man_t * p, Msat_IntVec_t * vPats );
static Fraig_NodeVec_t * Fraig_FeedBackCoveringStart( Fraig_Man_t * pMan );
static int Fraig_GetSmallestColumn( int * pHits, int nHits );
static int Fraig_GetHittingPattern( Fraig_Sims_t * pSims, int nWords );
static void Fraig_CancelCoveredColumns( Fraig_NodeVec_t * vColumns, int * pHits, int iPat );
static void  Fraig_FeedBackCheckTable( Fraig_Man_t * p );
static void  Fraig_FeedBackCheckTableF0( Fraig_Man_t * p );


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Initializes the feedback information.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_FeedBackInit( Fraig_Man_t * p )
{
    p->vCones    = Fraig_NodeVecAlloc( 500 );
    p->vPatsReal = Msat_IntVecAlloc( 1000 ); 
    p->pSimsReal = (Fraig_Sims_t *)Fraig_MemFixedEntryFetch( p->mmSims );
    memset( p->pSimsReal, 0, sizeof(Fraig_Sims_t) );
    p->pSimsTemp = (Fraig_Sims_t *)Fraig_MemFixedEntryFetch( p->mmSims );
    p->pSimsDiff = (Fraig_Sims_t *)Fraig_MemFixedEntryFetch( p->mmSims );
}

/**Function*************************************************************

  Synopsis    [Processes the feedback from teh solver.]

  Description [Array pModel gives the value of each variable in the SAT 
  solver. Array vVars is the array of integer numbers of variables
  involves in this conflict.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_FeedBack( Fraig_Man_t * p, int * pModel, Msat_IntVec_t * vVars, Fraig_Node_t * pOld, Fraig_Node_t * pNew )
{
    int nVarsPi, nWords;
    int i, clk = clock();

    // get the number of PI vars in the feedback (also sets the PI values)
    nVarsPi = Fraig_FeedBackPrepare( p, pModel, vVars );

    // set the PI values
    nWords = Fraig_FeedBackInsert( p, nVarsPi );
    assert( p->iWordStart + nWords <= FRAIG_SIM_ROUNDS );

    // resimulates the words from p->iWordStart to iWordStop
//clk1 = clock();
    for ( i = 1; i < p->vNodes->nSize; i++ )
        if ( Fraig_NodeIsAnd(p->vNodes->pArray[i]) )
            Fraig_NodeSimulate( p->vNodes->pArray[i], p->iWordStart, p->iWordStart + nWords, 0 );
//p->time1 += clock() - clk1;

//clk1 = clock();
    if ( p->fDoSparse )
        Fraig_TableRehashF0( p, 0 );
//p->time2 += clock() - clk1;

    if ( !p->fChoicing )
    Fraig_FeedBackVerify( p, pOld, pNew );

    // if there is no room left, compress the patterns
    if ( p->iWordStart + nWords == FRAIG_SIM_ROUNDS )
        p->iWordStart = Fraig_FeedBackCompress( p );
    else  // otherwise, update the starting word
        p->iWordStart += nWords;

p->timeFeed += clock() - clk;
}

/**Function*************************************************************

  Synopsis    [Get the number and values of the PI variables.]

  Description [Returns the number of PI variables involved in this feedback.
  Fills in the internal presence and value data for the primary inputs.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_FeedBackPrepare( Fraig_Man_t * p, int * pModel, Msat_IntVec_t * vVars )
{
    Fraig_Node_t * pNode;
    int i, nVars, nVarsPis, * pVars;

    // clean the presence flag for all PIs
    for ( i = 0; i < p->vInputs->nSize; i++ )
    {
        pNode = p->vInputs->pArray[i];
        pNode->fFeedUse = 0;
    }

    // get the variables involved in the feedback
    nVars = Msat_IntVecReadSize(vVars);
    pVars = Msat_IntVecReadArray(vVars);

    // set the values for the present variables
    nVarsPis = 0;
    for ( i = 0; i < nVars; i++ )
    {
        pNode = p->vNodes->pArray[ pVars[i] ];
        if ( !Fraig_NodeIsVar(pNode) )
            continue;
        // set its value
        pNode->fFeedUse = 1;
        pNode->fFeedVal = !MSAT_LITSIGN(pModel[pVars[i]]);
        nVarsPis++;
    }
    return nVarsPis;
}

/**Function*************************************************************

  Synopsis    [Inserts the new simulation patterns.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_FeedBackInsert( Fraig_Man_t * p, int nVarsPi )
{
    Fraig_Node_t * pNode;
    int nWords, iPatFlip, nPatFlipLimit, i, w;
    int fUseNoPats = 0;
    int fUse2Pats = 0;

    // get the number of words 
    if ( fUse2Pats )
        nWords = FRAIG_NUM_WORDS( 2 * nVarsPi + 1 );
    else if ( fUseNoPats )
        nWords = 1;
    else
        nWords = FRAIG_NUM_WORDS( nVarsPi + 1 );
    // update the number of words if they do not fit into the simulation info
    if ( nWords > FRAIG_SIM_ROUNDS - p->iWordStart )
        nWords = FRAIG_SIM_ROUNDS - p->iWordStart; 
    // determine the bound on the flipping bit
    nPatFlipLimit = nWords * 32 - 2;

    // mark the real pattern
    Msat_IntVecPush( p->vPatsReal, p->iWordStart * 32 ); 
    // record the real pattern
    Fraig_BitStringSetBit( p->pSimsReal->uTests, p->iWordStart * 32 );

    // set the values at the PIs
    iPatFlip = 1;
    for ( i = 0; i < p->vInputs->nSize; i++ )
    {
        pNode = p->vInputs->pArray[i];
        for ( w = p->iWordStart; w < p->iWordStart + nWords; w++ )
            if ( !pNode->fFeedUse )
                pNode->pSimsD->uTests[w] = FRAIG_RANDOM_UNSIGNED;
            else if ( pNode->fFeedVal )
                pNode->pSimsD->uTests[w] = FRAIG_FULL;
            else // if ( !pNode->fFeedVal )
                pNode->pSimsD->uTests[w] = 0;

        if ( fUse2Pats )
        {
            // flip two patterns
            if ( pNode->fFeedUse && 2 * iPatFlip < nPatFlipLimit )
            {
                Fraig_BitStringXorBit( pNode->pSimsD->uTests + p->iWordStart, 2 * iPatFlip - 1 );
                Fraig_BitStringXorBit( pNode->pSimsD->uTests + p->iWordStart, 2 * iPatFlip     );
                Fraig_BitStringXorBit( pNode->pSimsD->uTests + p->iWordStart, 2 * iPatFlip + 1 );
                iPatFlip++;
            }
        }
        else if ( fUseNoPats )
        {
        }
        else
        {
            // flip the diagonal
            if ( pNode->fFeedUse && iPatFlip < nPatFlipLimit )
            {
                Fraig_BitStringXorBit( pNode->pSimsD->uTests + p->iWordStart, iPatFlip );
                iPatFlip++;
    //            Extra_PrintBinary( stdout, &pNode->pSimsD->uTests, 45 ); printf( "\n" );
            }
        }
        // clean the use mask
        pNode->fFeedUse = 0;

        // add the info to the D hash value of the PIs
        for ( w = p->iWordStart; w < p->iWordStart + nWords; w++ )
            pNode->pSimsD->uHash ^= pNode->pSimsD->uTests[w] * s_FraigPrimes[w];

    }
//    assert( fUse2Pats || iPatFlip == nVarsPi + 1 || nWords == FRAIG_SIM_ROUNDS - p->iWordStart );
    return nWords;
}


/**Function*************************************************************

  Synopsis    [Checks that the SAT solver pattern indeed distinquishes the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_FeedBackVerify( Fraig_Man_t * p, Fraig_Node_t * pOld, Fraig_Node_t * pNew )
{
    int fValue1, fValue2, iPat;
    iPat   = Msat_IntVecReadEntry( p->vPatsReal, Msat_IntVecReadSize(p->vPatsReal)-1 );
    fValue1 = (Fraig_BitStringHasBit( pOld->pSimsD->uTests, iPat ));
    fValue2 = (Fraig_BitStringHasBit( pNew->pSimsD->uTests, iPat ));
/*
Fraig_PrintNode( p, pOld );
printf( "\n" );
Fraig_PrintNode( p, pNew );
printf( "\n" );
*/
    assert( fValue1 != fValue2 );
}

/**Function*************************************************************

  Synopsis    [Compress the simulation patterns.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_FeedBackCompress( Fraig_Man_t * p )
{
    Fraig_Sims_t * pSims;
    int i, w, t, nPats, * pPats;
    int fPerformChecks = (p->nBTLimit == -1);

    // solve the covering problem
    if ( fPerformChecks )
    {
        Fraig_FeedBackCheckTable( p );
        if ( p->fDoSparse ) 
            Fraig_FeedBackCheckTableF0( p );
    }

    // solve the covering problem
    Fraig_FeedBackCovering( p, p->vPatsReal );


    // get the number of additional patterns
    nPats = Msat_IntVecReadSize( p->vPatsReal );
    pPats = Msat_IntVecReadArray( p->vPatsReal );
    // get the new starting word
    p->iWordStart = FRAIG_NUM_WORDS( p->iPatsPerm + nPats );

    // set the simulation info for the PIs
    for ( i = 0; i < p->vInputs->nSize; i++ )
    {
        // get hold of the simulation info for this PI
        pSims = p->vInputs->pArray[i]->pSimsD;
        // clean the storage for the new patterns
        for ( w = p->iWordPerm; w < p->iWordStart; w++ )
            p->pSimsTemp->uTests[w] = 0;
        // set the patterns
        for ( t = 0; t < nPats; t++ )
            if ( Fraig_BitStringHasBit( pSims->uTests, pPats[t] ) )
            {
                // check if this pattern falls into temporary storage
                if ( p->iPatsPerm + t < p->iWordPerm * 32 )
                    Fraig_BitStringSetBit( pSims->uTests, p->iPatsPerm + t );
                else
                    Fraig_BitStringSetBit( p->pSimsTemp->uTests, p->iPatsPerm + t );
            }
        // copy the pattern 
        for ( w = p->iWordPerm; w < p->iWordStart; w++ )
            pSims->uTests[w] = p->pSimsTemp->uTests[w];
        // recompute the hashing info
        pSims->uHash = 0;
        for ( w = 0; w < p->iWordStart; w++ )
            pSims->uHash ^= pSims->uTests[w] * s_FraigPrimes[w];
    }

    // update info about the permanently stored patterns
    p->iWordPerm = p->iWordStart;
    p->iPatsPerm += nPats;
    assert( p->iWordPerm == FRAIG_NUM_WORDS( p->iPatsPerm ) );

    // resimulate and recompute the hash values
    for ( i = 1; i < p->vNodes->nSize; i++ )
        if ( Fraig_NodeIsAnd(p->vNodes->pArray[i]) )
        {
            p->vNodes->pArray[i]->pSimsD->uHash = 0;
            Fraig_NodeSimulate( p->vNodes->pArray[i], 0, p->iWordPerm, 0 );
        }

    // double-check that the nodes are still distinguished
    if ( fPerformChecks )
        Fraig_FeedBackCheckTable( p );

    // rehash the values in the F0 table
    if ( p->fDoSparse ) 
    {
        Fraig_TableRehashF0( p, 0 );
        if ( fPerformChecks )
            Fraig_FeedBackCheckTableF0( p );
    }

    // set the real patterns
    Msat_IntVecClear( p->vPatsReal );
    memset( p->pSimsReal, 0, sizeof(Fraig_Sims_t) );
    for ( w = 0; w < p->iWordPerm; w++ )
        p->pSimsReal->uTests[w] = FRAIG_FULL;
    if ( p->iPatsPerm % 32 > 0 )
        p->pSimsReal->uTests[p->iWordPerm-1] = FRAIG_MASK( p->iPatsPerm % 32 );

//    printf( "The number of permanent words = %d.\n", p->iWordPerm );
    return p->iWordStart;
}




/**Function*************************************************************

  Synopsis    [Checks the correctness of the functional simulation table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_FeedBackCovering( Fraig_Man_t * p, Msat_IntVec_t * vPats )
{
    Fraig_NodeVec_t * vColumns;
    Fraig_Sims_t * pSims;
    int * pHits, iPat, iCol, i;
    int nOnesTotal, nSolStarting;

    // collect the pairs to be distinguished
    vColumns = Fraig_FeedBackCoveringStart( p );
    // collect the number of 1s in each simulation vector
    nOnesTotal = 0;
    pHits = ALLOC( int, vColumns->nSize );
    for ( i = 0; i < vColumns->nSize; i++ )
    {
        pSims = (Fraig_Sims_t *)vColumns->pArray[i];
        pHits[i] = Fraig_BitStringCountOnes( pSims->uTests, p->iWordStart );
        nOnesTotal += pHits[i];
//        assert( pHits[i] > 0 );
    }
 
    // go through the patterns
    nSolStarting = Msat_IntVecReadSize(vPats);
    while ( (iCol = Fraig_GetSmallestColumn( pHits, vColumns->nSize )) != -1 )
    {
        // find the pattern, which hits this column
        iPat = Fraig_GetHittingPattern( (Fraig_Sims_t *)vColumns->pArray[iCol], p->iWordStart );
        // cancel the columns covered by this pattern
        Fraig_CancelCoveredColumns( vColumns, pHits, iPat );
        // save the pattern
        Msat_IntVecPush( vPats, iPat );
    }

    // free the set of columns
    for ( i = 0; i < vColumns->nSize; i++ )
        Fraig_MemFixedEntryRecycle( p->mmSims, (char *)vColumns->pArray[i] );

    if ( p->fVerbose )
    {
        printf( "Cols (pairs) = %5d.  ", vColumns->nSize );
        printf( "Rows (pats) = %5d.  ", p->iWordStart * 32 );
        printf( "Dens = %6.2f %%.  ", 100.0 * nOnesTotal / vColumns->nSize / p->iWordStart / 32 );
        printf( "Sols = %3d (%3d).  ", Msat_IntVecReadSize(vPats), nSolStarting );
        printf( "Perm = %3d.\n", p->iWordPerm );
    }
    Fraig_NodeVecFree( vColumns );
    free( pHits );
}


/**Function*************************************************************

  Synopsis    [Checks the correctness of the functional simulation table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fraig_NodeVec_t * Fraig_FeedBackCoveringStart( Fraig_Man_t * p )
{
    Fraig_NodeVec_t * vColumns;
    Fraig_HashTable_t * pT = p->pTableF;
    Fraig_Node_t * pEntF, * pEntD;
    Fraig_Sims_t * pSims;
    unsigned * pUnsigned1, * pUnsigned2;
    int i, k, m, w;//, nOnes;

    // start the set of columns
    vColumns = Fraig_NodeVecAlloc( 100 );

    // go through the pairs of nodes to be distinguished
    for ( i = 0; i < pT->nBins; i++ )
    Fraig_TableBinForEachEntryF( pT->pBins[i], pEntF )
    {
        p->vCones->nSize = 0;
        Fraig_TableBinForEachEntryD( pEntF, pEntD )
            Fraig_NodeVecPush( p->vCones, pEntD );
        if ( p->vCones->nSize == 1 )
            continue;

        for ( k = 0; k < p->vCones->nSize; k++ )
            for ( m = k+1; m < p->vCones->nSize; m++ )
            {
                if ( !Fraig_CompareSimInfoUnderMask( p->vCones->pArray[k], p->vCones->pArray[m], p->iWordStart, 0, p->pSimsReal->uTests ) )
                    continue;

                // primary simulation patterns (counter-examples) cannot distinguish this pair
                // get memory to store the feasible simulation patterns
                pSims = (Fraig_Sims_t *)Fraig_MemFixedEntryFetch( p->mmSims );
                // find the pattern that distinguish this column, exept the primary ones
                pUnsigned1 = p->vCones->pArray[k]->pSimsD->uTests;
                pUnsigned2 = p->vCones->pArray[m]->pSimsD->uTests;
                for ( w = 0; w < p->iWordStart; w++ )
                    pSims->uTests[w] = (pUnsigned1[w] ^ pUnsigned2[w]) & ~p->pSimsReal->uTests[w];
                // store the pattern
                Fraig_NodeVecPush( vColumns, (Fraig_Node_t *)pSims );
//                nOnes = Fraig_BitStringCountOnes(pSims->uTests, p->iWordStart);
//                assert( nOnes > 0 );
            }      
    }

    // if the flag is not set, do not consider sparse nodes in p->pTableF0
    if ( !p->fDoSparse )
        return vColumns;

    // recalculate their hash values based on p->pSimsReal->uTests
    pT = p->pTableF0;
    for ( i = 0; i < pT->nBins; i++ )
    Fraig_TableBinForEachEntryF( pT->pBins[i], pEntF )
    {
        pSims = pEntF->pSimsD;
        pSims->uHash = 0;
        for ( w = 0; w < p->iWordStart; w++ )
            pSims->uHash += (pSims->uTests[w] & p->pSimsReal->uTests[w]) * s_FraigPrimes[w];
    }

    // rehash the table using these values
    Fraig_TableRehashF0( p, 1 );

    // collect the classes of equivalent node pairs
    for ( i = 0; i < pT->nBins; i++ )
    Fraig_TableBinForEachEntryF( pT->pBins[i], pEntF )
    {
        p->vCones->nSize = 0;
        Fraig_TableBinForEachEntryD( pEntF, pEntD )
            Fraig_NodeVecPush( p->vCones, pEntD );
        if ( p->vCones->nSize == 1 )
            continue;

        // primary simulation patterns (counter-examples) cannot distinguish all these pairs
        for ( k = 0; k < p->vCones->nSize; k++ )
            for ( m = k+1; m < p->vCones->nSize; m++ )
            {
                // get memory to store the feasible simulation patterns
                pSims = (Fraig_Sims_t *)Fraig_MemFixedEntryFetch( p->mmSims );
                // find the patterns that are not distinquished
                pUnsigned1 = p->vCones->pArray[k]->pSimsD->uTests;
                pUnsigned2 = p->vCones->pArray[m]->pSimsD->uTests;
                for ( w = 0; w < p->iWordStart; w++ )
                    pSims->uTests[w] = (pUnsigned1[w] ^ pUnsigned2[w]) & ~p->pSimsReal->uTests[w];
                // store the pattern
                Fraig_NodeVecPush( vColumns, (Fraig_Node_t *)pSims );
//                nOnes = Fraig_BitStringCountOnes(pSims->uTests, p->iWordStart);
//                assert( nOnes > 0 );
            }      
    }
    return vColumns;
}

/**Function*************************************************************

  Synopsis    [Selects the column, which has the smallest number of hits.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_GetSmallestColumn( int * pHits, int nHits )
{
    int i, iColMin = -1, nHitsMin = 1000000;
    for ( i = 0; i < nHits; i++ )
    {
        // skip covered columns
        if ( pHits[i] == 0 )
            continue;
        // take the column if it can only be covered by one pattern
        if ( pHits[i] == 1 )
            return i;
        // find the column, which requires the smallest number of patterns
        if ( nHitsMin > pHits[i] )
        {
            nHitsMin = pHits[i];
            iColMin = i;
        }
    }
    return iColMin;
}

/**Function*************************************************************

  Synopsis    [Select the pattern, which hits this column.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_GetHittingPattern( Fraig_Sims_t * pSims, int nWords )
{
    int i, b;
    for ( i = 0; i < nWords; i++ )
    {
        if ( pSims->uTests[i] == 0 )
            continue;
        for ( b = 0; b < 32; b++ )
            if ( pSims->uTests[i] & (1 << b) )
                return i * 32 + b;
    }
    return -1;
}

/**Function*************************************************************

  Synopsis    [Cancel covered patterns.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_CancelCoveredColumns( Fraig_NodeVec_t * vColumns, int * pHits, int iPat )
{
    Fraig_Sims_t * pSims;
    int i;
    for ( i = 0; i < vColumns->nSize; i++ )
    {
        pSims = (Fraig_Sims_t *)vColumns->pArray[i];
        if ( Fraig_BitStringHasBit( pSims->uTests, iPat ) )
            pHits[i] = 0;
    }
}


/**Function*************************************************************

  Synopsis    [Checks the correctness of the functional simulation table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_FeedBackCheckTable( Fraig_Man_t * p )
{
    Fraig_HashTable_t * pT = p->pTableF;
    Fraig_Node_t * pEntF, * pEntD;
    int i, k, m, nPairs;
    int clk = clock();

    nPairs = 0;
    for ( i = 0; i < pT->nBins; i++ )
    Fraig_TableBinForEachEntryF( pT->pBins[i], pEntF )
    {
        p->vCones->nSize = 0;
        Fraig_TableBinForEachEntryD( pEntF, pEntD )
            Fraig_NodeVecPush( p->vCones, pEntD );
        if ( p->vCones->nSize == 1 )
            continue;
/*
        if ( p->vCones->nSize > 10 )
        {
        if ( i == 0 ) printf( "*" );
        printf( "%d ", p->vCones->nSize );
        }
*/
        for ( k = 0; k < p->vCones->nSize; k++ )
            for ( m = k+1; m < p->vCones->nSize; m++ )
            {
                if ( Fraig_CompareSimInfo( p->vCones->pArray[k], p->vCones->pArray[m], p->iWordStart, 0 ) )
                    printf( "Nodes %d and %d have the same D simulation info.\n", 
                        p->vCones->pArray[k]->Num, p->vCones->pArray[m]->Num );
                nPairs++;
            }   
    }
//    printf( "\n" );
//    printf( "The total of %d node pairs have been verified.\n", nPairs );
}

/**Function*************************************************************

  Synopsis    [Checks the correctness of the functional simulation table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_FeedBackCheckTableF0( Fraig_Man_t * p )
{
    Fraig_HashTable_t * pT = p->pTableF0;
    Fraig_Node_t * pEntF;
    int i, k, m, nPairs;

    nPairs = 0;
    for ( i = 0; i < pT->nBins; i++ )
    {
        p->vCones->nSize = 0;
        Fraig_TableBinForEachEntryF( pT->pBins[i], pEntF )
            Fraig_NodeVecPush( p->vCones, pEntF );
        if ( p->vCones->nSize == 1 )
            continue;
/*
        if ( p->vCones->nSize > 10 )
        {
        if ( i == 0 ) printf( "*" );
        printf( "%d ", p->vCones->nSize );
        }
*/
        for ( k = 0; k < p->vCones->nSize; k++ )
            for ( m = k+1; m < p->vCones->nSize; m++ )
            {
                if ( Fraig_CompareSimInfo( p->vCones->pArray[k], p->vCones->pArray[m], p->iWordStart, 0 ) )
                    printf( "Nodes %d and %d have the same D simulation info.\n", 
                        p->vCones->pArray[k]->Num, p->vCones->pArray[m]->Num );
                nPairs++;
            }   
    }
//    printf( "\n" );
//    printf( "The total of %d node pairs have been verified.\n", nPairs );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


