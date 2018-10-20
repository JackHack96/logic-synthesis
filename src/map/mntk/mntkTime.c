/**CFile****************************************************************

  FileName    [mntkTime.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Generic mapped network.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 8, 2003.]

  Revision    [$Id: mntkTime.c,v 1.3 2005/03/10 04:23:53 satrajit Exp $]

***********************************************************************/

#include "mntkInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void Mntk_ComputeArrivalAll_rec( Mntk_Node_t * pNode, int fVerify );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Returns the arrival time of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mntk_Time_t * Mntk_NodeReadTimeArrival( Mntk_Node_t * pNode, int fPhase )
{
    static Mntk_Time_t TimeIn, * pTimeIn = &TimeIn;
    if ( fPhase )
    {
        *pTimeIn = pNode->tArrival;
    }
    else
    {
        pTimeIn->Rise = pNode->tArrival.Fall + pNode->p->tDelayInv.Rise;
        pTimeIn->Fall = pNode->tArrival.Rise + pNode->p->tDelayInv.Fall;
        pTimeIn->Worst = MNTK_MAX( pTimeIn->Rise, pTimeIn->Fall );
    }
    return pTimeIn;
}

/**Function*************************************************************

  Synopsis    [Computes the maximum arrival times.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mntk_TimesAreEqual( Mntk_Time_t * pTime1, Mntk_Time_t * pTime2, float tDelta )
{
    if ( pTime1->Rise < pTime2->Rise + tDelta && 
         pTime2->Rise < pTime1->Rise + tDelta &&
         pTime1->Fall < pTime2->Fall + tDelta &&
         pTime2->Fall < pTime1->Fall + tDelta )
         return 1;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Computes the maximum arrival times.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mntk_WorstsAreEqual( Mntk_Time_t * pTime1, Mntk_Time_t * pTime2, float tDelta )
{
    if ( pTime1->Worst < pTime2->Worst + tDelta && 
         pTime2->Worst < pTime1->Worst + tDelta )
         return 1;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Computes the arrival times of the cut recursively.]

  Description [When computing the arrival time for the previously unused 
  cuts, their arrival time may be incorrect because their fanins have 
  incorrect arrival time. This procedure is called to fix this problem.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mntk_NodeComputeArrival_rec( Mntk_Node_t * pNode )
{
    int i;
    for ( i = 0; i < (int)pNode->nLeaves; i++ )
        if ( Mntk_NodeReadRefTotal(pNode->ppLeaves[i]) == 0 )
            Mntk_NodeComputeArrival_rec( Mntk_Regular(pNode->ppLeaves[i]) );
    Mntk_NodeComputeArrival( pNode );
}

/**function*************************************************************

  synopsis    [Computes the arrival times of all nodes.]

  description []
               
  sideeffects []

  seealso     []

***********************************************************************/
void Mntk_ComputeArrivalAll( Mntk_Man_t * p, int fVerify )
{
    int i;
    for ( i = 0; i < p->vNodesAll->nSize; i++ )
        assert( p->vNodesAll->pArray[i]->fMark0 == 0 );
    // compute recursively
    for ( i = 0; i < p->vNodesAll->nSize; i++ )
        Mntk_ComputeArrivalAll_rec( p->vNodesAll->pArray[i], fVerify );
    // compute the for pseudo-node
    if ( fVerify )
        Mntk_NodeComputeArrivalVerify( p->pPseudo );
    else
        Mntk_NodeComputeArrival( p->pPseudo );
    // unmark the nodes
    for ( i = 0; i < p->vNodesAll->nSize; i++ )
        p->vNodesAll->pArray[i]->fMark0 = 0;
}

/**function*************************************************************

  synopsis    [Computes the arrival times of all nodes.]

  description []
               
  sideeffects []

  seealso     []

***********************************************************************/
void Mntk_ComputeArrivalAll_rec( Mntk_Node_t * pNode, int fVerify )
{
    int i;
    assert( !Mntk_IsComplement(pNode) );
    if ( pNode->fMark0 == 1 )
        return;
    if ( Mntk_NodeIsConst(pNode) )
        return;
    // call for the fanins
    for ( i = 0; i < pNode->nLeaves; i++ )
        Mntk_ComputeArrivalAll_rec( Mntk_Regular(pNode->ppLeaves[i]), fVerify );
    Mntk_NodeComputeArrival( pNode );
    assert( pNode->fMark0 == 0 );
    pNode->fMark0 = 1;
}

