/**CFile****************************************************************

  FileName    [ntkList.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Various node manipulation utilities.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ntkNode.c,v 1.44 2005/05/18 04:14:36 alanmi Exp $]

***********************************************************************/

#include "ntkInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Creates a new node with the given name, of the given type.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Node_t * Ntk_NodeCreate( Ntk_Network_t * pNet, char * pName, Ntk_NodeType_t Type, int nValues )
{
    Ntk_Node_t * pNode;
    // get memory to this node
    pNode = (Ntk_Node_t *)Extra_MmFixedEntryFetch( pNet->pManNode );
    // clean the memory
    memset( pNode, 0, sizeof(Ntk_Node_t) );
    // assign the node name and type 
    pNode->Id      = 0;
    pNode->Type    = Type;
    pNode->pName   = pName? Ntk_NetworkRegisterNewName( pNet, pName ): NULL;
    pNode->nValues = nValues;
    pNode->pNet    = pNet;
    pNode->pF      = Fnc_FunctionAlloc();
    pNode->dArrTimeRise = Ntk_NetworkReadDefaultArrTimeRise( pNet );
    pNode->dArrTimeFall = Ntk_NetworkReadDefaultArrTimeFall( pNet );
    // do not add the node to the network
    return pNode;
} 

/**Function*************************************************************

  Synopsis    [Creates the MV constant node.]

  Description [Creates the MV constant node, using memory avaiable from 
  memory manager of the given network (pNet). The number of values (nValues)
  and the polarity (Pol) are given by the user. The polarity has the most
  significant bit coming first. For example to create ternary constant 1, 
  the variables should be set as follows: nValues = 3, Pol = 2 (binary 010).]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Node_t * Ntk_NodeCreateConstant( Ntk_Network_t * pNet, int nValues, unsigned Pol )
{
    Ntk_Node_t * pNode;
    Fnc_Manager_t * pMan;
    Mvr_Relation_t * pMvr;
    // get the node
    pNode = Ntk_NodeCreate( pNet, NULL, MV_NODE_INT, nValues );
    // get the functionality manager
    pMan = Ntk_NodeReadMan( pNode );
    // create the relation
    pMvr = Mvr_RelationCreateConstant( Fnc_ManagerReadManMvr(pMan), Fnc_ManagerReadManVmx(pMan), Fnc_ManagerReadManVm(pMan), nValues, Pol );
    // set the variable map and the relation
    Ntk_NodeWriteFuncVm( pNode, Mvr_RelationReadVm(pMvr) );
    Ntk_NodeWriteFuncMvr( pNode, pMvr );
    return pNode;
}

/**Function*************************************************************

  Synopsis    [Creates the single input MV node.]

  Description [Creates the single input MV node, using memory avaiable from 
  memory manager of the given network (pNet). The number of input and
  output values (nValuesIn, nValuesOut) are given by the user. The polarity 
  is specified for each output value independently. The rule are similar to
  those described in Ntk_NodeCreateConstant(). For example, suppose we would
  like to create a standard ternary buffer. The variables should be set as
  follows: nValuesIn = 3, nValuesOut = 3, Pols[0] = 1, Pols[1] = 2, Pols[2] = 4.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Node_t * Ntk_NodeCreateOneInputNode( Ntk_Network_t * pNet, Ntk_Node_t * pFanin, int nValuesIn, int nValuesOut, unsigned Pols[] )
{
    Ntk_Node_t * pNode;
    Fnc_Manager_t * pMan;
    Mvr_Relation_t * pMvr;
    // get the node
    pNode = Ntk_NodeCreate( pNet, NULL, MV_NODE_INT, nValuesOut );
    // set the fanin (do not connect the fanout to the fanin)
    assert( pFanin == NULL || pFanin->nValues == nValuesIn );
    Ntk_NodeAddFanin( pNode, pFanin );
    // get the functionality manager
    pMan = Ntk_NodeReadMan( pNode );
    // create the relation
    pMvr = Mvr_RelationCreateOneInputOneOutput( Fnc_ManagerReadManMvr(pMan), Fnc_ManagerReadManVmx(pMan), Fnc_ManagerReadManVm(pMan), nValuesIn, nValuesOut, Pols );
    // set the variable map and the relation
    Ntk_NodeWriteFuncVm( pNode, Mvr_RelationReadVm(pMvr) );
    Ntk_NodeWriteFuncMvr( pNode, pMvr );
    return pNode;
}

/**Function*************************************************************

  Synopsis    [Creates one-input binary node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Node_t * Ntk_NodeCreateOneInputBinary( Ntk_Network_t * pNet, Ntk_Node_t * pFanin, bool fInv )
{
    unsigned uPolsBuf[2] = { 1, 2 };
    unsigned uPolsInv[2] = { 2, 1 };
    unsigned * uPols = (fInv? uPolsInv: uPolsBuf);
    return Ntk_NodeCreateOneInputNode( pNet, pFanin, 2, 2, uPols );
}

/**Function*************************************************************

  Synopsis    [Creates two-input binary node.]

  Description [Type is the trouth table of the two-input gate. 
  If Type == 8  (bin=1000), creates two-input AND; 
  if Type == 14 (bin=1110), creates two-input OR.
  Fanins should be ordered.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Node_t * Ntk_NodeCreateTwoInputBinary( Ntk_Network_t * pNet, Ntk_Node_t * ppFanins[], unsigned TruthTable )
{
    Ntk_Node_t * pNode;
    Fnc_Manager_t * pMan;
    Mvr_Relation_t * pMvr;
    // get the node
    pNode = Ntk_NodeCreate( pNet, NULL, MV_NODE_INT, 2 );
    // set the fanin (do not connect the fanout to the fanin)
    Ntk_NodeAddFanin( pNode, ppFanins[0] );
    Ntk_NodeAddFanin( pNode, ppFanins[1] );
    // get the functionality manager
    pMan = Ntk_NodeReadMan( pNode );
    // create the relation
    pMvr = Mvr_RelationCreateTwoInputBinary( Fnc_ManagerReadManMvr(pMan), Fnc_ManagerReadManVmx(pMan), Fnc_ManagerReadManVm(pMan), TruthTable );
    // set the variable map and the relation
    Ntk_NodeWriteFuncVm( pNode, Mvr_RelationReadVm(pMvr) );
    Ntk_NodeWriteFuncMvr( pNode, pMvr );
    return pNode;
}

/**Function*************************************************************

  Synopsis    [Creates binary OR node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Node_t * Ntk_NodeCreateSimpleBinary( Ntk_Network_t * pNet, Ntk_Node_t * ppFanins[], int pCompls[], int nFanins, int fGateOr )
{
    Ntk_Node_t * pNode;
    Fnc_Manager_t * pMan;
    Mvr_Relation_t * pMvr;
    int i;
    // get the node
    pNode = Ntk_NodeCreate( pNet, NULL, MV_NODE_INT, 2 );
    // set the fanin (do not connect the fanout to the fanin)
    for ( i = 0; i < nFanins; i++ )
        Ntk_NodeAddFanin( pNode, ppFanins[i] );
    // get the functionality manager
    pMan = Ntk_NodeReadMan( pNode );
    // create the relation
    pMvr = Mvr_RelationCreateSimpleBinary( Fnc_ManagerReadManMvr(pMan), Fnc_ManagerReadManVmx(pMan), Fnc_ManagerReadManVm(pMan), pCompls, nFanins, fGateOr );
    // set the variable map and the relation
    Ntk_NodeWriteFuncVm( pNode, Mvr_RelationReadVm(pMvr) );
    Ntk_NodeWriteFuncMvr( pNode, pMvr );
    return pNode;
}

/**Function*************************************************************

  Synopsis    [Creates a binary MUX.]

  Description [The first node is the control variable, the second
  nodes is the positive cofactor, the third node is the negative cofactor.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Node_t * Ntk_NodeCreateMuxBinary( Ntk_Network_t * pNet, Ntk_Node_t * pNode1, Ntk_Node_t * pNode2, Ntk_Node_t * pNode3 )
{
    Ntk_Node_t * pNode;
    Fnc_Manager_t * pMan;
    Mvr_Relation_t * pMvr;
    // get the node
    pNode = Ntk_NodeCreate( pNet, NULL, MV_NODE_INT, 2 );
    // set the fanin (do not connect the fanout to the fanin)
    Ntk_NodeAddFanin( pNode, pNode1 );
    Ntk_NodeAddFanin( pNode, pNode2 );
    Ntk_NodeAddFanin( pNode, pNode3 );
    // get the functionality manager
    pMan = Ntk_NodeReadMan( pNode );
    // create the relation
    pMvr = Mvr_RelationCreateMuxBinary( Fnc_ManagerReadManMvr(pMan), Fnc_ManagerReadManVmx(pMan), Fnc_ManagerReadManVm(pMan) );
    // set the variable map and the relation
    Ntk_NodeWriteFuncVm( pNode, Mvr_RelationReadVm(pMvr) );
    Ntk_NodeWriteFuncMvr( pNode, pMvr );
    return pNode;
}

/**Function*************************************************************

  Synopsis    [Creates the decode for the two nodes.]

  Description [Creates a two-input node that works like a decoder for the two
  nodes. The created node is internal and is not added to the network.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Node_t * Ntk_NodeCreateDecoder( Ntk_Network_t * pNet, Ntk_Node_t * pNode1, Ntk_Node_t * pNode2 )
{
    Ntk_Node_t * pNode;
    Fnc_Manager_t * pMan;
    Mvr_Relation_t * pMvr;
    // get the node
    pNode = Ntk_NodeCreate( pNet, NULL, MV_NODE_INT, pNode1->nValues * pNode2->nValues );
    // set the fanin (do not connect the fanout to the fanin)
    Ntk_NodeAddFanin( pNode, pNode1 );
    Ntk_NodeAddFanin( pNode, pNode2 );
    // get the functionality manager
    pMan = Ntk_NodeReadMan( pNode );
    // create the relation
    pMvr = Mvr_RelationCreateDecoder( Fnc_ManagerReadManMvr(pMan), Fnc_ManagerReadManVmx(pMan), Fnc_ManagerReadManVm(pMan), pNode1->nValues, pNode2->nValues );
    // set the variable map and the relation
    Ntk_NodeWriteFuncVm( pNode, Mvr_RelationReadVm(pMvr) );
    Ntk_NodeWriteFuncMvr( pNode, pMvr );
    return pNode;
}

/**Function*************************************************************

  Synopsis    [Creates the decoder for several binary nodes.]

  Description [This procedure does not delete or remove these nodes
  from the network. The resulting node is not in the network.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Node_t *
Ntk_NodeCreateDecoderGeneral(
    Ntk_Node_t ** ppNodes, // the set of (binary) input nodes
    int   nNodes,          // size of the previous array 
    int * pValueAssign,    // pValueAssign[i] = j, if the ith value 
                           // (spanned by ppNodes) is assigned to output 
                           // value j. 
                           // pValueAssign[i] = -1, if the ith value is 
                           // unused. 
    int   nTotalValues,    // size of the previous array (= 2^{nNodes})
    int   nOutputValues)   // num of values of the original MV node
{
    Vm_VarMap_t * pVm;
    Mvr_Relation_t * pMvr;
    Ntk_Network_t * pNet;
    Ntk_Node_t * pNodeDec;
    int i;

    // make sure the input parameters are reasonable
    assert( nNodes > 0 );
    assert( (1<<nNodes) == nTotalValues );
    assert( nOutputValues <= nTotalValues );
    assert( nTotalValues <= 32 );

    // make sure the codes are within a proper range
    for ( i = 0; i < nTotalValues; i++ )
        assert( pValueAssign[i] >= -1 && pValueAssign[i] < nOutputValues );

    // create the decoder node
    pNet = ppNodes[0]->pNet;
    pNodeDec = Ntk_NodeCreate( pNet, NULL, MV_NODE_INT, nOutputValues );
    // add fanins
    for ( i = 0; i < nNodes; i++ )
        Ntk_NodeAddFanin( pNodeDec, ppNodes[i] );
 
    // create the relation
    pMvr = Mvr_RelationCreateDecoderGeneral( 
               Fnc_ManagerReadManMvr(pNet->pMan), 
               Fnc_ManagerReadManVmx(pNet->pMan), 
               Fnc_ManagerReadManVm(pNet->pMan),
               nNodes, pValueAssign, nTotalValues, nOutputValues );

    // get hold of Vm
    pVm = Mvr_RelationReadVm(pMvr);

    // set the map and the relation
    Ntk_NodeWriteFuncVm(pNodeDec, pVm);
    Ntk_NodeWriteFuncMvr(pNodeDec, pMvr);
    return pNodeDec;
}

/**Function*************************************************************

  Synopsis    [Encodes the given MV node using the provided coding.]

  Description [This procedure takes an MV-output node and derives
  a set of binary nodes according to the provided coding. The MV 
  node is not deleted or removed from the network. The returned binary 
  nodes are not in the network.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodeCreateEncoded( 
    Ntk_Node_t * pNode,    // the MV node to be encoded
    Ntk_Node_t ** ppNodes, // returned binary nodes
    int   nNodes,          // size of the previous array 
    int * pValueAssign,    // pValueAssign[i] = j, if the ith value 
                           // (spanned by ppNodes) is assigned to 
                           // output value j. 
                           // pValueAssign[i] = -1, if the ith value 
                           // is unused. 
    int   nTotalValues,    // size of the previous array (= 2^{nNodes})
    int   nOutputValues)   // num of values of the original MV node
{
    Vm_VarMap_t * pVm;
    Mvr_Relation_t * ppMvrs[10];
    Ntk_Network_t * pNet;
    Ntk_Node_t * pNodeEnc0, * pNodeEnc, * pFanin;
    Ntk_Pin_t * pPin;
    int i;

    // make sure the input parameters are reasonable
    assert( nNodes > 0 );
    assert( (1<<nNodes) == nTotalValues );
    assert( nOutputValues <= nTotalValues );
    assert( nTotalValues <= 32 );

    // make sure the codes are within a proper range
    for ( i = 0; i < nTotalValues; i++ )
        assert( pValueAssign[i] >= -1 && pValueAssign[i] < nOutputValues );

    // create the prototype of encoded node
    pNet = pNode->pNet;
    pNodeEnc0 = Ntk_NodeCreate( pNet, NULL, MV_NODE_INT, 2 );
    // create the same fanins as the original node
    Ntk_NodeForEachFanin( pNode, pPin, pFanin )
        Ntk_NodeAddFanin( pNodeEnc0, pFanin );
 
    // create the relations
    Mvr_RelationCreateEncoded( Fnc_ManagerReadManMvr(pNet->pMan), 
        Fnc_ManagerReadManVmx(pNet->pMan), Fnc_ManagerReadManVm(pNet->pMan),
        nNodes, pValueAssign, nTotalValues, nOutputValues, 
        ppMvrs, Ntk_NodeReadFuncMvr(pNode) );

    // get hold of Vm
    pVm = Mvr_RelationReadVm(ppMvrs[0]);

    // create the encoded nodes
    for ( i = 0; i < nNodes; i++ )
    {
        // replicate the node (use pNodeEnc0 for the last one)
        if ( i == nNodes - 1 )
            pNodeEnc = pNodeEnc0;
        else
            pNodeEnc = Ntk_NodeDup( pNet, pNodeEnc0 );
        // set the map and the relation
        Ntk_NodeWriteFuncVm(pNodeEnc, pVm);
        Ntk_NodeWriteFuncMvr(pNodeEnc, ppMvrs[i]);
        // assign the node
        ppNodes[i] = pNodeEnc;
    }
}


/**Function*************************************************************

  Synopsis    [Creates two encoders for the given node.]

  Description [Creates two single-input encoders, which have the input
  equal to the given node (pNode) and output values (nValues1 and nValues2).
  It is assumed that pNode1 encodes less significant bits.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodeCreateEncoders( Ntk_Network_t * pNet, Ntk_Node_t * pNode, int nValues1, int nValues2,
    Ntk_Node_t ** ppNode1, Ntk_Node_t ** ppNode2 )
{
    unsigned Pols1[32], Pols2[32];
    int i;

    assert( nValues1 * nValues2 == pNode->nValues );
    assert( pNode->nValues <= 32 );

    // create the polarities of the single-input nodes
    memset( Pols1, 0, sizeof(unsigned) * nValues1 );
    memset( Pols2, 0, sizeof(unsigned) * nValues2 );
    for ( i = 0; i < pNode->nValues; i++ )
    {
        Pols1[i / nValues2] |= (1<<i);
        Pols2[i % nValues2] |= (1<<i);
    }

    // create the decoders
    *ppNode1 = Ntk_NodeCreateOneInputNode( pNet, pNode, pNode->nValues, nValues1, Pols1 );
    *ppNode2 = Ntk_NodeCreateOneInputNode( pNet, pNode, pNode->nValues, nValues2, Pols2 );
}

/**Function*************************************************************

  Synopsis    [Creates the collector node.]

  Description [The collector node is the n-valued node, which has 
  n binary fanins, which are converted into the corresponding i-sets.
  The fanins should be ordered.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Node_t * Ntk_NodeCreateCollector( Ntk_Network_t * pNet, Ntk_Node_t * ppFanins[], int nValues, int DefValue )
{
    Ntk_Node_t * pNode;
    Fnc_Manager_t * pMan;
    Mvr_Relation_t * pMvr;
    int i;
    // get the node
    pNode = Ntk_NodeCreate( pNet, NULL, MV_NODE_INT, nValues );
    // set the fanin (do not connect the fanout to the fanin)
    for ( i = 0; i < nValues; i++ )
        if ( ppFanins[i] )
            Ntk_NodeAddFanin( pNode, ppFanins[i] );
        else
        {
            assert( i == DefValue );
        }
    // get the functionality manager
    pMan = Ntk_NodeReadMan( pNode );
    // create the relation
    pMvr = Mvr_RelationCreateCollector( Fnc_ManagerReadManMvr(pMan), Fnc_ManagerReadManVmx(pMan), Fnc_ManagerReadManVm(pMan), nValues, DefValue );
    // set the variable map and the relation
    Ntk_NodeWriteFuncVm( pNode, Mvr_RelationReadVm(pMvr) );
    Ntk_NodeWriteFuncMvr( pNode, pMvr );
    return pNode;
}

/**Function*************************************************************

  Synopsis    [Creates the selector node.]

  Description [The selector node is the binary node, which is equal to
  1 when the corresponding i-set of the given MV node is equal to 1.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Node_t * Ntk_NodeCreateSelector( Ntk_Network_t * pNet, Ntk_Node_t * pNode, int iSet )
{
    unsigned Pols[2];

    Pols[1] = (1 << iSet);
    Pols[0] = FT_MV_MASK(pNode->nValues) ^ Pols[1];

    return Ntk_NodeCreateOneInputNode( pNet, pNode, pNode->nValues, 2, Pols );
}

/**Function*************************************************************

  Synopsis    [Creates the binary node equal to an i-set of the given node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Node_t * Ntk_NodeCreateIset( Ntk_Network_t * pNet, Ntk_Node_t * pNode, int iSet )
{
    Ntk_Node_t * pNodeNew;
    Vm_VarMap_t * pVm, * pVmNew;
    Cvr_Cover_t * pCvr, * pCvrNew;
    Mvc_Cover_t ** ppIsets, ** ppIsetsNew;

    assert( pNode->Type == MV_NODE_INT );
    pNodeNew = Ntk_NodeClone( pNet, pNode );
    pNodeNew->pName = NULL;
    pNodeNew->nValues = 2;

    // create the var map and cover
    pVm  = Ntk_NodeReadFuncVm( pNode );
    pCvr = Ntk_NodeGetFuncCvr( pNode );
    ppIsets = Cvr_CoverReadIsets( pCvr );

    pVmNew = Vm_VarMapCreateOneDiff( pVm, Ntk_NodeReadFaninNum(pNode), 2 );
    ppIsetsNew = ALLOC( Mvc_Cover_t *, 2 );
    ppIsetsNew[0] = NULL;
    ppIsetsNew[1] = ppIsets[iSet]; 
    pCvrNew = Cvr_CoverCreate( pVmNew, ppIsetsNew );

    // set the new functionality
    Ntk_NodeWriteFuncVm( pNodeNew, pVmNew );
    Ntk_NodeWriteFuncCvr( pNodeNew, pCvrNew );
    return pNodeNew;
}

/**Function*************************************************************

  Synopsis    [Creates the node with fanins being the network CIs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Node_t * Ntk_NodeCreateFromNetwork( Ntk_Network_t * pNet, char ** psCiNames )
{
    Ntk_Node_t * pNodeNew, * pFanin;
    int nCis, i;
    // create a new node
    pNodeNew = (Ntk_Node_t *)Ntk_NodeCreate( pNet, NULL, MV_NODE_INT, -1 );
    // create the fanin list
    if ( psCiNames )
    {
        nCis = Ntk_NetworkReadCiNum( pNet );
        for ( i = 0; i < nCis; i++ )
        {
            pFanin = Ntk_NetworkFindNodeByName( pNet, psCiNames[i] );
            assert( pFanin );
            Ntk_NodeAddFanin( pNodeNew, pFanin );
        }
    }
    else
    {
        Ntk_NetworkForEachCi( pNet, pFanin )
            Ntk_NodeAddFanin( pNodeNew, pFanin );
    }
    return pNodeNew;
}

/**Function*************************************************************

  Synopsis    [Duplicates the node.]

  Description [Duplicates the node. Copies the fanin list but does not
  create the fanout list. Copies the local function, assuming the same
  manager is used.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Node_t * Ntk_NodeDup( Ntk_Network_t * pNetNew, Ntk_Node_t * pNode )
{
    Ntk_Node_t * pNodeNew, * pFanin;
    Fnc_Function_t * pF, * pFNew;
    Ntk_Pin_t * pPin;
    if ( pNode == NULL )
        return NULL;
    // create a new node
    pNodeNew = (Ntk_Node_t *)Ntk_NodeCreate( pNetNew, pNode->pName, pNode->Type, pNode->nValues );
    // copy the subtype
    pNodeNew->Subtype = pNode->Subtype;
    // duplicate the fanin list
    Ntk_NodeForEachFanin( pNode, pPin, pFanin )
        Ntk_NodeAddFanin( pNodeNew, pFanin );
    // copy the functionality
    pF    = Ntk_NodeReadFunc( pNode );
    pFNew = Ntk_NodeReadFunc( pNodeNew );
    Fnc_FunctionDup( pNode->pNet->pMan, pNetNew->pMan, pF, pFNew ); 
    // attach the new node to the temporary place in the node
    pNode->pCopy = pNodeNew;
    pNodeNew->dArrTimeRise = pNode->dArrTimeRise;
    pNodeNew->dArrTimeFall = pNode->dArrTimeFall;
    return pNodeNew;
}

/**Function*************************************************************

  Synopsis    [Duplicates the node without functionality.]

  Description [Duplicates the node. Copies the fanin list but does not
  create the fanout list. Does not copy the local function.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Node_t * Ntk_NodeClone( Ntk_Network_t * pNetNew, Ntk_Node_t * pNode )
{
    Ntk_Node_t * pNodeNew, * pFanin;
    Ntk_Pin_t * pPin;
    if ( pNode == NULL )
        return NULL;
    // create a new node
    pNodeNew = (Ntk_Node_t *)Ntk_NodeCreate( pNetNew, pNode->pName, pNode->Type, pNode->nValues );
    // copy the subtype
    pNodeNew->Subtype = pNode->Subtype;
    // duplicate the fanin list
    Ntk_NodeForEachFanin( pNode, pPin, pFanin )
        Ntk_NodeAddFanin( pNodeNew, pFanin );
    // attach the new node to the temporary place in the node
    pNode->pCopy = pNodeNew;
    return pNodeNew;
}

/**Function*************************************************************

  Synopsis    [Replaces pNode by pNodeNew in the network.]

  Description [This procedure replaces pNode by pNodeNew, and
  updates the fanout structures of the fanins to point to pNodeNew
  instead of pNode. This procedure disposes of pNodeNew.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodeReplace( Ntk_Node_t * pNode, Ntk_Node_t * pNodeNew )
{
    Fnc_Function_t * pF;
    Ntk_Network_t * pNet;
    Ntk_Pin_t * pPin, * pPin2;
    Ntk_Node_t * pTemp;

    assert( pNode->Type == MV_NODE_INT && pNodeNew->Type == MV_NODE_INT );
//    assert( pNode->nValues == pNodeNew->nValues );
//    assert( Ntk_NodeReadFanoutNum(pNode) > 0 );
    assert( Ntk_NodeReadFanoutNum(pNodeNew) == 0 );

    // set the new number of values
    pNode->nValues = pNodeNew->nValues;
    // delete the fanout structures of the fanins of "pNode"
    pNet = pNode->pNet;
    Ntk_NodeDeleteFaninFanout( pNet, pNode );
    // dispose of the fanin pins of pNode
    Ntk_NodeForEachFaninSafe( pNode, pPin, pPin2, pTemp )
        Extra_MmFixedEntryRecycle( pNet->pManPin, (char *)pPin );
    // transfer the fanin list and the functionality from pNodeNew to pNode
    pNode->lFanins = pNodeNew->lFanins;
    // no need to clean, because will be deallocated
    // invalidate the old functionality
    pF = Ntk_NodeReadFunc( pNode );
    Fnc_FunctionClean( pF );     // invalidate all
    // set the new functionality
    Fnc_FunctionCopy( pNode->pF, pNodeNew->pF );
    // no need to clean, because will be deallocated
    // create the fanin fanout structures
    Ntk_NodeAddFaninFanout( pNet, pNode );
    // dispose of the node
    FREE( pNodeNew->pF );
    Extra_MmFixedEntryRecycle( pNet->pManNode, (char *)pNodeNew );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodeDelete( Ntk_Node_t * pNode )
{
    Ntk_NodeFreeBinding( pNode );
 //   assert( pNode->pNet == NULL || pNode->Id < pNode->pNet->nIdsAlloc );
    // deref the functionality
    Fnc_FunctionDelete( pNode->pF );
    Fnc_GlobalFree( pNode->pG );
    // recycle the node
    Ntk_NodeRecycle( pNode->pNet, pNode );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodeRecycle( Ntk_Network_t * pNet, Ntk_Node_t * pNode )
{
    Ntk_Pin_t * pPin, * pPin2;
    Ntk_Node_t * pTemp;
    Ntk_NodeForEachFaninSafe( pNode, pPin, pPin2, pTemp )
        Extra_MmFixedEntryRecycle( pNet->pManPin, (char *)pPin );
    Ntk_NodeForEachFanoutSafe( pNode, pPin, pPin2, pTemp )
        Extra_MmFixedEntryRecycle( pNet->pManPin, (char *)pPin );
    Extra_MmFixedEntryRecycle( pNet->pManNode, (char *)pNode );
}

/**Function********************************************************************

  Synopsis    [a quick check on if the node is too large.]

  Description [used in merge and simplify.]
  
  SideEffects []

  SeeAlso     []

******************************************************************************/
bool Ntk_NodeIsTooLarge( Ntk_Node_t * pNode ) 
{
    Fnc_Function_t * pF;
    Mvc_Cover_t ** pCovers;
    Cvr_Cover_t * pCvr;
    int nValues, nCubes, nBits, i;

    if ( pNode == NULL )
        return FALSE;

    pF   = Ntk_NodeReadFunc( pNode );
    pCvr = Fnc_FunctionReadCvr( pF );
    pCovers = Cvr_CoverReadIsets( pCvr );

    if ( pCovers == NULL )
        return FALSE;

    nValues = Ntk_NodeReadValueNum( pNode );
    nBits   = Vm_VarMapReadValuesInNum( Fnc_FunctionReadVm(pF) );

    for ( i = 0; i < nValues; i++ )
    {
        if ( pCovers[i] == NULL )
            continue;
        nCubes = Mvc_CoverReadCubeNum(pCovers[i]);
        if ( nCubes >= 10  && nBits >= 90 ) return TRUE;
        if ( nCubes >= 100 && nBits >= 60 ) return TRUE;
        if ( nCubes >= 400 && nBits >= 50 ) return TRUE;
        if ( nCubes >= 800 && nBits >= 30 ) return TRUE;
        //if ( nCubes >= 900 && nBits >= 20 ) continue;
    }
    return FALSE;
}

