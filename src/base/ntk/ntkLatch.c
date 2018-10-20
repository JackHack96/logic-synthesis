/**CFile****************************************************************

  FileName    [ntkLatch.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Latch handing functions.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ntkLatch.c,v 1.26 2004/05/12 04:30:08 alanmi Exp $]

***********************************************************************/

#include "ntkInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void Ntk_LatchEncode( Ntk_Latch_t * pLatch );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Creates the new latch with the given parameters.]

  Description [The network is where the latch belongs. The node
  represents the reset condition of the latch. If may have fanins 
  (the inputs of the latch reset table) but it cannot have the fanouts.
  The names are the char strings of the input and the output nodes of the 
  latch. The latch input and output should be the already existent nodes
  in the network. If the latch input/output in an internal node (rather
  than the PO/PI of the network), a new PO/PI is created, which is marked 
  as "latch only" in the subtype field (pNode->Subtype). Latches should 
  be added to the network after all other nodes have already been added.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Latch_t * Ntk_LatchCreate( Ntk_Network_t * pNet, Ntk_Node_t * pNode, int ResValue, char * pNameInput, char * pNameOutput )
{
    Ntk_Node_t * pInput, * pOutput;
    Ntk_Latch_t * pLatch;
    int Value;

    // if the node is a constant node, get its reset value and remove the node
    if ( pNode && (Value = Ntk_NodeReadConstant(pNode)) >= 0 )
    {
        // what if the node is a non-deterministic constant???
        Ntk_NodeDelete(pNode);
        pNode = NULL;
        ResValue = Value;
    }

    // check if latch input/output already exist as PIs/POs
    pInput = Ntk_NetworkFindNodeByName( pNet, pNameInput );
//    assert( pInput->Type == MV_NODE_INT );

    pOutput = Ntk_NetworkFindNodeByName( pNet, pNameOutput );
//    assert( pOutput->Type == MV_NODE_CI );

    // create a new latch
    pLatch = (Ntk_Latch_t *)Extra_MmFixedEntryFetch( pNet->pManNode );
    // clean the memory
    memset( pLatch, 0, sizeof(Ntk_Latch_t) );
    pLatch->pNet    = pNet;
    pLatch->pInput  = pInput;
    pLatch->pOutput = pOutput;
    pLatch->pNode   = pNode;
    if ( pNode == NULL )
    {
        assert( ResValue >= 0 && ResValue < pLatch->pInput->nValues );
        pLatch->Reset = ResValue;
    }
    return pLatch;
}

/**Function*************************************************************

  Synopsis    [Duplicates the latch.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Latch_t * Ntk_LatchDup( Ntk_Network_t * pNet, Ntk_Latch_t * pLatch )
{
    Ntk_Latch_t * pLatchNew;
    Ntk_Node_t * pInput, * pOutput;

    // check if latch input/output already exist as PIs/POs
    pInput = Ntk_NetworkFindNodeByName( pLatch->pNet, pLatch->pInput->pName );
    assert( pInput->Type == MV_NODE_CO );

    pOutput = Ntk_NetworkFindNodeByName( pLatch->pNet, pLatch->pOutput->pName );
    assert( pOutput->Type == MV_NODE_CI );

    // create a new latch
    pLatchNew = (Ntk_Latch_t *)Extra_MmFixedEntryFetch( pNet->pManNode );
    // clean the memory
    memset( pLatchNew, 0, sizeof(Ntk_Latch_t) );
    pLatchNew->pNet    = pNet;
    pLatchNew->pInput  = pLatch->pInput;
    pLatchNew->pOutput = pLatch->pOutput;
    pLatchNew->Reset   = pLatch->Reset;
    pLatchNew->pNode   = Ntk_NodeDup( pNet, pLatch->pNode );
    // save the pointer to this latch in the old latch
    pLatch->pData = (char *)pLatchNew;
    return pLatchNew;
}

/**Function*************************************************************

  Synopsis    [Deletes the latch.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_LatchDelete( Ntk_Latch_t * pLatch )
{
    // remove the latch node
    if ( pLatch->pNode )
    {
        // remove functionality
        Fnc_FunctionDelete( Ntk_NodeReadFunc(pLatch->pNode) );
        // recycle the node
        Ntk_NodeRecycle( pLatch->pNet, pLatch->pNode );
    }
    // recycle the latch (memory-wise, it is the same as the node)
    Extra_MmFixedEntryRecycle( pLatch->pNet->pManNode, (char *)pLatch );
}

/**Function*************************************************************

  Synopsis    [Adjusts the latch input after the CO nodes have been added.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_LatchAdjustInput( Ntk_Network_t * pNet, Ntk_Latch_t * pLatch )
{
    Ntk_Node_t * pFanout;
    Ntk_Pin_t * pPin;
    if ( pLatch->pInput->pName == NULL )
    {
        Ntk_NodeForEachFanout( pLatch->pInput, pPin, pFanout )
            if ( Ntk_NodeIsCo(pFanout) )
            {
                pLatch->pInput = pFanout;
                break;
            }
        assert( pLatch->pInput->pName );
    }
}

/**Function*************************************************************

  Synopsis    [Adds a new latch to the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkAddLatch( Ntk_Network_t * pNet, Ntk_Latch_t * pLatch )
{
    // TO DO: create the hash table of latches and add the latch to it
    // TO DO: make sure the latch with this I/O does not exist
    // add the latch to the linked list of latches
    Ntk_NetworkLatchListAddLast( pLatch );
}

/**Function*************************************************************

  Synopsis    [Deletes the latch from the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkDeleteLatch( Ntk_Latch_t * pLatch )
{
    // delete the latch from the list of latches
    Ntk_NetworkLatchListDelete( pLatch );

    // delete the latch input/output if it belongs only to the latch
    if ( pLatch->pInput->Subtype == MV_BELONGS_TO_LATCH )
        Ntk_NetworkDeleteNode( pLatch->pNet, pLatch->pInput, 1, 1 );
    if ( pLatch->pOutput->Subtype == MV_BELONGS_TO_LATCH )
        Ntk_NetworkDeleteNode( pLatch->pNet, pLatch->pOutput, 1, 1 );

    // delete the latch
    Ntk_LatchDelete( pLatch );
}


/**Function*************************************************************

  Synopsis    [Encodes the latches of the network in positional notation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkEncodeLatches( Ntk_Network_t * pNet )
{
    Ntk_Latch_t ** ppLatches, * pLatch;
    int nLatches, i;

    // put the latches into one array because they will be removed
    nLatches = Ntk_NetworkReadLatchNum( pNet );
    ppLatches = ALLOC( Ntk_Latch_t *, nLatches );
    i = 0;
    Ntk_NetworkForEachLatch( pNet, pLatch )
        ppLatches[i++] = pLatch;

    // process latches one by one
    for ( i = 0; i < nLatches; i++ )
        Ntk_LatchEncode( ppLatches[i] );
    FREE( ppLatches );

    // check that the network is okay
    if ( !Ntk_NetworkCheck( pNet ) )
        fprintf( Ntk_NetworkReadMvsisOut(pNet), "Ntk_NetworkEncodeLatches(): Network check has failed.\n" );
}


/**Function*************************************************************

  Synopsis    [Encodes one latch using positional notation.]

  Description [Modifies the fanins/fanouts of the latch. Introduces new
  latches and their new inputs/outputs. Removes the old latch and its 
  input/output.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_LatchEncode( Ntk_Latch_t * pLatch )
{
    char Buffer[1000];
    Ntk_Network_t * pNet = pLatch->pNet;
    Ntk_Node_t ** ppSelectors;
    Ntk_Node_t ** ppNewPis, ** ppNewPos;
    Ntk_Latch_t ** ppNewLatches;
    Ntk_Node_t * pCollector;
    Ntk_Node_t * pDriver;
    Ntk_Node_t ** ppFanouts;
    Cvr_Cover_t * pCvr;
    Mvc_Cover_t ** ppIsets;
    Ntk_Node_t * pNodeOut, * pNodeOutDriver = NULL;
    int nFanouts;
    int nValues;
    int nDigits;
    int ResValue;
    int i;

    // allocate storage for the temporary nodes
    assert( pLatch->pInput->nValues == pLatch->pOutput->nValues );
    nValues = pLatch->pInput->nValues;
    ppSelectors  = ALLOC( Ntk_Node_t *, nValues );
    ppNewPos     = ALLOC( Ntk_Node_t *, nValues );
    ppNewPis     = ALLOC( Ntk_Node_t *, nValues );
    ppNewLatches = ALLOC( Ntk_Latch_t *, nValues );

    // get the number of digits in the index of the input/output variable
	for ( nDigits = 0,  i = nValues - 1;  i;  i /= 10,  nDigits++ );

    // get the latch driver
    pDriver = Ntk_NodeReadFaninNode( pLatch->pInput, 0 );
    // create the selector nodes and add them to the network
    for ( i = 0; i < nValues; i++ )
    {
        ppSelectors[i] = Ntk_NodeCreateSelector( pNet, pDriver, i );
        Ntk_NetworkAddNode( pNet, ppSelectors[i], 1 );

//        ppSelectors[i] = Ntk_NodeCreateIset( pNet, pDriver, i );
//        Ntk_NetworkAddNode( pNet, ppSelectors[i], 1 );
//        Ntk_NodeMakeMinimumBase( ppSelectors[i] );

        // get the input name
	    sprintf( Buffer, "%s_%0*d", pLatch->pInput->pName, nDigits, i );
        Ntk_NodeAssignName( ppSelectors[i], Ntk_NetworkRegisterNewName(pNet, Buffer) );
    }

    // create the new PI (latch output) nodes
    for ( i = 0; i < nValues; i++ )
    {
        // get the input name
	    sprintf( Buffer, "%s_%0*d", pLatch->pOutput->pName, nDigits, i );
        // add the input node
        ppNewPis[i] = Ntk_NodeCreate( pNet, Buffer, MV_NODE_CI, 2 );
        Ntk_NetworkAddNode( pNet, ppNewPis[i], 1 );
    }

    // introduce new latches 
    for ( i = 0; i < nValues; i++ )
    {
        ResValue = (i == pLatch->Reset)? 1: 0;
        // create the latch
        ppNewLatches[i] = Ntk_LatchCreate( pNet, NULL, ResValue, ppSelectors[i]->pName, ppNewPis[i]->pName );
        // add the latch to the network
        Ntk_NetworkAddLatch( pNet, ppNewLatches[i] );
        // create the new CO node       
        Ntk_NetworkAddNodeCo( pNet, ppNewLatches[i]->pInput, 1 );
        // adjust latch input nodes 
        Ntk_LatchAdjustInput( pNet, ppNewLatches[i] );
    
        // set the subtype    
        if ( Ntk_NodeReadSubtype( pLatch->pInput ) == MV_BELONGS_TO_LATCH )
            Ntk_NodeSetSubtype( ppNewLatches[i]->pInput,  MV_BELONGS_TO_LATCH );
        if ( Ntk_NodeReadSubtype( pLatch->pOutput ) == MV_BELONGS_TO_LATCH )
            Ntk_NodeSetSubtype( ppNewLatches[i]->pOutput, MV_BELONGS_TO_LATCH );
    }

    // create the collector node
    pCollector = Ntk_NodeCreateCollector( pNet, ppNewPis, nValues, -1 );

    // patch the fanin of the fanouts of the old latch
    // to point to the collector rather than to the old latch output
    
    nFanouts = Ntk_NodeReadFanoutNum( pLatch->pOutput );
    ppFanouts = ALLOC( Ntk_Node_t *, nFanouts );
    nFanouts = Ntk_NodeReadFanouts( pLatch->pOutput, ppFanouts );
    // for each fanout, patch fanin corresponding to pCollector 
    // with pNode1Enc and collapse pNode1Enc into the fanout
    for ( i = 0; i < nFanouts; i++ )
    {
        // patch the fanin of the fanout to point to the transformer node
        Ntk_NodePatchFanin( ppFanouts[i], pLatch->pOutput, pCollector );
        // consider the case when the fanout is a PO
        if ( ppFanouts[i]->Type == MV_NODE_CO )
            continue;
        // collapse pCollector into ppFanouts[i]
        Ntk_NetworkCollapseNodes( ppFanouts[i], pCollector );


        // determine the drive of the output node, into which we collapse
        if ( Ntk_NodeReadFanoutNum(ppFanouts[i]) == 0 )
            continue;
        pNodeOut = Ntk_NodeReadFanoutNode(ppFanouts[i], 0);
        if ( pNodeOut->Type == MV_NODE_CO &&  // the node is CO
             pNodeOut->Subtype != MV_BELONGS_TO_LATCH && // the CO does not belong to latch
             pNodeOut->nValues == 2 ) // it is binary
        {
            if ( pNodeOutDriver == NULL )
                pNodeOutDriver = ppFanouts[i];
            else
                pNodeOutDriver = NULL;
        }
    }
    // the node's fanout are now transferred to pCollector or its fanins (the new PIs)
    assert( Ntk_NodeReadFanoutNum( pLatch->pOutput ) == 0 );
    FREE( ppFanouts );

    // finally connect the selectors to their fanins
//    for ( i = 0; i < nValues; i++ )
//        Ntk_NodeAddFaninFanout( pNet, ppSelectors[i] );

    // if the collector is no longer useful, remove it
    if ( Ntk_NodeReadFanoutNum( pCollector ) )
        Ntk_NetworkAddNode( pNet, pCollector, 1 );
    else
        Ntk_NodeDelete( pCollector );

    // remove the latch and its associated input/output
    Ntk_NetworkDeleteLatch( pLatch );


    // collapse the driver node into the selectors
    for ( i = 0; i < nValues; i++ )
    {
        Ntk_NetworkCollapseNodes( ppSelectors[i], pDriver );

        // remove the non-determinism if it is present
        pCvr = Ntk_NodeGetFuncCvr( ppSelectors[i] );
        ppIsets = Cvr_CoverReadIsets( pCvr );
        if ( ppIsets[0] && ppIsets[1] )
        {
            Mvc_CoverFree( ppIsets[0] );
            ppIsets[0] = NULL;

            // set the new cover
            Ntk_NodeSetFuncCvr( ppSelectors[i], pCvr );
            Ntk_NodeMakeMinimumBase( ppSelectors[i] );
        }
    }

     
    // remove the non-determinism of the output node if it is present
    if ( pNodeOutDriver )
    {
        pCvr = Ntk_NodeGetFuncCvr( pNodeOutDriver );
        ppIsets = Cvr_CoverReadIsets( pCvr );
        if ( ppIsets[0] && ppIsets[1] )
        {
            Mvc_CoverFree( ppIsets[0] );
            ppIsets[0] = NULL;

            // set the new cover
            Ntk_NodeSetFuncCvr( pNodeOutDriver, pCvr );
            Ntk_NodeMakeMinimumBase( pNodeOutDriver );
        }
    }



    // remove the driver
    if ( Ntk_NodeReadFanoutNum( pDriver ) == 0 )
        Ntk_NetworkDeleteNode( pNet, pDriver, 1, 1 );


    FREE( ppSelectors );
    FREE( ppNewPos );
    FREE( ppNewPis );
    FREE( ppNewLatches );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