/**function*************************************************************

  synopsis    [Computes the arrival times of the node.]

  description []
               
  sideeffects []

  seealso     []

***********************************************************************/
void Mntk_NodeComputeArrival( Mntk_Node_t * pNode )
{
    float tDelayBlockRise, tDelayBlockFall;
    Map_Time_t TimeIn, * pTimeIn = &TimeIn;
    Mntk_Node_t * pChild;
    Mio_PinPhase_t PinPhase;
    Mio_Pin_t * pPin;
    int i, fPhaseC;

    if ( Mntk_NodeIsVar(pNode) )
        return;

    // every node that ends up here should have a gate assigned
    assert( pNode->pGate != NULL );

    // save the previous time
    pNode->tPrevious = pNode->tArrival;

    // start the arrival times of the positive phase
    pNode->tArrival.Rise = pNode->tArrival.Fall = 0; 
    pPin = Mio_GateReadPins(pNode->pGate);
    for ( i = 0; i < (int)pNode->nLeaves; i++, pPin = Mio_PinReadNext(pPin) )
    {
        pChild  =  Mntk_Regular(pNode->ppLeaves[i]);
        fPhaseC = !Mntk_IsComplement(pNode->ppLeaves[i]);
        if ( fPhaseC )
        {
            pTimeIn->Rise = pChild->tArrival.Rise;
            pTimeIn->Fall = pChild->tArrival.Fall;
        }
        else
        {
            pTimeIn->Rise = pChild->tArrival.Fall + pNode->p->tDelayInv.Rise;
            pTimeIn->Fall = pChild->tArrival.Rise + pNode->p->tDelayInv.Fall;
        }

        // get the interesting parameters of this pin
        PinPhase = Mio_PinReadPhase(pPin);
        tDelayBlockRise = (float)Mio_PinReadDelayBlockRise( pPin );  
        tDelayBlockFall = (float)Mio_PinReadDelayBlockFall( pPin );  
        // compute the arrival times of the positive phase
        if ( PinPhase != MIO_PHASE_INV )  // NONINV phase is present
        {
            if ( pNode->tArrival.Rise < pTimeIn->Rise + tDelayBlockRise )
                pNode->tArrival.Rise = pTimeIn->Rise + tDelayBlockRise;
            if ( pNode->tArrival.Fall < pTimeIn->Fall + tDelayBlockFall )
                pNode->tArrival.Fall = pTimeIn->Fall + tDelayBlockFall;
        }
        if ( PinPhase != MIO_PHASE_NONINV )  // INV phase is present
        {
            if ( pNode->tArrival.Rise < pTimeIn->Fall + tDelayBlockRise )
                pNode->tArrival.Rise = pTimeIn->Fall + tDelayBlockRise;
            if ( pNode->tArrival.Fall < pTimeIn->Rise + tDelayBlockFall )
                pNode->tArrival.Fall = pTimeIn->Rise + tDelayBlockFall;
        }
    }
    pNode->tArrival.Worst = MNTK_MAX( pNode->tArrival.Rise, pNode->tArrival.Fall );
}

