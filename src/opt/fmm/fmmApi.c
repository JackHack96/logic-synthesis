/**CFile****************************************************************

  FileName    [fmmApi.c]

  PackageName [Multi-valued flexibility manager.]

  Synopsis    [APIs of the flexibility manager.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 3.0. Started - September 27, 2003.]

  Revision    [$Id: fmmApi.c,v 1.2 2004/05/12 04:30:13 alanmi Exp $]

***********************************************************************/

#include "fmmInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     EXPORTED FUNCTIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Fmm_ManagerComputeGlobalDcNode( Fmm_Manager_t * p, Ntk_Node_t * pPivot )
{
    Ntk_Node_t * pNode;
    DdNode * bFlex, * bTemp;
    DdManager * dd = p->ddGlo;
    int * pBitsFirst, nInputs, v;
    int TimeLimit;

    assert( pPivot->Type == MV_NODE_INT );

    // create the global flexibility variable map
    p->pVmxG = Fmm_UtilsCreateGlobalVmx( p, pPivot );
    nInputs = Vm_VarMapReadVarsInNum( Vmx_VarMapReadVm(p->pVmxG) );
    // USE SET VARS
    p->pVmxGS = Fmm_UtilsCreateGlobalSetVmx( p, pPivot );
    // set the set global inputs (GloZ) for the given node
    pBitsFirst = Vmx_VarMapReadBitsFirst( p->pVmxGS );
    for ( v = 0; v < pPivot->nValues; v++ )
        p->pbArray[v] = p->pbVars0[ pBitsFirst[nInputs+v] ];
    Ntk_NodeWriteFuncGlobZ( pPivot, p->pbArray );

    // collect the nodes in the TFO of the given node
    Ntk_NetworkDfsFromNode( p->pNet, pPivot );
    assert( p->pNet->pOrder == pPivot );
    //Ntk_NetworkPrintSpecial( pPivot->pNet );

    // set the timeout
    TimeLimit = (int)(FMM_UPDATE_TIMEOUT * (float)(CLOCKS_PER_SEC) / 1000 ) + clock();

    // compute GloZ BDDs for these nodes
p->timeTemp = clock();
    Ntk_NetworkForEachNodeSpecialStart( Ntk_NodeReadOrder(pPivot), pNode )
        if ( Ntk_NodeGlobalComputeZ( pNode, TimeLimit ) )
        { // timeout occurred
//            printf( "Update timeout.\n" );
            // deref GloZ BDDs
            Ntk_NetworkForEachNodeSpecialStart( pPivot, pNode )
                Ntk_NodeDerefFuncGlobZ( pNode );
p->timeGloZ += clock() - p->timeTemp;
            bFlex = Fmm_UtilsRelationGlobalCompute( dd, pPivot, p->pVmxG ); 
            return bFlex;
        }
p->timeGloZ += clock() - p->timeTemp;

    // derive the flexibility
    // it is important that this call is before GloZ is derefed
p->timeTemp = clock();
    bFlex = Fmm_FlexComputeGlobal( p );
p->timeFlex += clock() - p->timeTemp;
    if ( bFlex == NULL )
    { // the flexibility computation has timed out
//printf( "Timeout\n" );
        bFlex = Fmm_UtilsRelationGlobalCompute( dd, pPivot, p->pVmxG ); 
        Cudd_Ref( bFlex );
    }
    else
    {
        Cudd_Ref( bFlex );
        // transfer from the set inputs
        bFlex = Fmm_UtilsTransferFromSetOutputs( p, bTemp = bFlex ); Cudd_Ref( bFlex );
        Cudd_RecursiveDeref( dd, bTemp );
    }

    // deref GloZ BDDs
    Ntk_NetworkForEachNodeSpecialStart( pPivot, pNode )
        Ntk_NodeDerefFuncGlobZ( pNode );
   
    // make sure that the global flexibility is well defined
    // one of the reasons why the domain may be computed incorrectly
    // is when the global BDDs are not updated properly arter a node has changed..
    assert( Fmm_UtilsRelationDomainCheck( dd, bFlex, p->pVmxG ) ); 

    Cudd_Deref( bFlex );
    return bFlex;
}

/**Function*************************************************************

  Synopsis    [Computes complete flexibility of the node.]

  Description [Returns the complete flexibility of a node expressed
  as an MV relation with the same relation parameters as the original
  MV relation of the node. (The returned relation may be not well-defined
  if the network was out of spec at the moment of calling this procedure.)]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvr_Relation_t * Fmm_ManagerComputeLocalDcNode( Fmm_Manager_t * p, Ntk_Node_t * pNode )
{
    Mvr_Relation_t * pMvrLoc;
    DdNode * bGlobal;
    Ntk_Node_t ** ppFanins;
    int nFanins;

    bGlobal = Fmm_ManagerComputeGlobalDcNode( p, pNode );  Cudd_Ref( bGlobal );
    // set the fanins
    nFanins = Ntk_NodeReadFaninNum( pNode );
    ppFanins = ALLOC( Ntk_Node_t *, nFanins );
    nFanins = Ntk_NodeReadFanins( pNode, ppFanins );

    // get the local variable map
    p->pVmxL = Fmm_UtilsCreateLocalVmx( p, pNode, ppFanins, nFanins );
p->timeTemp = clock();
    pMvrLoc  = Fmm_FlexComputeLocal( p, bGlobal, ppFanins, nFanins );
    if ( pMvrLoc == NULL )
    {
        pMvrLoc = Ntk_NodeGetFuncMvr( pNode );
        pMvrLoc = Mvr_RelationDup( pMvrLoc );
    }
p->timeImage += clock() - p->timeTemp;
    Cudd_RecursiveDeref( p->ddGlo, bGlobal );

    FREE( ppFanins );
    return pMvrLoc;
}


/**Function*************************************************************

  Synopsis    [Updates the global functions of the nodes in the network.]

  Description [This function should be called each time a node's local function
  is changed.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fmm_ManagerUpdateGlobalFunctions( Fmm_Manager_t * p, Ntk_Node_t * pNode )
{
    // collect the nodes in the TFO of the given node
    Ntk_NetworkDfsFromNode( p->pNet, pNode );
    // update the global BDDs of these nodes
    Ntk_NetworkForEachNodeSpecial( p->pNet, pNode )
    {
        Ntk_NodeDerefFuncGlob( pNode );
        Ntk_NodeGlobalCompute( p->ddGlo, pNode, 0 );
    }
}

////////////////////////////////////////////////////////////////////////
///                         END OF FILE                              ///
////////////////////////////////////////////////////////////////////////


