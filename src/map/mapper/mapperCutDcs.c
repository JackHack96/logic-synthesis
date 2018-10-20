/**CFile****************************************************************

  FileName    [mapperCutDcs.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Generic technology mapping engine.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 8, 2003.]

  Revision    [$Id: mapperCutDcs.c,v 1.3 2005/07/08 01:01:24 alanmi Exp $]

***********************************************************************/

#include "mapperInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define MAP_DC_SIZE      15       // the largest number of the fanins in the DC cut
#define MAP_DC_WORDS     32       // the number of words to store the info
#define MAP_DC_POWER     1024     // the largest of minterms in the info
#define MAP_DC_TSIZE     199      // should be larger than 64 but not too much

static void Map_ComputeDcsNode( Map_Man_t * p, Map_Node_t * pNode );
extern void Map_ComputeDcsNodeCut( Map_Node_t * pNode, Map_NodeVec_t * vInside, Map_NodeVec_t * vFanins );
static int  Map_ComputeDcsNodeCut_int( Map_NodeVec_t * vInside, Map_NodeVec_t * vFanins );
static void Map_ComputeDcsSimulateCut( Map_Man_t * p, Map_NodeVec_t * vInside, Map_NodeVec_t * vFanins );
static void Map_ComputeDcsRecord( Map_Node_t * pNode, Map_NodeVec_t * vInside, Map_NodeVec_t * vFanins );
static void Map_ComputeDcsRecord2( Map_Node_t * pNode, Map_NodeVec_t * vInside, Map_NodeVec_t * vFanins );
static int  Map_ComputeFaninCost( Map_Node_t * pNode );
static int  Map_MappingCountDcCuts( Map_Man_t * pMan, int * pnMints, int * pnMintsAll );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Computes don't-cares for all cuts of the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_ComputeDcs( Map_Man_t * p )
{
    ProgressBar * pProgress;
    Map_Node_t * pNode;
    int TimePrint, nNodes, i;
    unsigned * ppSims;

    // allocate memory for the simulation info
    ppSims = ALLOC( unsigned, p->vAnds->nSize * MAP_DC_WORDS );
    p->vAnds->pArray[0]->pSims = ppSims;
    for ( i = 1; i < p->vAnds->nSize; i++ )
        p->vAnds->pArray[i]->pSims = p->vAnds->pArray[i-1]->pSims + MAP_DC_WORDS;

    // compute the cuts for the internal nodes
    nNodes = p->vAnds->nSize;
    pProgress = Extra_ProgressBarStart( stdout, nNodes );
    TimePrint = clock() + CLOCKS_PER_SEC;
    for ( i = 0; i < nNodes; i++ )
    {
        pNode = p->vAnds->pArray[i];
        if ( !Map_NodeIsAnd( pNode ) )
            continue;
        Map_ComputeDcsNode( p, pNode );
        if ( i % 10 == 0 && clock() > TimePrint )
        {
            Extra_ProgressBarUpdate( pProgress, i, "Dcs ..." );
            TimePrint = clock() + CLOCKS_PER_SEC;
        }
    }
    Extra_ProgressBarStop( pProgress );
    free( ppSims );

    // report the stats
    if ( p->fVerbose )
    {
        int nMints = 0, nMintsAll = 0;
        int nCuts, nCutsDc;
        nCuts   = Map_MappingCountAllCuts(p);
        nCutsDc = Map_MappingCountDcCuts(p, &nMints, &nMintsAll);
        printf( "Nodes = %6d. Cuts = %6d. Cuts w/DCs = %6d. Ave mints = %4.1f. Ave DC mints = %4.1f.\n",
            p->vAnds->nSize, nCuts, nCutsDc, ((float)nMintsAll)/nCutsDc, ((float)nMints)/nCutsDc );
    }
}

