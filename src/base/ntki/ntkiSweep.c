/**CFile****************************************************************

  FileName    [ntkiSweep.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Implementation of the standard network sweep.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ntkiSweep.c,v 1.10 2005/04/20 07:38:23 alanmi Exp $]

***********************************************************************/

#include "ntkiInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static bool Ntk_NetworkSweepIsetsDeriveTransformers( Ntk_Node_t * pNode, 
     Ntk_Node_t ** ppTrans1, Ntk_Node_t ** ppTrans2, bool fVerbose );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Standard network sweep.]

  Description [This command does the following interatively till 
  convergence: (1) removes the nodes that does not fanout (except PI nodes);
  (2) propagates constant nodes; (3) propagates single-output nodes whenever 
  possible (for example if the inverter feeds into a PO node, it is not 
  possible to collapse it into its fanout (it may be impossible if the 
  fanout in the primary output; (4) sweeps the constant-0 and duplicated
  i-sets of each node. Returns 1 if changes have been made to the network.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Ntk_NetworkSweep( Ntk_Network_t * pNet, bool fSweepConsts, bool fSweepBuffers, bool fSweepIsets, bool fVerbose )
{
    Ntk_Node_t * pNode, * pNode2;
    bool fChange, fRecentChange;

    // assume there will be no changes
    fChange = 0;

    // perform topological cleanup
    do 
    {
        fRecentChange = 0;
        Ntk_NetworkForEachNodeSafe( pNet, pNode, pNode2 )
        {
            if ( Ntk_NodeReadFanoutNum(pNode) == 0 ) 
            {
                Ntk_NetworkDeleteNode( pNet, pNode, 1, 1 );
                fChange = fRecentChange = 1;
            }
        }
    }
    while ( fRecentChange );

    // perform combinational sweep
    if ( fSweepConsts || fSweepBuffers )
    {
        Ntk_NetworkDfs( pNet, 1 );
        Ntk_NetworkForEachNodeSpecial( pNet, pNode )
        {
            if ( Ntk_NetworkSweepNode( pNode, fSweepConsts, fSweepBuffers, fVerbose ) )
                fChange = 1;
        }
    }

    // in the MV networks, also sweep the i-sets
    if ( fSweepIsets )
    {
        Ntk_NetworkDfs( pNet, 1 );
        Ntk_NetworkForEachNodeSpecial( pNet, pNode )
        {
            if ( Ntk_NetworkSweepIsets( pNode, fVerbose ) )
                fChange = 1;
        }
    }

    // perform topological cleanup
    do 
    {
        fRecentChange = 0;
        Ntk_NetworkForEachNodeSafe( pNet, pNode, pNode2 )
        {
            if ( Ntk_NodeReadFanoutNum(pNode) == 0 ) 
            {
                Ntk_NetworkDeleteNode( pNet, pNode, 1, 1 );
                fChange = fRecentChange = 1;
            }
        }
    }
    while ( fRecentChange );

    if ( !Ntk_NetworkCheck( pNet ) )
        fprintf( Ntk_NetworkReadMvsisOut(pNet), "Ntk_NetworkSweep(): Network check has failed.\n" );
    return fChange;
}

/**Function*************************************************************

  Synopsis    [Node sweep.]

  Description [If the first flag (fSweepConsts) is set, this procedure
  looks at the fanins of the node and collapses the constant fanins.
  Similarly, if the second flex (fSweepBuffers) is set, it collapses
  single-input fanins (these are buffers/inverters in the binary network,
  but may have more complex functionality in an MV network).]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Ntk_NetworkSweepNode( Ntk_Node_t * pNode, bool fSweepConsts, bool fSweepBuffers, bool fVerbose )
{
    Ntk_Node_t * pFanin, * pFaninFanin;
    Ntk_Pin_t * pPin;
    bool fRecentChange;
    bool fChange;

    // TO DO: add here skipping the 'persistent' nodes

    // if this is a PO node and the fanin is the buffer, sweep it
    if ( pNode->Type == MV_NODE_CO && fSweepBuffers )
    {
        pFanin = Ntk_NodeReadFaninNode( pNode, 0 );
        // if the fanin has only one input, make sure its Cvr is derived
        if ( Ntk_NodeReadFaninNum(pFanin) == 1 )
            Ntk_NodeGetFuncCvr(pFanin);
        // check if the node is the binary buffer (this function uses Cvr)
        if ( Ntk_NodeIsBinaryBuffer(pFanin) )
        {
            // get the fanin's fanin
            pFaninFanin = Ntk_NodeReadFaninNode( pFanin, 0 );
            // patch the fanin of the CO node with the new fanin
            Ntk_NodePatchFanin( pNode, pFanin, pFaninFanin );
            // if the buffer does not fanout, it will be swept later
            return 1;
        }
        return 0;
    }

    // skip the nodes without functionality
    if ( pNode->Type != MV_NODE_INT )
        return 0;

    // collapse constant and single-input nodes
    fChange = 0;
    if ( fSweepConsts || fSweepBuffers )
    { 
        do
        {
            fRecentChange = 0;
            Ntk_NodeForEachFanin( pNode, pPin, pFanin )
            {
                if ( pFanin->Type == MV_NODE_CI )
                    continue;
                if ( fSweepConsts && Ntk_NodeReadFaninNum(pFanin) == 0 )
                {
                    if ( Ntk_NetworkCollapseNodes( pNode, pFanin ) )
                    {
                        fRecentChange = fChange = 1;
                        break;
                    }
                }
                if ( fSweepBuffers && Ntk_NodeReadFaninNum(pFanin) == 1 )
                {
                    if ( Ntk_NetworkCollapseNodes( pNode, pFanin ) )
                    {
                        fRecentChange = fChange = 1;
                        break;
                    }
                }
            }
        }
        while ( fRecentChange );
    }

    return fChange;
}


/**Function*************************************************************

  Synopsis    [I-set sweep.]

  Description [This procedure looks at the i-sets of the node.
  If the node has an empty i-set or a duplicated i-set these are 
  reduced and the fanout nodes are updated accordingly.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Ntk_NetworkSweepIsets( Ntk_Node_t * pNode, bool fVerbose )
{
    Ntk_Node_t * pTrans1, * pTrans2;

    // skip the nodes without functionality
    // TO DO: add here skipping the 'persistent' nodes
    if ( pNode->Type != MV_NODE_INT )
        return 0;
    // value sweep makes no sense for binary nodes
    if ( pNode->nValues == 2 )
        return 0;
    // values cannot be swept if the node fans out directly into a CO
    if ( Ntk_NodeIsCoFanin(pNode) )
        return 0;
    // don't i-set-sweep constant nodes
    if ( Ntk_NodeReadFaninNum(pNode) == 0 )
        return 0;

    // if the i-sets can be cleaned, find the transformer nodes
    // pTrans1 transforms from the node into the reduced set of values
    // pTrans2 transforms from the reduced set of values into the node
    // the first one is useful to collapse the node to be cleaned into it
    // the second one is useful to collapse it into the fanouts
    if ( Ntk_NetworkSweepIsetsDeriveTransformers( pNode, &pTrans1, &pTrans2, fVerbose ) )
    {
        Ntk_NetworkSweepTransformNode( pNode, pTrans1, pTrans2 );        
        // mark the change
        return 1;
    }
    return 0;
}


/**Function*************************************************************

  Synopsis    [Updates the node and its fanouts using the pair of transformer nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkSweepTransformNode( Ntk_Node_t * pNode, Ntk_Node_t * pTrans1, Ntk_Node_t * pTrans2 )
{
    Ntk_Network_t * pNet;
    Ntk_Node_t * pFanout;
    Ntk_Pin_t * pPin;

    pNet = pNode->pNet;
    // add the first transformer to the network to maintain ordering
    Ntk_NetworkAddNode( pNet, pTrans1, 0 );
    // transform the fanouts of pNode to point to pTrans1
    Ntk_NetworkSweepTransformFanouts( pNode, pTrans2 );
    // the node's fanout are now transferred to pTrans1
    assert( Ntk_NodeReadFanoutNum( pNode )   == 0 );
//    assert( Ntk_NodeReadFanoutNum( pTrans2 ) == 0 );
    // the temporary node only remain in the network
    // if the fanouts of pNode include COs
    // but this in impossible (see Ntk_NodeIsCoFanin() above) 
    // therefore, remove the temporary node
//    Ntk_NodeDelete( pTrans2 );
    if ( Ntk_NodeReadFanoutNum( pTrans2 ) )
        Ntk_NetworkAddNode( pNet, pTrans2, 1 );
    else
        Ntk_NodeDelete( pTrans2 );

    // connect pTrans1 fanin pNode
    Ntk_NodeAddFaninFanout( pNet, pTrans1 );
    // collapse the original node into pTrans1
    Ntk_NetworkCollapseNodes( pTrans1, pNode );
    // make sure the name of pNode is transfered
    if ( pNode->pName )
    {
        Ntk_NodeAssignName( pTrans1, Ntk_NodeRemoveName(pNode) );
        // sort the fanouts' fanin lists after the node name has changed
        Ntk_NodeForEachFanout( pTrans1, pPin, pFanout )
            Ntk_NodeOrderFanins( pFanout );
    }
    // delete the orginal node
    Ntk_NetworkDeleteNode( pNet, pNode, 1, 1 );
    // from now on, pTrans1 plays the role of pNode in the network
}


/**Function*************************************************************

  Synopsis    [Derives the transformer nodes for the i-set sweep.]

  Description [Detects empty and indentical i-sets and derives the
  single-input node, which transforms the "clean" output of the node
  into the modified one. Also, modifies the current node.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Ntk_NetworkSweepIsetsDeriveTransformers( Ntk_Node_t * pNode, 
        Ntk_Node_t ** ppTrans1, Ntk_Node_t ** ppTrans2, bool fVerbose )
{
    DdManager * dd;
    Vm_VarMap_t * pVm;
    Mvr_Relation_t * pMvr;
    unsigned PolsOld2New[32], PolsNew2Old[32];
    int IsetReductions[32];
    int nOutputValues, nRemIsets, fFoundEqual, i, k;
    DdNode ** pbIsets;

    // so far, cannot sweep large number of values
    if ( pNode->nValues > 32 ) 
        return 0;

    // get the MV var map of this relation
    pVm = Ntk_NodeReadFuncVm(pNode);
    // get the number of output values
    nOutputValues = Vm_VarMapReadValuesOutput( pVm );
    // allocate room for cofactors
    pbIsets = ALLOC( DdNode *, nOutputValues );
    // derive the i-sets w.r.t the last variable
    pMvr = Ntk_NodeGetFuncMvr(pNode);
    dd = Mvr_RelationReadDd( pMvr );
    Mvr_RelationCofactorsDerive( pMvr, pbIsets, Vm_VarMapReadVarsInNum(pVm), nOutputValues );

    // go through the i-sets and count how many remain
    // additionally, collect additional info about i-set reductions
    // i-set X can be reduced to i-set Y if they are functionally equal
    nRemIsets = 0;
    for ( i = 0; i < nOutputValues; i++ )
        if ( pbIsets[i] != b0 )
        {
            // compare it with the previous ones
            fFoundEqual = 0;
            for ( k = 0; k < i; k++ )
                if ( pbIsets[i] == pbIsets[k] )
                {
                    IsetReductions[i] = IsetReductions[k];
                    fFoundEqual = 1;
                    break;
                }
            if ( !fFoundEqual )
                IsetReductions[i] = nRemIsets++;
        }
        else
            IsetReductions[i] = -1;

    // deref the cofactors
    Mvr_RelationCofactorsDeref( pMvr, pbIsets, Vm_VarMapReadVarsInNum(pVm), nOutputValues );
    FREE( pbIsets );

    // check for the case, when no i-sets can be swept
    if ( nRemIsets == nOutputValues )
    {
        *ppTrans1 = NULL;
        *ppTrans2 = NULL;
        return 0;
    }

    // derive the polarities of the transformer nodes
    for ( i = 0; i < nOutputValues; i++ )
    {
        if ( IsetReductions[i] == -1 )
        {
            PolsOld2New[i] = 0;
            continue;
        }
        PolsOld2New[i]                 = (1 << IsetReductions[i]);
        PolsNew2Old[IsetReductions[i]] = (1 << i);
    }

    // create the transformer nodes
    *ppTrans1 = Ntk_NodeCreateOneInputNode( pNode->pNet, pNode, 
        pNode->nValues, nRemIsets, PolsNew2Old );
    *ppTrans2 = Ntk_NodeCreateOneInputNode( pNode->pNet, *ppTrans1, 
        nRemIsets, pNode->nValues, PolsOld2New );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Collapses the transformer into the fanouts of the node.]

  Description [In some cases, we make some changes to a node and
  would like these changes to be propagated to the node's fanouts.
  Here is one such situation: during the i-set sweep, we have reduced 
  the number of values of a node, but the fanouts of the node still think 
  the node has the old number of values. In this case, it is convenient
  to employ the following strategy: express the change from the new
  node to the old node as a single-input single-output transformer node,
  make the new node (after the value sweep) the only fanin of this transfomer, 
  and collapse the transformer into all the fanouts of the node. This will
  have the effect of (1) updating the functionality of the fanouts and 
  (2) adjusting the fanins of the fanouts to point to the new node rather
  than the old node before the value sweep. This procedure does both.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkSweepTransformFanouts( Ntk_Node_t * pNode, Ntk_Node_t * pTrans )
{
    Ntk_Node_t ** ppFanouts;
    int nFanouts, i;

    // make sure pTrans has the only fanin
    // (this fanin is the node replacing pNode after the transformation)
    assert( Ntk_NodeReadFaninNum(pTrans) == 1 );
    // make sure pTrans has no fanouts
    assert( Ntk_NodeReadFanoutNum(pTrans) == 0 );
    // make sure the transformer some non-zero node ID
    // otherwise, collapsing will not work
//   assert( pTrans->Id > 0 );

    // Temporary, we use a hack here:
    // The nodes are distinquished by name or by node ID, but currently
    // both pNode and pTrans may be not in the network and no name or ID.
    // Here, we assign unrealistically large fake node ID to the transformer,
    // this will help distinguish it from other nodes in Ntk_NodeCollapse()
    pTrans->Id = 1000000;

    // collapse the encoder nodes into the fanout of pNode1
    // store the fanouts of the node
    nFanouts = Ntk_NodeReadFanoutNum( pNode );
    ppFanouts = ALLOC( Ntk_Node_t *, nFanouts );
    nFanouts = Ntk_NodeReadFanouts( pNode, ppFanouts );
    // for each fanout, patch fanin corresponding to pNode1 
    // with pNode1Enc and collapse pNode1Enc into the fanout
    for ( i = 0; i < nFanouts; i++ )
    {
        // patch the fanin of the fanout to point to the transformer node
        Ntk_NodePatchFanin( ppFanouts[i], pNode, pTrans );
        // consider the case when the fanout is a PO
        if ( ppFanouts[i]->Type == MV_NODE_CO )
            continue;
        // collapse pTrans2 into ppFanouts[i]
        Ntk_NetworkCollapseNodes( ppFanouts[i], pTrans );
    }
    // the node's fanout are now transferred to pTrans or its fanin (the new node)
    assert( Ntk_NodeReadFanoutNum( pNode ) == 0 );
    FREE( ppFanouts );
    // at this point, pNode is useless and can be removed from the network
    // (we do not do it here, because it is better to do it in the user's code)
    // typically, the transformer can also be removed
    // however, if among the fanouts there was a CO,
    // we could not collapse pTrans into this CO; in this case, 
    // pTrans remains in the network to account for the tranformation
}


/**Function*************************************************************

  Synopsis    [This network only sweeps the inverters appearing at POs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Ntk_NetworkSweepFpga( Ntk_Network_t * pNet )
{
    Ntk_Node_t * pNode, * pDriver, * pFanin;
    Ntk_NetworkForEachCoDriver( pNet, pNode, pDriver )
    {
        // skip node with the number of inputs other than 1
        if ( Ntk_NodeReadFaninNum(pDriver) != 1 )
            continue;
        // get the fanin of this node
        pFanin = Ntk_NodeReadFaninNode(pDriver, 0);
        if ( pFanin->Type != MV_NODE_INT )
            continue;
        // collapse the fanin into the driver
        Ntk_NetworkCollapseNodes( pDriver, pFanin );
        // delete the node if it has no fanouts
        if ( Ntk_NodeReadFanoutNum( pFanin ) == 0 )
            Ntk_NetworkDeleteNode( pNet, pFanin, 1, 1 );
    }
    return 1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


