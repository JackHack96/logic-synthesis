/**CFile****************************************************************

  FileName    [verSweep.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Sweeping the functionally equivalent nodes of the network.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: verSweep.c,v 1.7 2004/10/06 03:33:55 alanmi Exp $]

***********************************************************************/

#include "verInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static st_table * Ver_NetworkCollectEquiv( Ntk_Network_t * pNet, Ver_Manager_t * p, bool fVerbose );
static void Ntk_NetworkMergeClass( Ntk_Network_t * pNet, Ntk_Node_t * pChain, int fVerbose, int fMergeInvertedNodes );
static int Ntk_NetworkNodeDroppintCost( Ntk_Node_t * pNode );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Performs SAT-sweep of the network.]

  Description []

  SideEffects []

  SeeAlso     []

 ***********************************************************************/
void Ver_NetworkSweep( Ntk_Network_t * pNet, int fVerbose, int fMergeInvertedNodes, int fSplitLimit )
{
    Sh_Manager_t * pShMan;
    Sh_Network_t * pShNet;
    st_table *     tEquiv;

    if ( !Ntk_NetworkIsBinary(pNet) )
    {
        printf( "Currently can only SAT-sweep binary networks.\n" );
        return;
    }

    // strash both networks into the same ShManager
    pShMan = Sh_ManagerCreate( Ntk_NetworkReadCiNum(pNet), 100000, 1 );
//    Sh_ManagerTwoLevelEnable( pShMan );
    Sh_ManagerTwoLevelDisable( pShMan );
    pShNet = Ver_NetworkStrashInt( pShMan, pNet );

    tEquiv = Ver_NetworkDetectEquiv( pNet, pShMan, pShNet, fVerbose, fSplitLimit );

    if ( fVerbose )
        printf( "The original network has %d classes of equivalent nets.\n", st_count(tEquiv) );
    Ver_NetworkMergeEquiv( pNet, tEquiv, fVerbose, fMergeInvertedNodes );
    st_free_table( tEquiv );

    Sh_NetworkFree( pShNet );
    Sh_ManagerFree( pShMan, 1 );
}

/**Function*************************************************************

  Synopsis    [Detects equivalent pairs of nodes of the network. Returns a table of equivalences.]

  Description [(May have clients outside ver package e.g. in map package)]

  SideEffects []

  SeeAlso     []

 ***********************************************************************/
st_table * Ver_NetworkDetectEquiv( Ntk_Network_t * pNet, Sh_Manager_t * pShMan, Sh_Network_t * pShNet, 
        int fVerbose, int fSplitLimit )
{
    Ver_Manager_t * pMan;
    st_table * tEquiv;
    int i, clk;

    // order the nodes in the network and back-annotate eash node to its place
    Sh_NetworkCollectInternal( pShNet );
    for ( i = 0; i < pShNet->nNodes; i++ )
        pShNet->ppNodes[i]->pData2 = i;
    if ( fVerbose )
        printf( "The strashed network is composed of %d AND gates.\n", pShNet->nNodes );

    pMan = ALLOC( Ver_Manager_t, 1 );
    memset( pMan, 0, sizeof(Ver_Manager_t) );
    pMan->pShMan = pShMan;
    pMan->pShNet = pShNet;
    pMan->fMiter = 0;

    clk = clock();
    Ver_VerificationSimulate( pMan, 10 );
    if ( fVerbose ) { PRT( "Simulation time", clock() - clk ); }
    if ( fVerbose )
        printf( "Simulation detected %d candidate equivalence classes.\n", pMan->nClasses );

    if ( pMan->nClasses )
    {
        clk = clock();
        Ver_NetworkSweepSat( pMan, fSplitLimit );
        if ( fVerbose )
            printf( "SAT proves confirmed %d equivalence classes.\n", pMan->nClasses );
        if ( fVerbose ) { PRT( "SAT solver time", clock() - clk ); }
    }

    // go through the nodes and check if the corresponding AI-graph nodes have equivalences
    tEquiv = Ver_NetworkCollectEquiv( pNet, pMan, fVerbose );

    FREE( pMan->ppClasses );
    FREE( pMan );

    return tEquiv;
}

