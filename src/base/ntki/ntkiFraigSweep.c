/**CFile****************************************************************

  FileName    [ntkiFraigSweep.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Merges the nets equivalence up to the complement.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ntkiFraigSweep.c,v 1.9 2005/05/18 04:14:37 alanmi Exp $]

***********************************************************************/

#include "ntkiInt.h"
#include "fraigInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static st_table *   Ntk_NetworkDetectEquiv( Fraig_Man_t * p, Ntk_Network_t * pNet, int fVerbose );
static void         Ntk_NetworkTransformEquiv( Ntk_Network_t * pNet, st_table * tEquiv );
static void         Ntk_NetworkMergeClassMapped( Ntk_Network_t * pNet, Ntk_Node_t * pChain, int fVerbose );
static void         Ntk_NetworkMergeClass( Ntk_Network_t * pNet, Ntk_Node_t * pChain, int fVerbose );
static int          Ntk_NetworkNodeDroppintCost( Ntk_Node_t * pNode );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////
     
/**Function*************************************************************

  Synopsis    [Performs FRAIG-sweep of the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Net_NetworkEquiv( Ntk_Network_t * pNet, int fFuncRed, int fFeedBack, int fDoSparse, int fVerbose )
{
    Fraig_Man_t * pMan;
    st_table * tEquiv;
    int clk, clkTotal = clock();
    int fRefCount = 0; // no ref counting
    int fChoicing = 0; // no choice nodes
    int fBalance  = 0; // no balancing

    if ( pNet == NULL )
        return;

    // quit if the network is not binary
    if ( !Ntk_NetworkIsBinary(pNet) )
    {
        printf( "Currently can only SAT-sweep binary networks.\n" );
        return;
    }
    // derive the FRAIG manager containing the network
    pMan = Ntk_NetworkFraig( pNet, fFuncRed, fBalance, fDoSparse, fChoicing, fVerbose );
    // collect the classes of equivalence nets
    tEquiv = Ntk_NetworkDetectEquiv( pMan, pNet, fVerbose );
    // transform the network into the equivalent one
clk = clock();
    Ntk_NetworkTransformEquiv( pNet, tEquiv );
Fraig_ManSetTimeToNet( pMan, clock() - clk );
    st_free_table( tEquiv );
Fraig_ManSetTimeTotal( pMan, clock() - clkTotal );
    // free the manager
    Fraig_ManSetVerbose( pMan, 0 );
    Fraig_ManFree( pMan );

}

/**Function*************************************************************

  Synopsis    [Collects the equivalent pairs of nodes of the original network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
st_table * Ntk_NetworkDetectEquiv( Fraig_Man_t * p, Ntk_Network_t * pNet, bool fVerbose )
{
    Ntk_Node_t * pNode;
    Ntk_Node_t * pList;
    Fraig_Node_t * gNode;
    Ntk_Node_t ** ppSlot;
    st_table * tStrash2Net;
    st_table * tResult;
    st_generator * gen;
    int c;

    // create mapping of strashed nodes into the corresponding network nodes
    tStrash2Net = st_init_table(st_ptrcmp,st_ptrhash);
    Ntk_NetworkForEachNode( pNet, pNode )
    {
        gNode = NTK_STRASH_READ( pNode );
        if ( !st_find_or_add( tStrash2Net, (char *)Fraig_Regular(gNode), (char ***)&ppSlot ) )
            *ppSlot = NULL;
        // add the node to the list
        pNode->pOrder = *ppSlot;
        *ppSlot = pNode;
        // mark the node if it is complemented
        pNode->fNdTfi = Fraig_IsComplement(gNode);
    }

    // print the classes
    c = 0;
    tResult = st_init_table(st_ptrcmp,st_ptrhash);
    st_foreach_item( tStrash2Net, gen, (char **)&gNode, (char **)&pList )
    {
        // skip the unique classes
        if ( pList == NULL || pList->pOrder == NULL )
            continue;
        st_insert( tResult, (char *)pList, NULL );
/*
        if ( fVerbose )
        {
            printf( "Class %2d : {", c );
            for ( pNode = pList; pNode; pNode = pNode->pOrder )
            {
                printf( " %s", Ntk_NodeGetNamePrintable(pNode) );
                if ( pNode->fNdTfi )  printf( "(*)" );
            }
            printf( " }\n" );
            c++;
        }
*/
    }
    if ( fVerbose )
    {
        printf( "Network \"%s\" has %d different functions (up to complementation).\n", pNet->pName, st_count(tStrash2Net) );
        printf( "Network \"%s\" has %d non-trivial classes of equivalent nets.\n", pNet->pName, st_count(tResult) );
    }
    st_free_table( tStrash2Net );
    return tResult;
}