/**function*************************************************************

  synopsis    [Computes the arrival times of the node.]

  description []
               
  sideeffects []

  seealso     []

***********************************************************************/
void Mntk_NodeComputeArrivalVerify( Mntk_Node_t * pNode )
{
    float tDelayBlockRise, tDelayBlockFall;
    Map_Time_t tArrivalTest, TimeIn, * pTimeIn = &TimeIn;
    Mntk_Node_t * pChild;
    Mio_PinPhase_t PinPhase;
    Mio_Pin_t * pPin;
    int i, fPhaseC;

    if ( Mntk_NodeIsVar(pNode) )
        return;

    // start the arrival times of the positive phase
    tArrivalTest.Rise = tArrivalTest.Fall = 0; 
    pPin = Mio_GateReadPins(pNode->pGate);
    for ( i = 0; i < (int)pNode->nLeaves; i++, pPin = Mio_PinReadNext(pPin) )
    {
        pChild  =  Mntk_Regular(pNode->ppLeaves[i]);
        fPhaseC = !Mntk_IsComplement(pNode->ppLeaves[i]);
        if ( fPhaseC )
        {
            pTimeIn->Rise = pChild->tRequired[1].Rise;
            pTimeIn->Fall = pChild->tRequired[1].Fall;
        }
        else
        {
            pTimeIn->Rise = pChild->tRequired[1].Fall + pNode->p->tDelayInv.Rise;
            pTimeIn->Fall = pChild->tRequired[1].Rise + pNode->p->tDelayInv.Fall;
        }

        // get the interesting parameters of this pin
        PinPhase = Mio_PinReadPhase(pPin);
        tDelayBlockRise = (float)Mio_PinReadDelayBlockRise( pPin );  
        tDelayBlockFall = (float)Mio_PinReadDelayBlockFall( pPin );  
        // compute the arrival times of the positive phase
        if ( PinPhase != MIO_PHASE_INV )  // NONINV phase is present
        {
            if ( tArrivalTest.Rise < pTimeIn->Rise + tDelayBlockRise )
                tArrivalTest.Rise = pTimeIn->Rise + tDelayBlockRise;
            if ( tArrivalTest.Fall < pTimeIn->Fall + tDelayBlockFall )
                tArrivalTest.Fall = pTimeIn->Fall + tDelayBlockFall;
        }
        if ( PinPhase != MIO_PHASE_NONINV )  // INV phase is present
        {
            if ( tArrivalTest.Rise < pTimeIn->Fall + tDelayBlockRise )
                tArrivalTest.Rise = pTimeIn->Fall + tDelayBlockRise;
            if ( tArrivalTest.Fall < pTimeIn->Rise + tDelayBlockFall )
                tArrivalTest.Fall = pTimeIn->Rise + tDelayBlockFall;
        }
    }
    tArrivalTest.Worst = MNTK_MAX( pNode->tArrival.Rise, pNode->tArrival.Fall );
    assert( tArrivalTest.Worst < pNode->tRequired[1].Worst + pNode->p->fEpsilon );
}