/**Function*************************************************************

  Synopsis    [Collects the equivalent pairs of nodes of the original network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
st_table * Ver_NetworkCollectEquiv( Ntk_Network_t * pNet, Ver_Manager_t * p, bool fVerbose )
{
    Ntk_Node_t * pNode;
    Ntk_Node_t * pList1, * pList2;
    Sh_Node_t * pShNode, * pShNodeIterator;
    Ntk_Node_t * pNodeAttach;
    Ntk_Node_t ** ppSlot;
    st_table * tStrash2Net;
    st_table * tResult;
    st_generator * gen;
    int c;

    // create mapping of strashed nodes into the corresponding network nodes
    tStrash2Net = st_init_table(st_ptrcmp,st_ptrhash);
    Ntk_NetworkForEachNode( pNet, pNode )
    {
        pShNode = VER_STRASH_READ( pNode );
        if ( !st_find_or_add( tStrash2Net, (char *)Sh_Regular(pShNode), (char ***)&ppSlot ) )
            *ppSlot = NULL;
        // add the node to the list
        pNode->pOrder = *ppSlot;
        *ppSlot = pNode;
        // mark the node if it is complemented
        pNode->fNdTfi = Sh_IsComplement(pShNode);
    }

    for ( c = 0; c < p->nClasses; c++ )
    {
        pShNode = p->ppClasses[c];
        
        // get the first linked list
        if ( !st_find_or_add( tStrash2Net, (char *)pShNode, (char ***)&ppSlot ) )
        {
            assert( 0 );
        }
        pList1 = *ppSlot;

        // Find the attachment point as the end of the first list
        for ( pNodeAttach = pList1; pNodeAttach->pOrder; pNodeAttach = pNodeAttach->pOrder )
            /* do nothing */ ;
        
        // Henceforth pNodeAttach will be the location where the new list may be attached

        for ( pShNodeIterator = pShNode->pOrder; pShNodeIterator; pShNodeIterator = pShNodeIterator->pOrder )
        { 
            // get the second linked list
            if ( !st_find_or_add( tStrash2Net, (char *)pShNodeIterator, (char ***)&ppSlot ) )
            {
                assert( 0 );
            }
            pList2 = *ppSlot;
            *ppSlot = NULL;

            // if they have different polarity, complement the nodes in list2
            if ( pShNode->fMark != pShNodeIterator->fMark )
                for ( pNode = pList2; pNode; pNode = pNode->pOrder )
                    pNode->fNdTfi = !pNode->fNdTfi;

            /* 
               Greenspun's Tenth Rule of Programming: any sufficiently complicated C or Fortran program 
               contains an ad hoc informally-specified bug-ridden slow implementation of half of Common Lisp.

             */

            // attach list2 to the end of list1
            pNodeAttach->pOrder = pList2;

            // find the new attachment point
            for ( ; pNodeAttach->pOrder; pNodeAttach = pNodeAttach->pOrder ) 
                /* do nothing */;
        }
    }

    // print the classes and add to tResult
    c = 0;
    tResult = st_init_table(st_ptrcmp,st_ptrhash);
    st_foreach_item( tStrash2Net, gen, (char **)&pShNode, (char **)&pList1 )
    {
        // skip the unique classes
        if ( pList1 == NULL || pList1->pOrder == NULL )
            continue;
        st_insert( tResult, (char *)pList1, NULL );
/*
        if ( fVerbose )
        {
            printf( "Class %2d : {", c );
            for ( pNode = pList1; pNode; pNode = pNode->pOrder )
            {
                printf( " %s", Ntk_NodeGetNamePrintable(pNode) );
                if ( pNode->fNdTfi )  printf( "(*)" );
            }
            printf( " }\n" );
            c++;
        }
*/
    }
    st_free_table( tStrash2Net );
    return tResult;
}


