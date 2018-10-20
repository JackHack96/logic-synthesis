/**CFile****************************************************************

  FileName    [resmNode.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Generic resynthesis engine.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 8, 2003.]

  Revision    [$Id: resmNode.c,v 1.2 2005/02/28 05:34:33 alanmi Exp $]

***********************************************************************/

#include "resmInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static Resm_Node_t * Resm_NodeUpdate( Resm_Node_t * pNode, Resm_Cut_t * pCutBest );
static Resm_Node_t * Resm_NodeSelectWorstFanin( Resm_Man_t * p, Resm_Node_t * pNode );
static int           Resm_NodeFaninIsCritical( Resm_Node_t * pFanin );
static int           Resm_CompareNodes( Resm_Node_t * pNodeOld, Resm_Node_t * pNodeNew );

static Resm_Node_t * Resm_NodeSelectBestCut( Resm_Man_t * p, Resm_Node_t * pNode, Resm_Cut_t * pCutList );
static Resm_Node_t * Resm_NodeSelectBestCutInt( Resm_Man_t * p, Resm_Node_t * pNode, Resm_Cut_t * pCutList );
static int           Resm_CompareCuts( Resm_Man_t * p, Resm_Cut_t * pCutOld, Resm_Cut_t * pCutNew );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Resynthesizes one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Resm_NodeResynthesize( Resm_Node_t * pNode )
{
    Resm_Man_t * p = pNode->p;
    Resm_Node_t * pFanin, * pNodeNew;
    int fChange;

    p->nNodesTried++;

    assert( !Resm_IsComplement(pNode) );
    assert( Resm_NodeReadRefTotal(pNode) > 0 );
    assert( Resm_CheckNoDuplicates( pNode, pNode->ppLeaves, pNode->nLeaves ) );

if ( p->fVerbose > 1 )
printf( "NODE %4d: (%2d, %2d) Fans = %d. Arr = %4.2f. Req = %4.2f. Area = %4.2f.\n", 
       pNode->Num, pNode->nRefs[0], pNode->nRefs[1], pNode->nLeaves, 
       pNode->tArrival.Worst, pNode->tRequired[1].Worst, pNode->aArea );

    // iterate through the critical fanins
    // try each fanin and reset the count once a change has happened
    fChange = 0;
    pNode->uPhase = 0;
    while ( 1 )
    {
        // get the most timing critical fanin
        pFanin = Resm_NodeSelectWorstFanin( p, pNode );
        if ( pFanin == NULL )  // happens when all the fanins are PIs
            break;
        pNodeNew = Resm_NodeResynthesizeFanin( pNode, pFanin );
        if ( pNodeNew == NULL )
            break;
        if ( pNodeNew != pNode )
        {
            // new node is derived, update it
            fChange = 1;
            pNode = pNodeNew;
            pNodeNew->uPhase = 0;
        }
    }
    return fChange;
}