/**Function*************************************************************

  Synopsis    [Computes the required times of all nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mntk_TimeComputeRequiredGlobal( Mntk_Man_t * p )
{
    if ( p->DelayLimit <= p->pPseudo->tArrival.Worst )
        p->fRequiredGlo = p->pPseudo->tArrival.Worst;
    else
        p->fRequiredGlo = p->DelayLimit;
    p->fRequiredGain = p->fRequiredStart - p->fRequiredGlo;
    Mntk_TimeComputeRequired( p, p->fRequiredGlo );
}

/**Function*************************************************************

  Synopsis    [Computes the required times of all nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mntk_TimeComputeRequired( Mntk_Man_t * p, float fRequired )
{
    Mntk_NodeVec_t * vNodes;
    int i;

    for ( i = 0; i < p->vNodesAll->nSize; i++ )
    {
        p->vNodesAll->pArray[i]->tRequired[0].Rise = MNTK_FLOAT_LARGE;
        p->vNodesAll->pArray[i]->tRequired[0].Fall = MNTK_FLOAT_LARGE;
        p->vNodesAll->pArray[i]->tRequired[0].Worst = MNTK_FLOAT_LARGE;
        p->vNodesAll->pArray[i]->tRequired[1].Rise = MNTK_FLOAT_LARGE;
        p->vNodesAll->pArray[i]->tRequired[1].Fall = MNTK_FLOAT_LARGE;
        p->vNodesAll->pArray[i]->tRequired[1].Worst = MNTK_FLOAT_LARGE;
    }

    // set the required times for the POs
    p->pPseudo->tRequired[1].Rise  = fRequired;
    p->pPseudo->tRequired[1].Fall  = fRequired;
    p->pPseudo->tRequired[1].Worst = fRequired;

    // collect nodes reachable from POs in the DFS order through the best cuts
//    vNodes = Mntk_CollectNodesDfs( p );
    vNodes = Mntk_NodeVecDup( p->vNodesAll );
    Mntk_TimePropagateRequired( p, vNodes );
    Mntk_NodeVecFree( vNodes );
}

/**Function*************************************************************

  Synopsis    [Computes the required times of the given nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mntk_TimePropagateRequired( Mntk_Man_t * p, Mntk_NodeVec_t * vNodes )
{
    int k;
    // sorts the nodes in the decreasing order of arrival times
    Mntk_NodeVecSortByArrival( vNodes, 0 );
    for ( k = 1; k < vNodes->nSize; k++ )
        assert( vNodes->pArray[k-1]->tArrival.Worst >= vNodes->pArray[k]->tArrival.Worst );
    // go through the nodes in the reverse topological order
    for ( k = 0; k < vNodes->nSize; k++ )
        if ( vNodes->pArray[k] != p->pPseudo )
            Mntk_NodeComputeRequired( vNodes->pArray[k] );
}

/**function*************************************************************

  Synopsis    [Propagates required times for one node.]

  Description [Propagates the required times for the node.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mntk_NodeComputeRequired( Mntk_Node_t * pNode )
{
    Map_Time_t ReqIn, * pReqIn = &ReqIn;
    float tDelayBlockRise, tDelayBlockFall;
    Mio_PinPhase_t PinPhase;
    Mio_Pin_t * pPin;
    Mntk_Node_t * pFanout;
    int i, iPin, fPhase;

//    assert( pNode->vFanouts->nSize > 0 );

    // save the previous time
    pNode->tPrevious = pNode->tRequired[1];

    // start the required times
    pNode->tRequired[0].Rise = pNode->tRequired[0].Fall = MNTK_FLOAT_LARGE;
    pNode->tRequired[1].Rise = pNode->tRequired[1].Fall = MNTK_FLOAT_LARGE;
    // go through the fanouts of this gate
    for ( i = 0; i < pNode->vFanouts->nSize; i++ )
    {
        pFanout = pNode->vFanouts->pArray[i];
        assert( pFanout->tArrival.Rise < pFanout->tRequired[1].Rise + pNode->p->fEpsilon );
        assert( pNode->tArrival.Fall < pFanout->tRequired[1].Fall + pNode->p->fEpsilon );
        assert( pFanout->tArrival.Rise < pFanout->tRequired[1].Rise + pNode->p->fEpsilon );
        assert( pNode->tArrival.Fall < pFanout->tRequired[1].Fall + pNode->p->fEpsilon );
        // find the corresponding pin of the fanout gate
        pPin = Mio_GateReadPins(pFanout->pGate);
        for ( iPin = 0; iPin < pFanout->nLeaves; iPin++, pPin = Mio_PinReadNext(pPin) )
            if ( Mntk_Regular(pFanout->ppLeaves[iPin]) == pNode )
                break;
        assert( iPin != pFanout->nLeaves ); // node is not found in the fanout's fanin list
        // get the phase of the node in the fanout's fanin list
        fPhase = !Mntk_IsComplement(pFanout->ppLeaves[iPin]);

        // get the interesting parameters of this pin
        PinPhase = Mio_PinReadPhase(pPin);
        tDelayBlockRise = (float)Mio_PinReadDelayBlockRise( pPin );  
        tDelayBlockFall = (float)Mio_PinReadDelayBlockFall( pPin );  
        // compute the required times of the pin input
        pReqIn->Rise = pReqIn->Fall = MNTK_FLOAT_LARGE;
        if ( PinPhase != MIO_PHASE_INV )  // NONINV phase is present
        {
//            if ( pNode->tArrival.Rise < pTimeIn->Rise + tDelayBlockRise )
//                pNode->tArrival.Rise = pTimeIn->Rise + tDelayBlockRise;
            if ( pReqIn->Rise > pFanout->tRequired[1].Rise - tDelayBlockRise )
                pReqIn->Rise = pFanout->tRequired[1].Rise - tDelayBlockRise;
//            if ( pNode->tArrival.Fall < pTimeIn->Fall + tDelayBlockFall )
//                pNode->tArrival.Fall = pTimeIn->Fall + tDelayBlockFall;
            if ( pReqIn->Fall > pFanout->tRequired[1].Fall - tDelayBlockFall )
                pReqIn->Fall = pFanout->tRequired[1].Fall - tDelayBlockFall;
        }
        if ( PinPhase != MIO_PHASE_NONINV )  // INV phase is present
        {
//            if ( pNode->tArrival.Rise < pTimeIn->Fall + tDelayBlockRise )
//                pNode->tArrival.Rise = pTimeIn->Fall + tDelayBlockRise;
            if ( pReqIn->Fall > pFanout->tRequired[1].Rise - tDelayBlockRise )
                pReqIn->Fall = pFanout->tRequired[1].Rise - tDelayBlockRise;
//            if ( pNode->tArrival.Fall < pTimeIn->Rise + tDelayBlockFall )
//                pNode->tArrival.Fall = pTimeIn->Rise + tDelayBlockFall;
            if ( pReqIn->Rise > pFanout->tRequired[1].Fall - tDelayBlockFall )
                pReqIn->Rise = pFanout->tRequired[1].Fall - tDelayBlockFall;
        }

        // compute the required times of the node
        if ( fPhase )
        {
//            pTimeIn->Rise = pChild->tArrival.Fall + pNode->p->tDelayInv.Rise;
            if ( pNode->tRequired[0].Rise > pReqIn->Fall - pNode->p->tDelayInv.Fall )
                pNode->tRequired[0].Rise = pReqIn->Fall - pNode->p->tDelayInv.Fall;
//            pTimeIn->Fall = pChild->tArrival.Rise + pNode->p->tDelayInv.Fall;
            if ( pNode->tRequired[0].Fall > pReqIn->Rise - pNode->p->tDelayInv.Rise )
                pNode->tRequired[0].Fall = pReqIn->Rise - pNode->p->tDelayInv.Rise;

//            pTimeIn->Rise = pChild->tArrival.Rise;
            if ( pNode->tRequired[1].Rise > pReqIn->Rise )
                pNode->tRequired[1].Rise = pReqIn->Rise;
//            pTimeIn->Fall = pChild->tArrival.Fall;
            if ( pNode->tRequired[1].Fall > pReqIn->Fall )
                pNode->tRequired[1].Fall = pReqIn->Fall;
        }
        else
        {
//            pTimeIn->Rise = pChild->tArrival.Rise;
            if ( pNode->tRequired[0].Rise > pReqIn->Rise )
                pNode->tRequired[0].Rise = pReqIn->Rise;
//            pTimeIn->Fall = pChild->tArrival.Fall;
            if ( pNode->tRequired[0].Fall > pReqIn->Fall )
                pNode->tRequired[0].Fall = pReqIn->Fall;

//            pTimeIn->Rise = pChild->tArrival.Fall + pNode->p->tDelayInv.Rise;
            if ( pNode->tRequired[1].Rise > pReqIn->Fall - pNode->p->tDelayInv.Fall )
                pNode->tRequired[1].Rise = pReqIn->Fall - pNode->p->tDelayInv.Fall;
//            pTimeIn->Fall = pChild->tArrival.Rise + pNode->p->tDelayInv.Fall;
            if ( pNode->tRequired[1].Fall > pReqIn->Rise - pNode->p->tDelayInv.Rise )
                pNode->tRequired[1].Fall = pReqIn->Rise - pNode->p->tDelayInv.Rise;
        }
    }
    pNode->tRequired[0].Worst = MNTK_MIN( pNode->tRequired[0].Rise, pNode->tRequired[0].Fall );
    pNode->tRequired[1].Worst = MNTK_MIN( pNode->tRequired[1].Rise, pNode->tRequired[1].Fall );
    assert( pNode->tArrival.Fall < pNode->tRequired[1].Fall + pNode->p->fEpsilon );
    assert( pNode->tArrival.Rise < pNode->tRequired[1].Rise + pNode->p->fEpsilon );
}

/**function*************************************************************

  Synopsis    [Propagates required times for one node.]

  Description [Propagates the required times for the fanins.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mntk_NodeComputeRequiredFanins( Mntk_Node_t * pNode )
{
    Map_Time_t ReqIn, * pReqIn = &ReqIn, * pReqInReal0, * pReqInReal1;
    float tDelayBlockRise, tDelayBlockFall;
    Mio_PinPhase_t PinPhase;
    Mio_Pin_t * pPin;
    Mntk_Node_t * pChild;
    int i, fPhaseC;

    // save the previous time
//    pNode->tPrevious = pNode->tRequired;
    // makes no sense because the node's required time does not change

    // go through the pins of this gate
    if ( !Mntk_NodeIsVar(pNode) )
    for ( i = 0, pPin = Mio_GateReadPins(pNode->pGate); 
          i < (int)pNode->nLeaves; 
          i++,   pPin = Mio_PinReadNext(pPin) )
    {
        pChild  =  Mntk_Regular(pNode->ppLeaves[i]);
        fPhaseC = !Mntk_IsComplement(pNode->ppLeaves[i]);

        // get the interesting parameters of this pin
        PinPhase = Mio_PinReadPhase(pPin);
        tDelayBlockRise = (float)Mio_PinReadDelayBlockRise( pPin );  
        tDelayBlockFall = (float)Mio_PinReadDelayBlockFall( pPin );  
        // compute the arrival times of the positive phase
        pReqIn->Rise = pReqIn->Fall = MNTK_FLOAT_LARGE;
        if ( PinPhase != MIO_PHASE_INV )  // NONINV phase is present
        {
//            if ( pNode->tArrival.Rise < pTimeIn->Rise + tDelayBlockRise )
//                pNode->tArrival.Rise = pTimeIn->Rise + tDelayBlockRise;
            if ( pReqIn->Rise > pNode->tRequired[1].Rise - tDelayBlockRise )
                pReqIn->Rise = pNode->tRequired[1].Rise - tDelayBlockRise;
//            if ( pNode->tArrival.Fall < pTimeIn->Fall + tDelayBlockFall )
//                pNode->tArrival.Fall = pTimeIn->Fall + tDelayBlockFall;
            if ( pReqIn->Fall > pNode->tRequired[1].Fall - tDelayBlockFall )
                pReqIn->Fall = pNode->tRequired[1].Fall - tDelayBlockFall;
        }
        if ( PinPhase != MIO_PHASE_NONINV )  // INV phase is present
        {
//            if ( pNode->tArrival.Rise < pTimeIn->Fall + tDelayBlockRise )
//                pNode->tArrival.Rise = pTimeIn->Fall + tDelayBlockRise;
            if ( pReqIn->Fall > pNode->tRequired[1].Rise - tDelayBlockRise )
                pReqIn->Fall = pNode->tRequired[1].Rise - tDelayBlockRise;
//            if ( pNode->tArrival.Fall < pTimeIn->Rise + tDelayBlockFall )
//                pNode->tArrival.Fall = pTimeIn->Rise + tDelayBlockFall;
            if ( pReqIn->Rise > pNode->tRequired[1].Fall - tDelayBlockFall )
                pReqIn->Rise = pNode->tRequired[1].Fall - tDelayBlockFall;
        }

        // convert to the real required times
        pReqInReal0 = &pChild->tRequired[0];
        pReqInReal1 = &pChild->tRequired[1];
        if ( fPhaseC )
        {
//            pTimeIn->Rise = pChild->tArrival.Fall + pNode->p->tDelayInv.Rise;
            if ( pReqInReal0->Rise > pReqIn->Fall - pNode->p->tDelayInv.Fall )
                pReqInReal0->Rise = pReqIn->Fall - pNode->p->tDelayInv.Fall;
//            pTimeIn->Fall = pChild->tArrival.Rise + pNode->p->tDelayInv.Fall;
            if ( pReqInReal0->Fall > pReqIn->Rise - pNode->p->tDelayInv.Rise )
                pReqInReal0->Fall = pReqIn->Rise - pNode->p->tDelayInv.Rise;

//            pTimeIn->Rise = pChild->tArrival.Rise;
            if ( pReqInReal1->Rise > pReqIn->Rise )
                pReqInReal1->Rise = pReqIn->Rise;
//            pTimeIn->Fall = pChild->tArrival.Fall;
            if ( pReqInReal1->Fall > pReqIn->Fall )
                pReqInReal1->Fall = pReqIn->Fall;
        }
        else
        {
//            pTimeIn->Rise = pChild->tArrival.Rise;
            if ( pReqInReal0->Rise > pReqIn->Rise )
                pReqInReal0->Rise = pReqIn->Rise;
//            pTimeIn->Fall = pChild->tArrival.Fall;
            if ( pReqInReal0->Fall > pReqIn->Fall )
                pReqInReal0->Fall = pReqIn->Fall;

//            pTimeIn->Rise = pChild->tArrival.Fall + pNode->p->tDelayInv.Rise;
            if ( pReqInReal1->Rise > pReqIn->Fall - pNode->p->tDelayInv.Fall )
                pReqInReal1->Rise = pReqIn->Fall - pNode->p->tDelayInv.Fall;
//            pTimeIn->Fall = pChild->tArrival.Rise + pNode->p->tDelayInv.Fall;
            if ( pReqInReal1->Fall > pReqIn->Rise - pNode->p->tDelayInv.Rise )
                pReqInReal1->Fall = pReqIn->Rise - pNode->p->tDelayInv.Rise;
        }
    }
    pNode->tRequired[0].Worst = MNTK_MIN( pNode->tRequired[0].Rise, pNode->tRequired[0].Fall );
    pNode->tRequired[1].Worst = MNTK_MIN( pNode->tRequired[1].Rise, pNode->tRequired[1].Fall );
    assert( pNode->tArrival.Worst < pNode->tRequired[1].Worst + pNode->p->fEpsilon );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


