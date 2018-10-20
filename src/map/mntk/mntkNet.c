/**CFile****************************************************************

  FileName    [mntkNet.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Generic mapped network.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 8, 2003.]

  Revision    [$Id: mntkNet.c,v 1.1 2005/02/28 05:34:31 alanmi Exp $]

***********************************************************************/

#include "mntkInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static Mntk_Node_t * Mntk_ConstructSuper_rec( Mntk_Man_t * p, Map_Super_t * pSuper, Mntk_Node_t * ppInputs[], int nInputs );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Deallocates the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mntk_Node_t * Mntk_NodeAlloc( Mntk_Man_t * p, Mntk_Node_t ** ppLeaves, int nLeaves, Mio_Gate_t * pGate )
{
    Mntk_Node_t * pNode;
    int i;
    // set the memory for the new node
    pNode = (Mntk_Node_t *)Extra_MmFixedEntryFetch( p->mmNodes );
    memset( pNode, 0, sizeof(Mntk_Node_t) );
    // add the node to the list
    pNode->p = p;
    pNode->nLeaves = nLeaves;
    if ( nLeaves > MNTK_MAX_FANINS )
        pNode->ppLeaves = ALLOC( Mntk_Node_t *, nLeaves );
    else
        pNode->ppLeaves = pNode->ppArray;
    for ( i = 0; i < nLeaves; i++ )
        pNode->ppLeaves[i] = ppLeaves[i];
    pNode->pGate = pGate;
    return pNode;
}

/**Function*************************************************************

  Synopsis    [Creates the new node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mntk_NodeRegister( Mntk_Node_t * pNode )
{
    pNode->Num = pNode->p->nNodes++;
    if ( pNode->Num >= 0 )
        Mntk_NodeVecPush( pNode->p->vNodesAll, pNode );
    // create room for fanouts
    pNode->vFanouts = Mntk_NodeVecAlloc( 5 );
    // set the gate and simm info
    pNode->pSims = (unsigned *)Extra_MmFixedEntryFetch( pNode->p->mmSims );
//    assert( pNode->Num < pNode->p->nInputs || pNode->nLeaves > 1 );
}

/**Function*************************************************************

  Synopsis    [Deallocates the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mntk_NodeFree( Mntk_Node_t * pNode )
{
    if ( pNode == NULL )
        return;
    if ( pNode->vFanouts )
        Mntk_NodeVecFree( pNode->vFanouts );
    if ( pNode->ppLeaves != pNode->ppArray )
        free( pNode->ppLeaves );
    Extra_MmFixedEntryRecycle( pNode->p->mmNodes, (char *)pNode );
}


/**function*************************************************************

  synopsis    [Recursively adds the node to the fanout lists of the fanins.]

  description []
               
  sideeffects []

  seealso     []

***********************************************************************/
float Mntk_NodeInsertFaninFanouts( Mntk_Node_t * pNode, int fRecursive )
{
    float Area;
    int i;
    Area = pNode->pGate? (float)Mio_GateReadArea( pNode->pGate ) : 0;
    if ( pNode->nRefs[0] > 0 )
        Area += pNode->p->AreaInv;
    for ( i = 0; i < (int)pNode->nLeaves; i++ )
    {
        // make sure the leaf has a legal number of references
        assert( Mntk_NodeReadRef(pNode->ppLeaves[i]) >= 0 );
        // call recursively for the fanin if it is not referenced
        if ( fRecursive && Mntk_NodeReadRefTotal(pNode->ppLeaves[i]) == 0 )
            Area += Mntk_NodeInsertFaninFanouts( Mntk_Regular(pNode->ppLeaves[i]), fRecursive );
        // if inverter should be added, add the area of interver
        if ( Mntk_IsComplement(pNode->ppLeaves[i]) && Mntk_NodeReadRef(pNode->ppLeaves[i]) == 0 )
            Area += pNode->p->AreaInv;
        // reference the leaf and add the fanout
        Mntk_NodeRef( pNode->ppLeaves[i] );
        Mntk_NodeVecPush( Mntk_Regular(pNode->ppLeaves[i])->vFanouts, pNode );
    }
    return Area;
}