/**Function*************************************************************

  Synopsis    [Merges equivalent nodes in the network as per table tEquiv.]

  Description [(May have clients outside ver package e.g. in map package)]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ver_NetworkMergeEquiv( Ntk_Network_t * pNet, st_table * tEquiv, int fVerbose, int fMergeInvertedNodes )
{
    st_generator * gen;
    Ntk_Node_t * pList;

    if ( st_count(tEquiv) == 0 )
        return;

    // assign levels to the nodes of the network
    Ntk_NetworkGetNumLevels( pNet );
    // merge nodes in the classes
    st_foreach_item( tEquiv, gen, (char **)&pList, NULL )
        Ntk_NetworkMergeClass( pNet, pList, fVerbose, fMergeInvertedNodes  );

    // reorder the fanins of the nodes
    Ntk_NetworkOrderFanins( pNet );
    // check the network
    if ( !Ntk_NetworkCheck( pNet ) )
        fprintf( Ntk_NetworkReadMvsisOut(pNet), "Ver_NetworkMergeEquiv(): Network check has failed.\n" );
}

/**Function*************************************************************

  Synopsis    [Process one equivalence class of nodes.]

  Description [This function does not remove the nodes. It only switches 
  around the connections.  ]
               
  SideEffects [Changes the network structure.] 

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkMergeClass( Ntk_Network_t * pNet, Ntk_Node_t * pChain, int fVerbose, int fMergeInvertedNodes )
{
    Ntk_Node_t * pListDir, * pListInv;
    Ntk_Node_t * pNodeMin, * pNodeMinInv;
    Ntk_Node_t * pNode, * pNext;

    assert( pChain );
    assert( pChain->pOrder );

    pNodeMin = pChain;
    for ( pNode = pChain->pOrder; pNode; pNode = pNode->pOrder )
        if (  pNodeMin->Level >  pNode->Level || 
            ( pNodeMin->Level == pNode->Level && 
              Ntk_NetworkNodeDroppintCost(pNodeMin) < Ntk_NetworkNodeDroppintCost(pNode) ) )
            pNodeMin = pNode;

    // divide the nodes into two parts: 
    // those that need the invertor and those that don't need
    pListDir = pListInv = NULL;
    for ( pNode = pChain, pNext = pChain->pOrder; pNode; pNode = pNext, pNext = pNode? pNode->pOrder : NULL )
    {
        if ( pNode == pNodeMin )
            continue;
        // check to which class the node belongs
        if ( pNodeMin->fNdTfi == pNode->fNdTfi )
        {
            pNode->pOrder = pListDir;
            pListDir = pNode;
        }
        else
        {
            pNode->pOrder = pListInv;
            pListInv = pNode;
        }
    }

    // move the fanouts of the direct nodes
    for ( pNode = pListDir; pNode; pNode = pNode->pOrder )
        Ntk_NodeTransferFanout( pNode, pNodeMin );

    // skip if there are no inverted nodes
    if ( pListInv == NULL )
        return;

    // if we are asked not to merge two nodes which differ by an inverter then
    // we go through the inverted nodes and choose the lowest depth one from among them
    // and merge those together as above; otherwise we add an inverted to the lowest
    // depth node pNodeMin discovered above

    if (fMergeInvertedNodes)
    {
        // add the invertor
        pNodeMinInv = Ntk_NodeCreateOneInputBinary( pNet, pNodeMin, 1 );
        Ntk_NetworkAddNode( pNet, pNodeMinInv, 1 );
    }
    else
    {
        pNodeMinInv = pListInv;
        for ( pNode = pListInv->pOrder; pNode; pNode = pNode->pOrder )
            if (  pNodeMinInv->Level >  pNode->Level || 
                    ( pNodeMinInv->Level == pNode->Level && 
                      Ntk_NetworkNodeDroppintCost(pNodeMinInv) < Ntk_NetworkNodeDroppintCost(pNode) ) )
                pNodeMinInv = pNode;
    }

    // move the fanouts of the inverted nodes
    for ( pNode = pListInv; pNode; pNode = pNode->pOrder )
        if (pNode != pNodeMinInv)
            Ntk_NodeTransferFanout( pNode, pNodeMinInv );
}


/**Function*************************************************************

  Synopsis    [Returns the number of literals saved if this node becomes useless.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_NetworkNodeDroppintCost( Ntk_Node_t * pNode )
{ // borrowed from "seqbdd/prl_equiv.c": static void compute_node_cost_rec() -- incorrect
    return 1;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