/**Function*************************************************************

  Synopsis    [Returns true if the node is the binary buffer.]

  Description []
               
  SideEffects []

  SeeAlso     []
 
***********************************************************************/
bool Ntk_NodeIsBinaryBuffer( Ntk_Node_t * pNode )
{
    Cvr_Cover_t * pCvr;
    Mvc_Cover_t ** ppIsets;
    if ( Ntk_NodeReadValueNum(pNode) != 2 )
        return 0;
    if ( Ntk_NodeReadFaninNum(pNode) != 1 )
        return 0;
//    pCvr = Ntk_NodeReadFuncCvr(pNode);
    pCvr = Ntk_NodeGetFuncCvr(pNode);
    if ( pCvr == NULL )
        return 0;
    ppIsets = Cvr_CoverReadIsets(pCvr);
    if ( ppIsets[1] == NULL )
        return 0;
    if ( ppIsets[0] != NULL )
        return 0;
    return Mvc_CoverIsBinaryBuffer( ppIsets[1] );
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the nodes are not in the TFI/TFO cones of each other.]

  Description []
               
  SideEffects []

  SeeAlso     []
 
***********************************************************************/
bool Ntk_NodesCanBeMerged( Ntk_Node_t * pNode1, Ntk_Node_t * pNode2 )
{
    Ntk_Node_t * pNodeSpec;

    // get the TFI of pNode1
    Ntk_NetworkComputeNodeTfi( pNode1->pNet, &pNode1, 1, 10000, 1, 1 );
    // check if pNodes2 is among these nodes
    Ntk_NetworkForEachNodeSpecial( pNode1->pNet, pNodeSpec )
        if ( pNodeSpec == pNode2 )
            return 0;

    // go the same with the TFO of pNode1
    Ntk_NetworkComputeNodeTfo( pNode1->pNet, &pNode1, 1, 10000, 1, 1 );
    // check if pNodes2 is among these nodes
    Ntk_NetworkForEachNodeSpecial( pNode1->pNet, pNodeSpec )
        if ( pNodeSpec == pNode2 )
            return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Determinizes the node.]

  Description [Returns 1 if the node was determinized; 0 if it was
  already deterministic.]
               
  SideEffects []

  SeeAlso     []
 
