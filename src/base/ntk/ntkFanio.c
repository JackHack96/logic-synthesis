/**CFile****************************************************************

  FileName    [ntkFanio.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Procedures with work with fanin/fanout of the node.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ntkFanio.c,v 1.24 2003/11/18 18:54:54 alanmi Exp $]

***********************************************************************/

#include "ntkInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Node_t * Ntk_NodeReadFaninNode( Ntk_Node_t * pNode, int Num )
{
    Ntk_Node_t * pFanin;
    Ntk_Pin_t * pPin;
    int Counter = 0;
    Ntk_NodeForEachFanin( pNode, pPin, pFanin ) 
        if ( Counter++ == Num )
            return pFanin;
    return NULL;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_NodeReadFaninIndex( Ntk_Node_t * pNode, Ntk_Node_t * pNodeFanin )
{
    Ntk_Node_t * pFanin;
    Ntk_Pin_t * pPin;
    int i;
    Ntk_NodeForEachFaninWithIndex( pNode, pPin, pFanin, i ) 
        if ( pFanin == pNodeFanin )
            return i;
    return -1;
}



/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Node_t * Ntk_NodeReadFanoutNode( Ntk_Node_t * pNode, int Num )
{
    Ntk_Node_t * pFanout;
    Ntk_Pin_t * pPin;
    int Counter = 0;
    Ntk_NodeForEachFanout( pNode, pPin, pFanout ) 
        if ( Counter++ == Num )
            return pFanout;
    return NULL;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_NodeReadFanoutIndex( Ntk_Node_t * pNode, Ntk_Node_t * pNodeFanout )
{
    Ntk_Node_t * pFanout;
    Ntk_Pin_t * pPin;
    int i;
    Ntk_NodeForEachFanoutWithIndex( pNode, pPin, pFanout, i ) 
        if ( pFanout == pNodeFanout )
            return i;
    return -1;
}

/**Function*************************************************************

  Synopsis    [Fills out the user-specified array with the fanin pointers.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_NodeReadFanins( Ntk_Node_t * pNode, Ntk_Node_t * pFanins[] )
{
    Ntk_Node_t * pFanin;
    Ntk_Pin_t * pPin;
    int i;
    Ntk_NodeForEachFaninWithIndex( pNode, pPin, pFanin, i ) 
        pFanins[i] = pFanin;
    return i;
}

/**Function*************************************************************

  Synopsis    [Fills out the user-specified array with the fanin pointers.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_NodeReadFanouts( Ntk_Node_t * pNode, Ntk_Node_t * pFanouts[] )
{
    Ntk_Node_t * pFanout;
    Ntk_Pin_t * pPin;
    int i;
    Ntk_NodeForEachFanoutWithIndex( pNode, pPin, pFanout, i ) 
        pFanouts[i] = pFanout;
    return i;
}

/**Function*************************************************************

  Synopsis    [Collect the input/output values.]

  Description [Fills the user-given array with information about
  the number of values of the input and output variables. The output
  values is written last in the array.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodeReadFanioValues( Ntk_Node_t * pNode, int * pValues )
{
    Ntk_Node_t * pFanin;
    Ntk_Pin_t * pPin;
    int iFanin;

    Ntk_NodeForEachFaninWithIndex( pNode, pPin, pFanin, iFanin )
    {
        assert( pFanin->nValues > 1 );
        pValues[iFanin] = pFanin->nValues;
    }
    // add the number of values of the output of this node
    pValues[iFanin] = pNode->nValues;
}

/**Function*************************************************************

  Synopsis    [Cleans the fanin space of the node.]

  Description [Removes those fanins of the node, which do not belong
  to the true support of the node. The support is given by array pSupport.
  Entry 0 in pSupport means that the fanin does not belong to the support.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodeReduceFanins( Ntk_Node_t * pNode, int * pSupport )
{
    Ntk_Pin_t * pPin, * pPin2;
    Ntk_Node_t * pFanin;
    int nFanins, iFanin;

    // remove missing variables from the fanin list
    nFanins = Ntk_NodeReadFaninNum( pNode );
    Ntk_NodeForEachFaninWithIndexSafe( pNode, pPin, pPin2, pFanin, iFanin )
        if ( pSupport[iFanin] == 0 )
        { // the current fanin is not in the local BDD
            // remove the fanin's fanout if the node is in the network
            if ( pNode->pNet && pPin->pLink )
            {
                // remove the fanout
                Ntk_NodeFanoutListDelete( pFanin, pPin->pLink );
                // recycle the link
                Extra_MmFixedEntryRecycle( pNode->pNet->pManPin, (char *)pPin->pLink );
            }
            // remove the fanin
            Ntk_NodeFaninListDelete( pNode, pPin );
            // recycle the link
            Extra_MmFixedEntryRecycle( pNode->pNet->pManPin, (char *)pPin );
        }
    assert( iFanin == nFanins );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Pin_t * Ntk_NodeAddFanin( Ntk_Node_t * pNode, Ntk_Node_t * pFanin )
{
    Ntk_Pin_t * pPin;
    // get the new pin
    pPin = (Ntk_Pin_t *)Extra_MmFixedEntryFetch( pNode->pNet->pManPin );
    // fill out the pin
    pPin->pLink = NULL;
    pPin->pNode = pFanin;
    // add the link to the fanin list
    Ntk_NodeFaninListAddLast( pNode, pPin );
    return pPin;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Pin_t * Ntk_NodeAddFanout( Ntk_Node_t * pNode, Ntk_Node_t * pFanout )
{
    Ntk_Pin_t * pPin;
    // get the new pin
    pPin = (Ntk_Pin_t *)Extra_MmFixedEntryFetch( pNode->pNet->pManPin );
    // fill out the pin
    pPin->pLink = NULL;
    pPin->pNode = pFanout;
    // add the link to the fanin list
    Ntk_NodeFanoutListAddLast( pNode, pPin );
    return pPin;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodeAddFaninFanout( Ntk_Network_t * pNet, Ntk_Node_t * pNode )
{
    Ntk_Pin_t *  pPinFanin;
    Ntk_Pin_t *  pPinFanout;
    Ntk_Node_t * pFanin;
    int nPins;

    nPins = 0;
    assert( pNode->pNet == pNet );
    Ntk_NodeForEachFanin( pNode, pPinFanin, pFanin )
    {
        // add a new fanout to the node
        pPinFanout = Ntk_NodeAddFanout( pFanin, pNode );
        // interconnect the pins
        pPinFanout->pLink = pPinFanin;
        pPinFanin->pLink  = pPinFanout;
        nPins++;
    }
    assert( nPins == Ntk_NodeReadFaninNum(pNode) );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodeDeleteFaninFanout( Ntk_Network_t * pNet, Ntk_Node_t * pNode )
{
    Ntk_Pin_t *  pPinFanin;
    Ntk_Node_t * pFanin;
    int nPins;

    nPins = 0;
    assert( pNode->pNet == pNet );
    Ntk_NodeForEachFanin( pNode, pPinFanin, pFanin )
    {
        assert( pPinFanin->pLink->pNode == pNode );
        // remove the pin from the fanin's fanout structure
        Ntk_NodeFanoutListDelete( pFanin, pPinFanin->pLink );
        // recycle the pin
        Extra_MmFixedEntryRecycle( pNet->pManPin, (char *)pPinFanin->pLink );
        // clear the mirror pin
        pPinFanin->pLink = NULL;
        nPins++;
    }
    assert( nPins == Ntk_NodeReadFaninNum(pNode) );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodeTransferFanout( Ntk_Node_t * pNodeFrom, Ntk_Node_t * pNodeTo )
{
    Ntk_Node_t * pFanout;
    Ntk_Pin_t *  pPin, * pPin2;
    Ntk_NodeForEachFanoutSafe( pNodeFrom, pPin, pPin2, pFanout )
    {
        // extract the fanout pin of "nodeFrom"
        Ntk_NodeFanoutListDelete( pNodeFrom, pPin );
        // insert the fanout pin into "nodeTo"
        Ntk_NodeFanoutListAddLast( pNodeTo, pPin );
        // pFanout still points to pNodeFrom as fanin - fix this
        assert( pPin->pLink->pNode == pNodeFrom );
        pPin->pLink->pNode = pNodeTo;
    }
    assert( Ntk_NodeReadFanoutNum( pNodeFrom ) == 0 );
}

/**Function*************************************************************

  Synopsis    [Replaces a fanin by another fanin of the node.]

  Description [If the node is in the networ, this procedure also updates 
  the pin of the fanout node to point to the new fanin.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodePatchFanin( Ntk_Node_t * pNode, Ntk_Node_t * pFaninOld, Ntk_Node_t * pFaninNew )
{
    Ntk_Node_t * pFanin;
    Ntk_Pin_t * pPinFanout;
    Ntk_Pin_t * pPinFanin;
    Ntk_Pin_t * pPin;

    // get the corresponding fanin pin of the node
    pPinFanin = NULL;
    Ntk_NodeForEachFanin( pNode, pPin, pFanin )
        if ( pFanin == pFaninOld )
        {
            pPinFanin = pPin;
            break;
        }
    assert( pPinFanin );

    // update the readings in the fanin pin
    assert( pPinFanin->pNode == pFaninOld );
    pPinFanin->pNode = pFaninNew;

    // get the fanout pin of the fanin
    pPinFanout = pPinFanin->pLink;
    if ( pPinFanout )
    { // the fanout pin is present, if the node is in the network
        // remove the fanout pin from the old fanin
        Ntk_NodeFanoutListDelete( pFaninOld, pPinFanout );
        // insert the fanout pin into the new fanin
        Ntk_NodeFanoutListAddLast( pFaninNew, pPinFanout );
    }
    // make sure the fanins of the node are sorted
    Ntk_NodeOrderFanins( pNode );
}

/**Function*************************************************************

  Synopsis    [Assign the MV variable map.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vm_VarMap_t * Ntk_NodeAssignVm( Ntk_Node_t * pNode )
{
	Fnc_Manager_t * pMan;
	Vm_VarMap_t * pVm;
	int * pValues;
	int nVars;

	// get temporary storage for the input/output values
	pMan    = Ntk_NodeReadMan( pNode );
    pValues = ALLOC( int, Ntk_NodeReadFaninNum(pNode) + 1 );
    // collect the number of input/output values of this node
    Ntk_NodeReadFanioValues( pNode, pValues );
	// get the number of variables
	nVars = Ntk_NodeReadFaninNum(pNode);
    // create the variable map 
	pVm = Vm_VarMapLookup( Fnc_ManagerReadManVm(pMan), nVars, 1, pValues );
    FREE( pValues );
	// set the variable map
	assert( Ntk_NodeReadFuncVm( pNode ) == NULL );
	Ntk_NodeWriteFuncVm( pNode, pVm );
    return pVm;
}


/**Function*************************************************************

  Synopsis    [Returns 1 if Node2's support is contained in that of Node1.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Ntk_NodeSupportContain( Ntk_Node_t * pNode1, Ntk_Node_t * pNode2 )
{
    Ntk_Node_t * pFanin;
    Ntk_Pin_t * pPin;
    int Index;
    Ntk_NodeForEachFanin( pNode2, pPin, pFanin )
    {
        Index = Ntk_NodeReadFaninIndex( pNode1, pFanin );
        if ( Index == -1 ) // pFanin is not a fanin of pNode1
            return 0;
    }
    return 1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


