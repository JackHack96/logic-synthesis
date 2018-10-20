/**CFile****************************************************************

  FileName    [fraigImp.c]

  PackageName [FRAIG: Functionally reduced AND-INV graphs.]

  Synopsis    [Considers implications.]

  Author      [Alan Mishchenko <alanmi@eecs.berkeley.edu>]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.1. Started - October 1, 2004]

  Revision    [$Id: fraigImp.c,v 1.2 2005/07/08 01:01:31 alanmi Exp $]

***********************************************************************/

#include "fraigInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void Fraig_ManCollectRefCone_rec( Fraig_Man_t * pMan, Fraig_Node_t * pNode, Fraig_NodeVec_t * vNodes );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [This procedure proves some implications for the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_ManProveSomeImplications( Fraig_Man_t * pMan, Fraig_Node_t * pRoot )
{
    Fraig_NodeVec_t * vSuper;
    Fraig_NodeVec_t * vRefCone;
    Fraig_Node_t * pNode;
    int i, Counter, nProved, clk;

    if ( pRoot->Num % 10 )
        return;

    // collect the implication supergate
    vSuper = Fraig_CollectSupergate( pRoot, 0 );
    // start the DFS, collect all referenced nodes, but do not collect roots
    vRefCone = Fraig_NodeVecAlloc( 1000 );
    pMan->nTravIds++;
    for ( i = 0; i < vSuper->nSize; i++ )
    {
        pNode = Fraig_Regular(vSuper->pArray[i]);
        if ( !Fraig_NodeIsAnd(pNode) )
            continue;
        Fraig_ManCollectRefCone_rec( pMan, Fraig_Regular(pNode->p1), vRefCone );
        Fraig_ManCollectRefCone_rec( pMan, Fraig_Regular(pNode->p2), vRefCone );
    }
    Fraig_NodeVecSortByRefCount( vRefCone );
//    Fraig_NodeVecSortByLevel( vRefCone );
 
    // count how many nodes have containing simulation info
clk = clock();
    Counter = nProved = 0;
//    for ( i = 0; i < vRefCone->nSize; i++ )
    for ( i = vRefCone->nSize - 1; i >= 0; i-- )
    {
        pNode = vRefCone->pArray[i];
        if ( Fraig_NodeSimsContained( pMan, pNode, pRoot ) )
        {
            if ( Fraig_NodeIsImplication( pMan, pNode, pRoot, 50 ) )
                nProved++;
            Counter++;
        }
        else if ( Fraig_NodeSimsContained( pMan, pRoot, pNode ) )
        {
            if ( Fraig_NodeIsImplication( pMan, pRoot, pNode, 50 ) )
                nProved++;
            Counter++;
        }
        if ( nProved == 1 )
            break;
    }

pMan->time4 += clock() - clk;

//if ( Counter )
//printf( "%d(%d,%d) ", vRefCone->nSize, Counter, nProved );


    Fraig_NodeVecFree( vRefCone );
    Fraig_NodeVecFree( vSuper );
}

/**Function*************************************************************

  Synopsis    [This procedure proves some implications for the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_ManCollectRefCone_rec( Fraig_Man_t * pMan, Fraig_Node_t * pNode, Fraig_NodeVec_t * vNodes )
{
    assert( !Fraig_IsComplement(pNode) );
    // skip the visited node
    if ( pNode->TravId == pMan->nTravIds )
        return;
    pNode->TravId = pMan->nTravIds;
    // visit the transitive fanin
    if ( Fraig_NodeIsAnd(pNode) )
    {
        Fraig_ManCollectRefCone_rec( pMan, Fraig_Regular(pNode->p1), vNodes );
        Fraig_ManCollectRefCone_rec( pMan, Fraig_Regular(pNode->p2), vNodes );
    }
    // save the node
    if ( pNode->nRefs > 10 )
        Fraig_NodeVecPush( vNodes, pNode );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