/**Function*************************************************************

  Synopsis    [Resynthesizes one fanin of a node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Resm_Node_t * Resm_NodeResynthesizeFanin( Resm_Node_t * pNode, Resm_Node_t * pFanin )
{
    Resm_Man_t * p = pNode->p;
    Resm_NodeVec_t * vCands;
    Resm_Node_t * pNodeNew;
    Resm_Cut_t * pCutBest, * pCutList;
    int clk;

    assert( !Resm_IsComplement(pNode) );
    assert( !Resm_IsComplement(pFanin) );

    p->nFaninsTried++;

    // collect the delay-improving candidates
clk = clock();
    Resm_CollectRelatedLogic( pNode );
    vCands = pNode->p->vWinCands;
p->timeLogic += clock() - clk;

if ( p->fVerbose > 1 )
printf( "Collected %d candidates.\n", vCands? vCands->nSize : 0 ); 

    if ( vCands == NULL || vCands->nSize == 0 )
        return NULL;

//    Resm_NodeBddsPrepare( pNode );

    // perform bit parallel simulation of the given part
clk = clock();
    Resm_SimulateWindow( p );
p->timeSimul += clock() - clk;

    // find possible new cuts
clk = clock();
    pCutList = Resm_NodeReplaceOneFanin( pNode, pFanin, vCands );
p->timeResub += clock() - clk;

//    Resm_NodeBddsCleanup( pNode );

if ( p->fVerbose > 1 )
printf( "Derived %d alternative cuts. \n", Resm_NodeListCount(pCutList) ); 
    if ( pCutList == NULL )
        return NULL;

    // match the cuts in the list
clk = clock();
    Resm_MatchCuts( pNode, pCutList );
p->timeMatch += clock() - clk;

    // select the best cut for delay
    pCutBest = Resm_NodeSelectBestCut( p, pNode, pCutList );
    // remove the current cut 
    Resm_NodeListRecycle( p, pCutList, pCutBest );
    if ( pCutBest == NULL )
        return NULL;

    if ( pCutBest->nLeaves == 6 )
        printf( "USING 6\n\n" );

    p->nCutsUsed++;
    p->nCutsGen[pCutBest->nVolume]++;

    // print the present and the new cut
//if ( p->fVerbose )
//    printf( "Accepting change (%d):\n", nVolume );

if ( p->fVerbose )
{
    printf( "OLD " );
    Resm_NodePrint( pNode );
    printf( "NEW " );
    Resm_NodePrint( pCutBest );
}

clk = clock();
    pNodeNew = Resm_NodeUpdate( pNode, pCutBest );
p->timeUpdate += clock() - clk;

if ( p->fVerbose )
{
    printf( "FIN " );
    Resm_NodePrint( pNodeNew );
    printf( "\n" );
}
    return pNodeNew;
}

/**Function*************************************************************

  Synopsis    [Removes duplicated fanins of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Resm_Node_t * Resm_NodeRemoveDupFanins( Resm_Node_t * pNode )
{
    pNode->p->nNodesTried++;
    assert( !Resm_CheckNoDuplicates( pNode, pNode->ppLeaves, pNode->nLeaves ) );
    return Resm_NodeResynthesizeFanin( pNode, NULL );
}

/**Function*************************************************************

  Synopsis    [Updates the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Resm_Node_t * Resm_NodeUpdate( Resm_Node_t * pNodeOld, Resm_Cut_t * pCutBest )
{   
    Resm_Man_t * p = pNodeOld->p;
    Resm_Node_t * pNodeNew;
    float AreaOld;
    int fComplInv;

//    assert( Resm_VerifyRefs( p ) );
    // remove old node
    AreaOld = Resm_NodeRemoveFaninFanouts( pNodeOld, 1 );
    // derive the nodes corresponding to the new cut
    pNodeNew = Rems_ConstructSuper( pCutBest );
    fComplInv = Resm_IsComplement(pNodeNew);
    assert( fComplInv == 0 );
    pNodeNew = Resm_Regular(pNodeNew);
    // compute the arrival times of the intermediate nodes
    Resm_NodeComputeArrival_rec( pNodeNew );
    if ( !fComplInv )
    assert( Resm_TimesAreEqual( &pNodeNew->tArrival, &pCutBest->tArrival, p->fEpsilon ) );
    // move the fanouts from the old node to the new node
    Resm_NodeTransferFanouts( pNodeOld, pNodeNew, pCutBest->fCompl ^ fComplInv );
    // insert the new node
    pNodeNew->aArea = Resm_NodeInsertFaninFanouts( pNodeNew, 1 );
    if ( !fComplInv )
    assert( pNodeNew->aArea == pCutBest->aArea );
    Resm_NodeFree( pCutBest );

    // from now on, pNodeOld will be a floating node???
//    assert( Resm_VerifyRefs( p ) );

    // update the arrival/required times
//    Resm_NodeUpdateArrivalTimeSlow( pNodeNew );
//    Resm_NodeUpdateRequiredTimeSlow( pNodeNew );
    Resm_NodeUpdateArrivalTime( pNodeNew );
    Resm_NodeUpdateRequiredTime( pNodeOld, pNodeNew );
    Resm_ComputeArrivalAll( p, 1 );

    // add to the area gain
    p->fAreaGain += AreaOld - pNodeNew->aArea;
    return pNodeNew;
}


/**Function*************************************************************

  Synopsis    [Returns the index of the most timing critical fanin.]

  Description [Marks and skips the fanins that have already need tried.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Resm_Node_t * Resm_NodeSelectWorstFanin( Resm_Man_t * p, Resm_Node_t * pNode )
{
    Resm_Node_t * pFanin, * pFaninWorst;
    int i, iNum;

    iNum = -1;
    pFaninWorst = NULL;
    for ( i = 0; i < (int)pNode->nLeaves; i++ )
    {
        pFanin = Resm_Regular(pNode->ppLeaves[i]);
        if ( Resm_NodeIsVar(pFanin) )
            continue;
//        if ( !Resm_NodeFaninIsCritical(pFanin) )
//            continue;
        // check if this fanin was already tried
        if ( pNode->uPhase & (1 << i) )
            continue;
        // update the fanin if it is worse
        if ( pFaninWorst == NULL || Resm_CompareNodes( pFanin, pFaninWorst ) )
        {
            iNum = i;
            pFaninWorst = pFanin;
        }
    }
    // mark this fanin as tried
    if ( iNum >= 0 )
        pNode->uPhase |= (1 << iNum);
    return pFaninWorst;
}


/**Function*************************************************************

  Synopsis    [Finds the permutation of the fanin indexes.]

  Description [The resulting permutation will be used to try to remove
  the fanins. This way the most offending fanins will be removed first.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Resm_NodeOrderFanins( Resm_Node_t * pNode, int * pPerm )
{
    Resm_Node_t * pLeaf1, * pLeaf2;
    int fChanges, Temp, i, k;

    // start with the identity permutation
    for ( i = 0; i < (int)pNode->nLeaves; i++ )
        pPerm[i] = i;

    // perform pair permutation until no change
    do {
        fChanges = 0;
        for ( i = 0; i < (int)pNode->nLeaves-1; i++ )
        {
            pLeaf1 = pNode->ppLeaves[pPerm[i+1]];
            pLeaf2 = pNode->ppLeaves[pPerm[i  ]];
            assert( !Resm_IsComplement(pLeaf1) );
            assert( !Resm_IsComplement(pLeaf2) );
            if ( Resm_CompareNodes( pLeaf1, pLeaf2 ) )
            {
                Temp       = pPerm[i];
                pPerm[i]   = pPerm[i+1];
                pPerm[i+1] = Temp;
                fChanges   = 1;
            }
        }
    } while ( fChanges );

    // make sure this is indeed a permutation
    for ( i = 0; i < (int)pNode->nLeaves; i++ )
    {
        for ( k = 0; k < (int)pNode->nLeaves; k++ )
            if ( i == pPerm[k] )
                break;
        if ( k == (int)pNode->nLeaves )
        {
            printf( "Resm_NodeOrderFanins(): Permutation is wrong.\n" );
            assert( 0 );
        }
    }
}

/**Function*************************************************************

  Synopsis    [Detects the critical node.]

  Description [Returns 1 the node is critical. Otherwise returns 0.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Resm_NodeIsCritical( Resm_Node_t * pNode )
{
    if ( !pNode->p->fDoingArea )
    {
        // the node's arrival time and required time are close
        if ( Resm_WorstsAreEqual( &pNode->tArrival, &pNode->tRequired[1], pNode->p->fDelayWindow + pNode->p->fEpsilon ) )
            return 1;
    }
    else
    {
        int i;
        // the fanin has only one reference (it is easier to drop)
        for ( i = 0; i < (int)pNode->nLeaves; i++ )
            if ( Resm_NodeReadRefTotal(pNode->ppLeaves[i]) == 1 )
                return 1;
    }
    return 0;
}

/**Function*************************************************************

  Synopsis    [Detects the critical fanin of a node.]

  Description [Returns 1 the fanin is critical. Otherwise returns 0.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Resm_NodeFaninIsCritical( Resm_Node_t * pFanin )
{
    if ( !pFanin->p->fDoingArea )
    {
        // the fanin's arrival time and required time are close
//        if ( Resm_WorstsAreEqual( p, &pFanin->tArrival, &pFanin->tRequired, pFanin->p->fEpsilon ) )
        if ( Resm_WorstsAreEqual( &pFanin->tArrival, &pFanin->tRequired[1], pFanin->p->fDelayWindow + pFanin->p->fEpsilon ) )
            return 1;
    }
    else
    {
        // the fanin has only one reference (it is easier to drop)
        if ( Resm_NodeReadRefTotal(pFanin) == 1 )
            return 1;
    }
    return 0;
}

/**Function*************************************************************

  Synopsis    [Compares two nodes.]

  Description [Returns 1 if the new cut is better. Otherwise returns 0.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Resm_CompareNodes( Resm_Node_t * pNodeOld, Resm_Node_t * pNodeNew )
{
    if ( !pNodeNew->p->fDoingArea )
    {
        // compare the arrival times
        if ( pNodeOld->tArrival.Worst < pNodeNew->tArrival.Worst - pNodeNew->p->fEpsilon )
            return 0;
        if ( pNodeOld->tArrival.Worst > pNodeNew->tArrival.Worst + pNodeNew->p->fEpsilon )
            return 1;
        // compare the areas
        if ( Resm_NodeReadRefTotal(pNodeOld) > Resm_NodeReadRefTotal(pNodeNew) )
            return 0;
        if ( Resm_NodeReadRefTotal(pNodeOld) < Resm_NodeReadRefTotal(pNodeNew) )
            return 1;
        // compare the number of leaves
        if ( pNodeOld->nLeaves < pNodeNew->nLeaves )
            return 0;
        if ( pNodeOld->nLeaves > pNodeNew->nLeaves )
            return 1;
        // otherwise prefer the old cut
        return 0;
    }
    else
    {
       // compare the areas
        if ( Resm_NodeReadRefTotal(pNodeOld) > Resm_NodeReadRefTotal(pNodeNew) )
            return 0;
        if ( Resm_NodeReadRefTotal(pNodeOld) < Resm_NodeReadRefTotal(pNodeNew) )
            return 1;
        // compare the number of leaves
        if ( pNodeOld->nLeaves < pNodeNew->nLeaves )
            return 0;
        if ( pNodeOld->nLeaves > pNodeNew->nLeaves )
            return 1;
        // compare the arrival times
        if ( pNodeOld->tArrival.Worst < pNodeNew->tArrival.Worst - pNodeNew->p->fEpsilon )
            return 0;
        if ( pNodeOld->tArrival.Worst > pNodeNew->tArrival.Worst + pNodeNew->p->fEpsilon )
            return 1;
        // otherwise prefer the old cut
        return 0;
    }
}


/**Function*************************************************************

  Synopsis    [Consider the cuts in the list and choose the best.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Resm_Node_t * Resm_NodeSelectBestCut( Resm_Man_t * p, Resm_Node_t * pNode, Resm_Cut_t * pCutList )
{
    Resm_Cut_t * pCutBest;
    pCutBest = Resm_NodeSelectBestCutInt( p, pNode, pCutList );
    if ( pCutBest == NULL || p->fAcceptMode == 2 ) // always accept
        return pCutBest;
    // comparison will be done using the required time of the corresponding phase
    // assign the previous required time of the old node
    pNode->tPrevious = pNode->tRequired[1];
    // compare the node with the best cut
    if ( Resm_MatchCompare( pNode, pCutBest ) )
        return pCutBest;
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Consider the cuts in the list and choose the best.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Resm_Node_t * Resm_NodeSelectBestCutInt( Resm_Man_t * p, Resm_Node_t * pNode, Resm_Cut_t * pCutList )
{
    Resm_Cut_t * pCut, * pCutBest;
    // go through the cuts and find the best one
    pCutBest = NULL;
    for ( pCut = pCutList; pCut; pCut = pCut->pNext )
    {
        if ( pCut->pSuperBest == NULL )
            continue;
        // the arrival time should not exceed the required time
        if ( pCut->tPrevious.Worst != RESM_FLOAT_LARGE )
        assert( Resm_TimesAreEqual( &pCut->tPrevious, &pNode->tRequired[!pCut->fCompl], p->fEpsilon ) );
        if ( pCut->tArrival.Worst > pCut->tPrevious.Worst + p->fEpsilon )
            continue;
        // if there no best cut, take this one as best
        if ( pCutBest == NULL )
        {
            pCutBest = pCut;
            continue;
        }
        // if the new cut is better than the currently best cut, update
//        if ( Resm_CompareCuts( p, pCutBest, pCut ) )
        if ( Resm_MatchCompare( pCutBest, pCut ) )
            pCutBest = pCut;
    }
    return pCutBest;
}

/**Function*************************************************************

  Synopsis    [Compares two cuts of the same node.]

  Description [Returns 1 if the new cut is better. Otherwise returns 0.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Resm_CompareCuts( Resm_Man_t * p, Resm_Cut_t * pCutOld, Resm_Cut_t * pCutNew )
{
    if ( !p->fDoingArea )
    {
        // compare the arrival times
        if ( pCutOld->tArrival.Worst < pCutNew->tArrival.Worst - p->fEpsilon )
            return 0;
        if ( pCutOld->tArrival.Worst > pCutNew->tArrival.Worst + p->fEpsilon )
            return 1;
        // compare the areas
        if ( pCutOld->aArea < pCutNew->aArea - p->fEpsilon )
            return 0;
        if ( pCutOld->aArea > pCutNew->aArea + p->fEpsilon )
            return 1;
        // compare the number of leaves
//        if ( pCutOld->nLeaves < pCutNew->nLeaves )
//            return 0;
//        if ( pCutOld->nLeaves > pCutNew->nLeaves )
//            return 1;
        // otherwise prefer the old cut
        return 0;
    }
    else
    {
        // compare the areas
        if ( pCutOld->aArea < pCutNew->aArea - p->fEpsilon )
            return 0;
        if ( pCutOld->aArea > pCutNew->aArea + p->fEpsilon )
            return 1;
        // compare the arrival times
        if ( pCutOld->tArrival.Worst < pCutNew->tArrival.Worst - p->fEpsilon )
            return 0;
        if ( pCutOld->tArrival.Worst > pCutNew->tArrival.Worst + p->fEpsilon )
            return 1;
        // compare the number of leaves
//        if ( pCutOld->nLeaves < pCutNew->nLeaves )
//            return 0;
//        if ( pCutOld->nLeaves > pCutNew->nLeaves )
//            return 1;
        // otherwise prefer the old cut
        return 0;
    }
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