/**function*************************************************************

  synopsis    [Recursively dereferences the node in the fanins' lists.]

  description []
               
  sideeffects []

  seealso     []

***********************************************************************/
float Mntk_NodeRemoveFaninFanouts( Mntk_Node_t * pNode, int fRecursive )
{
//    Mntk_NodeVec_t * vFanouts;
//    int k;
    float Area;
    int i;
    Area = pNode->pGate? (float)Mio_GateReadArea( pNode->pGate ) : 0;
    if ( pNode->nRefs[0] > 0 )
        Area += pNode->p->AreaInv;
    for ( i = 0; i < (int)pNode->nLeaves; i++ )
    {
        // dereference the leaf and remove the fanout
        Mntk_NodeDeref( pNode->ppLeaves[i] );
/*
        vFanouts = Mntk_Regular(pNode->ppLeaves[i])->vFanouts;
        for ( k = 0; k < vFanouts->nSize; k++ )
            if ( vFanouts->pArray[k] == pNode )
                break;
        assert( k != vFanouts->nSize );
        for ( k++; k < vFanouts->nSize; k++ )
            vFanouts->pArray[k-1] = vFanouts->pArray[k];
        vFanouts->nSize--;
*/
        // make sure the leaf has a legal number of references
        assert( Mntk_NodeReadRef(pNode->ppLeaves[i]) >= 0 );
        // if inverter should be added, count the area of interver
        if ( Mntk_IsComplement(pNode->ppLeaves[i]) && Mntk_NodeReadRef(pNode->ppLeaves[i]) == 0 )
            Area += pNode->p->AreaInv;
        // call recursively for the fanin if it is not referenced
        if ( fRecursive && Mntk_NodeReadRefTotal(pNode->ppLeaves[i]) == 0 )
            Area += Mntk_NodeRemoveFaninFanouts( Mntk_Regular(pNode->ppLeaves[i]), fRecursive );
    }
    return Area;
}

/**function*************************************************************

  synopsis    [Recursively adds the node to the fanout lists of the fanins.]

  description []
               
  sideeffects []

  seealso     []

***********************************************************************/
void Mntk_NodeInsertFaninFanoutsOnly( Mntk_Node_t * pNode )
{
    int i;
    for ( i = 0; i < (int)pNode->nLeaves; i++ )
        Mntk_NodeVecPush( Mntk_Regular(pNode->ppLeaves[i])->vFanouts, pNode );
}

/**function*************************************************************

  synopsis    [Recursively removes the node to the fanout lists of the fanins.]

  description []
               
  sideeffects []

  seealso     []

***********************************************************************/
void Mntk_NodeRemoveFaninFanoutsOnly( Mntk_Node_t * pNode )
{
    Mntk_NodeVec_t * vFanouts;
    int i, k;
    for ( i = 0; i < (int)pNode->nLeaves; i++ )
    {
        vFanouts = Mntk_Regular(pNode->ppLeaves[i])->vFanouts;
        for ( k = 0; k < vFanouts->nSize; k++ )
            if ( vFanouts->pArray[k] == pNode )
                break;
        assert( k != vFanouts->nSize );
        for ( k++; k < vFanouts->nSize; k++ )
            vFanouts->pArray[k-1] = vFanouts->pArray[k];
        vFanouts->nSize--;
    }
}