***********************************************************************/
bool Ntk_NodeDeterminize( Ntk_Node_t * pNode )
{
    Mvc_Data_t * pData;
    Cvr_Cover_t * pCvr;
    int Default;
    Mvr_Relation_t * pMvr;
    Mvc_Cover_t ** ppIsets, * pMvcTemp;
    int fIsNd;

    fIsNd = 0;
    if ( ( pCvr = Ntk_NodeReadFuncCvr( pNode ) ) )
    {
        // get any cover of pCvrFshift
        Default  = Cvr_CoverReadDefault(pCvr);
        ppIsets  = Cvr_CoverReadIsets(pCvr);
        pMvcTemp = (Default != 0)? ppIsets[0]: ppIsets[1];

        // get the MV data for the new man
        pData = Mvc_CoverDataAlloc( Ntk_NodeReadFuncVm(pNode), pMvcTemp );
        fIsNd = Cvr_CoverIsND( pData, pCvr );
        Mvc_CoverDataFree( pData, pMvcTemp );
        if ( fIsNd )
        {
            pMvr = Ntk_NodeGetFuncMvr( pNode );
            Mvr_RelationDeterminize(pMvr);
            Ntk_NodeSetFuncMvr( pNode, pMvr );
        }
    }
    else
    {
        pMvr = Ntk_NodeGetFuncMvr( pNode );
        fIsNd = Mvr_RelationDeterminize(pMvr);
        if ( fIsNd )
            Ntk_NodeSetFuncMvr( pNode, pMvr );
    }
    return fIsNd;
}

