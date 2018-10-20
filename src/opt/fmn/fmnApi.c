/**CFile****************************************************************

  FileName    [dcmnApi.c]

  PackageName [New don't care manager.]

  Synopsis    [APIs of the flexibility manager.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - July 13, 2003.]

  Revision    [$Id: fmnApi.c,v 1.2 2003/11/18 18:55:12 alanmi Exp $]

***********************************************************************/

#include "fmnInt.h"

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
Dcmn_Man_t * Dcmn_ManagerStart( Ntk_Network_t * pNet, Dcmn_Par_t * pPars )
{
    Dcmn_Man_t * p;
    Ntk_Node_t * pNode;
//    Ntk_Network_t * pNetSpec;
    int nInputs, nOutputs, nSuppMax, nBits, i;
    DdNode ** pbFuncs;
    DdNode ** pbCodes;

    // allocate the node structures
//    Dcmn_NetworkAlloc( pNet );

    // allocate the manager
    p = ALLOC( Dcmn_Man_t, 1 );
    memset( p, 0, sizeof(Dcmn_Man_t) );

    // set the parameters
    p->pPars = pPars;

    // count the largest local support size
    nSuppMax = 0;
    Ntk_NetworkForEachNode( pNet, pNode )
    {
        nBits = Vm_VarMapGetBitsNum( Ntk_NodeReadFuncVm(pNode) );
        if ( nSuppMax < nBits )
            nSuppMax = nBits;
    }
   
    p->pVmx  = Dcmn_UtilsGetNetworkVmx(pNet);
    p->pVm   = Vmx_VarMapReadVm(p->pVmx);

    // start both managers, local and global
    nInputs  = Ntk_NetworkReadCiNum(pNet);
    nOutputs = Ntk_NetworkReadCoNum(pNet);
    nBits    = Vmx_VarMapReadBitsNum(p->pVmx) + 30;

    // start the managers
//    p->ddLoc = Cudd_Init( nSuppMax + DCMN_MAX_FANIN_GROWTH, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
    p->ddGlo = Cudd_Init( 2 * nBits,                        0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
	// data related to the network
    p->pNet = pNet;
    p->nArrayAlloc = 1000;
    p->pbArray   = ALLOC( DdNode *, p->nArrayAlloc );
    p->pbArray2  = ALLOC( DdNode *, p->nArrayAlloc );

    // get the number of special nodes
    p->nSpecialsAlloc = Vm_VarMapReadVarsNum(p->pVm) - nInputs - nOutputs;
    p->ppSpecials     = ALLOC( Ntk_Node_t *, p->nSpecialsAlloc + 1 );

    // data related to the global BDDs
//    p->nExdcs  = Vm_VarMapReadValuesOutNum(p->pVm);
//    p->pbExdcs = ALLOC( DdNode *, p->nExdcs );
//    memset( p->pbExdcs, 0, sizeof(DdNode *) * p->nExdcs );

    // the elementary variables
    p->pbVars0 = ALLOC( DdNode *, nBits );
    p->pbVars1 = ALLOC( DdNode *, nBits );
    for ( i = 0; i < nBits; i++ )
    {
//        p->pbVars0[i] = p->ddGlo->vars[2*i+0];
//        p->pbVars1[i] = p->ddGlo->vars[2*i+1];
        p->pbVars0[i] = p->ddGlo->vars[i];
        p->pbVars1[i] = p->ddGlo->vars[nBits + i];
    }

    // encode the var map
    p->nCodes  = Vm_VarMapReadValuesNum( p->pVm );
    pbCodes    = Vmx_VarMapEncodeMapUsingVars( p->ddGlo, p->pbVars0, p->pVmx );
    p->pbCodes = ALLOC( DdNode *, p->nCodes );
    Dcmn_UtilsCopyFuncs( p->pbCodes, pbCodes, p->nCodes ); // take ref
    memset( pbCodes, 0, sizeof(DdNode *) * p->nCodes );


    // derive the global BDDs of the spec
    Dcmn_NetworkBuildGlo( p, pNet, 0, p->pPars->TypeSpec != DCMN_TYPE_SS );
    // store away the spec
    Ntk_NetworkForEachCo( pNet, pNode )
    {
        // store the spec in the S field
        pbFuncs = Ntk_NodeReadFuncGlob( pNode );
        Ntk_NodeWriteFuncGlobS( pNode, pbFuncs );

        // quantify the intermediate variables if necessary
        if ( p->pPars->TypeSpec != DCMN_TYPE_SS )
        {
            pbFuncs = Ntk_NodeReadFuncGlobS( pNode );
            Dcmn_FlexQuantifyFunctionArray( p, pbFuncs, pNode->nValues );
        }
//PRB( p->ddGlo, pbFuncs[0] );
//PRB( p->ddGlo, pbFuncs[1] );
    }


    // if the functionality type is different from the spec type, recompute
    if ( p->pPars->TypeSpec != p->pPars->TypeFlex )
    {
        // clean the intermediate BDDs and the global BDDs
        Ntk_NetworkForEachCi( pNet, pNode )
            Ntk_NodeDerefFuncGlob( pNode );
        Ntk_NetworkForEachNode( pNet, pNode )
        {
            Ntk_NodeDerefFuncGlob( pNode );
            Ntk_NodeDerefFuncGlobS( pNode );
        }
        Ntk_NetworkForEachCo( pNet, pNode )
            Ntk_NodeDerefFuncGlob( pNode );

        // compute the new global BDDs
        Dcmn_NetworkBuildGlo( p, pNet, 0, p->pPars->TypeFlex );
    }



    // derive the global BDDs of the spec network
    if ( p->pPars->fUseExdc && pNet->pSpec )
    {
        assert( 0 );
/*
        // set the spec network
        printf( "Using external spec \"%s\".\n", pNet->pSpec );
        pNetSpec = Io_ReadNetwork( Ntk_NetworkReadMvsis(pNet), pNet->pSpec );

        // compute its global functions
        Dcmn_NetworkBuildGlo( p, pNetSpec, 1 );

        // create the array of global spec MDDs for each PO i-set
        i = 0;
        Ntk_NetworkForEachCo( pNetSpec, pNode )
        {
            pbFuncs = Ntk_NodeReadFuncGlob( pNode );
            Dcmn_UtilsCopyFuncs( p->pbExdcs + i, pbFuncs, pNode->nValues );
            Dcmn_UtilsRefFuncs( p->pbExdcs + i, pNode->nValues );
            i += pNode->nValues;
        }
        assert( i == p->nExdcs );
        // delete the network; this will also free all MDDs
        Ntk_NetworkDelete( pNetSpec ); 
*/
    }
    else
    {
/*
        // use the given network as its own spec
        i = 0;
        Ntk_NetworkForEachCo( pNet, pNode )
        {
            pbFuncs = Ntk_NodeReadFuncGlob( pNode );
            Dcmn_UtilsCopyFuncs( p->pbExdcs + i, pbFuncs, pNode->nValues );
            Dcmn_UtilsRefFuncs( p->pbExdcs + i, pNode->nValues );
            i += pNode->nValues;
        }
//PRB( p->ddGlo, p->pbExdcs[0] );
//PRB( p->ddGlo, p->pbExdcs[1] );

        assert( i == p->nExdcs );
*/
    }

    return p;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dcmn_ManagerStop( Dcmn_Man_t * p )
{
    // remove the leaves
    if ( p->pLeaves ) 
        st_free_table( p->pLeaves );
    // deref the global EXDCs
//    Dcmn_UtilsDerefFuncs( p->ddGlo, p->pbExdcs, p->nExdcs );
    // deref the codes
    Dcmn_UtilsDerefFuncs( p->ddGlo, p->pbCodes, p->nCodes );
    // clean the network
//    Dcmn_NetworkClean( p->pNet );
    Ntk_NetworkGlobalClean( p->pNet );

    // stop the managers
//printf( "Local manager: \n" );
//    Extra_StopManager( p->ddLoc );
//printf( "Global manager:\n" );

    Extra_StopManager( p->ddGlo );
    FREE( p->ppSpecials );
    FREE( p->pPars );
//    FREE( p->pbExdcs );
    FREE( p->pbCodes );
    FREE( p->pbArray );
    FREE( p->pbArray2 );
    FREE( p->pbVars0 );
    FREE( p->pbVars1 );
    FREE( p );
}



/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Dcmn_ManagerComputeGlobalDcNode( Dcmn_Man_t * p, Ntk_Node_t * pPivot )
{
    Ntk_Node_t * pNode;
    DdNode * bFlex, * bTemp;
    DdNode ** pbFuncs;
    int v, * pBitsFirst;
    int nInputs;
    bool fNonDet;
    DdNode * bCubeOut, * bExist, * bDomain;
    DdManager * dd;

    assert( pPivot->Type == MV_NODE_INT );
    // collect the nodes in the TFO of the given node
    Ntk_NetworkDfsFromNode( p->pNet, pPivot );
    assert( p->pNet->pOrder == pPivot );

    // create the global flexibility variable map
    p->pVmxG = Dcmn_UtilsCreateGlobalVmx( p, pPivot );
    nInputs = Vm_VarMapReadVarsInNum( Vmx_VarMapReadVm(p->pVmxG) );
    if ( p->pPars->TypeFlex != DCMN_TYPE_SS )
    { // USE NORMAL VARS
        p->pVmxGS = NULL;
        // set the GloZ for the given node, which has the number "nInputs" in pVmxG
        pbFuncs = Vmx_VarMapEncodeVarUsingVars( p->ddGlo, p->pbVars0, p->pVmxG, nInputs );
        Ntk_NodeWriteFuncGlobZ( pPivot, pbFuncs );
        Vmx_VarMapEncodeDeref( p->ddGlo, p->pVmxG, pbFuncs );
    }
    else
    { // USE SET VARS
        p->pVmxGS = Dcmn_UtilsCreateGlobalSetVmx( p, pPivot );
        // set the set global inputs (GloZ) for the given node
        pBitsFirst = Vmx_VarMapReadBitsFirst( p->pVmxGS );
        for ( v = 0; v < pPivot->nValues; v++ )
            p->pbArray[v] = p->pbVars0[ pBitsFirst[nInputs+v] ];
        Ntk_NodeWriteFuncGlobZ( pPivot, p->pbArray );
    }
//Ntk_NetworkPrintSpecial( pPivot->pNet );

    // the global Z variable should override the global S variable
    // save the non-det status of the pivot node
    if ( p->pPars->TypeFlex != DCMN_TYPE_SS ) 
    {
        if ( fNonDet = Ntk_NodeReadFuncNonDet(pPivot) )
            Ntk_NodeWriteFuncNonDet( pPivot, 0 );
    }

    // compute GloZ BDDs for these nodes
    Ntk_NetworkForEachNodeSpecialStart( Ntk_NodeReadOrder(pPivot), pNode )
        Dcmn_NodeBuildGloZ( p, pNode, p->pPars->TypeFlex != DCMN_TYPE_SS );

    // restore the non-det status of the pivot node
    if ( p->pPars->TypeFlex != DCMN_TYPE_SS ) 
    {
//printf( "Computing NSC.\n" );

        if ( fNonDet )
            Ntk_NodeWriteFuncNonDet( pPivot, 1 );
    }
//    else
//        printf( "Computing SS.\n" );


    // derive the flexibility
    bFlex = Dcmn_FlexComputeGlobal( p );   Cudd_Ref( bFlex );
    // it is important that this call is before GloZ is derefed

    // deref GloZ BDDs
//    Ntk_NetworkForEachNodeSpecialStart( pPivot, pNode )
//        Ntk_NodeDerefFuncGlobZ( pNode );

    // transfer from the set inputs
    if ( p->pPars->TypeFlex == DCMN_TYPE_SS )
    {
//PRB( p->ddGlo, bFlex );
        bFlex = Dcmn_UtilsTransferFromSetOutputs( p, bTemp = bFlex ); Cudd_Ref( bFlex );
        Cudd_RecursiveDeref( p->ddGlo, bTemp );

//printf( "Removing set vars.\n" );

//PRB( p->ddGlo, bFlex );
    }

    
    // make sure that the global flexibility is well defined
    dd = p->ddGlo;



    if ( p->pPars->TypeFlex != DCMN_TYPE_SS ) 
        bExist   = Dcmn_FlexQuantifyFunction( p, bFlex );  
    else 
        bExist = bFlex;

    Cudd_Ref( bExist );
//PRB( dd, bExist );
//PRB( dd, Cudd_Not(bExist) );

    bCubeOut = Vmx_VarMapCharCubeOutput( dd, p->pVmxG );     Cudd_Ref( bCubeOut );
    bDomain  = Cudd_bddExistAbstract( dd, bExist, bCubeOut ); Cudd_Ref( bDomain );
    if ( bDomain != b1 )
    {
        printf( "Global flexibility of node \"%s\" is not well defined!\n", 
            Ntk_NodeGetNamePrintable(pPivot) );
//        PRB( dd, bDomain );
    }
    Cudd_RecursiveDeref( dd, bCubeOut );
    Cudd_RecursiveDeref( dd, bDomain );
    Cudd_RecursiveDeref( dd, bExist );


    Cudd_Deref( bFlex );
    return bFlex;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Dcmn_ManagerComputeGlobalDcNodeUseProduct( Dcmn_Man_t * p, Ntk_Node_t * pPivot, Ntk_Node_t * pNodeCo )
{
    Ntk_Node_t * pNode;
    DdNode * bFlex, * bTemp;
    DdNode ** pbFuncs;
    int v, * pBitsFirst;
    int nInputs;
    bool fNonDet;
//    DdNode * bCubeOut, * bExist, * bDomain;
    DdManager * dd;

    assert( pPivot->Type == MV_NODE_INT );
    // collect the nodes in the TFO of the given node
    Ntk_NetworkDfsFromNode( p->pNet, pPivot );
    assert( p->pNet->pOrder == pPivot );

    // create the global flexibility variable map
    p->pVmxG = Dcmn_UtilsCreateGlobalVmx( p, pPivot );
    nInputs = Vm_VarMapReadVarsInNum( Vmx_VarMapReadVm(p->pVmxG) );
    if ( p->pPars->TypeFlex != DCMN_TYPE_SS )
    { // USE NORMAL VARS
        p->pVmxGS = NULL;
        // set the GloZ for the given node, which has the number "nInputs" in pVmxG
        pbFuncs = Vmx_VarMapEncodeVarUsingVars( p->ddGlo, p->pbVars0, p->pVmxG, nInputs );
        Ntk_NodeWriteFuncGlobZ( pPivot, pbFuncs );
        Vmx_VarMapEncodeDeref( p->ddGlo, p->pVmxG, pbFuncs );
    }
    else
    { // USE SET VARS
        p->pVmxGS = Dcmn_UtilsCreateGlobalSetVmx( p, pPivot );
        // set the set global inputs (GloZ) for the given node
        pBitsFirst = Vmx_VarMapReadBitsFirst( p->pVmxGS );
        for ( v = 0; v < pPivot->nValues; v++ )
            p->pbArray[v] = p->pbVars0[ pBitsFirst[nInputs+v] ];
        Ntk_NodeWriteFuncGlobZ( pPivot, p->pbArray );
    }
//Ntk_NetworkPrintSpecial( pPivot->pNet );

    // the global Z variable should override the global S variable
    // save the non-det status of the pivot node
    if ( p->pPars->TypeFlex != DCMN_TYPE_SS ) 
    {
        if ( fNonDet = Ntk_NodeReadFuncNonDet(pPivot) )
            Ntk_NodeWriteFuncNonDet( pPivot, 0 );
    }

    // compute GloZ BDDs for these nodes
    Ntk_NetworkForEachNodeSpecialStart( Ntk_NodeReadOrder(pPivot), pNode )
        Dcmn_NodeBuildGloZ( p, pNode, p->pPars->TypeFlex != DCMN_TYPE_SS );

    // restore the non-det status of the pivot node
    if ( p->pPars->TypeFlex != DCMN_TYPE_SS ) 
    {
        if ( fNonDet )
            Ntk_NodeWriteFuncNonDet( pPivot, 1 );
    }

    // derive the flexibility
    bFlex = Dcmn_FlexComputeGlobalOne( p, pNodeCo );   Cudd_Ref( bFlex );

    // deref GloZ BDDs
//    Ntk_NetworkForEachNodeSpecialStart( pPivot, pNode )
//        Ntk_NodeDerefFuncGlobZ( pNode );


    // transfer from the set inputs
    if ( p->pPars->TypeFlex == DCMN_TYPE_SS )
    {
//PRB( p->ddGlo, bFlex );
        bFlex = Dcmn_UtilsTransferFromSetOutputs( p, bTemp = bFlex ); Cudd_Ref( bFlex );
        Cudd_RecursiveDeref( p->ddGlo, bTemp );
//PRB( p->ddGlo, bFlex );
    }

    
    // make sure that the global flexibility is well defined
    dd = p->ddGlo;

/*
    bExist   = Dcmn_FlexQuantifyFunction( p, bFlex );     Cudd_Ref( bExist );
    bCubeOut = Vmx_VarMapCharCubeOutput( dd, p->pVmxG );     Cudd_Ref( bCubeOut );
    bDomain  = Cudd_bddExistAbstract( dd, bExist, bCubeOut ); Cudd_Ref( bDomain );
    if ( bDomain != b1 )
    {
        printf( "Global flexibility of node \"%s\" is not well defined!\n", 
            Ntk_NodeGetNamePrintable(pPivot) );
//        PRB( dd, bDomain );
    }
    Cudd_RecursiveDeref( dd, bCubeOut );
    Cudd_RecursiveDeref( dd, bDomain );
    Cudd_RecursiveDeref( dd, bExist );
*/

    Cudd_Deref( bFlex );
    return bFlex;
}
/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Dcmn_ManagerComputeGlobalDcNodeUseNsSpec( Dcmn_Man_t * p, Ntk_Node_t * pPivot, DdNode * bSpecNs )
{
    Ntk_Node_t * pNode;
    DdNode * bFlex, * bTemp;
    DdNode ** pbFuncs;
    int v, * pBitsFirst;
    int nInputs;
    bool fNonDet;
    DdNode * bCubeOut, * bExist, * bDomain;
    DdManager * dd;

    assert( pPivot->Type == MV_NODE_INT );
    // collect the nodes in the TFO of the given node
    Ntk_NetworkDfsFromNode( p->pNet, pPivot );
    assert( p->pNet->pOrder == pPivot );

    // create the global flexibility variable map
    p->pVmxG = Dcmn_UtilsCreateGlobalVmx( p, pPivot );
    nInputs = Vm_VarMapReadVarsInNum( Vmx_VarMapReadVm(p->pVmxG) );
    if ( p->pPars->TypeFlex != DCMN_TYPE_SS )
    { // USE NORMAL VARS
        p->pVmxGS = NULL;
        // set the GloZ for the given node, which has the number "nInputs" in pVmxG
        pbFuncs = Vmx_VarMapEncodeVarUsingVars( p->ddGlo, p->pbVars0, p->pVmxG, nInputs );
        Ntk_NodeWriteFuncGlobZ( pPivot, pbFuncs );
        Vmx_VarMapEncodeDeref( p->ddGlo, p->pVmxG, pbFuncs );
    }
    else
    { // USE SET VARS
        p->pVmxGS = Dcmn_UtilsCreateGlobalSetVmx( p, pPivot );
        // set the set global inputs (GloZ) for the given node
        pBitsFirst = Vmx_VarMapReadBitsFirst( p->pVmxGS );
        for ( v = 0; v < pPivot->nValues; v++ )
            p->pbArray[v] = p->pbVars0[ pBitsFirst[nInputs+v] ];
        Ntk_NodeWriteFuncGlobZ( pPivot, p->pbArray );
    }
//Ntk_NetworkPrintSpecial( pPivot->pNet );

    // the global Z variable should override the global S variable
    // save the non-det status of the pivot node
    if ( p->pPars->TypeFlex != DCMN_TYPE_SS ) 
    {
        if ( fNonDet = Ntk_NodeReadFuncNonDet(pPivot) )
            Ntk_NodeWriteFuncNonDet( pPivot, 0 );
    }

    // compute GloZ BDDs for these nodes
    Ntk_NetworkForEachNodeSpecialStart( Ntk_NodeReadOrder(pPivot), pNode )
        Dcmn_NodeBuildGloZ( p, pNode, p->pPars->TypeFlex != DCMN_TYPE_SS );

    // restore the non-det status of the pivot node
    if ( p->pPars->TypeFlex != DCMN_TYPE_SS ) 
    {
        if ( fNonDet )
            Ntk_NodeWriteFuncNonDet( pPivot, 1 );
    }

    // derive the flexibility
    bFlex = Dcmn_FlexComputeGlobalUseSpecNs( p, bSpecNs );   Cudd_Ref( bFlex );

    // deref GloZ BDDs
//    Ntk_NetworkForEachNodeSpecialStart( pPivot, pNode )
//        Ntk_NodeDerefFuncGlobZ( pNode );

    // transfer from the set inputs
    if ( p->pPars->TypeFlex == DCMN_TYPE_SS )
    {
//PRB( p->ddGlo, bFlex );
        bFlex = Dcmn_UtilsTransferFromSetOutputs( p, bTemp = bFlex ); Cudd_Ref( bFlex );
        Cudd_RecursiveDeref( p->ddGlo, bTemp );
//PRB( p->ddGlo, bFlex );
    }

    
    // make sure that the global flexibility is well defined
    dd = p->ddGlo;



    if ( p->pPars->TypeFlex != DCMN_TYPE_SS ) 
        bExist = Dcmn_FlexQuantifyFunction( p, bFlex );  
    else 
        bExist = bFlex;

    Cudd_Ref( bExist );
//PRB( dd, bExist );
//PRB( dd, Cudd_Not(bExist) );

    bCubeOut = Vmx_VarMapCharCubeOutput( dd, p->pVmxG );     Cudd_Ref( bCubeOut );
    bDomain  = Cudd_bddExistAbstract( dd, bExist, bCubeOut ); Cudd_Ref( bDomain );
    if ( bDomain != b1 )
    {
        printf( "Global flexibility of node \"%s\" is not well defined!\n", 
            Ntk_NodeGetNamePrintable(pPivot) );
//        PRB( dd, bDomain );
    }
    Cudd_RecursiveDeref( dd, bCubeOut );
    Cudd_RecursiveDeref( dd, bDomain );
    Cudd_RecursiveDeref( dd, bExist );


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
Mvr_Relation_t * Dcmn_ManagerComputeLocalDcNode( Dcmn_Man_t * p, Ntk_Node_t * pNode )
{
    Mvr_Relation_t * pMvrLoc;
    DdNode * bGlobal;
    Ntk_Node_t ** ppFanins;
    int nFanins;
    Ntk_Node_t * pTemp;

    bGlobal = Dcmn_ManagerComputeGlobalDcNode( p, pNode );  Cudd_Ref( bGlobal );
//PRB( p->ddGlo, bGlobal );
    // set the fanins
    nFanins = Ntk_NodeReadFaninNum( pNode );
    ppFanins = ALLOC( Ntk_Node_t *, nFanins );
    nFanins = Ntk_NodeReadFanins( pNode, ppFanins );

    // get the local variable map
    p->pVmxL = Dcmn_UtilsCreateLocalVmx( p, pNode, ppFanins, nFanins );
    pMvrLoc  = Dcmn_FlexComputeLocal( p, bGlobal, ppFanins, nFanins );  
    Cudd_RecursiveDeref( p->ddGlo, bGlobal );


    // deref GloZ BDDs
    Ntk_NetworkForEachNodeSpecialStart( pNode, pTemp )
        Ntk_NodeDerefFuncGlobZ( pTemp );


    FREE( ppFanins );
    return pMvrLoc;
}


/**Function*************************************************************

  Synopsis    [Computes complete flexibility of the node using the product.]

  Description [Returns the complete flexibility of a node expressed
  as an MV relation with the same relation parameters as the original
  MV relation of the node. (The returned relation may be not well-defined
  if the network was out of spec at the moment of calling this procedure.)]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvr_Relation_t * Dcmn_ManagerComputeLocalDcNodeUseProduct( Dcmn_Man_t * p, Ntk_Node_t * pNode )
{
    Mvr_Relation_t * pMvrLoc;
//    DdNode * bGlobal;
    Ntk_Node_t ** ppFanins;
    int nFanins;
//    Ntk_Node_t * pTemp;

//    bGlobal = Dcmn_ManagerComputeGlobalDcNode( p, pNode );  Cudd_Ref( bGlobal );

    // set the fanins
    nFanins = Ntk_NodeReadFaninNum( pNode );
    ppFanins = ALLOC( Ntk_Node_t *, nFanins );
    nFanins = Ntk_NodeReadFanins( pNode, ppFanins );
    // get the local variable map
    p->pVmxL = Dcmn_UtilsCreateLocalVmx( p, pNode, ppFanins, nFanins );
    pMvrLoc  = Dcmn_FlexComputeLocalUseProduct( p, ppFanins, nFanins, pNode );  
//    Cudd_RecursiveDeref( p->ddGlo, bGlobal );

    // deref GloZ BDDs
//    Ntk_NetworkForEachNodeSpecialStart( pNode, pTemp )
//        Ntk_NodeDerefFuncGlobZ( pTemp );
    // should not be there because they are in Dcmn_FlexComputeLocalUseProduct()

    FREE( ppFanins );
    return pMvrLoc;
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
Mvr_Relation_t * Dcmn_ManagerComputeLocalDcNodeUseNsSpec( Dcmn_Man_t * p, Ntk_Node_t * pNode, DdNode * bSpecNs )
{
    Mvr_Relation_t * pMvrLoc;
    DdNode * bGlobal;
    Ntk_Node_t ** ppFanins;
    int nFanins;
    Ntk_Node_t * pTemp;

    bGlobal = Dcmn_ManagerComputeGlobalDcNodeUseNsSpec( p, pNode, bSpecNs );  Cudd_Ref( bGlobal );
//PRB( p->ddGlo, bGlobal );
    // set the fanins
    nFanins = Ntk_NodeReadFaninNum( pNode );
    ppFanins = ALLOC( Ntk_Node_t *, nFanins );
    nFanins = Ntk_NodeReadFanins( pNode, ppFanins );

    // get the local variable map
    p->pVmxL = Dcmn_UtilsCreateLocalVmx( p, pNode, ppFanins, nFanins );
    pMvrLoc  = Dcmn_FlexComputeLocal( p, bGlobal, ppFanins, nFanins );  
    Cudd_RecursiveDeref( p->ddGlo, bGlobal );


    // deref GloZ BDDs
    Ntk_NetworkForEachNodeSpecialStart( pNode, pTemp )
        Ntk_NodeDerefFuncGlobZ( pTemp );


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
void Dcmn_ManagerUpdateGlobalFunctions( Dcmn_Man_t * p, Ntk_Node_t * pNode )
{
    // collect the nodes in the TFO of the given node
    Ntk_NetworkDfsFromNode( p->pNet, pNode );
    // update the global BDDs of these nodes
    Ntk_NetworkForEachNodeSpecial( p->pNet, pNode )
    {
        Ntk_NodeDerefFuncGlob( pNode );
        Dcmn_NodeBuildGlo( p, pNode, p->pPars->TypeFlex != DCMN_TYPE_SS );
    }
}


/**Function*************************************************************

  Synopsis    [Sets the DCMN parameters.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dcmn_ManagerSetParameters( Dcmn_Man_t * p, Dcmn_Par_t * pPars )
{
    p->pPars = pPars;
}


/**Function*************************************************************

  Synopsis    [Returns the local manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdManager * Dcmn_ManagerReadDdLoc( Dcmn_Man_t * p )
{
    return NULL;//p->ddLoc;
}

/**Function*************************************************************

  Synopsis    [Returns the global manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdManager * Dcmn_ManagerReadDdGlo( Dcmn_Man_t * p )
{
    return p->ddGlo;
}

/**Function*************************************************************

  Synopsis    [Returns the global manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dcmn_ManagerReadVerbose( Dcmn_Man_t * p )
{
    return p->pPars->fVerbose;
}

////////////////////////////////////////////////////////////////////////
///                         END OF FILE                              ///
////////////////////////////////////////////////////////////////////////