/**Function*************************************************************

  Synopsis    [Transforms the network using the equivalence relation on nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkTransformEquiv( Ntk_Network_t * pNet, st_table * tEquiv )
{
    st_generator * gen;
    Ntk_Node_t * pList;
    int fMapped;

    if ( st_count(tEquiv) == 0 )
        return;

    // check if the network is mapped
    fMapped = Ntk_NetworkIsMapped( pNet );

    // assign levels to the nodes of the network
    Ntk_NetworkGetNumLevels( pNet );
    // merge nodes in the classes
    st_foreach_item( tEquiv, gen, (char **)&pList, NULL )
    {
        if ( fMapped )
            Ntk_NetworkMergeClassMapped( pNet, pList, 0 );
        else
            Ntk_NetworkMergeClass( pNet, pList, 0 );
    }
    // reorder the fanins of the nodes
    Ntk_NetworkOrderFanins( pNet );
    // check the network
    if ( !Ntk_NetworkCheck( pNet ) )
       fprintf( Ntk_NetworkReadMvsisOut(pNet), "Ntk_NetworkTransformEquiv(): Network check has failed.\n" );
}


/**Function*************************************************************

  Synopsis    [Transforms the list of one-phase equivalent nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkMergeClassMapped( Ntk_Network_t * pNet, Ntk_Node_t * pChain, int fVerbose )
{
    Ntk_Node_t * pListDir, * pListInv;
    Ntk_Node_t * pNodeMin, ** pNodes;
    Ntk_Node_t * pNode, * pNext, * pFanout;
    Ntk_Pin_t * pPin;
    Ntk_NodeBinding_t * pBind1, * pBind2;
    float Arrival1, Arrival2;
    int nNodes, fChange, i;

    assert( pChain );
    assert( pChain->pOrder );

    // divide the nodes into two parts: 
    // those that need the invertor and those that don't need
    pListDir = pListInv = NULL;
    for ( pNode = pChain, pNext = pChain->pOrder; pNode; pNode = pNext, pNext = pNode? pNode->pOrder : NULL )
    {
        // check to which class the node belongs
        if ( pNode->fNdTfi == 1 )
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

    // find the node with the smallest number of logic levels
    pNodeMin = pListDir;
    for ( pNode = pListDir; pNode; pNode = pNode->pOrder )
    {
        pBind1 = Ntk_NodeReadMap( pNodeMin );
        pBind2 = Ntk_NodeReadMap( pNode );
        Arrival1 = Ntk_NodeBindingReadArrival( pBind1 );
        Arrival2 = Ntk_NodeBindingReadArrival( pBind2 );
        assert( pNodeMin->Type == MV_NODE_CI || Arrival1 > 0 );
        assert( pNode->Type == MV_NODE_CI || Arrival2 > 0 );
        if (  Arrival1 > Arrival2 ||
              Arrival1 == Arrival2 && pNodeMin->Level >  pNode->Level || 
              Arrival1 == Arrival2 && pNodeMin->Level == pNode->Level && 
              Ntk_NetworkNodeDroppintCost(pNodeMin) < Ntk_NetworkNodeDroppintCost(pNode)  )
            pNodeMin = pNode;
    }

    // move the fanouts of the direct nodes
    for ( pNode = pListDir; pNode; pNode = pNode->pOrder )
        if ( pNode != pNodeMin )
        {
            // update the mapping binding of fanout nodes
            Ntk_NodeForEachFanout( pNode, pPin, pFanout )
            {
                if ( Ntk_NodeReadMap(pFanout) == NULL )
                    continue;
                nNodes = Ntk_NodeReadFaninNum(pFanout);
                pNodes = Ntk_NodeBindingReadFanInArray( Ntk_NodeReadMap(pFanout) );
                fChange = 0;
                for ( i = 0; i < nNodes; i++ )
                    if ( pNodes[i] == pNode )
                    {
                        pNodes[i] = pNodeMin;
                        fChange = 1;
                    }
                assert( fChange );
            }
            Ntk_NodeTransferFanout( pNode, pNodeMin );
        }


    // find the node with the smallest number of logic levels
    pNodeMin = pListInv;
    for ( pNode = pListInv; pNode; pNode = pNode->pOrder )
    {
        pBind1 = Ntk_NodeReadMap( pNodeMin );
        pBind2 = Ntk_NodeReadMap( pNode );
        Arrival1 = Ntk_NodeBindingReadArrival( pBind1 );
        Arrival2 = Ntk_NodeBindingReadArrival( pBind2 );
//        assert( pNodeMin->Type == MV_NODE_CI || Arrival1 > 0 );
//        assert( pNode->Type == MV_NODE_CI || Arrival2 > 0 );
        if (  Arrival1 > Arrival2 ||
              Arrival1 == Arrival2 && pNodeMin->Level >  pNode->Level || 
              Arrival1 == Arrival2 && pNodeMin->Level == pNode->Level && 
              Ntk_NetworkNodeDroppintCost(pNodeMin) < Ntk_NetworkNodeDroppintCost(pNode)  )
            pNodeMin = pNode;
    }

    // move the fanouts of the direct nodes
    for ( pNode = pListInv; pNode; pNode = pNode->pOrder )
        if ( pNode != pNodeMin )
        {
            // update the mapping binding of fanout nodes
            Ntk_NodeForEachFanout( pNode, pPin, pFanout )
            {
                if ( Ntk_NodeReadMap(pFanout) == NULL )
                    continue;
                nNodes = Ntk_NodeReadFaninNum(pFanout);
                pNodes = Ntk_NodeBindingReadFanInArray( Ntk_NodeReadMap(pFanout) );
                fChange = 0;
                for ( i = 0; i < nNodes; i++ )
                    if ( pNodes[i] == pNode )
                    {
                        pNodes[i] = pNodeMin;
                        fChange = 1;
                    }
                assert( fChange );
            }
            Ntk_NodeTransferFanout( pNode, pNodeMin );
        }
}

/**Function*************************************************************

  Synopsis    [Process one equivalence class of nodes.]

  Description [This function does not remove the nodes. It only switches 
  around the connections.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkMergeClass( Ntk_Network_t * pNet, Ntk_Node_t * pChain, int fVerbose )
{
    Ntk_Node_t * pListDir, * pListInv;
    Ntk_Node_t * pNodeMin, * pNodeMinInv;
    Ntk_Node_t * pNode, * pNext;

    assert( pChain );
    assert( pChain->pOrder );

    // find the node with the smallest number of logic levels
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

    // add the invertor
    pNodeMinInv = Ntk_NodeCreateOneInputBinary( pNet, pNodeMin, 1 );
    Ntk_NetworkAddNode( pNet, pNodeMinInv, 1 );

    // move the fanouts of the inverted nodes
    for ( pNode = pListInv; pNode; pNode = pNode->pOrder )
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