/**Function*************************************************************

  Synopsis    [Delete the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mntk_NodeTransferFanouts( Mntk_Node_t * pNodeOld, Mntk_Node_t * pNodeNew, int fCompl )
{
    Mntk_NodeVec_t * vFanouts;
    Mntk_Node_t * pFanout;
    int i, k, fFound;
    // transfer the references
    assert( Mntk_NodeReadRefTotal(pNodeOld) >  0 );
    assert( Mntk_NodeReadRefTotal(pNodeNew) == 0 );
    pNodeNew->nRefs[0] = pNodeOld->nRefs[fCompl ^ 0];
    pNodeNew->nRefs[1] = pNodeOld->nRefs[fCompl ^ 1];
    pNodeOld->nRefs[0] = 0;
    pNodeOld->nRefs[1] = 0;

    // transfer the fanouts
    vFanouts = pNodeOld->vFanouts;
    pNodeOld->vFanouts = pNodeNew->vFanouts;
    pNodeNew->vFanouts = vFanouts;

    // set the required times of the new node
    pNodeNew->tRequired[0] = pNodeOld->tRequired[fCompl ^ 0];
    pNodeNew->tRequired[1] = pNodeOld->tRequired[fCompl ^ 1];
    // set the required times of the old node
    pNodeOld->tRequired[0].Rise  = pNodeOld->tRequired[1].Rise  = MNTK_FLOAT_LARGE;
    pNodeOld->tRequired[0].Fall  = pNodeOld->tRequired[1].Fall  = MNTK_FLOAT_LARGE;
    pNodeOld->tRequired[0].Worst = pNodeOld->tRequired[1].Worst = MNTK_FLOAT_LARGE;
    // no need to update the arrival time because it is already set

    // update the fanin lists of the fanouts
    fFound = 0;
    for ( i = 0; i < vFanouts->nSize; i++ )
    {
        pFanout = vFanouts->pArray[i];
        for ( k = 0; k < (int)pFanout->nLeaves; k++ )
            if ( Mntk_Regular(pFanout->ppLeaves[k]) == pNodeOld )
            {
                pFanout->ppLeaves[k] = Mntk_NotCond( pNodeNew, fCompl ^ Mntk_IsComplement(pFanout->ppLeaves[k]) );
//                break;
                fFound = 1;
            }
/*
        if ( k == (int)pFanout->nLeaves )`
        {
            assert( 0 ); // old fanin is not in the fanout's fanin list
        }
*/
        assert( fFound );
    }
}

/**Function*************************************************************

  Synopsis    [Constructs the nodes corrresponding to one supergate.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mntk_Node_t * Mntk_ConstructSuper( Mntk_Cut_t * pCut )
{
    int i;
    // set the correct phase
    for ( i = 0; i < (int)pCut->nLeaves; i++ )
        pCut->ppLeaves[i] = Mntk_NotCond( pCut->ppLeaves[i], ((pCut->uPhase & (1<<i)) > 0) );
    return Mntk_ConstructSuper_rec( pCut->p, pCut->pSuperBest, pCut->ppLeaves, pCut->nLeaves );
}

/**Function*************************************************************

  Synopsis    [Constructs the nodes corrresponding to one supergate.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mntk_Node_t * Mntk_ConstructSuper_rec( Mntk_Man_t * p, Map_Super_t * pSuper, Mntk_Node_t * ppInputs[], int nInputs )
{
    Mio_Gate_t * pGate;
    Map_Super_t ** ppFaninSupers;
    Mntk_Node_t * pNodeNew;
    Mntk_Node_t * ppFaninNodes[6];
    int nFaninSupers, Number, i;

    // get the parameters of the supergate
    pGate  = Map_SuperReadRoot(pSuper);
    if ( pGate == NULL )
    {
        Number = Map_SuperReadNum(pSuper);
        assert( Number >= 0 && Number < nInputs );
        return ppInputs[Number];
    }
    // create or collect the node's fanins
    nFaninSupers  = Map_SuperReadFaninNum( pSuper );
    ppFaninSupers = Map_SuperReadFanins( pSuper );
    // if the gate is equal to the inverter, return complemented node
    if ( nFaninSupers == 1 )
    {
        // make sure supergate library does not contain buffers!!!
//        assert( pGate == Mio_LibraryReadInv( Map_SuperLibReadGenLib(p->pSuperLib) ) );
        pNodeNew = Mntk_ConstructSuper_rec( p, ppFaninSupers[0], ppInputs, nInputs );
        return Mntk_Not(pNodeNew);
    }
    // consider the general case
    for ( i = 0; i < nFaninSupers; i++ )
        ppFaninNodes[i] = Mntk_ConstructSuper_rec( p, ppFaninSupers[i], ppInputs, nInputs );
    // create the new node
    assert( nFaninSupers == Mio_GateReadInputs(pGate) );
    pNodeNew = Mntk_NodeAlloc( p, ppFaninNodes, nFaninSupers, pGate );
    Mntk_NodeRegister( pNodeNew );
    return pNodeNew;
}





////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


