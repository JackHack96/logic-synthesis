/**CFile****************************************************************

  FileName    [ntkUtils.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Various network manipulation utilities.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ntkUtils.c,v 1.43 2005/05/18 04:14:36 alanmi Exp $]

***********************************************************************/

#include "ntkInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Create a new network.]

  Description [If the relation manager is not NULL, the same manager is
  used. Otherwise, a new manager is created.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Network_t * Ntk_NetworkAlloc( Mv_Frame_t * pMvsis )
{
    Ntk_Network_t * pNet;
    pNet = ALLOC( Ntk_Network_t, 1 );
    memset( pNet, 0, sizeof(Ntk_Network_t) );
    // set affiliation of this network
    pNet->pMvsis = pMvsis;
    // start the hash table
    pNet->tName2Node = st_init_table(strcmp, st_strhash);
    // get the relation manager
    pNet->pMan = Mv_FrameReadMan(pMvsis); 
    Fnc_ManagerRef( pNet->pMan );
    // start the memory managers
    pNet->pManNode  = Extra_MmFixedStart( sizeof( Ntk_Node_t ), 10000, 1000 );
    pNet->pManPin   = Extra_MmFixedStart( sizeof( Ntk_Pin_t ), 10000, 1000 );
    pNet->pNames    = Extra_MmFlexStart( 10000, 1000 );
    // get ready to assign the first node ID
    pNet->nIds = 1;
    pNet->nTravIds = 1;
    // alloc storage for network IDs
    pNet->nIdsAlloc = 1000;
    pNet->pId2Node = ALLOC( Ntk_Node_t *, pNet->nIdsAlloc );
    pNet->pId2Node[0] = NULL;
    // temporary storage (this is very bad; we should get rid of this)
    pNet->nArraysAlloc  = 2000;
    pNet->pArray1 = ALLOC( Ntk_Node_t *, pNet->nArraysAlloc );
    pNet->pArray2 = ALLOC( Ntk_Node_t *, pNet->nArraysAlloc );
    pNet->pArray3 = ALLOC( Ntk_Node_t *, pNet->nArraysAlloc );
    pNet->dDefaultArrTimeRise = 0.0;
    pNet->dDefaultArrTimeFall = 0.0;
    return pNet;
}

/**Function*************************************************************

  Synopsis    [Duplicates the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Network_t * Ntk_NetworkDup( Ntk_Network_t * pNet, Fnc_Manager_t * pMan )
{
    Ntk_Network_t * pNetNew;
    Ntk_Node_t * pNode, * pFanin, * pNodeNew;
    Ntk_Latch_t * pLatch, * pLatchNew;
    Ntk_Pin_t * pPin;

    if ( pNet == NULL )
        return NULL;

    // allocate the empty network
    pNetNew = Ntk_NetworkAlloc( pNet->pMvsis );

    // register the name 
    if ( pNet->pName )
        pNetNew->pName = Ntk_NetworkRegisterNewName( pNetNew, pNet->pName );
    // register the network spec file name
    if ( pNet->pSpec )
        pNetNew->pSpec = Ntk_NetworkRegisterNewName( pNetNew, pNet->pSpec );

    // copy and add the CI nodes
    Ntk_NetworkForEachCi( pNet, pNode )
    {
        pNodeNew = Ntk_NodeDup( pNetNew, pNode );
        Ntk_NetworkAddNode( pNetNew, pNodeNew, 1 );
    }
    // copy the internal nodes
    Ntk_NetworkForEachNode( pNet, pNode )
        Ntk_NodeDup( pNetNew, pNode );
    // add the internal nodes
    Ntk_NetworkForEachNode( pNet, pNode )
    {
        pNodeNew = pNode->pCopy;
        // adjust the fanins of the new node to point to the new fanins
        // below, "pFanin" is the old fanin set by Ntk_NodeDup()
        Ntk_NodeForEachFanin( pNodeNew, pPin, pFanin ) 
            pPin->pNode = pFanin->pCopy;
        Ntk_NetworkAddNode( pNetNew, pNodeNew, 1 );
    }
    // copy and add the CO nodes
    Ntk_NetworkForEachCo( pNet, pNode )
    {
        pNodeNew = Ntk_NodeDup( pNetNew, pNode );
        // adjust the fanins of the new node to point to the new fanins
        // below, "pFanin" is the old fanin set by Ntk_NodeDup()
        Ntk_NodeForEachFanin( pNodeNew, pPin, pFanin ) 
            pPin->pNode = pFanin->pCopy;
        Ntk_NetworkAddNode( pNetNew, pNodeNew, 1 );
    }

    // copy and add the latches
    Ntk_NetworkForEachLatch( pNet, pLatch )
    {
        // get the new latch
        pLatchNew = Ntk_LatchDup( pNetNew, pLatch );
        // set the correct inputs/outputs
        pLatchNew->pInput  = pLatch->pInput->pCopy;
        pLatchNew->pOutput = pLatch->pOutput->pCopy;
        // add the new latch to the network
        Ntk_NetworkAddLatch( pNetNew, pLatchNew );
    }

    // copy the EXDC network
    if ( pNet->pNetExdc )
        pNetNew->pNetExdc = Ntk_NetworkDup( pNet->pNetExdc, pNet->pMan );

    // put IDs into a proper order
    Ntk_NetworkReassignIds( pNetNew );

    pNetNew->dDefaultArrTimeRise = pNet->dDefaultArrTimeRise;
    pNetNew->dDefaultArrTimeFall = pNet->dDefaultArrTimeFall;

    if ( !Ntk_NetworkCheck( pNetNew ) )
        fprintf( Ntk_NetworkReadMvsisOut(pNet), "Ntk_NetworkDup(): Network check has failed.\n" );
    return pNetNew;
}

/**Function*************************************************************

  Synopsis    [Appends the second network to the first one.]

  Description [The same-named PIs and internal nodes are reused. 
  The same-named POs are not allowed. The first network is modified.
  The second one is not changed.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Network_t * Ntk_NetworkAppend( Ntk_Network_t * pNet1, Ntk_Network_t * pNet2 )
{
    Ntk_Node_t * pNode, * pFanin, * pNodeNew;
    Ntk_Latch_t * pLatch, * pLatchNew;
    Ntk_Pin_t * pPin;
    FILE * pOutput;
    int fCommonPOs;

    // get the output stream
    pOutput = Ntk_NetworkReadMvsisOut(pNet1);

    // check that they have different PO nodes
    fCommonPOs = 0;
    Ntk_NetworkForEachCo( pNet2, pNode )
    {
        // check if the node with this name exists in pNet1
        if ( Ntk_NetworkFindNodeByName(pNet1, pNode->pName) )
        {
            fprintf( pOutput, "Ntk_NetworkAppend(): PO node \"%s\" exists in both networks.\n", pNode->pName );
            fCommonPOs = 1;
        }
    }
    if ( fCommonPOs )
        return NULL;

    // remove the spec of the first network if present
    pNet1->pSpec = NULL;

    // copy and add the CI nodes of the second network
    Ntk_NetworkForEachCi( pNet2, pNode )
    {
        // check if the node with this name exists in pNet1
        if ( pNodeNew = Ntk_NetworkFindNodeByName(pNet1, pNode->pName) )
        {
            fprintf( pOutput, "Warning: The PI \"%s\" already exists in the first network.\n", pNode->pName );
            pNode->pCopy = pNodeNew; 
            continue;
        }
        // create this node
        pNodeNew = Ntk_NodeDup( pNet1, pNode );
        Ntk_NetworkAddNode( pNet1, pNodeNew, 1 );
    }
    // copy and add the internal nodes in the DFS order
    Ntk_NetworkDfs( pNet2, 1 );
    Ntk_NetworkForEachNodeSpecial( pNet2, pNode )
    {
        if ( pNode->Type != MV_NODE_INT )
            continue;
        // check if the node with this name exists in pNet1
        if ( pNode->pName && (pNodeNew = Ntk_NetworkFindNodeByName(pNet1, pNode->pName)) )
        {
            fprintf( pOutput, "Warning: Internal node \"%s\" already exists in the first network.\n", pNode->pName );
            pNode->pCopy = pNodeNew; 
            continue;
        }
        pNodeNew = Ntk_NodeDup( pNet1, pNode );
        // adjust the fanins of the new node to point to the new fanins
        Ntk_NodeForEachFanin( pNodeNew, pPin, pFanin ) 
            pPin->pNode = pFanin->pCopy;
        Ntk_NetworkAddNode( pNet1, pNodeNew, 1 );
    }
    // copy and add the CO nodes
    Ntk_NetworkForEachCo( pNet2, pNode )
    {
        // check if the node with this name exists in pNet1
        assert( Ntk_NetworkFindNodeByName(pNet1, pNode->pName) == NULL );
        pNodeNew = Ntk_NodeDup( pNet1, pNode );
        // adjust the fanins of the new node to point to the new fanins
        Ntk_NodeForEachFanin( pNodeNew, pPin, pFanin ) 
            pPin->pNode = pFanin->pCopy;
        Ntk_NetworkAddNode( pNet1, pNodeNew, 1 );
    }

    // copy and add the latches
    Ntk_NetworkForEachLatch( pNet2, pLatch )
    {
        // get the new latch
        pLatchNew = Ntk_LatchDup( pNet1, pLatch );
        // set the correct inputs/outputs
        pLatchNew->pInput  = pLatch->pInput->pCopy;
        pLatchNew->pOutput = pLatch->pOutput->pCopy;
        assert( pLatchNew->pInput && pLatchNew->pOutput );
        // add the new latch to the network
        Ntk_NetworkAddLatch( pNet1, pLatchNew );
    }

    // append the EXDC network
    if ( pNet2->pNetExdc )
        pNet1->pNetExdc = Ntk_NetworkAppend( pNet1->pNetExdc, pNet2->pNetExdc );

    // put IDs into a proper order
    Ntk_NetworkReassignIds( pNet1 );

    if ( !Ntk_NetworkCheck( pNet1 ) )
        fprintf( pOutput, "Ntk_NetworkAppend(): Network check has failed.\n" );
    return pNet1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkDelete( Ntk_Network_t * pNet )
{
    int TotalMemory;
    Ntk_Node_t * pNode;
    Ntk_Latch_t * pLatch;

    if ( pNet == NULL )
        return;

    Ntk_NetworkFreeBinding( pNet );

    // copy the EXDC network
    if ( pNet->pNetExdc )
        Ntk_NetworkDelete( pNet->pNetExdc );

    // deref and delete the functionality
    Ntk_NetworkForEachCi( pNet, pNode )
    {
        Fnc_FunctionDelete( pNode->pF );
        Fnc_GlobalFree( pNode->pG );
    }
    Ntk_NetworkForEachCo( pNet, pNode )
    {
        Fnc_FunctionDelete( pNode->pF );
        Fnc_GlobalFree( pNode->pG );
    }
    Ntk_NetworkForEachNode( pNet, pNode )
    {
        Fnc_FunctionDelete( pNode->pF );
        Fnc_GlobalFree( pNode->pG );
    }

    // delete the latch functions
    Ntk_NetworkForEachLatch( pNet, pLatch )
        if ( pLatch->pNode )
            Fnc_FunctionDelete( Ntk_NodeReadFunc(pLatch->pNode) );

    // free BDD managers
//    Fnc_ManagerDeref( pNet->pMan );
    if ( pNet->pDdGlo )
        Extra_StopManager( pNet->pDdGlo );

    if ( pNet->nLevels )
        FREE( pNet->ppLevels );
    if ( pNet->nIdsAlloc )
        FREE( pNet->pId2Node );

    // free the hash table of node name into node ID
    st_free_table( pNet->tName2Node );
    TotalMemory  = 0;
    TotalMemory += Extra_MmFixedReadMemUsage(pNet->pManNode);
    TotalMemory += Extra_MmFixedReadMemUsage(pNet->pManPin);
    TotalMemory += Extra_MmFlexReadMemUsage(pNet->pNames);
//  fprintf( Ntk_NetworkReadMvsisOut(pNet), "The total memory allocated internally by the network = %d.\n", TotalMemory );
    // free the storage allocated for nodes and pins
    Extra_MmFixedStop( pNet->pManNode, 0 );
    Extra_MmFixedStop( pNet->pManPin, 0 );
    // free the storage allocated for names
    Extra_MmFlexStop( pNet->pNames, 0 );
    free( pNet->pArray1 );
    free( pNet->pArray2 );
    free( pNet->pArray3 );
    free( pNet );
}


/**Function*************************************************************

  Synopsis    [Creates the network computes of one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Network_t * Ntk_NetworkCreateFromNode( Mv_Frame_t * pMvsis, Ntk_Node_t * pNode )
{
    Ntk_Network_t * pNetNew;
    Ntk_Node_t * pNodePi, * pFanin, * pNodeDup;
    Ntk_Pin_t * pPin;
    char * pName;

    if ( pNode == NULL )
        return NULL;

    // allocate the empty network
    pNetNew = Ntk_NetworkAlloc( pMvsis );

    // register the name of the node as the name of the network
    if ( pNode->pName )
        pNetNew->pName = Ntk_NetworkRegisterNewName( pNetNew, pNode->pName );

    // create the CI nodes
    Ntk_NodeForEachFanin( pNode, pPin, pFanin )
    {
        pName = Ntk_NodeGetNamePrintable(pFanin);
        pNodePi = Ntk_NodeCreate( pNetNew, pName, MV_NODE_CI, pFanin->nValues );
        Ntk_NetworkAddNode( pNetNew, pNodePi, 1 );
        // remember this node in the original node
        pFanin->pCopy = pNodePi;
    }
    // copy the given node
    pNodeDup = Ntk_NodeDup( pNetNew, pNode );
    // adjust the fanins of the new node to point to the new fanins
    // below, "pFanin" is the old fanin set by Ntk_NodeDup()
    Ntk_NodeForEachFanin( pNodeDup, pPin, pFanin ) 
        pPin->pNode = pFanin->pCopy;
    // add the internal node
    Ntk_NetworkAddNode( pNetNew, pNodeDup, 1 );

    // create the CO node
    Ntk_NetworkAddNodeCo( pNetNew, pNodeDup, 1 );

    // put IDs into a proper order
    Ntk_NetworkReassignIds( pNetNew );

    if ( !Ntk_NetworkCheck( pNetNew ) )
        fprintf( Mv_FrameReadOut(pMvsis), "Ntk_NetworkCreateFromNode(): Network check has failed.\n" );
    return pNetNew;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkAddNode( Ntk_Network_t * pNet, Ntk_Node_t * pNode, bool fConnect )
{
    // set the network
    pNode->pNet = pNet;
    // assign the unique ID
    pNode->Id = pNet->nIds++;  
    // add the node to the Id2Node table
    if ( pNode->Id == pNet->nIdsAlloc )
    {
        pNet->nIdsAlloc *= 2;
        pNet->pId2Node = REALLOC( Ntk_Node_t *, pNet->pId2Node, pNet->nIdsAlloc );
    }
    pNet->pId2Node[pNode->Id] = pNode;
    // add the name to the table
    if ( pNode->pName )
    if ( st_insert( pNet->tName2Node, pNode->pName, (char *)pNode ) )
    {
        assert( 0 );
    }
    // add the node to the linked lists of nodes
    if ( pNode->Type == MV_NODE_CI )
        Ntk_NetworkNodeCiListAddLast( pNode );
    else if ( pNode->Type == MV_NODE_CO )
        Ntk_NetworkNodeCoListAddLast( pNode );
    else if ( pNode->Type == MV_NODE_INT )
        Ntk_NetworkNodeListAddLast( pNode );
    else
    {
        assert( 0 );
    }
    // create the fanout structures for the fanin nodes
    if ( fConnect && pNode->Type != MV_NODE_CI )
        Ntk_NodeAddFaninFanout( pNet, pNode );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkAddNode2( Ntk_Network_t * pNet, Ntk_Node_t * pNode, bool fConnect )
{
    // set the network
    pNode->pNet = pNet;
    // assign the unique ID
    pNode->Id = pNet->nIds++;  
    // add the node to the Id2Node table
    if ( pNode->Id == pNet->nIdsAlloc )
    {
        pNet->nIdsAlloc *= 2;
        pNet->pId2Node = REALLOC( Ntk_Node_t *, pNet->pId2Node, pNet->nIdsAlloc );
    }
    pNet->pId2Node[pNode->Id] = pNode;
    // add the name to the table
    if ( pNode->pName )
    if ( st_insert( pNet->tName2Node, pNode->pName, (char *)pNode ) )
    {
        assert( 0 );
    }
/*
    // add the node to the linked lists of nodes
    if ( pNode->Type == MV_NODE_CI )
        Ntk_NetworkNodeCiListAddLast( pNode );
    else if ( pNode->Type == MV_NODE_CO )
        Ntk_NetworkNodeCoListAddLast( pNode );
    else if ( pNode->Type == MV_NODE_INT )
        Ntk_NetworkNodeListAddLast( pNode );
    else
    {
        assert( 0 );
    }
*/
        Ntk_NetworkNodeListAddLast2( pNode );

    // create the fanout structures for the fanin nodes
    if ( fConnect && pNode->Type != MV_NODE_CI )
        Ntk_NodeAddFaninFanout( pNet, pNode );
}

