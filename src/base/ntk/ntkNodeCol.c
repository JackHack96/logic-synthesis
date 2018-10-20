/**CFile****************************************************************

  FileName    [ntkCollapse.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Procedures to perform collapsing of two nodes.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ntkNodeCol.c,v 1.25 2003/11/18 18:54:54 alanmi Exp $]

***********************************************************************/

#include "ntkInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static Ntk_Node_t * Ntk_NodeCollapseSpecialCase( Ntk_Node_t * pNode, Ntk_Node_t * pFanin );
static Ntk_Node_t * Ntk_NodeCombineFanins( Ntk_Node_t * pNode, Ntk_Node_t * pFanin, 
     int * pTransMapNInv, int * pTransMapF );


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Collapses the fanin into the node.]

  Description [Derives the node, which is the result of substitution of 
  pFanin into pNode. Makes this node minimum base and sorts the nodes
  fanins to be in the accepted fanin order. Replaces pNode by the new
  node in the network. Leaves pFanin unchanged and does not remove it
  from the network, even if its only fanout was pNode.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Ntk_NetworkCollapseNodes( Ntk_Node_t * pNode, Ntk_Node_t * pFanin )
{
    Ntk_Node_t * pNodeNew;
    // derive the collapsed node 
    // this node is not minimum-base and is not in the network
    pNodeNew = Ntk_NodeCollapse( pNode, pFanin );
    if ( pNodeNew == NULL )
        return 0;
    // make the node minimum-base
    Ntk_NodeMakeMinimumBase( pNodeNew );
    // replace the old node with the new one
    Ntk_NodeReplace( pNode, pNodeNew ); // disposes of pNodeNew
    return 1;
}

/**Function*************************************************************

  Synopsis    [Eliminates the node into its fanouts.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkCollapseNode( Ntk_Network_t * pNet, Ntk_Node_t * pNode )
{
    Ntk_Node_t * pFanout;
    Ntk_Pin_t * pPin, * pPin2;

    // for each fanout of the node, collapse the node into the fanout
    Ntk_NodeForEachFanoutSafe( pNode, pPin, pPin2, pFanout )
    {
        if ( Ntk_NodeIsCo( pFanout ) )
            continue;
        // collapse the node
        Ntk_NetworkCollapseNodes( pFanout, pNode );
    }

    // delete the node if it has no fanouts
    if ( Ntk_NodeReadFanoutNum( pNode ) == 0 )
        Ntk_NetworkDeleteNode( pNet, pNode, 1, 1 );
}


/**Function*************************************************************

  Synopsis    [Derives the collapsed node.]

  Description [Derives the collapsed node without making it minimum base
  and without adding it to the network. This node contains the relation 
  after collapsing, which can be used to approximate the complexity of 
  the collapsed node, for example, during eliminate.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Node_t * Ntk_NodeCollapse( Ntk_Node_t * pNode, Ntk_Node_t * pFanin )
{
    Ntk_Node_t * pNodeNew;
    Vm_VarMap_t * pVm, * pVmNew;
    Mvr_Relation_t * pMvrN, * pMvrF, * pMvrNew;
    Cvr_Cover_t * pCvrN, * pCvrF, * pCvrNew;
    int * pTransMapNInv, * pTransMapF;

    // check conditions when collapsing makes no sense
    if ( Ntk_NodeReadFaninIndex( pNode, pFanin ) == -1 )
        return NULL;
    if ( pNode->Type != MV_NODE_INT || pFanin->Type != MV_NODE_INT )
        return NULL;

    // consider the special case when pFanin is a constant or a buffer
    // and both nodes are binary and have SOP representation available
    if ( pNodeNew = Ntk_NodeCollapseSpecialCase( pNode, pFanin ) )
        return pNodeNew;

    // get the temporary storage for "translation maps"
    pVm           = Ntk_NodeReadFuncVm( pNode );   
    pTransMapNInv = Vm_VarMapGetStorageSupport1( pVm );
    pTransMapF    = Vm_VarMapGetStorageSupport2( pVm );

    // derive the collapsed node; fill the translation maps 
    pNodeNew = Ntk_NodeCombineFanins( pNode, pFanin, pTransMapNInv, pTransMapF );
    pVmNew  = Ntk_NodeReadFuncVm( pNodeNew );   

    // consider the case when both Cvrs are available
    // in this case, it is better to use the SOP representation for collapsing
    pCvrN = Ntk_NodeReadFuncCvr( pNode );
    pCvrF = Ntk_NodeReadFuncCvr( pFanin );
    if ( pCvrN && pCvrF )
    { 
        // derive the relation of the collapsed node
        pCvrNew = Cvr_CoverCreateCollapsed( pCvrN, pCvrF, pVmNew, pTransMapNInv, pTransMapF );
        // set the new relation at the new node
        Ntk_NodeWriteFuncCvr( pNodeNew, pCvrNew );
    }
    else // the SOPs are not available, use BDDs
    {
        pMvrN = Ntk_NodeGetFuncMvr( pNode );
        pMvrF = Ntk_NodeGetFuncMvr( pFanin );
        // derive the relation of the collapsed node
        pMvrNew = Mvr_RelationCreateCollapsed( pMvrN, pMvrF, pVmNew, pTransMapNInv, pTransMapF );
        // set the new relation at the new node
        Ntk_NodeWriteFuncMvr( pNodeNew, pMvrNew );
    }
    return pNodeNew;
}

/**Function*************************************************************

  Synopsis    [Derives the collapsed node when the fanin is a simple node.]

  Description [Derives the collapsed node without making iy minimum base
  and without adding it to the network. Considers the special case when
  pFanin is an inverter or a buffer, and when both the node and the fanin 
  are binary nodes with SOPs derived.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Node_t * Ntk_NodeCollapseSpecialCase( Ntk_Node_t * pNode, Ntk_Node_t * pFanin )
{
    Ntk_Node_t * pNodeNew, * pFaninFanin;
    Cvr_Cover_t * pCvrNode, * pCvrFanin, * pCvrNew;
    Mvc_Cover_t * pMvcNode, * pMvcFanin, * pMvcNew;
    Mvc_Cover_t ** pIsetsNew;
    Vm_VarMap_t * pVm;
    int DefNode, DefFanin;
    int VarNum,  ValueFirst;
    int VarNumF, ValueFirstF;
    bool fNeedToPatchFanin;

    if ( pNode->nValues != 2 || pFanin->nValues != 2 )
        return NULL;
    if ( Ntk_NodeReadFaninNum(pFanin) > 1 )
        return NULL;

    pCvrNode  = Ntk_NodeReadFuncCvr(pNode);
    pCvrFanin = Ntk_NodeReadFuncCvr(pFanin);
    if ( pCvrNode == NULL || pCvrFanin == NULL )
        return NULL;

    DefNode  = Cvr_CoverReadDefault( pCvrNode );
    DefFanin = Cvr_CoverReadDefault( pCvrFanin );
    if ( DefNode < 0 || DefFanin < 0 )
        return NULL;
 
    // get the covers
    pMvcNode  = Cvr_CoverReadIsetByIndex( pCvrNode,  DefNode  ^ 1 );
    pMvcFanin = Cvr_CoverReadIsetByIndex( pCvrFanin, DefFanin ^ 1 );
//    assert( pMvcFanin->nBits == 0 || pMvcFanin->nBits == 2 );
    if ( pMvcFanin->nBits != 0 && pMvcFanin->nBits != 2 )
        return NULL;

    // fix the case when the fanin appears to be single-input 
    // (that is, pMvcFanin->nBits == 2) but is in fact constant 1
    if ( pMvcFanin->nBits == 2 && Mvc_CoverReadCubeNum(pMvcFanin) != 1 )
    {
        Mvc_CoverContain( pMvcFanin );
        if ( Mvc_CoverReadCubeNum(pMvcFanin) == 2 )
            Mvc_CoverMakeTautology( pMvcFanin );
        Ntk_NodeMakeMinimumBase( pFanin );
        // get the new version of pMvcFanin
        pCvrFanin = Ntk_NodeReadFuncCvr(pFanin);
        DefFanin  = Cvr_CoverReadDefault( pCvrFanin );
        pMvcFanin = Cvr_CoverReadIsetByIndex( pCvrFanin, DefFanin ^ 1 );
    }

    // get the number of pFanin in pNode and the starting bit
    VarNum = Ntk_NodeReadFaninIndex( pNode, pFanin );
    assert( VarNum >= 0 );
    pVm    = Ntk_NodeReadFuncVm( pNode );
    ValueFirst = Vm_VarMapReadValuesFirst( pVm, VarNum );
 
    // consider several cases
    fNeedToPatchFanin = 0;
    if ( pMvcFanin->nBits == 0 ) // constant node
    {
        if ( Mvc_CoverIsEmpty(pMvcFanin) ^ DefFanin )
            pMvcNew = Mvc_CoverCofactor( pMvcNode, ValueFirst, ValueFirst + 1 );
        else
            pMvcNew = Mvc_CoverCofactor( pMvcNode, ValueFirst + 1, ValueFirst );
        Mvc_CoverContain( pMvcNew );
    }
    else // buffer or interter
    {

        // two cases are possible: 
        // (1) the fanin's fanin is in the current support of the node
        // (2) the fanin's fanin is not in the current support of the node
        pFaninFanin = Ntk_NodeReadFaninNode( pFanin, 0 );
        VarNumF     = Ntk_NodeReadFaninIndex( pNode, pFaninFanin );

        if ( VarNumF >= 0 )
        { // case (1) 
            ValueFirstF = Vm_VarMapReadValuesFirst( pVm, VarNumF );
            // transform the cover by univerisally quantifying pFanin
            if ( Mvc_CoverIsBinaryBuffer(pMvcFanin) ^ DefFanin )
                pMvcNew = Mvc_CoverUnivQuantify( pMvcNode, ValueFirstF, ValueFirstF + 1, ValueFirst, ValueFirst + 1 );
            else
                pMvcNew = Mvc_CoverUnivQuantify( pMvcNode, ValueFirstF, ValueFirstF + 1, ValueFirst + 1, ValueFirst );
            Mvc_CoverContain( pMvcNew );
            // as a result of this operation, pFanin becomes a redundant variable of pNode
        }
        else
        { // case (2)
            if ( Mvc_CoverIsBinaryBuffer(pMvcFanin) ^ DefFanin )
                pMvcNew = Mvc_CoverDup( pMvcNode );
            else
                pMvcNew = Mvc_CoverFlipVar( pMvcNode, ValueFirst, ValueFirst + 1 );
            // this is the only case, when we need to patch the fanin
            fNeedToPatchFanin = 1;
        }
    }
    
    // create the empty Cvr
    pIsetsNew = ALLOC( Mvc_Cover_t *, 2 );
    pIsetsNew[DefNode  ] = NULL;
    pIsetsNew[DefNode^1] = pMvcNew;
    pCvrNew = Cvr_CoverCreate( pVm, pIsetsNew );

    // create the same node without functionality
    pNodeNew = Ntk_NodeClone( pNode->pNet, pNode );
    // set the functionality of the node
    Ntk_NodeWriteFuncVm( pNodeNew, pVm );
    Ntk_NodeWriteFuncCvr( pNodeNew, pCvrNew );

    // patch the new node's fanin (this will reorder fanins, if necessary)
    if ( fNeedToPatchFanin )
        Ntk_NodePatchFanin( pNodeNew, pFanin, pFaninFanin );
    return pNodeNew;
}


/**Function*************************************************************

  Synopsis    [Merges the fanins of the nodes that are collapsed.]

  Description [This procedure looks at the fanins of pNode and pFanin
  and performs the following tasks: (1) merges two ordered fanin lists 
  into one ordered list and creates the new node with this fanin list;
  (2) assigns the "translation maps" for pNode/pFanin (an entry in the 
  translation map shows where the given fanin of pNode/pFanin goes
  in the new variable map); (3) creates the new variable map and 
  attaches it to the new node.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Node_t * Ntk_NodeCombineFanins( Ntk_Node_t * pNode, Ntk_Node_t * pFanin, 
         int * pTransMapNInv, int * pTransMapF )
{
    Vm_Manager_t * pManVm;
    Vm_VarMap_t * pVm;
    Ntk_Node_t * pTopN, * pTopF, * pNodeCur, * pNodeNew;
    Ntk_Node_t ** pInputsN, ** pInputsF;
    int nInputsN, nInputsF;
    int * pVarValues, nVars; 
    int iTopN, iTopF, iNode, iNodePos;
    int CompValue, i;


    // collect the fanins of pNode
    nInputsN = Ntk_NodeReadFaninNum( pNode );
    assert( nInputsN > 0 );
    pInputsN = pNode->pNet->pArray1; 
    Ntk_NodeReadFanins( pNode, pInputsN );
    // make sure the fanins are ordered
    for ( i = 1; i < nInputsN; i++ )
    {
        assert( Ntk_NodeCompareByNameAndId( pInputsN + i-1, pInputsN + i ) < 0 );
    }

    // collect the fanins of pFanin
    nInputsF = Ntk_NodeReadFaninNum( pFanin );
    assert( nInputsF >= 0 );
    pInputsF = pNode->pNet->pArray2; 
    Ntk_NodeReadFanins( pFanin, pInputsF );
    // make sure the fanins are ordered
    for ( i = 1; i < nInputsF; i++ )
    {
        assert( Ntk_NodeCompareByNameAndId( pInputsF + i-1, pInputsF + i ) < 0 );
    }


    // create the new node
    pNodeNew = Ntk_NodeCreate( pNode->pNet, NULL, MV_NODE_INT, pNode->nValues );

    // get temp storage for the array of fanin values
    pVarValues = Vm_VarMapGetStorageArray1( Ntk_NodeReadFuncVm(pNode) );

    // clean the inverse array
    for ( i = 0; i <= nInputsN + nInputsF; i++ )
        pTransMapNInv[i] = -1;

    // the output node of pFanin is one of the fanins of pNode
    iNode = Ntk_NodeReadFaninIndex( pNode, pFanin );
    assert( iNode != -1 );

    // sort through the fanins of pNode and pFanin
    pTopN = pInputsN[0];
    pTopF = nInputsF? pInputsF[0]: NULL;
    iTopN = 1;
    iTopF = nInputsF? 1: 0;
    nVars = 0;
    while ( pTopN || pTopF )
    {
        // compare the topmost fanin of the two nodes
        if ( pTopN == NULL )
            CompValue = -1;
        else if ( pTopF == NULL ) 
            CompValue = 1;
        else
            CompValue = Ntk_NodeCompareByNameAndId( &pTopF, &pTopN );

        // use the comparison to create the merged fanin list
        if ( CompValue < 0 ) // pTopF is in pFanin and not in pNode
        {
//            assert( Ntk_NodeReadFaninIndex( pNode,  pTopF ) == -1 );
//            assert( Ntk_NodeReadFaninIndex( pFanin, pTopF ) != -1 );
            // set the current node
            pNodeCur = pTopF;
            // add to the translation map from pFanin into pNodeNew
            pTransMapF[ iTopF-1 ] = nVars;
            // get the next fanin in pFanin
            pTopF = (iTopF == nInputsF)? NULL: pInputsF[iTopF++];
        }
        else if ( CompValue == 0 ) // the same fanin (pTopF==pTopN) is in both lists
        {
//            assert( Ntk_NodeReadFaninIndex( pNode,  pTopF ) != -1 );
//            assert( Ntk_NodeReadFaninIndex( pFanin, pTopF ) != -1 );
            // set the current node
            pNodeCur = pTopF;
            // set the translation map from pFanin into pNodeNew
//          pTransMapN[ iTopN-1 ] = nVars;
            pTransMapNInv[ nVars ] = iTopN-1;
            pTransMapF[ iTopF-1 ] = nVars;
            // get the next fanin in pFanin/pNode
            pTopF = (iTopF == nInputsF)? NULL: pInputsF[iTopF++];
            pTopN = (iTopN == nInputsN)? NULL: pInputsN[iTopN++];
        }
        else // pTopN is in pNode and not in pFanins
        {
//            assert( Ntk_NodeReadFaninIndex( pNode,  pTopN ) != -1 );
//            assert( Ntk_NodeReadFaninIndex( pFanin, pTopN ) == -1 );
            // set the current node
            pNodeCur = pTopN;
            // add to the translation map from pNode into pNodeNew
//          pTransMapN[ iTopN-1 ] = nVars;
            pTransMapNInv[ nVars ] = iTopN-1;
            if ( iNode == iTopN-1 )
                iNodePos = nVars;
            // get the next fanin in pFanin
            pTopN = (iTopN == nInputsN)? NULL: pInputsN[iTopN++];
        }

        // add this fanin to the fanin list in the new node
        Ntk_NodeAddFanin( pNodeNew, pNodeCur );
        // set the number of values of this fanin
        pVarValues[ nVars ] = pNodeCur->nValues;
        // increment the counter of all inputs processed
        nVars++;
    }

    // treat the output nodes
    // set the translation map for the output of pFanin
    pTransMapF[ iTopF++ ] = iNodePos;

    // the output node of pNode is the output of the resulting collapsed node
    // set the translation map for the output of pNode
//  pTransMapN[ iTopN++ ] = nVars;
    pTransMapNInv[ nVars ] = iTopN++;
    // set the number of output values of the collapsed node
    pVarValues[ nVars ] = pNode->nValues;

    // sanity checks
    // the number of nodes processed in equal to the number of fanins
    assert( iTopF == Ntk_NodeReadFaninNum(pFanin) + 1 );
    assert( iTopN == Ntk_NodeReadFaninNum(pNode) + 1 );

    // create the new variable map for the node
    pManVm = Ntk_NetworkReadManVm( pNode->pNet );
    pVm    = Vm_VarMapLookup( pManVm, nVars, 1, pVarValues );
    Ntk_NodeWriteFuncVm( pNodeNew, pVm );
    return pNodeNew;
}


/**Function*************************************************************

  Synopsis    [Merges the fanins of the nodes.]

  Description [This procedure looks at the fanins of pNode1 and pNode2
  and performs the following tasks: (1) merges two ordered fanin lists 
  into one ordered list and creates the new node with this fanin list;
  (2) creates a new variable map and attaches it to the new node;
  (3) if variables pTransMapN1/pTransMapN2 are not NULL, this procedure
  also assigns the "translation maps" for pNode1/pNode2 (an entry in the 
  translation map shows where the given fanin of pNode1/pNode2 goes
  in the variable map of the new node).]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Node_t * Ntk_NodeMakeCommonBase( Ntk_Node_t * pNode1, Ntk_Node_t * pNode2, 
     int * pTransMapN1, int * pTransMapN2 )
{
    Vm_Manager_t * pManVm;
    Vm_VarMap_t * pVm;
    Ntk_Node_t * pTopN1, * pTopN2, * pNodeCur, * pNodeNew;
    Ntk_Node_t ** pInputsN1, ** pInputsN2;
    int nInputsN1, nInputsN2;
    int * pVarValues, nVars; 
    int iTopN1, iTopN2;
    int CompValue, i;


    // collect the fanins of pNode1
    nInputsN1 = Ntk_NodeReadFaninNum( pNode1 );
    pInputsN1 = pNode1->pNet->pArray1; 
    Ntk_NodeReadFanins( pNode1, pInputsN1 );
    // make sure the fanins are ordered
    for ( i = 1; i < nInputsN1; i++ )
    {
        assert( Ntk_NodeCompareByNameAndId( pInputsN1 + i-1, pInputsN1 + i ) < 0 );
    }

    // collect the fanins of pNode2
    nInputsN2 = Ntk_NodeReadFaninNum( pNode2 );
    pInputsN2 = pNode2->pNet->pArray2; 
    Ntk_NodeReadFanins( pNode2, pInputsN2 );
    // make sure the fanins are ordered
    for ( i = 1; i < nInputsN2; i++ )
    {
        assert( Ntk_NodeCompareByNameAndId( pInputsN2 + i-1, pInputsN2 + i ) < 0 );
    }

    // create the new node
    pNodeNew = Ntk_NodeCreate( pNode1->pNet, NULL, MV_NODE_INT, -1 );

    // get temp storage for the array of fanin values
    pVarValues = Vm_VarMapGetStorageArray1( Ntk_NodeReadFuncVm(pNode1) );

    // sort through the fanins of pNode1 and pNode2
    pTopN1 = pInputsN1[0];
    pTopN2 = pInputsN2[0];
    iTopN1 = 0;  //changed by wjiang
    iTopN2 = 0;  //changed by wjiang
    nVars = 0;
    while ( iTopN1 < nInputsN1 || iTopN2 < nInputsN2 )
    {
        // compare the topmost fanin of the two nodes
        if ( iTopN1 == nInputsN1 )
            CompValue = -1;
        else if ( iTopN2 == nInputsN2 ) 
            CompValue = 1;
        else
            CompValue = Ntk_NodeCompareByNameAndId( &pTopN2, &pTopN1 );

        // use the comparison to create the merged fanin list
        if ( CompValue < 0 ) // pTopN2 is in pNode2 and not in pNode1
        {
            // set the current node
            pNodeCur = pTopN2;
            // add to the translation map from pNode2 into pNodeNew
            if ( pTransMapN2 )
                pTransMapN2[ iTopN2 ] = nVars;
            // get the next fanin in pNode2
            pTopN2 = pInputsN2[++iTopN2]; //changed by wjiang
        }
        else if ( CompValue == 0 ) // the same fanin (pTopN2==pTopN1) is in both lists
        {
            // set the current node
            pNodeCur = pTopN2;
            // set the translation map from pNode2 into pNodeNew
            if ( pTransMapN1 )
                pTransMapN1[ iTopN1 ] = nVars;
            if ( pTransMapN2 )
                pTransMapN2[ iTopN2 ] = nVars;
            // get the next fanin in pNode2/pNode1
            pTopN2 = pInputsN2[++iTopN2]; //changed by wjiang
            pTopN1 = pInputsN1[++iTopN1]; //changed by wjiang
        }
        else // pTopN1 is in pNode1 and not in pNode2s
        {
            // set the current node
            pNodeCur = pTopN1;
            // add to the translation map from pNode1 into pNodeNew
            if ( pTransMapN1 )
                pTransMapN1[ iTopN1 ] = nVars;
            // get the next fanin in pNode2
            pTopN1 = pInputsN1[++iTopN1]; //changed by wjiang
        }

        // add this fanin to the fanin list in the new node
        Ntk_NodeAddFanin( pNodeNew, pNodeCur );
        // set the number of values of this fanin
        pVarValues[ nVars ] = pNodeCur->nValues;
        // increment the counter of all inputs processed
        nVars++;
    }

    // sanity checks
    // the number of nodes processed in equal to the number of fanins
    assert( iTopN1 == Ntk_NodeReadFaninNum(pNode1) );
    assert( iTopN2 == Ntk_NodeReadFaninNum(pNode2) );
    
    // output values is the same as the first node! (wjiang)
    pVm = Ntk_NodeReadFuncVm(pNode1);
    pVarValues[nVars] = Vm_VarMapReadValuesOutNum(pVm);

    // create the new variable map for the node
    pManVm = Ntk_NetworkReadManVm( pNode1->pNet );
    pVm    = Vm_VarMapLookup( pManVm, nVars, 1, pVarValues );
    Ntk_NodeWriteFuncVm( pNodeNew, pVm );
    return pNodeNew;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


