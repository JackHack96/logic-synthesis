/**CFile****************************************************************

  FileName    [resNode.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Generic resynthesis engine.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 8, 2003.]

  Revision    [$Id: resNode.c,v 1.1 2005/01/23 06:59:49 alanmi Exp $]

***********************************************************************/

#include "resInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static Fpga_Node_t * Res_NodeSelectWorstFanin( Res_Man_t * p, Fpga_Node_t * pNode );
static int           Res_NodeFaninIsCritical( Res_Man_t * p, Fpga_Node_t * pFanin );
static int           Res_CompareNodes( Res_Man_t * p, Fpga_Node_t * pNodeOld, Fpga_Node_t * pNodeNew );

static Fpga_Cut_t *  Res_NodeSelectBestCut( Res_Man_t * p, Fpga_Node_t * pNode, Fpga_Cut_t * pCutList );
static int           Res_CompareCuts( Res_Man_t * p, Fpga_Cut_t * pCutOld, Fpga_Cut_t * pCutNew );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Resynthesizes one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Res_NodeResynthesize( Res_Man_t * p, Fpga_Node_t * pNode )
{
    Fpga_NodeVec_t * vCands;
    Fpga_Node_t * pFanin;
    Fpga_Cut_t * pCutBest, * pCutList;
    float AreaOld, AreaNew;
    int clk, i, v;
    int fTryTwo = 1;

    p->nNodesTried++;

    ///////////////////////////////////////////////
    for ( i = 0; i < pNode->pCutBest->nLeaves; i++ )
    for ( v = i + 1; v < pNode->pCutBest->nLeaves; v++ )
        if ( pNode->pCutBest->ppLeaves[v] == pNode->pCutBest->ppLeaves[i] )
        {
            printf( "Res_NodeResynthesize(): Duplicated nodes.\n" );
            assert( 0 );
        }
    ///////////////////////////////////////////////

if ( p->fVerbose )
printf( "NODE %4d:  Arrival = %4.2f. Required = %4.2f. Area = %4.2f. Fanins = %d. Refs = %d.\n", 
       pNode->Num, pNode->pCutBest->tArrival, pNode->tRequired, pNode->pCutBest->aFlow, pNode->pCutBest->nLeaves, pNode->nRefs );

    // get the most timing critical fanin
    pFanin = Res_NodeSelectWorstFanin( p, pNode );
    if ( pFanin == NULL )  // happens when all the fanins are PIs
        return 0;

    // collect the delay-improving candidates
clk = clock();
    if ( p->fGlobalCones )
        vCands = Res_CollectRelatedLogic( p, pNode );
    else
    {
        Res_CollectRelatedLogic2( p, pNode );
        vCands = p->vWinCands;
    }

p->timeSelect += clock() - clk;
if ( p->fVerbose )
printf( "Collected %d candidates.\n", vCands? vCands->nSize : 0 ); 
    if ( vCands == NULL || vCands->nSize == 0 )
        return 0;

    // find possible new cuts
clk = clock();
    pCutList = Res_NodeReplaceOneFanin( p, pNode, pFanin, vCands, fTryTwo );
p->timeResub += clock() - clk;
if ( p->fVerbose )
printf( "Derived %d alternative cuts. \n", Fpga_CutListCount(pCutList) ); 
    if ( p->fGlobalCones )
        Fpga_NodeVecFree( vCands );
    if ( pCutList == NULL )
        return 0;

    // select the best cut for delay
    pCutBest = Res_NodeSelectBestCut( p, pNode, pCutList );
    // remove the current cut 
    Fpga_CutListRecycle( p->pMan, pCutList, pCutBest );
    if ( pCutBest == NULL )
        return 0;

    p->nCutsUsed++;
    p->nCutsGen[pCutBest->nVolume]++;

clk = clock();
    // remember the old cut
    pNode->pCutOld = pNode->pCutBest;
    // replace the cut
    AreaOld = Fpga_CutDeref( p->pMan, pNode, pNode->pCutBest, 1 );
    pNode->pCutBest = pCutBest;
    AreaNew = Fpga_CutRef( p->pMan, pNode, pNode->pCutBest, 1 );
    // update the area
    p->pMan->fAreaGain += AreaOld - AreaNew;
    // update the arrival times
    Res_NodeUpdateArrivalTimeSlow( p, pNode );
    // update the required times
    Res_NodeUpdateRequiredTimeSlow( p, pNode );
p->timeUpdate += clock() - clk;

    // print the present and the new cut
if ( p->fVerbose )
{
    printf( "Accepting change (%d):\n", pCutBest->nVolume );
    printf( "    Old: " );
    Fpga_CutPrint( p->pMan, pNode, pNode->pCutOld );
    printf( "    New: " );
    Fpga_CutPrint( p->pMan, pNode, pNode->pCutBest );
}
    return 1;
}

