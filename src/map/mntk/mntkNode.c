/**CFile****************************************************************

  FileName    [mntkNode.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Generic mapped network.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 8, 2003.]

  Revision    [$Id: mntkNode.c,v 1.1 2005/02/28 05:34:32 alanmi Exp $]

***********************************************************************/

#include "mntkInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static Mntk_Node_t * Mntk_NodeUpdate( Mntk_Node_t * pNode, Mntk_Cut_t * pCutBest );
static Mntk_Node_t * Mntk_NodeSelectWorstFanin( Mntk_Man_t * p, Mntk_Node_t * pNode );
static int           Mntk_NodeFaninIsCritical( Mntk_Node_t * pFanin );
static int           Mntk_CompareNodes( Mntk_Node_t * pNodeOld, Mntk_Node_t * pNodeNew );

static Mntk_Node_t * Mntk_NodeSelectBestCut( Mntk_Man_t * p, Mntk_Node_t * pNode, Mntk_Cut_t * pCutList );
static Mntk_Node_t * Mntk_NodeSelectBestCutInt( Mntk_Man_t * p, Mntk_Node_t * pNode, Mntk_Cut_t * pCutList );
static int           Mntk_CompareCuts( Mntk_Man_t * p, Mntk_Cut_t * pCutOld, Mntk_Cut_t * pCutNew );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Resynthesizes one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mntk_NodeResynthesize( Mntk_Node_t * pNode )
{
    Mntk_Man_t * p = pNode->p;
    Mntk_Node_t * pFanin, * pNodeNew;
    int fChange;

    p->nNodesTried++;

    assert( !Mntk_IsComplement(pNode) );
    assert( Mntk_NodeReadRefTotal(pNode) > 0 );
    assert( Mntk_CheckNoDuplicates( pNode, pNode->ppLeaves, pNode->nLeaves ) );

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
        pFanin = Mntk_NodeSelectWorstFanin( p, pNode );
        if ( pFanin == NULL )  // happens when all the fanins are PIs
            break;
        pNodeNew = Mntk_NodeResynthesizeFanin( pNode, pFanin );
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
Mntk_Node_t * Mntk_NodeResynthesizeFanin( Mntk_Node_t * pNode, Mntk_Node_t * pFanin )
{
    Mntk_Man_t * p = pNode->p;
    Mntk_NodeVec_t * vCands;
    Mntk_Node_t * pNodeNew;
    Mntk_Cut_t * pCutBest, * pCutList;
    int clk;

    assert( !Mntk_IsComplement(pNode) );
    assert( !Mntk_IsComplement(pFanin) );

    p->nFaninsTried++;

    // collect the delay-improving candidates
clk = clock();
    Mntk_CollectRelatedLogic( pNode );
    vCands = pNode->p->vWinCands;
p->timeLogic += clock() - clk;

if ( p->fVerbose > 1 )
printf( "Collected %d candidates.\n", vCands? vCands->nSize : 0 ); 

    if ( vCands == NULL || vCands->nSize == 0 )
        return NULL;

//    Mntk_NodeBddsPrepare( pNode );

    // perform bit parallel simulation of the given part
clk = clock();
    Mntk_SimulateWindow( p );
p->timeSimul += clock() - clk;

    // find possible new cuts
clk = clock();
    pCutList = Mntk_NodeReplaceOneFanin( pNode, pFanin, vCands );
p->timeResub += clock() - clk;

//    Mntk_NodeBddsCleanup( pNode );

if ( p->fVerbose > 1 )
printf( "Derived %d alternative cuts. \n", Mntk_NodeListCount(pCutList) ); 
    if ( pCutList == NULL )
        return NULL;

    // match the cuts in the list
clk = clock();
    Mntk_MatchCuts( pNode, pCutList );
p->timeMatch += clock() - clk;

    // select the best cut for delay
    pCutBest = Mntk_NodeSelectBestCut( p, pNode, pCutList );
    // remove the current cut 
    Mntk_NodeListRecycle( p, pCutList, pCutBest );
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
    Mntk_NodePrint( pNode );
    printf( "NEW " );
    Mntk_NodePrint( pCutBest );
}

clk = clock();
    pNodeNew = Mntk_NodeUpdate( pNode, pCutBest );
p->timeUpdate += clock() - clk;

if ( p->fVerbose )
{
    printf( "FIN " );
    Mntk_NodePrint( pNodeNew );
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
Mntk_Node_t * Mntk_NodeRemoveDupFanins( Mntk_Node_t * pNode )
{
    pNode->p->nNodesTried++;
    assert( !Mntk_CheckNoDuplicates( pNode, pNode->ppLeaves, pNode->nLeaves ) );
    return Mntk_NodeResynthesizeFanin( pNode, NULL );
}

/**Function*************************************************************

  Synopsis    [Updates the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mntk_Node_t * Mntk_NodeUpdate( Mntk_Node_t * pNodeOld, Mntk_Cut_t * pCutBest )
{   
    Mntk_Man_t * p = pNodeOld->p;
    Mntk_Node_t * pNodeNew;
    float AreaOld;
    int fComplInv;

//    assert( Mntk_VerifyRefs( p ) );
    // remove old node
    AreaOld = Mntk_NodeRemoveFaninFanouts( pNodeOld, 1 );
    // derive the nodes corresponding to the new cut
    pNodeNew = Rems_ConstructSuper( pCutBest );
    fComplInv = Mntk_IsComplement(pNodeNew);
    assert( fComplInv == 0 );
    pNodeNew = Mntk_Regular(pNodeNew);
    // compute the arrival times of the intermediate nodes
    Mntk_NodeComputeArrival_rec( pNodeNew );
    if ( !fComplInv )
    assert( Mntk_TimesAreEqual( &pNodeNew->tArrival, &pCutBest->tArrival, p->fEpsilon ) );
    // move the fanouts from the old node to the new node
    Mntk_NodeTransferFanouts( pNodeOld, pNodeNew, pCutBest->fCompl ^ fComplInv );
    // insert the new node
    pNodeNew->aArea = Mntk_NodeInsertFaninFanouts( pNodeNew, 1 );
    if ( !fComplInv )
    assert( pNodeNew->aArea == pCutBest->aArea );
    Mntk_NodeFree( pCutBest );

    // from now on, pNodeOld will be a floating node???
//    assert( Mntk_VerifyRefs( p ) );

    // update the arrival/required times
//    Mntk_NodeUpdateArrivalTimeSlow( pNodeNew );
//    Mntk_NodeUpdateRequiredTimeSlow( pNodeNew );
    Mntk_NodeUpdateArrivalTime( pNodeNew );
    Mntk_NodeUpdateRequiredTime( pNodeOld, pNodeNew );
    Mntk_ComputeArrivalAll( p, 1 );

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
Mntk_Node_t * Mntk_NodeSelectWorstFanin( Mntk_Man_t * p, Mntk_Node_t * pNode )
{
    Mntk_Node_t * pFanin, * pFaninWorst;
    int i, iNum;

    iNum = -1;
    pFaninWorst = NULL;
    for ( i = 0; i < (int)pNode->nLeaves; i++ )
    {
        pFanin = Mntk_Regular(pNode->ppLeaves[i]);
        if ( Mntk_NodeIsVar(pFanin) )
            continue;
//        if ( !Mntk_NodeFaninIsCritical(pFanin) )
//            continue;
        // check if this fanin was already tried
        if ( pNode->uPhase & (1 << i) )
            continue;
        // update the fanin if it is worse
        if ( pFaninWorst == NULL || Mntk_CompareNodes( pFanin, pFaninWorst ) )
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
void Mntk_NodeOrderFanins( Mntk_Node_t * pNode, int * pPerm )
{
    Mntk_Node_t * pLeaf1, * pLeaf2;
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
            assert( !Mntk_IsComplement(pLeaf1) );
            assert( !Mntk_IsComplement(pLeaf2) );
            if ( Mntk_CompareNodes( pLeaf1, pLeaf2 ) )
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
            printf( "Mntk_NodeOrderFanins(): Permutation is wrong.\n" );
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
int Mntk_NodeIsCritical( Mntk_Node_t * pNode )
{
    if ( !pNode->p->fDoingArea )
    {
        // the node's arrival time and required time are close
        if ( Mntk_WorstsAreEqual( &pNode->tArrival, &pNode->tRequired[1], pNode->p->fDelayWindow + pNode->p->fEpsilon ) )
            return 1;
    }
    else
    {
        int i;
        // the fanin has only one reference (it is easier to drop)
        for ( i = 0; i < (int)pNode->nLeaves; i++ )
            if ( Mntk_NodeReadRefTotal(pNode->ppLeaves[i]) == 1 )
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
int Mntk_NodeFaninIsCritical( Mntk_Node_t * pFanin )
{
    if ( !pFanin->p->fDoingArea )
    {
        // the fanin's arrival time and required time are close
//        if ( Mntk_WorstsAreEqual( p, &pFanin->tArrival, &pFanin->tRequired, pFanin->p->fEpsilon ) )
        if ( Mntk_WorstsAreEqual( &pFanin->tArrival, &pFanin->tRequired[1], pFanin->p->fDelayWindow + pFanin->p->fEpsilon ) )
            return 1;
    }
    else
    {
        // the fanin has only one reference (it is easier to drop)
        if ( Mntk_NodeReadRefTotal(pFanin) == 1 )
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
int Mntk_CompareNodes( Mntk_Node_t * pNodeOld, Mntk_Node_t * pNodeNew )
{
    if ( !pNodeNew->p->fDoingArea )
    {
        // compare the arrival times
        if ( pNodeOld->tArrival.Worst < pNodeNew->tArrival.Worst - pNodeNew->p->fEpsilon )
            return 0;
        if ( pNodeOld->tArrival.Worst > pNodeNew->tArrival.Worst + pNodeNew->p->fEpsilon )
            return 1;
        // compare the areas
        if ( Mntk_NodeReadRefTotal(pNodeOld) > Mntk_NodeReadRefTotal(pNodeNew) )
            return 0;
        if ( Mntk_NodeReadRefTotal(pNodeOld) < Mntk_NodeReadRefTotal(pNodeNew) )
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
        if ( Mntk_NodeReadRefTotal(pNodeOld) > Mntk_NodeReadRefTotal(pNodeNew) )
            return 0;
        if ( Mntk_NodeReadRefTotal(pNodeOld) < Mntk_NodeReadRefTotal(pNodeNew) )
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
Mntk_Node_t * Mntk_NodeSelectBestCut( Mntk_Man_t * p, Mntk_Node_t * pNode, Mntk_Cut_t * pCutList )
{
    Mntk_Cut_t * pCutBest;
    pCutBest = Mntk_NodeSelectBestCutInt( p, pNode, pCutList );
    if ( pCutBest == NULL || p->fAcceptMode == 2 ) // always accept
        return pCutBest;
    // comparison will be done using the required time of the corresponding phase
    // assign the previous required time of the old node
    pNode->tPrevious = pNode->tRequired[1];
    // compare the node with the best cut
    if ( Mntk_MatchCompare( pNode, pCutBest ) )
        return pCutBest;
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Consider the cuts in the list and choose the best.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mntk_Node_t * Mntk_NodeSelectBestCutInt( Mntk_Man_t * p, Mntk_Node_t * pNode, Mntk_Cut_t * pCutList )
{
    Mntk_Cut_t * pCut, * pCutBest;
    // go through the cuts and find the best one
    pCutBest = NULL;
    for ( pCut = pCutList; pCut; pCut = pCut->pNext )
    {
        if ( pCut->pSuperBest == NULL )
            continue;
        // the arrival time should not exceed the required time
        if ( pCut->tPrevious.Worst != MNTK_FLOAT_LARGE )
        assert( Mntk_TimesAreEqual( &pCut->tPrevious, &pNode->tRequired[!pCut->fCompl], p->fEpsilon ) );
        if ( pCut->tArrival.Worst > pCut->tPrevious.Worst + p->fEpsilon )
            continue;
        // if there no best cut, take this one as best
        if ( pCutBest == NULL )
        {
            pCutBest = pCut;
            continue;
        }
        // if the new cut is better than the currently best cut, update
//        if ( Mntk_CompareCuts( p, pCutBest, pCut ) )
        if ( Mntk_MatchCompare( pCutBest, pCut ) )
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
int Mntk_CompareCuts( Mntk_Man_t * p, Mntk_Cut_t * pCutOld, Mntk_Cut_t * pCutNew )
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


