/**CFile****************************************************************

  FileName    [ntkSubnet.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Extracting and inserting subnetworks.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ntkSubnet.c,v 1.22 2004/05/12 04:30:09 alanmi Exp $]

***********************************************************************/

#include "ntkInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Creates the new network composed of nodes in the special list.]

  Description [The nodes in this list should only include internal nodes.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Network_t * Ntk_SubnetworkExtract( Ntk_Network_t * pNet )
{
    Ntk_Network_t * pNetNew;
    Ntk_Node_t * pNode, * pNodeNew;
    Ntk_Node_t * pFanin, * pFanout;
    Ntk_Pin_t * pPin;
    char * pName;

    // start the new network 
    pNetNew = Ntk_NetworkAlloc( pNet->pMvsis );
    pNetNew->pName = Ntk_NetworkRegisterNewName( pNetNew, pNet->pName );

    // increment the trav ID in the old network to mark the selected nodes
    Ntk_NetworkIncrementTravId( pNet );
    // mark the nodes encountered in the internal list and copy them
    Ntk_NetworkForEachNodeSpecial( pNet, pNode )
    {
        // make sure the node is internal
        assert( pNode->Type == MV_NODE_INT );
        // make sure the nodes are not duplicated in the internal list
        assert( !Ntk_NodeIsTravIdCurrent(pNode) );
        // mark the node by the current trav ID
        Ntk_NodeSetTravIdCurrent( pNode );
        // duplicate the node as part of the new network
        Ntk_NodeDup( pNetNew, pNode );
    }

    // create the CIs of the new network
    Ntk_NetworkForEachNodeSpecial( pNet, pNode )
    {
        // find the PIs (the fanins of nodes in the list that are not themselves in the list)
        Ntk_NodeForEachFanin( pNode, pPin, pFanin )
        {
            if ( !Ntk_NodeIsTravIdCurrent(pFanin) ) // not in the list
            { 
                // get the fanin's name
                pName = Ntk_NodeGetNameUniqueLong( pFanin );
                // if the node with this name already exists in the network, skip
                if ( Ntk_NetworkFindNodeByName( pNetNew, pName ) )
                    continue;
                // create the new CI
                pNodeNew = Ntk_NodeCreate( pNetNew, pName, MV_NODE_CI, pFanin->nValues );
                // parent remembers its child
                pFanin->pCopy = pNodeNew;
                // add the CI to the network
                Ntk_NetworkAddNode( pNetNew, pNodeNew, 1 );
            }
        }
    }

    // add the internal nodes
    Ntk_NetworkForEachNodeSpecial( pNet, pNode )
    {
        // get the duplicated node
        pNodeNew = pNode->pCopy;
        // remove the node's name because it may interfer with earlier names
//        pNodeNew->pName = NULL;
/*
        // change the node's name
        if ( pNode->pName )
        {
            pName = Extra_FileNameAppend( pNode->pName, "c" );
            pName = Ntk_NetworkRegisterNewName( pNetNew, pName );
            pNodeNew->pName = pName;
        }
*/
        // adjust the fanin pointers of the duplicated nodes to point to the new nodes
        Ntk_NodeForEachFanin( pNodeNew, pPin, pFanin ) 
            pPin->pNode = pFanin->pCopy;
        // add the new node to the network
        Ntk_NetworkAddNode( pNetNew, pNodeNew, 1 );
    }

    // create the CO nodes of the new network
    Ntk_NetworkForEachNodeSpecial( pNet, pNode )
    {
        Ntk_NodeForEachFanout( pNode, pPin, pFanout )
        {
            if ( !Ntk_NodeIsTravIdCurrent(pFanout) ) // pFanout is not in the list
            {  // add pNode to the set of COs of the subnetwork
                // get the duplicated node
                pNodeNew = pNode->pCopy;
                // if the node has no name, derive the name from its parent
                if ( pNodeNew->pName == NULL )
                {
                    pName = Ntk_NodeGetNameUniqueLong( pNode );
                    pName = Ntk_NetworkRegisterNewName( pNetNew, pName );
                    Ntk_NodeAssignName( pNodeNew, pName );
                }
                // create the CO node (this node will take the name)
                Ntk_NetworkAddNodeCo( pNetNew, pNodeNew, 1 );
                break;
            }
        }
    }

    // put IDs into a proper order
    Ntk_NetworkOrderFanins( pNetNew );

    // check the network
    if ( !Ntk_NetworkCheck( pNetNew ) )
       fprintf( Ntk_NetworkReadMvsisOut(pNet), "Ntk_SubnetworkExtract(): Network check has failed.\n" );
    return pNetNew;
}

/**Function*************************************************************

  Synopsis    [Inserts the subnetwork back into the original network.]

  Description [The following limitations hold. The original network is 
  modified by removing the original nodes, which were used to create the 
  subnetwork and inserting the copies of the internal nodes of the subnetwork. 
  If original network is needed for verification, a copy of it should be
  made before calling this procedure. The nodes in the internal list of the 
  original network should be those nodes that were used to create the subnetwork.
  The nodes in the internal list will be deleted from the old network. 
  Note that this procedure will not work, if the network was duplicated, 
  and the subnetwork is now inserted into the *copy* of the original network,
  because the copy network may have different assignment of node IDs,
  while the same node ID are needed to match the names of the CI/COs of the 
  subnetwork with the nodes of the original network. The subnetwork is not 
  deleted.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_SubnetworkInsert( Ntk_Network_t * pNet, Ntk_Network_t * pNetSub )
{
    Ntk_Node_t * pNodeOld, * pNodeNew, * pNode;
    Ntk_Node_t * pFanin, * pFanout;
    Ntk_Pin_t * pPin;

    // go through the CI nodes of the subnetwork, find the corresponding
    // nodes of the original network, and set the pointers to them
    Ntk_NetworkForEachCi( pNetSub, pNode )
    {
        // get the prototype of the CI in the old network
        pNodeOld = Ntk_NetworkFindNodeByNameLong( pNet, pNode->pName );
        assert( pNodeOld );
        // set the subnetwork's CO's "copy" to be the old node
        pNode->pCopy = pNodeOld;
    }

    // duplicate the internal nodes in the subnetwork as nodes of the old network
    Ntk_NetworkForEachNode( pNetSub, pNode )
    {
        // this procedure connect the new new node to pNode->pCopy
        Ntk_NodeDup( pNet, pNode );
        // check if this internal node fans out into a CO
        if ( pFanout = Ntk_NodeHasCoName( pNode ) )
        { 
            assert( pFanout->Type == MV_NODE_CO );
             // get the prototype of the CO in the old network
            pNodeOld = Ntk_NetworkFindNodeByNameLong( pNet, pFanout->pName );
            assert( pNodeOld );
            // remember the new node in the CO of subnetwork
            pFanout->pCopy = pNode->pCopy;
            // update the pointer to the new node to point to the old node 
            // pNodeOld is not deteled, so it should be used in the adjusted fanin
            pNode->pCopy = pNodeOld;
       }
    }

    // add the new nodes to the old network while adjusting fanins
    Ntk_NetworkIncrementTravId( pNet );
    Ntk_NetworkForEachNode( pNetSub, pNode )
    {
        // check if this internal node fans out into a CO
        if ( pFanout = Ntk_NodeHasCoName( pNode ) )
        { 
            // get the old node
            pNodeOld = pNode->pCopy;
            // get the new node, to replace the old node
            pNodeNew = pFanout->pCopy;
            // adjust the fanin pointers of the duplicated nodes to point to the new nodes
            Ntk_NodeForEachFanin( pNodeNew, pPin, pFanin ) 
                pPin->pNode = pFanin->pCopy;
            // replace
            Ntk_NodeReplace( pNodeOld, pNodeNew ); // pNodeNew is disposed of
            // mark this node
            Ntk_NodeSetTravIdCurrent( pNodeOld );
        }
        else
        {
            // get the duplicated node
            pNodeNew = pNode->pCopy;
            // adjust the fanin pointers of the duplicated nodes to point to the new nodes
            Ntk_NodeForEachFanin( pNodeNew, pPin, pFanin ) 
                pPin->pNode = pFanin->pCopy;
            // add the new node to the network
            Ntk_NetworkAddNode( pNet, pNodeNew, 1 );
        }
    }

    // remove the old nodes from the network without deleting the nodes
    Ntk_NetworkForEachNodeSpecial( pNet, pNodeOld )
        if ( !Ntk_NodeIsTravIdCurrent(pNodeOld) ) // should be removed
            Ntk_NetworkDeleteNode( pNet, pNodeOld, 1, 0 );

    // now that the removed nodes are completely disconnected remove them
    Ntk_NetworkForEachNodeSpecial( pNet, pNodeOld )
        if ( !Ntk_NodeIsTravIdCurrent(pNodeOld) ) // should be removed
        {
            assert( Ntk_NodeReadFanoutNum(pNodeOld) == 0 );
            Ntk_NodeDelete( pNodeOld );
        }

    // put IDs into a proper order
    Ntk_NetworkOrderFanins( pNet );

    // check the network
   if ( !Ntk_NetworkCheck( pNet ) )
       fprintf( Ntk_NetworkReadMvsisOut(pNet), "Ntk_SubnetworkInsert(): Network check has failed.\n" );
}

/**Function*************************************************************

  Synopsis    [Write into a file subnetwork composed of nodes in the internal list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_SubnetworkWriteIntoFile( Ntk_Network_t * pNet, char * FileName, bool fBinary )
{
    Ntk_Network_t * pNetNew;
    char * pFileGeneric;
//    Ntk_Network_t * pNetCopy;

    // extract the subnetwork and write it into a file
    pNetNew = Ntk_SubnetworkExtract( pNet );
    pFileGeneric = Extra_FileNameGeneric(FileName);
    pNetNew->pName = NULL;
    Ntk_NetworkSetName( pNetNew, Ntk_NetworkRegisterNewName(pNetNew, pFileGeneric) );
    FREE( pFileGeneric );
    Io_WriteNetwork( pNetNew, FileName, fBinary? IO_FILE_BLIF: IO_FILE_BLIF_MV, 1 );

    // try inserting the network back into the original network
//    pNetCopy = Ntk_NetworkDup( pNet, pNet->pMan );
//    Ntk_SubnetworkInsert( pNet, pNetNew );
//    Ntk_NetworkVerify( pNetCopy, pNet, 0 );
//    Ntk_NetworkDelete( pNetCopy );

    // delete the subnetwork
    Ntk_NetworkDelete( pNetNew );
}

/**Function*************************************************************

  Synopsis    [Replaces a set of nodes by another set.]

  Description [Takes a network and a set of nodes (aNodesOld) that are currently
  in the network. The set is in fanout-to-fanin ordering, the first node being 
  the root node of the cluster. Another set of nodes (aNodesNew) are in fanin-to
  fanout ordering, the root of the cluster is the last node. Important: the fanin
  pointers of the second cluster can refer to nodes that are not among the PIs of 
  the original cluster. As a result of replacement, only those nodes are removed 
  that do not fanout outside the cluster.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_SubnetworkReplace( Ntk_Network_t * pNet, array_t * aNodesOld, array_t * aNodesNew )
{
    Ntk_Node_t * pRootOld, * pRootNew, * pNode;
    int i;
    // replace the pRoot node
    pRootOld = array_fetch(Ntk_Node_t *, aNodesOld, 0);
    pRootNew = array_fetch(Ntk_Node_t *, aNodesNew, array_n(aNodesNew)-1 );
    Ntk_NodeReplace( pRootOld, pRootNew );
    // now, go through the old nodes, and remove those that do not fanout
    for ( i = 1; i < array_n(aNodesOld); i++ )
    {
        pNode = array_fetch(Ntk_Node_t *, aNodesOld, i);
        if ( Ntk_NodeReadFanoutNum(pNode) == 0 )
            Ntk_NetworkAddNode( pNet, pNode, 1 );
    }
    // go through the new nodes and add them
    for ( i = 0; i < array_n(aNodesNew)-1; i++ )
    {
        pNode = array_fetch(Ntk_Node_t *, aNodesNew, i);
        Ntk_NetworkAddNode( pNet, pNode, 1 );
    }
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


