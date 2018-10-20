/**CFile****************************************************************

  FileName    [ntkiOrder.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Computes a simple ordering of CI nodes of the network.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ntkiOrder.c,v 1.4 2003/09/01 04:56:08 alanmi Exp $]

***********************************************************************/

#include "ntkiInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static Ntk_Node_t ** Ntk_NetworkOrderOutputs( Ntk_Network_t * pNet );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Orders the CI nodes using DFS from outputs.]

  Description [Returns the table mapping CI node pointer into its number
  in the order.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
st_table * Ntk_NetworkOrder( Ntk_Network_t * pNet, bool fUseDfs )
{
    st_table * tLeaves;
    Ntk_Node_t ** ppNodes;
    Ntk_Node_t * pNode;
    int nNodes, i;

    // collect the CO nodes into an array
    nNodes = Ntk_NetworkReadCoNum(pNet);
    ppNodes = Ntk_NetworkOrderOutputs( pNet );;
    // get the ordering of nodes
    if ( fUseDfs )
        Ntk_NetworkComputeNodeSupport( pNet, ppNodes, nNodes );
    else
        Ntk_NetworkInterleaveNodes( pNet, ppNodes, nNodes );
    free( ppNodes );

    // put the nodes in the support into the hash table
    tLeaves = st_init_table(st_ptrcmp,st_ptrhash);
    i = 0;
    Ntk_NetworkForEachNodeSpecial( pNet, pNode )
        if ( pNode->Type == MV_NODE_CI )
            st_insert( tLeaves, (char *)pNode, (char *)i++ );

    // add those nodes that are not reachable from the COs
    Ntk_NetworkForEachCi( pNet, pNode )
        if ( !Ntk_NodeIsTravIdCurrent(pNode) ) // node is not visited during supp construction
            st_insert( tLeaves, (char *)pNode, (char *)i++ );

    return tLeaves;
}


/**Function*************************************************************

  Synopsis    [Orders the CI nodes using DFS from outputs.]

  Description [Returns the table mapping CI node name into its number
  in the order.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
st_table * Ntk_NetworkOrderByName( Ntk_Network_t * pNet, bool fUseDfs )
{
    st_table * tLeaves;
    Ntk_Node_t ** ppNodes;
    Ntk_Node_t * pNode;
    int nNodes, i;

    // collect the CO nodes into an array
    nNodes = Ntk_NetworkReadCoNum(pNet);
    ppNodes = Ntk_NetworkOrderOutputs( pNet );;
    // get the ordering of nodes
    if ( fUseDfs )
        Ntk_NetworkComputeNodeSupport( pNet, ppNodes, nNodes );
    else
        Ntk_NetworkInterleaveNodes( pNet, ppNodes, nNodes );
    free( ppNodes );

    // put the nodes in the support into the hash table
    tLeaves = st_init_table(strcmp, st_strhash);
    i = 0;
    Ntk_NetworkForEachNodeSpecial( pNet, pNode )
        if ( pNode->Type == MV_NODE_CI )
            st_insert( tLeaves, pNode->pName, (char *)i++ );

    // add those nodes that are not reachable from the COs
    Ntk_NetworkForEachCi( pNet, pNode )
        if ( !Ntk_NodeIsTravIdCurrent(pNode) ) // node is not visited during supp construction
            st_insert( tLeaves, pNode->pName, (char *)i++ );

    return tLeaves;
}

/**Function*************************************************************

  Synopsis    [Orders the CI nodes using DFS from outputs.]

  Description [Returns the table mapping CI node name into its number
  in the order.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char ** Ntk_NetworkOrderArrayByName( Ntk_Network_t * pNet, bool fUseDfs )
{
    char ** psLeaves;
    Ntk_Node_t ** ppNodes;
    Ntk_Node_t * pNode;
    int nNodes, i;

    // collect the CO nodes into an array
    nNodes = Ntk_NetworkReadCoNum(pNet);
    ppNodes = Ntk_NetworkOrderOutputs( pNet );;
    // get the ordering of nodes
    if ( fUseDfs )
        Ntk_NetworkComputeNodeSupport( pNet, ppNodes, nNodes );
    else
        Ntk_NetworkInterleaveNodes( pNet, ppNodes, nNodes );
    free( ppNodes );

    // put the nodes in the support into the hash table
    psLeaves = ALLOC( char *, Ntk_NetworkReadCiNum(pNet) );
    i = 0;
    Ntk_NetworkForEachNodeSpecial( pNet, pNode )
        if ( pNode->Type == MV_NODE_CI )
            psLeaves[i++] = pNode->pName;

    // add those nodes that are not reachable from the COs
    Ntk_NetworkForEachCi( pNet, pNode )
        if ( !Ntk_NodeIsTravIdCurrent(pNode) ) // node is not visited during supp construction
            psLeaves[i++] = pNode->pName;

    assert( i == Ntk_NetworkReadCiNum(pNet) ); 
    return psLeaves;
}


/**Function*************************************************************

  Synopsis    [Returns the array of outputs sorted by level.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Node_t ** Ntk_NetworkOrderOutputs( Ntk_Network_t * pNet )
{
    Ntk_Node_t ** ppNodes;
    Ntk_Node_t * pNode;
    int nNodes, i;

    // put the output into the array
    nNodes = Ntk_NetworkReadCoNum(pNet);
    ppNodes = ALLOC( Ntk_Node_t *, nNodes );
    i = 0;
    Ntk_NetworkForEachCo( pNet, pNode )
        ppNodes[i++] = pNode;

    // assign levels to the nodes
    Ntk_NetworkGetNumLevels( pNet );

    // sort the outputs by level (in decreasing order)
    qsort( (void *)ppNodes, nNodes, sizeof(Ntk_Node_t *), 
            (int (*)(const void *, const void *)) Ntk_NodeCompareByLevel );
    assert( Ntk_NodeCompareByLevel( ppNodes, ppNodes + nNodes - 1 ) <= 0 );
    return ppNodes;    
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkReorder( Ntk_Network_t * pNet, int fVerbose )
{
    Ntk_Node_t * pNode;
    Ntk_NetworkForEachNode( pNet, pNode )
    {
        if ( fVerbose )
            fprintf( Ntk_NetworkReadMvsisOut(pNet), "%10s: ", Ntk_NodeGetNamePrintable(pNode) );
        if ( fVerbose )
            fprintf( Ntk_NetworkReadMvsisOut(pNet), "Before = %5d  BDD nodes.  ", Mvr_RelationGetNodes( Ntk_NodeReadFuncMvr(pNode) ) );
        Ntk_NodeReorderMvr( pNode );
        if ( fVerbose )
            fprintf( Ntk_NetworkReadMvsisOut(pNet), "After = %5d  BDD nodes.\n", Mvr_RelationGetNodes( Ntk_NodeReadFuncMvr(pNode) ) );
    }
}

/**Function*************************************************************

  Synopsis    [Reorders the BEMDD representing the local relation of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Ntk_NodeReorderMvr( Ntk_Node_t * pNode )
{
    Mvr_Relation_t * pMvr;
    pMvr = Ntk_NodeReadFuncMvr( pNode );
    return Mvr_RelationReorder( pMvr );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