/**Function*************************************************************

  Synopsis    [Add a PO node functionally equivalent to the internal node.]

  Description [This function assumes that the internal node already
  exists in the network and is probably connected to other nodes 
  (both fanins and fanouts). This function adds a new node with the name
  equal to the name of the given node, and leaves the given node without name.
  The new node is a PO node, with a fanin equal to the given node.
  The fanin's fanout of this node is not connected.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Node_t * Ntk_NetworkAddNodeCo( Ntk_Network_t * pNet, Ntk_Node_t * pNode, bool fConnect )
{
    Ntk_Node_t * pNodePo;
    // the node should have the name
    assert( pNode->pName );
    // the node should be already in the network
    assert( pNode->pNet == pNet );
    assert( st_is_member( pNet->tName2Node, pNode->pName ) );
    // the node should be internal
    assert( pNode->Type == MV_NODE_INT );

    // remove the node from the table
    // because now this node has no name and should not be in the table
    st_delete( pNet->tName2Node, &pNode->pName, NULL );
    // the node remains in the list of internal nodes

    // create a new PO node
    pNodePo = Ntk_NodeCreate( pNet, NULL, MV_NODE_CO, pNode->nValues );
    // transfer the name
    pNodePo->pName = pNode->pName; 
    pNode->pName   = NULL;
    // transfer subtype
    pNodePo->Subtype = pNode->Subtype;
    pNode->Subtype  = 0;

    // set the fanin of the new PO node to be the internal node
    pNodePo->pNet = pNet;
    Ntk_NodeAddFanin( pNodePo, pNode );
    // add the new node to the network
    Ntk_NetworkAddNode( pNet, pNodePo, fConnect );
    return pNodePo;
}


/**Function*************************************************************

  Synopsis    [Adds a CO node which originally had the same name as the CI node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkTransformCiToCo( Ntk_Network_t * pNet, Ntk_Node_t * pNodeCi )
{
    Ntk_Node_t * pNodeCo;
    char * pNameCi;

    // make sure the given node is a CI node
    assert( Ntk_NodeIsCi(pNodeCi) );
    // create the new CO node
    pNodeCo = Ntk_NodeCreate( pNet, NULL, MV_NODE_CO, pNodeCi->nValues );
    // assign the CI name to the CO (this name is already registered)
    pNodeCo->pName = pNodeCi->pName;

    // remove the CI node with this name from the table
    st_delete( pNet->tName2Node, (char **)&pNodeCi->pName, NULL );
    // create a new CI node name (just like in SIS)
    pNameCi = ALLOC( char, strlen(pNodeCi->pName) + 5 );
    strcpy( pNameCi, "IN-" );
    strcat( pNameCi, pNodeCi->pName );
    // assign the new name to the CI node
    pNodeCi->pName = Ntk_NetworkRegisterNewName( pNet, pNameCi );
    FREE( pNameCi );
    // add the CI node with this name to the table
    if ( st_insert( pNet->tName2Node, pNodeCi->pName, (char *)pNodeCi ) )
    {
        assert( 0 );
    }

    // add the CI node as a fanin to the CO node 
    Ntk_NodeAddFanin( pNodeCo, pNodeCi );
    // add the CO node to the network
    Ntk_NetworkAddNode( pNet, pNodeCo, 1 );
}

/**Function*************************************************************

  Synopsis    [Updates the node from CO to CI.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkTransformNodeIntToCi( Ntk_Network_t * pNet, Ntk_Node_t * pNode )
{
    // make sure the given node is a CO node
    assert( Ntk_NodeIsInternal(pNode) );
    // the node should have the name
    assert( pNode->pName );

    // set the new node type
    pNode->Type = MV_NODE_CI;

    // remove the node from the linked list of CO nodes
    Ntk_NetworkNodeListDelete( pNode );
    // add the node to the linked list of CI nodes
    Ntk_NetworkNodeCiListAddLast( pNode );
    // the node remains in the table of nodes with names
}


/**Function*************************************************************

  Synopsis    [Updates the node from INT to CO.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkTransformNodeIntToCo( Ntk_Network_t * pNet, Ntk_Node_t * pNode )
{
    // the node should have the name
    assert( pNode->pName );
    // the node should be already in the network
    assert( pNode->pNet == pNet );
    assert( st_is_member( pNet->tName2Node, pNode->pName ) );
    // the node should be internal
    assert( pNode->Type == MV_NODE_INT );

    // set the new node type
    pNode->Type = MV_NODE_CO;

    // remove the nodes functionality
    if ( pNode->pF )
        Fnc_FunctionClean( pNode->pF );

    // remove the node from the linked list of int nodes
    Ntk_NetworkNodeListDelete( pNode );
    // add the node to the linked list of CO nodes
    Ntk_NetworkNodeCoListAddLast( pNode );
    // the node remains in the table of nodes with names
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkDeleteNode( Ntk_Network_t * pNet, Ntk_Node_t * pNode, bool fDisconnect, bool fDeleteNode )
{
    assert( pNode->pNet == pNet );
    // delete the name from the tables
    assert( pNode->Id > 0 && pNode->Id < pNet->nIdsAlloc );
    pNet->pId2Node[pNode->Id] = NULL;
    if ( pNode->pName )
    if ( !st_delete( pNet->tName2Node, (char **)&pNode->pName, (char **)&pNode ) )
    {
        assert( 0 );
    }

    // delete the node to the linked lists of nodes
    if ( pNode->Type == MV_NODE_CI )
        Ntk_NetworkNodeCiListDelete( pNode );
    else if ( pNode->Type == MV_NODE_CO )
        Ntk_NetworkNodeCoListDelete( pNode );
    else if ( pNode->Type == MV_NODE_INT )
        Ntk_NetworkNodeListDelete( pNode );
    else
    {
        assert( 0 );
    }

    // remove the fanout structures for the fanin nodes
    if ( fDisconnect && pNode->Type != MV_NODE_CI )
        Ntk_NodeDeleteFaninFanout( pNet, pNode );

    // delete the node that is no longer in the network
    if ( fDeleteNode )
        Ntk_NodeDelete( pNode );
}


/**Function*************************************************************

  Synopsis    [Returns 1 if the network is binary.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Ntk_NetworkIsBinary( Ntk_Network_t * pNet )
{
    Ntk_Node_t * pNode;
    Cvr_Cover_t * pCvr;
    Mvr_Relation_t * pMvr;

    // check if the network has MV nodes
    Ntk_NetworkForEachCi( pNet, pNode )
        if ( pNode->nValues > 2 )
            return 0;
    Ntk_NetworkForEachNode( pNet, pNode )
        if ( pNode->nValues > 2 )
            return 0;
    // the network has only binary nodes
    // check if the network has ISF nodes
    Ntk_NetworkForEachNode( pNet, pNode )
    {
        pCvr = Ntk_NodeReadFuncCvr( pNode );
        if ( pCvr )
        {
            if ( Cvr_CoverReadDefault(pCvr) == -1 ) // there is no default value
                return 0; // the network is binary but has ISF nodes
        }
        else
        {
            pMvr = Ntk_NodeReadFuncMvr( pNode );
            if ( Mvr_RelationIsND(pMvr) ) // relation is ND
                return 0; // the network is binary but has ISF nodes
        }
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the network is binary.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Ntk_NetworkIsND( Ntk_Network_t * pNet )
{
    Mvr_Relation_t * pMvr;
    Ntk_Node_t * pNode;
    Ntk_NetworkForEachNode( pNet, pNode )
    {
        pMvr = Ntk_NodeGetFuncMvr( pNode );
        if ( Mvr_RelationIsND( pMvr ) ) 
            return 1;
    }
    return 0;
}

/**Function*************************************************************

  Synopsis    [Returns the CI node with the given index.]

  Description [CI/CO hashing can be added later.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Node_t * Ntk_NetworkReadCiNode( Ntk_Network_t * pNet, int i )
{
    Ntk_Node_t * pNode;
    int Counter;
    Counter = 0;
    Ntk_NetworkForEachCi( pNet, pNode )
        if ( Counter++ == i )
            return pNode;
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Returns the CO node with the given index.]

  Description [CI/CO hashing can be added later.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Node_t * Ntk_NetworkReadCoNode( Ntk_Network_t * pNet, int i )
{
    Ntk_Node_t * pNode;
    int Counter;
    Counter = 0;
    Ntk_NetworkForEachCo( pNet, pNode )
        if ( Counter++ == i )
            return pNode;
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Returns the index of the given CI in the list of CIs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_NetworkReadCiIndex( Ntk_Network_t * pNet, Ntk_Node_t * pNode )
{
    Ntk_Node_t * pNodeCi;
    int Counter;
    assert( pNode->Type == MV_NODE_CI );
    Counter = 0;
    Ntk_NetworkForEachCi( pNet, pNodeCi )
        if ( pNodeCi == pNode )
            return Counter;
        else
            Counter++;
    return -1;
}

/**Function*************************************************************

  Synopsis    [Returns the index of the given CO in the list of COs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_NetworkReadCoIndex( Ntk_Network_t * pNet, Ntk_Node_t * pNode )
{
    Ntk_Node_t * pNodeCo;
    int Counter;
    assert( pNode->Type == MV_NODE_CO );
    Counter = 0;
    Ntk_NetworkForEachCo( pNet, pNodeCo )
        if ( pNodeCo == pNode )
            return Counter;
        else
            Counter++;
    return -1;
}

/**Function*************************************************************

  Synopsis    [Changes the name of the given node.]

  Description [Returns 1 if the name was successfully changed; 0 otherwise.]
               
  SideEffects []

  SeeAlso     [Ntk_NodeAssignName]

***********************************************************************/
int Ntk_NetworkChangeNodeName( Ntk_Node_t * pNode, char * pName )
{
    Ntk_Node_t * pFanout;
    Ntk_Pin_t * pPin;
    FILE * pOutput;
    // delete the name from the tables
    pOutput = Ntk_NetworkReadMvsisOut(pNode->pNet);
    if ( pNode->pName )
    if ( !st_delete( pNode->pNet->tName2Node, (char **)&pNode->pName, (char **)&pNode ) )
    {
        fprintf( pOutput, "Ntk_NetworkChangeNodeName(): The node with the name \"%s\" does not exist.\n", pNode->pName );
        return 0;
    }
    pNode->pName = Ntk_NetworkRegisterNewName( pNode->pNet, pName );
    if ( st_insert( pNode->pNet->tName2Node, (char *)pNode->pName, (char *)pNode ) )
    {
        fprintf( pOutput, "Ntk_NetworkChangeNodeName(): The node with the name \"%s\" already exists.\n", pNode->pName );
        return 0;
    }
    // the name of the node has changed - update the fanin ordering
    Ntk_NodeForEachFanout( pNode, pPin, pFanout )
        Ntk_NodeOrderFanins( pFanout );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Transforms the CI into the buffer with the given fanin..]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkTransformCi( Ntk_Node_t * pNode, Ntk_Node_t * pFanin )
{
    Fnc_Manager_t * pMan;
    Mvr_Relation_t * pMvr;
    unsigned Pols[32];
    int i;
    assert( pNode->nValues == pFanin->nValues );
    assert( pNode->pNet == pFanin->pNet );
    // make sure the node is indeed a CI
    assert( pNode->Type == MV_NODE_CI );
    // make sure the node does not belong to the latch
//    assert( pNode->Subtype != MV_BELONGS_TO_LATCH );
    // delete it from the list of CIs
    Ntk_NetworkNodeCiListDelete( pNode );
    // change the node type to be internal
    pNode->Type = MV_NODE_INT;
    // add the node to the list of internal nodes
    Ntk_NetworkNodeListAddLast( pNode );
    // add the fanin of the node
    Ntk_NodeAddFanin( pNode, pFanin );
    // connect the fanin
    Ntk_NodeAddFaninFanout( pNode->pNet, pNode );
    // add the function of the buffer
    // get the functionality manager
    pMan = Ntk_NodeReadMan( pFanin );
    // create the relation
    for ( i = 0; i < pNode->nValues; i++ )
        Pols[i] = (1<<i);
    pMvr = Mvr_RelationCreateOneInputOneOutput( Fnc_ManagerReadManMvr(pMan), Fnc_ManagerReadManVmx(pMan), 
        Fnc_ManagerReadManVm(pMan), pNode->nValues, pNode->nValues, Pols );
    // set the variable map and the relation
    Ntk_NodeWriteFuncVm( pNode, Mvr_RelationReadVm(pMvr) );
    Ntk_NodeWriteFuncMvr( pNode, pMvr );
}

/**Function*************************************************************

  Synopsis    [Collect the names of PIs/POs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char ** Ntk_NetworkCollectIONames( Ntk_Network_t * pNet, int fCollectPis )
{
    Ntk_Node_t * pNode;
    char ** ppNames;
    int i;

    if ( fCollectPis )
    {
        // collect PI names
        ppNames = ALLOC( char *, Ntk_NetworkReadCiNum(pNet) );
        i = 0;
        Ntk_NetworkForEachCi( pNet, pNode )
            ppNames[i++] = pNode->pName;
    }
    else
    {
        // collect output names
        ppNames = ALLOC( char *, Ntk_NetworkReadCoNum(pNet) );
        i = 0;
        Ntk_NetworkForEachCo( pNet, pNode )
            ppNames[i++] = pNode->pName;
    }
    return ppNames;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