/**Function*************************************************************

  Synopsis    [Returns the index of the most timing critical fanin.]

  Description [Marks and skips the fanins that have already need tried.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fpga_Node_t * Res_NodeSelectWorstFanin( Res_Man_t * p, Fpga_Node_t * pNode )
{
    Fpga_Node_t * pFanin, * pFaninWorst;
    int i, iNum;

    iNum = -1;
    pFaninWorst = NULL;
    for ( i = 0; i < pNode->pCutBest->nLeaves; i++ )
    {
        pFanin = pNode->pCutBest->ppLeaves[i];
        if ( !Fpga_NodeIsAnd(pFanin) )
            continue;
//        if ( !Res_NodeFaninIsCritical(p, pFanin) )
//            continue;
        // check if this fanin was already tried
        if ( pNode->pCutBest->uSign & (1 << i) )
            continue;
        // update the fanin if it is worse
        if ( pFaninWorst == NULL || Res_CompareNodes( p, pFanin, pFaninWorst ) )
        {
            iNum = i;
            pFaninWorst = pFanin;
        }
    }
    // mark this fanin as tried
//    if ( iNum >= 0 )
//        pNode->pCutBest->uSign |= (1 << iNum);
    return pFaninWorst;
}


/**Function*************************************************************

  Synopsis    [Finds the permutation of the fanin indexes.]

  Description [The resulting permutation will be used to try to remove
  the fanins. This way the most offending fanins will be removed first.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Res_NodeOrderFanins( Res_Man_t * p, Fpga_Cut_t * pCut, int * pPerm )
{
    int fChanges, Temp, i, k;

    // start with the identity permutation
    for ( i = 0; i < pCut->nLeaves; i++ )
        pPerm[i] = i;

    // perform pair permutation until no change
    do {
        fChanges = 0;
        for ( i = 0; i < pCut->nLeaves-1; i++ )
            if ( Res_CompareNodes( p, pCut->ppLeaves[pPerm[i+1]], pCut->ppLeaves[pPerm[i]] ) )
            {
                Temp       = pPerm[i];
                pPerm[i]   = pPerm[i+1];
                pPerm[i+1] = Temp;
                fChanges = 1;
            }
    } while ( fChanges );

    // make sure this is indeed a permutation
    for ( i = 0; i < pCut->nLeaves; i++ )
    {
        for ( k = 0; k < pCut->nLeaves; k++ )
            if ( i == pPerm[k] )
                break;
        if ( k == pCut->nLeaves )
        {
            printf( "Fpga_NodeCompressFaninsOrder(): Permutation is wrong.\n" );
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
int Res_NodeIsCritical( Res_Man_t * p, Fpga_Node_t * pNode )
{
    if ( !p->fDoingArea )
    {
        // the fanin's arrival time and required time are close
        if ( pNode->pCutBest->tArrival < pNode->tRequired + p->pMan->fDelayWindow + p->pMan->fEpsilon &&
             pNode->pCutBest->tArrival > pNode->tRequired - p->pMan->fDelayWindow - p->pMan->fEpsilon )
            return 1;
    }
    else
    {
        int i;
        // the fanin has only one reference (it is easier to drop)
        for ( i = 0; i < pNode->pCutBest->nLeaves; i++ )
            if ( pNode->pCutBest->ppLeaves[i]->nRefs == 1 )
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
int Res_NodeFaninIsCritical( Res_Man_t * p, Fpga_Node_t * pFanin )
{
    if ( !p->fDoingArea )
    {
        // the fanin's arrival time and required time are close
        if ( pFanin->pCutBest->tArrival < pFanin->tRequired + p->pMan->fEpsilon &&
             pFanin->pCutBest->tArrival > pFanin->tRequired - p->pMan->fEpsilon )
            return 1;
    }
    else
    {
        // the fanin has only one reference (it is easier to drop)
        if ( pFanin->nRefs == 1 )
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
int Res_CompareNodes( Res_Man_t * p, Fpga_Node_t * pNodeOld, Fpga_Node_t * pNodeNew )
{
    if ( !p->fDoingArea )
    {
        // compare the arrival times
        if ( pNodeOld->pCutBest->tArrival < pNodeNew->pCutBest->tArrival - p->pMan->fEpsilon )
            return 0;
        if ( pNodeOld->pCutBest->tArrival > pNodeNew->pCutBest->tArrival + p->pMan->fEpsilon )
            return 1;
        // compare the areas
        if ( pNodeOld->nRefs > pNodeNew->nRefs )
            return 0;
        if ( pNodeOld->nRefs < pNodeNew->nRefs )
            return 1;
        // compare the number of leaves
        if ( pNodeOld->pCutBest->nLeaves < pNodeNew->pCutBest->nLeaves )
            return 0;
        if ( pNodeOld->pCutBest->nLeaves > pNodeNew->pCutBest->nLeaves )
            return 1;
        // otherwise prefer the old cut
        return 0;
    }
    else
    {
        // compare the areas
        if ( pNodeOld->nRefs > pNodeNew->nRefs )
            return 0;
        if ( pNodeOld->nRefs < pNodeNew->nRefs )
            return 1;
        // compare the number of leaves
        if ( pNodeOld->pCutBest->nLeaves < pNodeNew->pCutBest->nLeaves )
            return 0;
        if ( pNodeOld->pCutBest->nLeaves > pNodeNew->pCutBest->nLeaves )
            return 1;
        // compare the arrival times
        if ( pNodeOld->pCutBest->tArrival < pNodeNew->pCutBest->tArrival - p->pMan->fEpsilon )
            return 0;
        if ( pNodeOld->pCutBest->tArrival > pNodeNew->pCutBest->tArrival + p->pMan->fEpsilon )
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
Fpga_Cut_t * Res_NodeSelectBestCut( Res_Man_t * p, Fpga_Node_t * pNode, Fpga_Cut_t * pCutList )
{
    Fpga_Cut_t * pCut, * pCutBest;
    pCutBest = pNode->pCutBest;
    for ( pCut = pCutList; pCut; pCut = pCut->pNext )
    {
        // the arrival time should not exceed the required time
        if ( pCut->tArrival > pNode->tRequired + p->pMan->fEpsilon )
            continue;
        // the number of inputs can be implemented
        if ( pCut->nLeaves > p->pMan->pLutLib->LutMax )
            continue;
        // if the new cut is better than the currently best cut, update
        if ( Res_CompareCuts( p, pCutBest, pCut ) )
            pCutBest = pCut;
    }
    if ( pCutBest == pNode->pCutBest )
        return NULL;
    assert( pCutBest->nLeaves <= p->pMan->pLutLib->LutMax );
    return pCutBest;
}

/**Function*************************************************************

  Synopsis    [Compares two cuts of the same node.]

  Description [Returns 1 if the new cut is better. Otherwise returns 0.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Res_CompareCuts( Res_Man_t * p, Fpga_Cut_t * pCutOld, Fpga_Cut_t * pCutNew )
{
    if ( !p->fDoingArea )
    {
        // compare the arrival times
        if ( pCutOld->tArrival < pCutNew->tArrival - p->pMan->fEpsilon )
            return 0;
        if ( pCutOld->tArrival > pCutNew->tArrival + p->pMan->fEpsilon )
            return 1;
        // compare the areas
        if ( pCutOld->aFlow < pCutNew->aFlow - p->pMan->fEpsilon )
            return 0;
        if ( pCutOld->aFlow > pCutNew->aFlow + p->pMan->fEpsilon )
            return 1;
        // compare the number of leaves
        if ( pCutOld->nLeaves < pCutNew->nLeaves )
            return 0;
        if ( pCutOld->nLeaves > pCutNew->nLeaves )
            return 1;
        // otherwise prefer the old cut
        return 0;
    }
    else
    {
        // compare the areas
        if ( pCutOld->aFlow < pCutNew->aFlow - p->pMan->fEpsilon )
            return 0;
        if ( pCutOld->aFlow > pCutNew->aFlow + p->pMan->fEpsilon )
            return 1;
        // compare the arrival times
        if ( pCutOld->tArrival < pCutNew->tArrival - p->pMan->fEpsilon )
            return 0;
        if ( pCutOld->tArrival > pCutNew->tArrival + p->pMan->fEpsilon )
            return 1;
        // compare the number of leaves
        if ( pCutOld->nLeaves < pCutNew->nLeaves )
            return 0;
        if ( pCutOld->nLeaves > pCutNew->nLeaves )
            return 1;
        // otherwise prefer the old cut
        return 0;
    }
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