/**Function*************************************************************

  Synopsis    [Compute don't-cares for all cuts of one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_ComputeDcsNode( Map_Man_t * p, Map_Node_t * pNode )
{
    Map_NodeVec_t * vInside = p->vInside;
    Map_NodeVec_t * vFanins = p->vFanins;
    int clk;

    assert( Map_NodeIsAnd(pNode) );

clk = clock();
    // detect the 10-var cut
    Map_ComputeDcsNodeCut( pNode, vInside, vFanins );
p->time1 += clock() - clk;

    // simulate the cut
clk = clock();
    Map_ComputeDcsSimulateCut( p, vInside, vFanins );
p->time2 += clock() - clk;

    // find the contained cuts and record don't-cares
clk = clock();
    Map_ComputeDcsRecord( pNode, vInside, vFanins );
p->time3 += clock() - clk;
}

/**Function*************************************************************

  Synopsis    [Compute don't-cares for all cuts of one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_ComputeDcsNodeCut( Map_Node_t * pNode, Map_NodeVec_t * vInside, Map_NodeVec_t * vFanins )
{
    int i;

    // mark the cone with fMark0
    Map_MappingMark_rec( pNode );

    // detect the 10-var cut
    Map_NodeVecClear( vInside );
    Map_NodeVecClear( vFanins );
    Map_NodeVecPush( vFanins, pNode );
    pNode->fMark1 = 1;

    while ( Map_ComputeDcsNodeCut_int( vInside, vFanins ) );

    // unmark the nodes of the sets
    for ( i = 0; i < vInside->nSize; i++ )
        vInside->pArray[i]->fMark1 = 0;
    for ( i = 0; i < vFanins->nSize; i++ )
        vFanins->pArray[i]->fMark1 = 0;

    // unmark the cone with fMark0
    Map_MappingUnmark_rec( pNode );
}

/**Function*************************************************************

  Synopsis    [Compute don't-cares for all cuts of one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Map_ComputeDcsNodeCut_int( Map_NodeVec_t * vInside, Map_NodeVec_t * vFanins )
{
    Map_Node_t * pNext, * pFaninBest;
    int CostBest, CostCur, i;
    // find the best fanin
    CostBest   = 100;
    pFaninBest = NULL;
    for ( i = 0; i < vFanins->nSize; i++ )
    {
        CostCur = Map_ComputeFaninCost( vFanins->pArray[i] );
        if ( CostBest > CostCur )
        {
            CostBest   = CostCur;
            pFaninBest = vFanins->pArray[i];
        }
    }
    if ( pFaninBest == NULL )
        return 0;
    assert( CostBest < 3 );
    if ( vFanins->nSize - 1 + CostBest > MAP_DC_SIZE )
        return 0;
    assert( Map_NodeIsAnd(pFaninBest) );
    // remove the node from the array
    Map_NodeVecRemove( vFanins, pFaninBest );
    // add the node to the set
    Map_NodeVecPush( vInside, pFaninBest );
    // add the left child to the fanins
    pNext = Map_Regular(pFaninBest->p1);
    if ( !pNext->fMark1 )
    {
        pNext->fMark1 = 1;
        Map_NodeVecPush( vFanins, pNext );
    }
    // add the right child to the fanins
    pNext = Map_Regular(pFaninBest->p2);
    if ( !pNext->fMark1 )
    {
        pNext->fMark1 = 1;
        Map_NodeVecPush( vFanins, pNext );
    }
    assert( vFanins->nSize <= MAP_DC_SIZE );
    // keep doing this
    return 1;
}

/**Function*************************************************************

  Synopsis    [Evaluate the fanin cost.]

  Description [Returns the number of fanins that will be brought in.
  Returns large number if the node cannot be added.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Map_ComputeFaninCost( Map_Node_t * pNode )
{
    Map_Node_t * pFanout;
    assert( pNode->fMark0 == 1 );  // mark of the node
    assert( pNode->fMark1 == 1 );  // collected node
    // check the PI node
    if ( !Map_NodeIsAnd(pNode) )
        return 999;
    // check the fanouts
    Map_NodeForEachFanout( pNode, pFanout )
        if ( pFanout->fMark0 && pFanout->fMark1 == 0 ) // in the cone but not in the set
            return 999;
    // the fanouts are okay
    assert( Map_Regular(pNode->p1)->fMark0 );
    assert( Map_Regular(pNode->p2)->fMark0 );
    return (!(Map_Regular(pNode->p1)->fMark1)) + (!(Map_Regular(pNode->p2)->fMark1));
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_ComputeDcsSimulateCut( Map_Man_t * p, Map_NodeVec_t * vInside, Map_NodeVec_t * vFanins )
{
    Map_Node_t * pNode, * pNode1, * pNode2;
    int i, k, fComp1, fComp2;
//printf( "%d ", vInside->nSize );

    // mark the nodes of the sets
    for ( i = 0; i < vInside->nSize; i++ )
        vInside->pArray[i]->fMark0 = 1;
    for ( i = 0; i < vFanins->nSize; i++ )
        vFanins->pArray[i]->fMark0 = 1;
    // check the nodes inside
    for ( i = 0; i < vInside->nSize; i++ )
    {
        pNode  = vInside->pArray[i];
        pNode1 = Map_Regular( pNode->p1 );
        pNode2 = Map_Regular( pNode->p2 );
        assert( pNode1->fMark0 == 1 );
        assert( pNode2->fMark0 == 1 );
    }
    // unmark the nodes of the sets
    for ( i = 0; i < vInside->nSize; i++ )
        vInside->pArray[i]->fMark0 = 0;
    for ( i = 0; i < vFanins->nSize; i++ )
        vFanins->pArray[i]->fMark0 = 0;

    // set the leaf info
    for ( i = 0; i < vFanins->nSize; i++ )
    {
        pNode = vFanins->pArray[i];
        assert( pNode->fMark0 == 0 );
        pNode->fMark0 = 1;
        for ( k = 0; k < MAP_DC_WORDS; k++ )
            pNode->pSims[k] = p->uTruthsLarge[i][k];
    }
    // simulate
//    for ( i = vInside->nSize - 1; i >= 0; i-- )
    Map_NodeVecSortByLevel( vInside );
    for ( i = 0; i < vInside->nSize; i++ )
    {
        pNode  = vInside->pArray[i];
        pNode1 = Map_Regular( pNode->p1 );
        pNode2 = Map_Regular( pNode->p2 );
        assert( pNode1->fMark0 == 1 );
        assert( pNode2->fMark0 == 1 );
        fComp1 = Map_IsComplement( pNode->p1 );
        fComp2 = Map_IsComplement( pNode->p2 );
        for ( k = 0; k < MAP_DC_WORDS; k++ )
            pNode->pSims[k] = (fComp1? ~pNode1->pSims[k] : pNode1->pSims[k]) & 
                              (fComp2? ~pNode2->pSims[k] : pNode2->pSims[k]);
        pNode->fMark0 = 1;
    }
    for ( i = 0; i < vInside->nSize; i++ )
        vInside->pArray[i]->fMark0 = 0;
    for ( i = 0; i < vFanins->nSize; i++ )
        vFanins->pArray[i]->fMark0 = 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_ComputeDcsRecord( Map_Node_t * pNode, Map_NodeVec_t * vInside, Map_NodeVec_t * vFanins )
{
    Map_Cut_t * pCut;
    int i, m, v, nMints, nTests, nUniMints;
    unsigned uSimInfo[MAP_DC_POWER];
    unsigned uMask, uUniMint;
    // unique table
    unsigned uTruthTable[MAP_DC_TSIZE];
    for ( i = 0; i < MAP_DC_TSIZE; i++ )
        uTruthTable[i] = MAP_FULL;

    // check the size of the fanin set
    assert( vFanins->nSize <= MAP_DC_SIZE );
//    assert( vFanins->nSize + vInside->nSize <= 32 );
    if ( vFanins->nSize + vInside->nSize > 32 )
    {
        Map_ComputeDcsRecord2( pNode, vInside, vFanins );
        return;
    }

    // clean the simulation info
    nTests = (1 << vFanins->nSize);
    for ( m = 0; m < nTests; m++ )
        uSimInfo[m] = 0;
    // transpose the simulation info
    // mark the nodes of the sets
    for ( i = 0; i < vFanins->nSize; i++ )
    {
        pNode = vFanins->pArray[i];
        pNode->fMark0 = 1;
        pNode->NumTemp   = i;
        for ( m = 0; m < nTests; m++ )
            if ( Map_InfoReadVar( pNode->pSims, m ) )
                uSimInfo[m] |= (1 << pNode->NumTemp);
    }
    for ( i = 0; i < vInside->nSize; i++ )
    {
        pNode = vInside->pArray[i];
        pNode->fMark0 = 1;
        pNode->NumTemp   = vFanins->nSize + i;
        for ( m = 0; m < nTests; m++ )
            if ( Map_InfoReadVar( pNode->pSims, m ) )
                uSimInfo[m] |= (1 << pNode->NumTemp);
    }

    // detect the cuts that are contained completely
    for ( pCut = pNode->pCuts->pNext; pCut; pCut = pCut->pNext )
    {
        assert( pCut->uTruthDc[0] == 0 );
        assert( pCut->nLeaves < 6 );
        // check if the cut is completely contained in the 10-var cut
        for ( i = 0; i < pCut->nLeaves; i++ )
            if ( pCut->ppLeaves[i]->fMark0 == 0 )
                break;
        if ( i != pCut->nLeaves )
            continue;

        // get the mask of the simulation info of this cut
        nMints = (1 << pCut->nLeaves);
        uMask = 0;
        for ( v = 0; v < pCut->nLeaves; v++ )
            uMask |= (1 << pCut->ppLeaves[v]->NumTemp);

        // collect unique info
        nUniMints = 0;
        for ( m = 0; m < nTests; m++ )
        {
            // get the minterm in terms of nodes
            uUniMint = uSimInfo[m] & uMask;
            for ( i = uUniMint % MAP_DC_TSIZE; uTruthTable[i] != MAP_FULL; i = (i + 1) % MAP_DC_TSIZE )
                if ( uTruthTable[i] == uUniMint )
                    break;
            if ( uTruthTable[i] == uUniMint ) // found this minterm
                continue;
            // add this minterm
            uTruthTable[i] = uUniMint;
            nUniMints++;
            // if the number of unique minterms is the largest, continue
            if ( nUniMints == nMints )
                break;
        }
        // if the number of unique minterms is the largest, continue
        if ( nUniMints == nMints )
        {
            for ( i = 0; i < MAP_DC_TSIZE; i++ )
                uTruthTable[i] = MAP_FULL;
            continue;
        }

        // go through the non-trivial minterms
        for ( i = 0; i < MAP_DC_TSIZE; i++ )
            if ( uTruthTable[i] != MAP_FULL )
            {
                // get this minterm
                uUniMint = 0;
                for ( v = 0; v < pCut->nLeaves; v++ )
                    if ( uTruthTable[i] & (1 << pCut->ppLeaves[v]->NumTemp) )
                        uUniMint |= (1 << v);
                // add this minterm for the truth table
                assert( (pCut->uTruthDc[0] & (1 << uUniMint)) == 0 );
                pCut->uTruthDc[0] |= (1 << uUniMint);
                // clean the table
                uTruthTable[i] = MAP_FULL;
                nUniMints--;
            }
        assert( nUniMints == 0 );

        // complement the truth table
        pCut->uTruthDc[0] = ~pCut->uTruthDc[0];
        pCut->uTruthDc[1] = ~pCut->uTruthDc[1];
        Map_MappingExpandTruth( pCut->uTruthDc, pCut->nLeaves );
    }
    // unmark the nodes of the sets
    for ( i = 0; i < vInside->nSize; i++ )
        vInside->pArray[i]->fMark0 = 0;
    for ( i = 0; i < vFanins->nSize; i++ )
        vFanins->pArray[i]->fMark0 = 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_ComputeDcsRecord2( Map_Node_t * pNode, Map_NodeVec_t * vInside, Map_NodeVec_t * vFanins )
{
    Map_Cut_t * pCut;
    int i, m, v, iMint, nTests;
    int CountOut = 0, CountIn = 0, Total = 0;
    // check the size of the fanin set
    assert( vFanins->nSize <= MAP_DC_SIZE );
    // mark the nodes of the sets
    for ( i = 0; i < vInside->nSize; i++ )
        vInside->pArray[i]->fMark0 = 1;
    for ( i = 0; i < vFanins->nSize; i++ )
        vFanins->pArray[i]->fMark0 = 1;
    // detect the cuts that are contained completely
    for ( pCut = pNode->pCuts->pNext; pCut; pCut = pCut->pNext )
    {
        assert( pCut->uTruthDc[0] == 0 );
        // check if the cut is completely contained in the 10-var cut
        for ( i = 0; i < pCut->nLeaves; i++ )
            if ( pCut->ppLeaves[i]->fMark0 == 0 )
                break;
        if ( i != pCut->nLeaves )
        {
            CountOut++;
            continue;
        }
        CountIn++;
        Total += pCut->nLeaves;
        // the cut is completely contained in the 10-var cut
        // collect the minterms
        nTests = (1 << vFanins->nSize);
        for ( m = 0; m < nTests; m++ )
        {
            iMint = 0;
            for ( v = 0; v < pCut->nLeaves; v++ )
                if ( Map_InfoReadVar(pCut->ppLeaves[v]->pSims, m) )
                    iMint |= (1 << v);
            pCut->uTruthDc[iMint/32] |= (1 << (iMint%32));
        }
        // complement the truth table
        pCut->uTruthDc[0] = ~pCut->uTruthDc[0];
        pCut->uTruthDc[1] = ~pCut->uTruthDc[1];
        Map_MappingExpandTruth( pCut->uTruthDc, pCut->nLeaves );
    }
//    printf( "Out = %d  In = %d  Volume = %d  Tot = %d\n", 
//        CountOut, CountIn, vInside->nSize, Total );
    // unmark the nodes of the sets
    for ( i = 0; i < vInside->nSize; i++ )
        vInside->pArray[i]->fMark0 = 0;
    for ( i = 0; i < vFanins->nSize; i++ )
        vFanins->pArray[i]->fMark0 = 0;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
/*
void Map_NodeVecClean( Map_NodeVec_t * p )
{
    memset( p, 0, sizeof(Map_NodeVec_t) );
    p->nCap = MAP_DC_SIZE + 2;
}
*/

