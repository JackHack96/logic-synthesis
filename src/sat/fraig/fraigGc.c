/**CFile****************************************************************

  FileName    [fraigRef.c]

  PackageName [FRAIG: Functionally reduced AND-INV graphs.]

  Synopsis    [Reference counting and garbage collection procedures.]

  Author      [Alan Mishchenko <alanmi@eecs.berkeley.edu>]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.1. Started - October 1, 2004]

  Revision    [$Id: fraigGc.c,v 1.2 2005/07/08 01:01:31 alanmi Exp $]

***********************************************************************/

#include "fraigInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Recursively dereferences the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_RefInternal( Fraig_Node_t * p )
{
//    printf( "Referencing node %d.\n", (Fraig_Regular(p))->Num );
    (Fraig_Regular(p))->nRefs++;
}

/**Function*************************************************************

  Synopsis    [Recursively dereferences the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_DerefInternal( Fraig_Node_t * p )
{
//    printf( "Dereferencing node %d.\n", (Fraig_Regular(p))->Num );
    (Fraig_Regular(p))->nRefs--;
}

/**Function*************************************************************

  Synopsis    [Recursively dereferences the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_RecursiveDerefInternal( Fraig_Man_t * pMan, Fraig_Node_t * gNode )
{
    Fraig_Node_t * gNodeR;
    if ( pMan->fRefCount == 0 )
        return;
    gNodeR = Fraig_Regular(gNode);
    if ( gNodeR->nRefs <= 0 )
    {
        if ( pMan->nRefErrors++ == 0 )
        {
            printf( "\n" );
            printf( "A reference counting error is detected in the AI-graph package.\n" );
            printf( "This error indicates that a new node was not referenced by\n" );
            printf( "calling Fraig_Ref(), or that a node was dereferenced twice.\n" );
            printf( "The package will fail after the next garbage collection.\n" );
            printf( "\n" );
//            assert( 0 );
        }
        return;
    }
//    printf( "Dereferencing node %d.\n", gNodeR->Num );
    Fraig_Deref(gNodeR);
    // if the node has ref-count 0, deref the node recursively
    if ( gNodeR->nRefs == 0 )
    {
        // deref the children
        if ( gNodeR->p1 ) 
        {
//            assert( Fraig_Regular(gNodeR->p1)->nRefs > 0 );
            Fraig_RecursiveDerefInternal( pMan, gNodeR->p1 );
        }
        if ( gNodeR->p2 ) 
        {
//            assert( Fraig_Regular(gNodeR->p2)->nRefs > 0 );
            Fraig_RecursiveDerefInternal( pMan, gNodeR->p2 );
        }
        // also, deref the equivalent nodes
        if ( gNodeR->pNextE )
        {
            // make sure the node has reference counter 1
//            assert( gNodeR->pNextE->pFanPivot == NULL );
//            assert( gNodeR->pNextE->nRefs == 1 );
            // recursively deref this node
            Fraig_RecursiveDerefInternal( pMan, gNodeR->pNextE );
        }
    }
}


/**Function*************************************************************

  Synopsis    [Recursively reclaims the node from the dead.]

  Description [The reference counter of the returned node is not changed,
  but the reference counters of all its children and their children is
  incremented by 1.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_TableReclaimNode( Fraig_Man_t * pMan, Fraig_Node_t * gNode )
{
    Fraig_Node_t * gNodeR;
    assert( !Fraig_IsComplement(gNode) );
    assert( gNode->nRefs == 0 );
    if ( !Fraig_NodeIsAnd(gNode) )
        return;
    // if the left son is dead, bring him to life
    gNodeR = Fraig_Regular(gNode->p1);
    if ( gNodeR->nRefs == 0 )
        Fraig_TableReclaimNode( pMan, gNodeR );
    // in any case, increment the reference counter
//printf( "Referencing node %d.\n", gNodeR->Num );
    Fraig_Ref(gNodeR);
    // if the right son is dead, bring him to life
    gNodeR = Fraig_Regular(gNode->p2);
    if ( gNodeR->nRefs == 0 )
        Fraig_TableReclaimNode( pMan, gNodeR );
    // in any case, increment the reference counter
//printf( "Referencing node %d.\n", gNodeR->Num );
    Fraig_Ref(gNodeR);
    // if there is an equivalent node, reclaim it too
    if ( gNode->pNextE )
    {
//printf( "Reclaiming happens.\n" );
        // make sure the node has zero references
        assert( gNode->pNextE->nRefs == 0 );
        // reclaim this node
        Fraig_TableReclaimNode( pMan, gNode->pNextE );
        // reference the reclaimed node (equivalent nodes have ref count 1)
        Fraig_Ref(gNode->pNextE);
    }
}

/**Function*************************************************************

  Synopsis    [Returns the number of nodes with non-zero reference count.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_ManagerCheckZeroRefs( Fraig_Man_t * pMan )
{
    Fraig_Node_t * pEnt;
    int i, nRefNodes;

    // deref const 0 and elementary variables
    Fraig_RecursiveDeref( pMan, pMan->pConst1 );
    for ( i = 0; i < pMan->vInputs->nSize; i++ )
        Fraig_RecursiveDeref( pMan, pMan->pInputs[i] );

    // clean normal nodes
    nRefNodes = 0;
    for ( i = 0; i < pMan->pTableS->nBins; i++ )
        Fraig_TableBinForEachEntryS( pMan->pTableS->pBins[i], pEnt )
        {
            nRefNodes += (pEnt->nRefs > 0);
//            printf( "Node %d has ref count %d.\n", pEnt->Num, pEnt->nRefs );
//           if ( pEnt->nRefs > 0 )
//                printf( " %d(%d)%s", pEnt->Num, pEnt->nRefs, (pEnt->pFanPivot && pEnt->pFanPivot->pRepr)? "***": "" );
        }
    // print out the statistics
    if ( nRefNodes )
    {
        printf( "\n" );
        printf( "The deleted AIG manager has %d referenced nodes.\n", nRefNodes );
        printf( "Reference leaks may adversely impact performance on large problems.\n" );
        printf( "\n" );
    }
    return nRefNodes;
}


/**Function*************************************************************

  Synopsis    [Performs garbage collection in the table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_ManagerGarbageCollect( Fraig_Man_t * pMan )
{
    Fraig_HashTable_t * pTableS = pMan->pTableS;
    Fraig_HashTable_t * pTableF = pMan->pTableF;
    Fraig_Node_t * pEntF, * pEntFnew, * pEntF2, * pEntS, * pEntS2, * pEntD, * pEntD2, * pEntE, * pEntE2;
    Fraig_Node_t ** ppListF, ** ppListS, ** ppListD;
    int nNodesNewF, nNodesNewS, i, clk;
clk = clock();

    // remove zero-ref nodes from the functional table
    nNodesNewF = 0;
    for ( i = 0; i < pTableF->nBins; i++ )
    {
        ppListF = pTableF->pBins + i;
        Fraig_TableBinForEachEntrySafeF( pTableF->pBins[i], pEntF, pEntF2 )
        {
            // consider pEntF and the linked list pEntF->pNextD
            // remove those of the nodes that have ref counts 0
            ppListD = &pEntFnew;
            Fraig_TableBinForEachEntrySafeD( pEntF, pEntD, pEntD2 )
                if ( pEntD->nRefs > 0 )
                {
                    *ppListD = pEntD;
                    ppListD = &pEntD->pNextD;
                }
            *ppListD = NULL;

            // if there is something in the list, add it to ppListF
            if ( pEntFnew )
            {
                *ppListF = pEntFnew;
                ppListF = &pEntFnew->pNextF;
                nNodesNewF++;
            }
        }
        *ppListF = NULL;
    }
    pTableF->nEntries = nNodesNewF;    

#ifdef FRAIG_ENABLE_FANOUTS
    // remove the fanouts that will be deleted
    for ( i = 0; i < pMan->pTableS->nBins; i++ )
        Fraig_TableBinForEachEntryS( pMan->pTableS->pBins[i], pEntS )
        {
            Fraig_Node_t * pFanout, * pFanout2, ** ppFanList;

            // skip the nodes to be removed
            if ( pEntS->nRefs == 0 )
                continue;
            // start the linked list of fanouts
            ppFanList = &pEntS->pFanPivot; 
            Fraig_NodeForEachFanoutSafe( pEntS, pFanout, pFanout2 )
            {
                if ( pFanout->nRefs == 0 )
                    continue;
                *ppFanList = pFanout;
                ppFanList = Fraig_NodeReadNextFanoutPlace( pEntS, pFanout );
            }
            *ppFanList = NULL;
        }
#endif

    // remove zero-ref nodes from the structural table
    nNodesNewS = 0;
    for ( i = 0; i < pTableS->nBins; i++ )
    {
        ppListS = pTableS->pBins + i;
        Fraig_TableBinForEachEntrySafeS( pTableS->pBins[i], pEntS, pEntS2 )
        {
            // leave the nodes that have references 
            if ( pEntS->nRefs > 0 )
            {
                *ppListS = pEntS;
                ppListS = &pEntS->pNextS;
                nNodesNewS++;
            }
            else
            {
                // recycle this node and all its functionally equivalent nodes
                Fraig_TableBinForEachEntrySafeE( pEntS, pEntE, pEntE2 )
                {
                    Fraig_MemFixedEntryRecycle( pMan->mmSims,  (char *)pEntE->pSimsR );
                    Fraig_MemFixedEntryRecycle( pMan->mmSims,  (char *)pEntE->pSimsD );
                    Fraig_MemFixedEntryRecycle( pMan->mmNodes, (char *)pEntE );
                    // TO ADD: recycle the dominators here
                }
            }
        }
        *ppListS = NULL;
    }

//    assert( nNodesNewS == nNodesNewF - pMan->nInputs - 1 );
    if ( pMan->fVerbose )
    {
        printf( "Garbage collection reduced nodes from %8d to %8d. ", pTableS->nEntries, nNodesNewS );
        Fraig_PrintTime( "Time", clock() - clk );
    }
    pTableS->nEntries = nNodesNewS;    
}

/**Function*************************************************************

  Synopsis    [The internal AND operation on two nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_CheckCorrectRefs( Fraig_Man_t * p )
{
    Fraig_Node_t * pEntF, * pEntD, * pEntE;
    int i;
    for ( i = 0; i < p->pTableF->nBins; i++ )
        Fraig_TableBinForEachEntryF( p->pTableF->pBins[i], pEntF )
            Fraig_TableBinForEachEntryD( pEntF, pEntD )
            {
                assert( pEntD->pRepr == NULL );
                Fraig_TableBinForEachEntryE( pEntD->pNextE, pEntE )
                {
                    assert( pEntE->pRepr != NULL );
                    assert( pEntE->nRefs == 1 );
                }
            }

}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


