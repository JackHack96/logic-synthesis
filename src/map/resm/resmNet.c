/**CFile****************************************************************

  FileName    [resmNet.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Generic resynthesis engine.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 8, 2003.]

  Revision    [$Id: resmNet.c,v 1.1 2005/01/23 06:59:53 alanmi Exp $]

***********************************************************************/

#include "resmInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static Resm_Node_t * Rems_ConstructSuper_rec( Resm_Man_t * p, Map_Super_t * pSuper, Resm_Node_t * ppInputs[], int nInputs );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Deallocates the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Resm_Node_t * Resm_NodeAlloc( Resm_Man_t * p, Resm_Node_t ** ppLeaves, int nLeaves, Mio_Gate_t * pGate )
{
    Resm_Node_t * pNode;
    int i;
    // set the memory for the new node
    pNode = (Resm_Node_t *)Extra_MmFixedEntryFetch( p->mmNodes );
    memset( pNode, 0, sizeof(Resm_Node_t) );
    // add the node to the list
    pNode->p = p;
    pNode->nLeaves = nLeaves;
    if ( nLeaves > RESM_MAX_FANINS )
        pNode->ppLeaves = ALLOC( Resm_Node_t *, nLeaves );
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
void Resm_NodeRegister( Resm_Node_t * pNode )
{
    pNode->Num = pNode->p->nNodes++;
    if ( pNode->Num >= 0 )
        Resm_NodeVecPush( pNode->p->vNodesAll, pNode );
    // create room for fanouts
    pNode->vFanouts = Resm_NodeVecAlloc( 5 );
    // set the gate and simm info
    pNode->pSims = (unsigned *)Extra_MmFixedEntryFetch( pNode->p->mmSims );
    assert( pNode->Num < pNode->p->nInputs || pNode->nLeaves > 1 );
}

/**Function*************************************************************

  Synopsis    [Deallocates the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Resm_NodeFree( Resm_Node_t * pNode )
{
    if ( pNode == NULL )
        return;
    if ( pNode->vFanouts )
        Resm_NodeVecFree( pNode->vFanouts );
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
float Resm_NodeInsertFaninFanouts( Resm_Node_t * pNode, int fRecursive )
{
    float Area;
    int i;
    Area = pNode->pGate? (float)Mio_GateReadArea( pNode->pGate ) : 0;
    if ( pNode->nRefs[0] > 0 )
        Area += pNode->p->AreaInv;
    for ( i = 0; i < (int)pNode->nLeaves; i++ )
    {
        // make sure the leaf has a legal number of references
        assert( Resm_NodeReadRef(pNode->ppLeaves[i]) >= 0 );
        // call recursively for the fanin if it is not referenced
        if ( fRecursive && Resm_NodeReadRefTotal(pNode->ppLeaves[i]) == 0 )
            Area += Resm_NodeInsertFaninFanouts( Resm_Regular(pNode->ppLeaves[i]), fRecursive );
        // if inverter should be added, add the area of interver
        if ( Resm_IsComplement(pNode->ppLeaves[i]) && Resm_NodeReadRef(pNode->ppLeaves[i]) == 0 )
            Area += pNode->p->AreaInv;
        // reference the leaf and add the fanout
        Resm_NodeRef( pNode->ppLeaves[i] );
        Resm_NodeVecPush( Resm_Regular(pNode->ppLeaves[i])->vFanouts, pNode );
    }
    return Area;
}

/**function*************************************************************

  synopsis    [Recursively dereferences the node in the fanins' lists.]

  description []
               
  sideeffects []

  seealso     []

***********************************************************************/
float Resm_NodeRemoveFaninFanouts( Resm_Node_t * pNode, int fRecursive )
{
//    Resm_NodeVec_t * vFanouts;
//    int k;
    float Area;
    int i;
    Area = pNode->pGate? (float)Mio_GateReadArea( pNode->pGate ) : 0;
    if ( pNode->nRefs[0] > 0 )
        Area += pNode->p->AreaInv;
    for ( i = 0; i < (int)pNode->nLeaves; i++ )
    {
        // dereference the leaf and remove the fanout
        Resm_NodeDeref( pNode->ppLeaves[i] );
/*
        vFanouts = Resm_Regular(pNode->ppLeaves[i])->vFanouts;
        for ( k = 0; k < vFanouts->nSize; k++ )
            if ( vFanouts->pArray[k] == pNode )
                break;
        assert( k != vFanouts->nSize );
        for ( k++; k < vFanouts->nSize; k++ )
            vFanouts->pArray[k-1] = vFanouts->pArray[k];
        vFanouts->nSize--;
*/
        // make sure the leaf has a legal number of references
        assert( Resm_NodeReadRef(pNode->ppLeaves[i]) >= 0 );
        // if inverter should be added, count the area of interver
        if ( Resm_IsComplement(pNode->ppLeaves[i]) && Resm_NodeReadRef(pNode->ppLeaves[i]) == 0 )
            Area += pNode->p->AreaInv;
        // call recursively for the fanin if it is not referenced
        if ( fRecursive && Resm_NodeReadRefTotal(pNode->ppLeaves[i]) == 0 )
            Area += Resm_NodeRemoveFaninFanouts( Resm_Regular(pNode->ppLeaves[i]), fRecursive );
    }
    return Area;
}

/**function*************************************************************

  synopsis    [Recursively adds the node to the fanout lists of the fanins.]

  description []
               
  sideeffects []

  seealso     []

***********************************************************************/
void Resm_NodeInsertFaninFanoutsOnly( Resm_Node_t * pNode )
{
    int i;
    for ( i = 0; i < (int)pNode->nLeaves; i++ )
        Resm_NodeVecPush( Resm_Regular(pNode->ppLeaves[i])->vFanouts, pNode );
}

/**function*************************************************************

  synopsis    [Recursively removes the node to the fanout lists of the fanins.]

  description []
               
  sideeffects []

  seealso     []

***********************************************************************/
void Resm_NodeRemoveFaninFanoutsOnly( Resm_Node_t * pNode )
{
    Resm_NodeVec_t * vFanouts;
    int i, k;
    for ( i = 0; i < (int)pNode->nLeaves; i++ )
    {
        vFanouts = Resm_Regular(pNode->ppLeaves[i])->vFanouts;
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
void Resm_NodeTransferFanouts( Resm_Node_t * pNodeOld, Resm_Node_t * pNodeNew, int fCompl )
{
    Resm_NodeVec_t * vFanouts;
    Resm_Node_t * pFanout;
    int i, k, fFound;
    // transfer the references
    assert( Resm_NodeReadRefTotal(pNodeOld) >  0 );
    assert( Resm_NodeReadRefTotal(pNodeNew) == 0 );
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
    pNodeOld->tRequired[0].Rise  = pNodeOld->tRequired[1].Rise  = RESM_FLOAT_LARGE;
    pNodeOld->tRequired[0].Fall  = pNodeOld->tRequired[1].Fall  = RESM_FLOAT_LARGE;
    pNodeOld->tRequired[0].Worst = pNodeOld->tRequired[1].Worst = RESM_FLOAT_LARGE;
    // no need to update the arrival time because it is already set

    // update the fanin lists of the fanouts
    fFound = 0;
    for ( i = 0; i < vFanouts->nSize; i++ )
    {
        pFanout = vFanouts->pArray[i];
        for ( k = 0; k < (int)pFanout->nLeaves; k++ )
            if ( Resm_Regular(pFanout->ppLeaves[k]) == pNodeOld )
            {
                pFanout->ppLeaves[k] = Resm_NotCond( pNodeNew, fCompl ^ Resm_IsComplement(pFanout->ppLeaves[k]) );
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
Resm_Node_t * Rems_ConstructSuper( Resm_Cut_t * pCut )
{
    int i;
    // set the correct phase
    for ( i = 0; i < (int)pCut->nLeaves; i++ )
        pCut->ppLeaves[i] = Resm_NotCond( pCut->ppLeaves[i], ((pCut->uPhase & (1<<i)) > 0) );
    return Rems_ConstructSuper_rec( pCut->p, pCut->pSuperBest, pCut->ppLeaves, pCut->nLeaves );
}

/**Function*************************************************************

  Synopsis    [Constructs the nodes corrresponding to one supergate.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Resm_Node_t * Rems_ConstructSuper_rec( Resm_Man_t * p, Map_Super_t * pSuper, Resm_Node_t * ppInputs[], int nInputs )
{
    Mio_Gate_t * pGate;
    Map_Super_t ** ppFaninSupers;
    Resm_Node_t * pNodeNew;
    Resm_Node_t * ppFaninNodes[6];
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
        pNodeNew = Rems_ConstructSuper_rec( p, ppFaninSupers[0], ppInputs, nInputs );
        return Resm_Not(pNodeNew);
    }
    // consider the general case
    for ( i = 0; i < nFaninSupers; i++ )
        ppFaninNodes[i] = Rems_ConstructSuper_rec( p, ppFaninSupers[i], ppInputs, nInputs );
    // create the new node
    assert( nFaninSupers == Mio_GateReadInputs(pGate) );
    pNodeNew = Resm_NodeAlloc( p, ppFaninNodes, nFaninSupers, pGate );
    Resm_NodeRegister( pNodeNew );
    return pNodeNew;
}





////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