/**Function*************************************************************

  Synopsis    [Counts all the cuts with DCs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Map_MappingCountDcCuts( Map_Man_t * pMan, int * pnMints, int * pnMintsAll )
{
    Map_Node_t * pNode;
    Map_Cut_t * pCut;
    int i, k, nCuts, Count, nMints;
    // go through all the nodes in the unique table of the manager
    nCuts = 0;
    for ( i = 0; i < pMan->nBins; i++ )
        for ( pNode = pMan->pBins[i]; pNode; pNode = pNode->pNext )
            for ( pCut = pNode->pCuts->pNext; pCut; pCut = pCut->pNext )
                if ( pCut->uTruthDc[0] || pCut->uTruthDc[1] )
                {
                    Count = 0;
                    nMints = (1 << pCut->nLeaves);
                    for ( k = 0; k < nMints; k++ )
                        if ( pCut->uTruthDc[k/32] & (1 << (k%32)) )
                            Count++;
                    if ( pnMints )
                        (*pnMints)    += Count;
                    if ( pnMintsAll )
                        (*pnMintsAll) += nMints;
                    pMan->nCounts[Count]++;
                    nCuts++;
                }
    return nCuts;
}

/**Function*************************************************************

  Synopsis    [Implements ISOP computation on truth tables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned Map_ComputeIsop_rec( Map_Man_t * p, unsigned uF, unsigned uFD, int iVar, int nVars, int fDir )
{
    unsigned uF0, uF1, uFD0, uFD1;
    unsigned uR0, uR1, uR2, uR;
    int iNext = fDir? iVar - 1 : iVar + 1;

    if ( uF == 0 )
        return 0;
    if ( uFD == MAP_FULL )
        return MAP_FULL;
    assert( iVar >= 0 && iVar < nVars );

    uF0  = uF & ~p->uTruths[iVar][0];
    uF0 |= (uF0 << (1 << iVar));

    uF1  = uF &  p->uTruths[iVar][0];
    uF1 |= (uF1 >> (1 << iVar));

    uFD0  = uFD & ~p->uTruths[iVar][0];
    uFD0 |= (uFD0 << (1 << iVar));

    uFD1  = uFD &  p->uTruths[iVar][0];
    uFD1 |= (uFD1 >> (1 << iVar));

    uR0 = Map_ComputeIsop_rec( p, uF0 & ~uFD1, uFD0, iNext, nVars, fDir );
    uR1 = Map_ComputeIsop_rec( p, uF1 & ~uFD0, uFD1, iNext, nVars, fDir );

    uR2 = Map_ComputeIsop_rec( p, (uF0 & ~uR0) | (uF1 & ~uR1), uFD0 & uFD1, iNext, nVars, fDir );

    uR = ((uR0 & ~p->uTruths[iVar][0]) | (uR1 & p->uTruths[iVar][0]) | uR2);
    assert( (uR & ~uFD) == 0 );
    assert( (uF & ~uR)  == 0 );
    return uR;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