/**Function*************************************************************

  Synopsis    [Creates the mapping binding for the node.]

  Description [Binds the node pNode to the gate pGate by constructing 
  the binding structure that stores which fanins of pNode map to which 
  inputs of the gate pGate. We need to store this information separately 
  as it may get lost on fanin re-ordering. ]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_NodeBinding_t * Ntk_NodeBindingCreate( char * pGate, Ntk_Node_t * pNode, float Arrival )
{
    Ntk_NodeBinding_t * pBinding;
    Ntk_Node_t *pFanIn;
    Ntk_Pin_t * pPin;
    int nFanins, i;
   
    nFanins = Ntk_NodeReadFaninNum( pNode );
    pBinding = ALLOC ( Ntk_NodeBinding_t, 1 );
    pBinding->iSignature = 0xDEADBEEF;
    pBinding->pGate = pGate;
    pBinding->Arrival = Arrival;
    pBinding->ppFaninOrder = ALLOC ( Ntk_Node_t *, nFanins );
    i = 0;
    Ntk_NodeForEachFanin( pNode, pPin, pFanIn )
        pBinding->ppFaninOrder[i++] = pFanIn;
    assert ( i == nFanins );
    return pBinding;
}

/**Function*************************************************************

  Synopsis    [Reads the gate from the binding.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Ntk_NodeBindingReadGate( Ntk_NodeBinding_t * pBinding )
{
    assert ( pBinding && pBinding->iSignature == 0xDEADBEEF );
    return (char *)pBinding->pGate;
}

/**Function*************************************************************

  Synopsis    [Reads the arrival time from the binding.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Ntk_NodeBindingReadArrival( Ntk_NodeBinding_t * pBinding )
{
    assert ( pBinding && pBinding->iSignature == 0xDEADBEEF );
    return pBinding->Arrival;
}

/**Function*************************************************************

  Synopsis    [Reads the node order array from the binding.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Node_t ** Ntk_NodeBindingReadFanInArray( Ntk_NodeBinding_t * pBinding )
{
    assert ( pBinding && pBinding->iSignature == 0xDEADBEEF );
    return pBinding->ppFaninOrder;
}

/**Function*************************************************************

  Synopsis    [Frees the node's binding.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodeFreeBinding( Ntk_Node_t * pNode )
{
    if ( pNode->pMap )
    {
        free( pNode->pMap->ppFaninOrder );
        free( pNode->pMap );
    }
    pNode->pMap = NULL;
}

/**Function*************************************************************

  Synopsis    [Frees the network's bindings.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkFreeBinding( Ntk_Network_t * pNet )
{
    Ntk_Node_t * pNode;
    Ntk_NetworkForEachNode( pNet, pNode )
        Ntk_NodeFreeBinding( pNode );
}

/**Function*************************************************************
 
  Synopsis    [Checks that all nodes the network are bound]
 
  Description []
 
  SideEffects [Changes the network structure.]
 
  SeeAlso     []

***********************************************************************/
int Ntk_NetworkCheckAllBound( Ntk_Network_t * pNet, char *pInfo )
{
    Ntk_Node_t * pNodeNew, * pExample = 0;
    int unmapped = 0;
    Ntk_NetworkForEachNode( pNet, pNodeNew )
    {
        if ( pNodeNew->pMap == 0)
        {
            pExample = pNodeNew;
            unmapped++;
        }
    }
    if (unmapped > 0 )
    {
        assert ( pExample );
        fprintf (stderr, "Warning (%s): %d nodes unmapped (for e.g. %s)\n", pInfo, unmapped, Ntk_NodeGetNameLong(pExample));
    }

    return unmapped;
}

/**Function*************************************************************

  Synopsis    [Returns the delay of the given node in terms of and-gate delay]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_NodeGetArrivalTimeDiscrete( Ntk_Node_t * pNode, double dAndGateDelay )
{
    double dRise = Ntk_NodeReadArrTimeRise( pNode );
    double dFall = Ntk_NodeReadArrTimeFall( pNode );
    double dDelay = dRise > dFall ? dRise : dFall;
    return (int) floor( dDelay / dAndGateDelay + 0.5 );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


