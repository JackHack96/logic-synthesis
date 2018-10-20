/**CFile****************************************************************

  FileName    [fpgaCutUtils.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Generic technology mapping engine.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 8, 2003.]

  Revision    [$Id: fpgaCutUtils.c,v 1.3 2005/07/08 01:01:23 alanmi Exp $]

***********************************************************************/

#include "fpgaInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Allocates the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fpga_Cut_t * Fpga_CutAlloc( Fpga_Man_t * p )
{
    Fpga_Cut_t * pCut;
    pCut = (Fpga_Cut_t *)Extra_MmFixedEntryFetch( p->mmCuts );
    memset( pCut, 0, sizeof(Fpga_Cut_t) );
    return pCut;
}

/**Function*************************************************************

  Synopsis    [Duplicates the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fpga_Cut_t * Fpga_CutDup( Fpga_Man_t * p, Fpga_Cut_t * pCutOld )
{
    Fpga_Cut_t * pCutNew;
    int i;
    pCutNew = Fpga_CutAlloc( p );
    pCutNew->pRoot   = pCutOld->pRoot;
    pCutNew->nLeaves = pCutOld->nLeaves;
    for ( i = 0; i < pCutOld->nLeaves; i++ )
        pCutNew->ppLeaves[i] = pCutOld->ppLeaves[i];
    return pCutNew;
}

/**Function*************************************************************

  Synopsis    [Deallocates the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_CutFree( Fpga_Man_t * p, Fpga_Cut_t * pCut )
{
    if ( pCut )
    Extra_MmFixedEntryRecycle( p->mmCuts, (char *)pCut );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_CutPrint( Fpga_Man_t * p, Fpga_Node_t * pRoot, Fpga_Cut_t * pCut )
{
    int i;
    printf( "CUT:  Delay = %4.2f. Area = %4.2f. Nodes = %d -> {", 
        pCut->tArrival, pCut->aFlow, pRoot->Num );
    for ( i = 0; i < pCut->nLeaves; i++ )
        printf( " %d", pCut->ppLeaves[i]->Num );
    printf( " } \n" );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fpga_Cut_t * Fpga_CutCreateSimple( Fpga_Man_t * p, Fpga_Node_t * pNode )
{
    Fpga_Cut_t * pCut;
    pCut = Fpga_CutAlloc( p );
    pCut->pRoot       = pNode;
    pCut->nLeaves     = 1;
    pCut->ppLeaves[0] = pNode;
    pCut->uSign = FPGA_SEQ_SIGN(pCut->ppLeaves[0]);
    return pCut;
}


/**function*************************************************************

  synopsis    [Computes the exact area associated with the cut.]

  description []
               
  sideeffects []

  seealso     []

***********************************************************************/
float Fpga_CutGetRootArea( Fpga_Man_t * p, Fpga_Cut_t * pCut )
{
    return p->pLutLib->pLutAreas[pCut->nLeaves];
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fpga_Cut_t * Fpga_CutListAppend( Fpga_Cut_t * pSetAll, Fpga_Cut_t * pSets )
{
    Fpga_Cut_t * pPrev, * pTemp;
    if ( pSetAll == NULL )
        return pSets;
    if ( pSets == NULL )
        return pSetAll;
    // find the last one
    for ( pTemp = pSets; pTemp; pTemp = pTemp->pNext )
        pPrev = pTemp;
    // append all the end of the current set
    assert( pPrev->pNext == NULL );
    pPrev->pNext = pSetAll;
    return pSets;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_CutListRecycle( Fpga_Man_t * p, Fpga_Cut_t * pSetList, Fpga_Cut_t * pSave )
{
    Fpga_Cut_t * pNext, * pTemp;
    for ( pTemp = pSetList, pNext = pTemp? pTemp->pNext : NULL; 
          pTemp; 
          pTemp = pNext, pNext = pNext? pNext->pNext : NULL )
        if ( pTemp != pSave )
            Extra_MmFixedEntryRecycle( p->mmCuts, (char *)pTemp );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fpga_CutListCount( Fpga_Cut_t * pSets )
{
    Fpga_Cut_t * pTemp;
    int i;
    for ( i = 0, pTemp = pSets; pTemp; pTemp = pTemp->pNext, i++ );
    return i;
}

#if 0

/**function*************************************************************

  synopsis    [Removes the fanouts of the cut.]

  description []
               
  sideeffects []

  seealso     []

***********************************************************************/
void Fpga_CutRemoveFanouts( Fpga_Man_t * p, Fpga_Node_t * pNode, Fpga_Cut_t * pCut )
{
    Fpga_NodeVec_t * vFanouts;
    int i, k;
    for ( i = 0; i < pCut->nLeaves; i++ )
    {
        vFanouts = pCut->ppLeaves[i]->vFanouts;
        for ( k = 0; k < vFanouts->nSize; k++ )
            if ( vFanouts->pArray[k] == pNode )
                break;
        assert( k != vFanouts->nSize );
        for ( k++; k < vFanouts->nSize; k++ )
            vFanouts->pArray[k-1] = vFanouts->pArray[k];
        vFanouts->nSize--;
    }
}

/**function*************************************************************

  synopsis    [Removes the fanouts of the cut.]

  description []
               
  sideeffects []

  seealso     []

***********************************************************************/
void Fpga_CutInsertFanouts( Fpga_Man_t * p, Fpga_Node_t * pNode, Fpga_Cut_t * pCut )
{
    int i;
    for ( i = 0; i < pCut->nLeaves; i++ )
        Fpga_NodeVecPush( pCut->ppLeaves[i]->vFanouts, pNode );
}
#endif

/**Function*************************************************************

  Synopsis    [Computes the arrival time and the area flow of the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_CutGetParameters( Fpga_Man_t * pMan, Fpga_Cut_t * pCut )
{
    Fpga_Cut_t * pFaninCut;
    int i;
    pCut->tArrival = -FPGA_FLOAT_LARGE;
    pCut->aFlow    = pMan->pLutLib->pLutAreas[pCut->nLeaves];
    for ( i = 0; i < pCut->nLeaves; i++ )
    {
        pFaninCut = pCut->ppLeaves[i]->pCutBest;
        if ( pCut->tArrival < pFaninCut->tArrival )
             pCut->tArrival = pFaninCut->tArrival;
        // if the fanout count is not set, assume it to be 1
        if ( pCut->ppLeaves[i]->nRefs == 0 )
            pCut->aFlow += pFaninCut->aFlow;
        else
//            pCut->aFlow += pFaninCut->aFlow / pCut->ppLeaves[i]->nRefs;
            pCut->aFlow += pFaninCut->aFlow / pCut->ppLeaves[i]->aEstFanouts;
    }
    pCut->tArrival += pMan->pLutLib->pLutDelays[pCut->nLeaves];
}


/**function*************************************************************

  synopsis    [Computes the area flow of the cut.]

  description []
               
  sideeffects []

  seealso     []

***********************************************************************/
float Fpga_CutGetAreaFlow( Fpga_Man_t * pMan, Fpga_Cut_t * pCut )
{
    Fpga_Cut_t * pCutFanin;
    int i;
    pCut->aFlow = pMan->pLutLib->pLutAreas[pCut->nLeaves];
    for ( i = 0; i < pCut->nLeaves; i++ )
    {
        // get the cut implementing this phase of the fanin
        pCutFanin  = pCut->ppLeaves[i]->pCutBest;   
        assert( pCutFanin );
        pCut->aFlow  += pCutFanin->aFlow / pCut->ppLeaves[i]->nRefs;
    }
    return pCut->aFlow;
}

/**function*************************************************************

  synopsis    [Computes the exact area associated with the cut.]

  description []
               
  sideeffects []

  seealso     []

***********************************************************************/
float Fpga_CutGetAreaRefed( Fpga_Man_t * pMan, Fpga_Cut_t * pCut )
{
    float aResult, aResult2;
    if ( pCut->nLeaves == 1 )
        return 0;
    aResult  = Fpga_CutDeref( pMan, NULL, pCut, 0 );
    aResult2 = Fpga_CutRef( pMan, NULL, pCut, 0 );
    assert( aResult == aResult2 );
    return aResult;
}

/**function*************************************************************

  synopsis    [Computes the exact area associated with the cut.]

  description []
               
  sideeffects []

  seealso     []

***********************************************************************/
float Fpga_CutGetAreaDerefed( Fpga_Man_t * pMan, Fpga_Cut_t * pCut )
{
    float aResult, aResult2;
    if ( pCut->nLeaves == 1 )
        return 0;
    aResult2 = Fpga_CutRef( pMan, NULL, pCut, 0 );
    aResult  = Fpga_CutDeref( pMan, NULL, pCut, 0 );
    assert( aResult == aResult2 );
    return aResult;
}

/**function*************************************************************

  synopsis    [References the cut.]

  description [This procedure is similar to the procedure NodeReclaim.]
               
  sideeffects []

  seealso     []

***********************************************************************/
float Fpga_CutRef( Fpga_Man_t * pMan, Fpga_Node_t * pNode, Fpga_Cut_t * pCut, int fFanouts )
{
    Fpga_Node_t * pNodeChild;
    float aArea;
    int i;

    // deref the fanouts
//    if ( fFanouts ) 
//        Fpga_CutInsertFanouts( pMan, pNode, pCut );

    // start the area of this cut
    aArea = pMan->pLutLib->pLutAreas[pCut->nLeaves];
    // go through the children
    for ( i = 0; i < pCut->nLeaves; i++ )
    {
        pNodeChild = pCut->ppLeaves[i];
        assert( pNodeChild->nRefs >= 0 );
        if ( pNodeChild->nRefs++ > 0 )  
            continue;
        if ( !Fpga_NodeIsAnd(pNodeChild) ) 
            continue;
        aArea += Fpga_CutRef( pMan, pNodeChild, pNodeChild->pCutBest, fFanouts );
    }
    return aArea;
}

/**function*************************************************************

  synopsis    [Dereferences the cut.]

  description [This procedure is similar to the procedure NodeRecusiveDeref.]
               
  sideeffects []

  seealso     []

***********************************************************************/
float Fpga_CutDeref( Fpga_Man_t * pMan, Fpga_Node_t * pNode, Fpga_Cut_t * pCut, int fFanouts )
{
    Fpga_Node_t * pNodeChild;
    float aArea;
    int i;

    // deref the fanouts
//    if ( fFanouts ) 
//        Fpga_CutRemoveFanouts( pMan, pNode, pCut );

    // start the area of this cut
    aArea = pMan->pLutLib->pLutAreas[pCut->nLeaves];
    // go through the children
    for ( i = 0; i < pCut->nLeaves; i++ )
    {
        pNodeChild = pCut->ppLeaves[i];
        assert( pNodeChild->nRefs > 0 );
        if ( --pNodeChild->nRefs > 0 )  
            continue;
        if ( !Fpga_NodeIsAnd(pNodeChild) ) 
            continue;
        aArea += Fpga_CutDeref( pMan, pNodeChild, pNodeChild->pCutBest, fFanouts );
    }
    return aArea;
}



/**function*************************************************************

  synopsis    [Computes the exact area associated with the cut.]

  description []
               
  sideeffects []

  seealso     []

***********************************************************************/
float Fpga_CutGetSwitchDerefed( Fpga_Man_t * pMan, Fpga_Node_t * pNode, Fpga_Cut_t * pCut )
{
    float aResult, aResult2;
    if ( pCut->nLeaves == 1 )
        return 0;
    aResult2 = Fpga_CutRefSwitch( pMan, pNode, pCut, 0 );
    aResult  = Fpga_CutDerefSwitch( pMan, pNode, pCut, 0 );
//    assert( aResult == aResult2 );
    return aResult;
}

/**function*************************************************************

  synopsis    [References the cut.]

  description [This procedure is similar to the procedure NodeReclaim.]
               
  sideeffects []

  seealso     []

***********************************************************************/
float Fpga_CutRefSwitch( Fpga_Man_t * pMan, Fpga_Node_t * pNode, Fpga_Cut_t * pCut, int fFanouts )
{
    Fpga_Node_t * pNodeChild;
    float aArea;
    int i;

    // deref the fanouts
//    if ( fFanouts ) 
//        Fpga_CutInsertFanouts( pMan, pNode, pCut );

    // start the area of this cut
    aArea = pNode->SwitchProb;
    // go through the children
    for ( i = 0; i < pCut->nLeaves; i++ )
    {
        pNodeChild = pCut->ppLeaves[i];
        assert( pNodeChild->nRefs >= 0 );
        if ( pNodeChild->nRefs++ > 0 )  
            continue;
        if ( !Fpga_NodeIsAnd(pNodeChild) ) 
        {
            aArea += pNodeChild->SwitchProb;
            continue;
        }
        aArea += Fpga_CutRefSwitch( pMan, pNodeChild, pNodeChild->pCutBest, fFanouts );
    }
    return aArea;
}

/**function*************************************************************

  synopsis    [Dereferences the cut.]

  description [This procedure is similar to the procedure NodeRecusiveDeref.]
               
  sideeffects []

  seealso     []

***********************************************************************/
float Fpga_CutDerefSwitch( Fpga_Man_t * pMan, Fpga_Node_t * pNode, Fpga_Cut_t * pCut, int fFanouts )
{
    Fpga_Node_t * pNodeChild;
    float aArea;
    int i;

    // deref the fanouts
//    if ( fFanouts ) 
//        Fpga_CutRemoveFanouts( pMan, pNode, pCut );

    // start the area of this cut
    aArea = pNode->SwitchProb;
    // go through the children
    for ( i = 0; i < pCut->nLeaves; i++ )
    {
        pNodeChild = pCut->ppLeaves[i];
        assert( pNodeChild->nRefs > 0 );
        if ( --pNodeChild->nRefs > 0 )  
            continue;
        if ( !Fpga_NodeIsAnd(pNodeChild) ) 
        {
            aArea += pNodeChild->SwitchProb;
            continue;
        }
        aArea += Fpga_CutDerefSwitch( pMan, pNodeChild, pNodeChild->pCutBest, fFanouts );
    }
    return aArea;
}



/**Function*************************************************************

  Synopsis    [Performs technology mapping for variable-size-LUTs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_CutCreateFromNode( Fpga_Man_t * p, int iRoot, int * pLeaves, int nLeaves )
{
    Fpga_Node_t * pRoot;
    Fpga_Cut_t * pCutUsed;
    int i;

    assert( iRoot < p->vNodesAll->nSize );
    // skip creating the cut for the constant node
    if ( iRoot == -1 )
        return;
    // skip the node whose cut is already added
    pRoot = p->vNodesAll->pArray[iRoot];
//    assert( pRoot->pCutOld == NULL );
    if ( pRoot->pCutOld )
        return;
    // create a new cut
    pCutUsed = Fpga_CutAlloc( p );
    pCutUsed->nLeaves = nLeaves;
    for ( i = 0; i < nLeaves; i++ )
        pCutUsed->ppLeaves[i] = p->vNodesAll->pArray[pLeaves[i]];
    // derive the truth table of this cut topologically
    Fpga_TruthsCutTopo( p, pRoot, pCutUsed, pCutUsed->uTruthTemp );
    pRoot->pCutOld = pCutUsed;
}

/**Function*************************************************************

  Synopsis    [Sets the used cuts to be the currently selected ones.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_MappingSetUsedCuts( Fpga_Man_t * pMan )
{
    int i;
    for ( i = 0; i < pMan->vNodesAll->nSize; i++ )
        if ( pMan->vNodesAll->pArray[i]->pCutOld )
        {
            pMan->vNodesAll->pArray[i]->pCutBest = pMan->vNodesAll->pArray[i]->pCutOld;
            pMan->vNodesAll->pArray[i]->pCutOld  = NULL;
        }
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


