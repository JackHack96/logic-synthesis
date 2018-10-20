/**CFile****************************************************************

  FileName    [ntkiFrames.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Creates the given number of time-frames from the network.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ntkiFrames.c,v 1.2 2005/07/08 01:01:22 alanmi Exp $]
 
***********************************************************************/

#include "ntkiInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void Ntk_NetworkAddFrame( Ntk_Network_t * pNetNew, Ntk_Network_t * pNet, int iFrame );
 void Ntk_NetworkReorderCiCo( Ntk_Network_t * pNet );

extern int  Ntk_NetworkVerifyVariables( Ntk_Network_t * pNet1, Ntk_Network_t * pNet2, int fVerbose );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Unrolls the network by the given number of time frames.]

  Description [The resulting circuit is the sequential circuit derived
  by concatenating the given number of time frames of the original circuit
  one after another. The latches have inputs produced by the last time
  frame and the outputs, which feed into the first time frame.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Network_t * Ntk_NetworkDeriveTimeFrames( Ntk_Network_t * pNet, int nFrames )
{
    Ntk_Network_t * pNetNew;
    Ntk_Latch_t * pLatch, * pLatchNew;
    Ntk_Node_t ** ppNodesReset;
    Ntk_Node_t * pFanin;
    Ntk_Pin_t * pPin;
    char * pNetNameNew;
    char Buffer[10];
    int nLatches, i;

    // allocate the empty network
    assert( nFrames > 0 );
    pNetNew = Ntk_NetworkAlloc( pNet->pMvsis );

    // register the name 
    if ( pNet->pName )
    {
        // create the suffix to be added to the network name
        sprintf( Buffer, "_%d_frames", nFrames );
        pNetNameNew = Extra_FileNameAppend( pNet->pName, Buffer );
        pNetNew->pName = Ntk_NetworkRegisterNewName( pNetNew, pNetNameNew );
//        pNetNew->pSpec = Ntk_NetworkRegisterNewName( pNetNew, pNetNameNew );
    }

    // reorder the inputs of the original network so that PIs preceded 
    // latch outputs, POs preceded latch inputs and the ordering of 
    // latch outputs/inputs in both lists was synchronized
    Ntk_NetworkReorderCiCo( pNet );

    // create latches
    nLatches = Ntk_NetworkReadLatchNum( pNet );
    ppNodesReset = ALLOC( Ntk_Node_t *, nLatches );
    i = 0;
    Ntk_NetworkForEachLatch( pNet, pLatch )
    {
        // get the new latch
        pLatchNew = Ntk_LatchDup( pNetNew, pLatch );
        // add the new latch to the network
        Ntk_NetworkAddLatch( pNetNew, pLatchNew );
        // duplicate the latch output but do not add it to the network
        ppNodesReset[i]    = Ntk_NodeDup( pNetNew, pLatch->pOutput );
        pLatchNew->pInput  = ppNodesReset[i];
        pLatchNew->pOutput = NULL;
        i++;
    }

    // unroll the network as many times
    for ( i = 0; i < nFrames; i++ )
        Ntk_NetworkAddFrame( pNetNew, pNet, i );

    // set the original latch inputs
    i = 0;
    pLatchNew = pNetNew->lLatches.pHead;
    Ntk_NetworkForEachLatch( pNet, pLatch )
    {
        pLatchNew->pInput  = Ntk_NodeDup( pNetNew, pLatch->pInput );
        pLatchNew->pOutput = ppNodesReset[i];
        // adjust the fanin of latch input
        Ntk_NodeForEachFanin( pLatchNew->pInput, pPin, pFanin ) 
            pPin->pNode = pFanin->pCopy;
        // add both latch nodes to the network
        Ntk_NetworkAddNode( pNetNew, pLatchNew->pInput,  1 );  // CO
        Ntk_NetworkAddNode( pNetNew, pLatchNew->pOutput, 1 );  // CI
        pLatchNew = pLatchNew->pNext;
        i++;
    }
    assert( pLatchNew == NULL );
    FREE( ppNodesReset );

    // put IDs into a proper order
    Ntk_NetworkReassignIds( pNetNew );
    Ntk_NetworkOrderFanins( pNetNew );

    if ( !Ntk_NetworkCheck( pNetNew ) )
        fprintf( Ntk_NetworkReadMvsisOut(pNet), "Ntk_NetworkDeriveTimeFrames(): Network check has failed.\n" );
    return pNetNew;

}


/**Function*************************************************************

  Synopsis    [Adds one time frame to the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkAddFrame( Ntk_Network_t * pNetNew, Ntk_Network_t * pNet, int iFrame )
{
    Ntk_Node_t * pNode, * pFanin, * pDriver, * pNodeNew;
    Ntk_Latch_t * pLatch;
    Ntk_Pin_t * pPin;
    char * pNameNew, * pNameOld;
    char Buffer[10];

    // create the prefix to be added to the node names
    sprintf( Buffer, "%02d_", iFrame );

    // move latch inputs to become latch outputs
    Ntk_NetworkForEachLatch( pNetNew, pLatch )
    {
        pLatch->pOutput = pLatch->pInput;
        pLatch->pInput  = NULL;
    }

    // duplicate all the nodes
    pLatch = pNetNew->lLatches.pHead;
    Ntk_NetworkForEachCi( pNet, pNode )
    {
        if ( pNode->Subtype == MV_BELONGS_TO_LATCH )
        {
            // point to the output node from the previous time frame
            pNode->pCopy = pLatch->pOutput;
            pLatch = pLatch->pNext;
            continue;
        }
        // create a new node without name
        pNameOld = pNode->pName;
        pNode->pName = NULL;
        pNodeNew = Ntk_NodeDup( pNetNew, pNode );
        pNode->pName = pNameOld;
        if ( pNameOld == NULL )
            continue;
        // create the new name corresponding to this time frame
        pNameNew = Extra_FileNameAppend( Buffer, pNameOld );
        pNodeNew->pName = Ntk_NetworkRegisterNewName( pNetNew, pNameNew );
        // add the PI
        Ntk_NetworkAddNode( pNetNew, pNodeNew, 1 );
    }
    assert( pLatch == NULL );
    Ntk_NetworkForEachNode( pNet, pNode )
    {
        // create a new node without name
        pNameOld = pNode->pName;
        pNode->pName = NULL;
        pNodeNew = Ntk_NodeDup( pNetNew, pNode );
        pNode->pName = pNameOld;
        if ( pNameOld == NULL )
            continue;
        // create the new name corresponding to this time frame
        pNameNew = Extra_FileNameAppend( Buffer, pNameOld );
        pNodeNew->pName = Ntk_NetworkRegisterNewName( pNetNew, pNameNew );
    }
    Ntk_NetworkForEachNode( pNet, pNode )
    {
        pNodeNew = pNode->pCopy;
        // adjust the fanins of the new node to point to the new fanins
        // below, "pFanin" is the old fanin set by Ntk_NodeDup()
        Ntk_NodeForEachFanin( pNodeNew, pPin, pFanin ) 
        {
            assert( pFanin->pCopy );
            pPin->pNode = pFanin->pCopy;
        }
        Ntk_NetworkAddNode( pNetNew, pNodeNew, 1 );
    }
    pLatch = pNetNew->lLatches.pHead;
    Ntk_NetworkForEachCoDriver( pNet, pNode, pDriver )
    {
        if ( pNode->Subtype == MV_BELONGS_TO_LATCH )
        {
            // point to the output node from the previous time frame
            pLatch->pInput = pDriver->pCopy;
            pLatch = pLatch->pNext;
            continue;
        }
        // create a new node without name
        pNameOld = pNode->pName;
        pNode->pName = NULL;
        pNodeNew = Ntk_NodeDup( pNetNew, pNode );
        pNode->pName = pNameOld;
        if ( pNameOld == NULL )
            continue;
        // create the new name corresponding to this time frame
        pNameNew = Extra_FileNameAppend( Buffer, pNameOld );
        pNodeNew->pName = Ntk_NetworkRegisterNewName( pNetNew, pNameNew );
        // adjust the fanins of the new node to point to the new fanins
        // below, "pFanin" is the old fanin set by Ntk_NodeDup()
        Ntk_NodeForEachFanin( pNodeNew, pPin, pFanin ) 
            pPin->pNode = pFanin->pCopy;
        Ntk_NetworkAddNode( pNetNew, pNodeNew, 1 );
    }
    assert( pLatch == NULL );
}

/**Function*************************************************************

  Synopsis    [Reorders inputs in such a way that PI/PO go first.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkReorderCiCo( Ntk_Network_t * pNet )
{
    Ntk_NodeList_t lCis;       // the linked list of inputs
    Ntk_NodeList_t lCos;       // the linked list of outputs
    Ntk_Node_t * pNode, * pNode2;
    Ntk_Latch_t * pLatch;

    lCis = pNet->lCis;
    pNet->lCis.nItems = 0;
    pNet->lCis.pHead = NULL;
    pNet->lCis.pTail = NULL;

    lCos = pNet->lCos;
    pNet->lCos.nItems = 0;
    pNet->lCos.pHead = NULL;
    pNet->lCos.pTail = NULL;

    // sort through the CI/COs using the latch list
    for ( pNode = lCis.pHead, pNode2 = pNode? pNode->pNext : NULL; pNode; 
              pNode = pNode2, pNode2 = pNode? pNode->pNext : NULL )
        if ( pNode->Subtype == MV_BELONGS_TO_NET )
            Ntk_NetworkNodeCiListAddLast( pNode );
    Ntk_NetworkForEachLatch( pNet, pLatch )
        Ntk_NetworkNodeCiListAddLast( pLatch->pOutput );

    // sort through the CI/COs using the latch list
    for ( pNode = lCos.pHead, pNode2 = pNode? pNode->pNext : NULL; pNode; 
              pNode = pNode2, pNode2 = pNode? pNode->pNext : NULL )
        if ( pNode->Subtype == MV_BELONGS_TO_NET )
            Ntk_NetworkNodeCoListAddLast( pNode );
    Ntk_NetworkForEachLatch( pNet, pLatch )
        Ntk_NetworkNodeCoListAddLast( pLatch->pInput );
}


/**Function*************************************************************

  Synopsis    [Propagate the initial state of the network.]

  Description [Takes a network with latches. Removes the latch inputs.
  Replaces the latch outputs by constants (corresponding to the init state).]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkMakeCombinational( Ntk_Network_t * pNet )
{
    Ntk_Latch_t * pLatch, * pLatch2;
    Ntk_Node_t * pConst1, * pConst0;
    int Reset;
 
    pConst0 = pConst1 = NULL;
    Ntk_NetworkForEachLatch( pNet, pLatch )
    {
        // transform the latch output into a constant node
        Reset = Ntk_LatchReadReset( pLatch );
        if ( Reset == 0 )
        {
            if ( pConst0 == NULL )
            {
                pConst0 = Ntk_NodeCreateConstant( pNet, 2, 1 );
                Ntk_NetworkAddNode( pNet, pConst0, 1 );
            }
            Ntk_NetworkTransformCi( pLatch->pOutput, pConst0 );
        }
        else
        {
            if ( pConst1 == NULL )
            {
                pConst1 = Ntk_NodeCreateConstant( pNet, 2, 2 );
                Ntk_NetworkAddNode( pNet, pConst1, 1 );
            }
            Ntk_NetworkTransformCi( pLatch->pOutput, pConst1 );
        }
        // delete the latch input as a CO
        Ntk_NetworkDeleteNode( pNet, pLatch->pInput, 1, 1 );
    }
    // remove latches
    Ntk_NetworkForEachLatchSafe( pNet, pLatch, pLatch2 )
        Ntk_LatchDelete( pLatch );

    pNet->lLatches.nItems = 0;
    pNet->lLatches.pHead = NULL;
    pNet->lLatches.pTail = NULL;

    if ( !Ntk_NetworkCheck( pNet ) )
        fprintf( Ntk_NetworkReadMvsisOut(pNet), "Ntk_NetworkMakeCombinational(): Network check has failed.\n" );
}



/**Function*************************************************************

  Synopsis    [Derives the miter of the two networks.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Network_t * Ntk_NetworkDeriveMiter( Ntk_Network_t * pNet1, Ntk_Network_t * pNet2 )
{
    Ntk_Network_t * pNetNew;
    Ntk_Node_t * pNode1, * pNode2, * pFanin, * pMiter, * pNodeNew, * pDriver;
    Ntk_Node_t * ppInputs[2];
    Ntk_Pin_t * pPin;
    char Buffer[10];
    char * pName;

    // verify the identity of network CI/COs
    if ( !Ntk_NetworkVerifyVariables( pNet1, pNet2, 0 ) )
        return NULL;

    // allocate the empty network
    pNetNew = Ntk_NetworkAlloc( Ntk_NetworkReadMvsis(pNet1) );

    // register the name 
    if ( pNet1->pName )
    {
        // create the suffix to be added to the network name
        sprintf( Buffer, "_miter" );
        pName = Extra_FileNameAppend( pNet1->pName, Buffer );
        pNetNew->pName = Ntk_NetworkRegisterNewName( pNetNew, pName );
    }

    // create the PIs of the new network
    // copy the new PIs into the old PIs of both network

    // copy and add the CI nodes 
    Ntk_NetworkForEachCi( pNet1, pNode1 )
    {
        // get the same-named node of the second network
        pNode2 = Ntk_NetworkFindNodeByName(pNet2, pNode1->pName);
        assert( pNode2 != NULL );
        // create this node
        pNodeNew = Ntk_NodeDup( pNetNew, pNode1 );
        Ntk_NetworkAddNode( pNetNew, pNodeNew, 1 );
        // store the new node
        pNode1->pCopy = pNodeNew;
        pNode2->pCopy = pNodeNew;
    }

    // copy and add the internal nodes of both networks
    Ntk_NetworkDfs( pNet1, 1 );
    Ntk_NetworkForEachNodeSpecial( pNet1, pNode1 )
    {
        if ( pNode1->Type != MV_NODE_INT )
            continue;
        // duplicate the node while remembering its name
        pName = pNode1->pName;  pNode1->pName = NULL;
        pNodeNew = Ntk_NodeDup( pNetNew, pNode1 );
        pNode1->pName = pName;
        // adjust the fanins of the new node to point to the new fanins
        Ntk_NodeForEachFanin( pNodeNew, pPin, pFanin ) 
            pPin->pNode = pFanin->pCopy;
        Ntk_NetworkAddNode( pNetNew, pNodeNew, 1 );
    }
    Ntk_NetworkDfs( pNet2, 1 );
    Ntk_NetworkForEachNodeSpecial( pNet2, pNode2 )
    {
        if ( pNode2->Type != MV_NODE_INT )
            continue;
        // duplicate the node while remembering its name
        pName = pNode2->pName;  pNode2->pName = NULL;
        pNodeNew = Ntk_NodeDup( pNetNew, pNode2 );
        pNode2->pName = pName;
        // adjust the fanins of the new node to point to the new fanins
        Ntk_NodeForEachFanin( pNodeNew, pPin, pFanin ) 
            pPin->pNode = pFanin->pCopy;
        Ntk_NetworkAddNode( pNetNew, pNodeNew, 1 );
    }

    // copy the names from the drivers into the POs
    Ntk_NetworkForEachCoDriver( pNet1, pNode1, pDriver )
        pNode1->pCopy = pDriver->pCopy;
    Ntk_NetworkForEachCoDriver( pNet2, pNode2, pDriver )
        pNode2->pCopy = pDriver->pCopy;

    // create the miter of the pairwise POs
    pMiter = NULL;
    Ntk_NetworkForEachCo( pNet1, pNode1 )
    {
        if ( pNode1->Subtype != MV_BELONGS_TO_NET )
            continue;
        // get the same-named node of the second network
        pNode2 = Ntk_NetworkFindNodeByName(pNet2, pNode1->pName);
        assert( pNode2 != NULL );
        // create the EXOR of the two nodes
        ppInputs[0] = pNode1->pCopy;
        ppInputs[1] = pNode2->pCopy;
        pNodeNew = Ntk_NodeCreateTwoInputBinary( pNetNew, ppInputs, (unsigned)6 );
        Ntk_NetworkAddNode( pNetNew, pNodeNew, 1 );
        // create the OR of the two nodes
        if ( pMiter == NULL )
        {
            pMiter = pNodeNew;
            continue;
        }
        ppInputs[0] = pMiter;
        ppInputs[1] = pNodeNew;
        pMiter = Ntk_NodeCreateTwoInputBinary( pNetNew, ppInputs, (unsigned)14 );
        Ntk_NetworkAddNode( pNetNew, pMiter, 1 );
    }

    // create the single output of the new network
    Ntk_NodeAssignName( pMiter, Ntk_NetworkRegisterNewName(pNetNew, "miter") );
    Ntk_NetworkAddNodeCo( pNetNew, pMiter, 1 );

    // put IDs into a proper order
    Ntk_NetworkReassignIds( pNetNew );
    Ntk_NetworkOrderFanins( pNetNew );

    if ( !Ntk_NetworkCheck( pNetNew ) )
        fprintf( Ntk_NetworkReadMvsisOut(pNetNew), "Ntk_NetworkDeriveMiter(): Network check has failed.\n" );
    return pNetNew;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


