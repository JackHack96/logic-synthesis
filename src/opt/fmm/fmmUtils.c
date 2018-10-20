/**CFile****************************************************************

  FileName    [fmmUtils.c]

  PackageName [Multi-valued flexibility manager.]

  Synopsis    [Various utilities.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 3.0. Started - September 27, 2003.]

  Revision    [$Id: fmmUtils.c,v 1.1 2003/11/18 18:55:12 alanmi Exp $]

***********************************************************************/

#include "fmmInt.h"
////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     EXPORTED FUNCTIONS                           ///
////////////////////////////////////////////////////////////////////////
/**Function*************************************************************

  Synopsis    [Determine the largest support size.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fmm_UtilsFaninMddSupportMax( Ntk_Network_t * pNet )
{
    Ntk_Node_t * pNode;
    int nSuppMax, nBits;

    // count the largest local support size
    nSuppMax = 0;
    Ntk_NetworkForEachNode( pNet, pNode )
    {
        nBits = Vm_VarMapGetBitsNum( Ntk_NodeReadFuncVm(pNode) );
        if ( nSuppMax < nBits )
            nSuppMax = nBits;
    }
    if ( pNet->pNetExdc )
    {
        Ntk_NetworkForEachNode( pNet->pNetExdc, pNode )
        {
            nBits = Vm_VarMapGetBitsNum( Ntk_NodeReadFuncVm(pNode) );
            if ( nSuppMax < nBits )
                nSuppMax = nBits;
        }
    }
    return nSuppMax;
}

/**Function*************************************************************

  Synopsis    [Returns the Vmx object that includes CI, interm, and CO nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vmx_VarMap_t * Fmm_UtilsGetNetworkVmx( Ntk_Network_t * pNet )
{
    Vm_VarMap_t * pVm;
    Mvr_Relation_t * pMvr;
    Ntk_Node_t * pNode;
    int * pVarValues;
    int nVars, nOuts;

    pVarValues = ALLOC( int, Ntk_NetworkReadNodeTotalNum(pNet) );

    // add the CI nodes
    nVars = 0;
    Ntk_NetworkForEachCi( pNet, pNode )
    {
        pVarValues[nVars] = pNode->nValues;
        Ntk_NodeWriteFuncNumber( pNode, nVars );
        Ntk_NodeWriteFuncNonDet( pNode, 0 );
        nVars++;
    }

    // check whether the nodes are non-deterministic, and mark them
    Ntk_NetworkForEachNode( pNet, pNode )
    {
        pMvr = Ntk_NodeGetFuncMvr( pNode );
        if ( Mvr_RelationIsND( pMvr ) ) 
        {
            pVarValues[nVars] = pNode->nValues;
            Ntk_NodeWriteFuncNumber( pNode, nVars );
            Ntk_NodeWriteFuncNonDet( pNode, 1 );
            nVars++;
        }
        else
            Ntk_NodeWriteFuncNonDet( pNode, 0 );

    }

    // add the CO nodes
    nOuts = Ntk_NetworkReadCoNum(pNet);
    Ntk_NetworkForEachCo( pNet, pNode )
    {
        pVarValues[nVars] = pNode->nValues;
        Ntk_NodeWriteFuncNumber( pNode, nVars );
        Ntk_NodeWriteFuncNonDet( pNode, 0 );
        nVars++;
    }

    // create the maps
    pVm = Vm_VarMapLookup( Ntk_NetworkReadManVm(pNet), nVars - nOuts, nOuts, pVarValues );
    FREE( pVarValues );
    return Vmx_VarMapLookup( Ntk_NetworkReadManVmx(pNet), pVm, -1, NULL );
}

/**Function*************************************************************

  Synopsis    [Computes the number of nodes in the shared BDD of the POs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fmm_UtilsNumSharedBddNodes( Fmm_Manager_t * pMan )
{
	Ntk_Node_t * pNode;
	DdNode ** pbOutputs, ** pbFuncs;
	int Counter, RetValue, i;
    int nOutputsAlloc;

    // allocate storage for the PO BDDs
    nOutputsAlloc = 1000;
	pbOutputs = ALLOC( DdNode *, nOutputsAlloc );
	// start the DFS search from the POs of this slice
	Counter = 0;
    Ntk_NetworkForEachNode( pMan->pNet, pNode )
    {
        pbFuncs = Ntk_NodeReadFuncGlob(pNode);
        for ( i = 0; i < pNode->nValues; i++ )
        {
            pbOutputs[Counter] = pbFuncs[Counter];
            Counter++;
            if ( Counter == nOutputsAlloc )
            {
                pbOutputs = REALLOC( DdNode *, pbOutputs, nOutputsAlloc * 2 );
                nOutputsAlloc *= 2;
            }
        }
    }
//    assert( Counter == pMan->nExdcs );
	RetValue = Cudd_SharingSize( pbOutputs, Counter );
	FREE( pbOutputs );
	return RetValue;

}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vmx_VarMap_t * Fmm_UtilsCreateGlobalVmx( Fmm_Manager_t * p, Ntk_Node_t * pNode )
{
/*
//    Ntk_Node_t * pNodeCi;
    Vm_VarMap_t * pVm;
    Vmx_VarMap_t * pVmx;
    int * pValues;
    int * pVarValues;
    int nInputs, i;
//    nInputs = Ntk_NetworkReadCiNum( p->pNet );
//    nInputs = Vm_VarMapReadVarsInNum(p->pVm);  /////// RECENT CHANGE
    nInputs = Vm_VarMapReadVarsNum(p->pVm);
    pVarValues = ALLOC( int, nInputs + 1 );
//    Ntk_NetworkForEachCi( p->pNet, pNodeCi )
//        pVarValues[i++] = pNodeCi->nValues;
    pValues = Vm_VarMapReadValuesArray(p->pVm);
    i = 0;
    for ( i = 0; i < nInputs; i++ )
        pVarValues[i] = pValues[i];
    pVarValues[i++] = pNode->nValues;

    pVm = Vm_VarMapLookup( Ntk_NodeReadManVm(pNode), nInputs, 1, pVarValues );
    pVmx = Vmx_VarMapLookup( Ntk_NodeReadManVmx(pNode), pVm, -1, NULL );
    FREE( pVarValues );
    return pVmx;
*/

    Vm_VarMap_t * pVm;
    pVm = Vm_VarMapCreateOnePlus( p->pVm, pNode->nValues );
    return Vmx_VarMapLookup( Ntk_NodeReadManVmx(pNode), pVm, -1, NULL );
}

/**Function*************************************************************

  Synopsis    [Check ND.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vmx_VarMap_t * Fmm_UtilsCreateGlobalSetVmx( Fmm_Manager_t * p, Ntk_Node_t * pNode )
{
/*
//    Ntk_Node_t * pNodeCi;
    Vm_VarMap_t * pVm;
    Vmx_VarMap_t * pVmx;
    int * pValues;
    int * pVarValues;
    int nInputs, i, k;
//    nInputs = Ntk_NetworkReadCiNum( p->pNet );
//    nInputs = Vm_VarMapReadVarsInNum(p->pVm);  /////// RECENT CHANGE
    nInputs = Vm_VarMapReadVarsNum(p->pVm);
    pVarValues = ALLOC( int, nInputs + pNode->nValues );
//    Ntk_NetworkForEachCi( p->pNet, pNodeCi )
//        pVarValues[i++] = pNodeCi->nValues;
    pValues = Vm_VarMapReadValuesArray(p->pVm);
    i = 0;
    for ( i = 0; i < nInputs; i++ )
        pVarValues[i] = pValues[i];
    for ( k = 0; k < pNode->nValues; k++ )
        pVarValues[i + k] = 2;

    pVm = Vm_VarMapLookup( Ntk_NodeReadManVm(pNode), nInputs, pNode->nValues, pVarValues );
    pVmx = Vmx_VarMapLookup( Ntk_NodeReadManVmx(pNode), pVm, -1, NULL );
    FREE( pVarValues );
    return pVmx;
*/

    Vm_VarMap_t * pVm, * pVmOut;
    pVmOut = Vm_VarMapCreateBinary( Ntk_NodeReadManVm(pNode), pNode->nValues, 0 );
    pVm = Vm_VarMapCreateInputOutput( p->pVm, pVmOut );
    return Vmx_VarMapLookup( Ntk_NodeReadManVmx(pNode), pVm, -1, NULL );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vmx_VarMap_t * Fmm_UtilsCreateLocalVmx( Fmm_Manager_t * p, Ntk_Node_t * pNode, Ntk_Node_t ** ppFanins, int nFanins )
{
    Vm_VarMap_t * pVm;
    Vmx_VarMap_t * pVmx;
    int * pVarValues, i;
    pVarValues = ALLOC( int, nFanins + 1 );
    for ( i = 0; i < nFanins; i++ )
        pVarValues[i] = ppFanins[i]->nValues;
    pVarValues[i] = pNode->nValues;
    pVm  = Vm_VarMapLookup( Ntk_NodeReadManVm(pNode), nFanins, 1, pVarValues );
    pVmx = Vmx_VarMapLookup( Ntk_NodeReadManVmx(pNode), pVm, -1, NULL );
    FREE( pVarValues );
    return pVmx;
}

/**Function*************************************************************

  Synopsis    [This procedure transfers from the set output to normal outputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Fmm_UtilsTransferFromSetOutputs( Fmm_Manager_t * pMan, DdNode * bFlexGlo )
{
    Vm_VarMap_t * pVm, * pVmInit;
    DdManager * dd;
    DdNode ** pbFuncs;
    DdNode ** pbCodes;
    DdNode * bResult;
    int nVarsIn, nValuesIn, nBits, i;
    int * pBitsFirst, * pValues;

    assert( pMan->pVmxGS );

    // get the parameters
    dd         = pMan->ddGlo;
    pVmInit    = Vmx_VarMapReadVm( pMan->pVmxG );
    pVm        = Vmx_VarMapReadVm( pMan->pVmxGS );
    nVarsIn    = Vm_VarMapReadVarsInNum( pVm );
    nValuesIn  = Vm_VarMapReadValuesInNum( pVm );
    nBits      = Vmx_VarMapReadBitsNum( pMan->pVmxGS );
    pBitsFirst = Vmx_VarMapReadBitsFirst( pMan->pVmxGS );
    pValues    = Vm_VarMapReadValuesArray( Vmx_VarMapReadVm(pMan->pVmxG) );

    assert( nBits - pBitsFirst[nVarsIn] == Vm_VarMapReadValuesOutNum(pVmInit) );

    // set up the vector-compose problem
    pbFuncs = ALLOC( DdNode *, dd->size );
    for ( i = 0; i < dd->size; i++ )
        pbFuncs[i] = dd->vars[i];

    // encode the output variable of the global map
    pbCodes  = Vmx_VarMapEncodeVar( dd, pMan->pVmxG, nVarsIn );
    assert( nBits - pBitsFirst[nVarsIn] == pValues[nVarsIn] );

    // modify by setting the composition relations
    for ( i = pBitsFirst[nVarsIn]; i < nBits; i++ )
    {
        pbFuncs[i] = pbCodes[i - pBitsFirst[nVarsIn]];
//printf( "i = %d\n", i );
//PRB( dd, pbCodes[nValuesIn + (i - pBitsFirst[nVarsIn])] );
//PRB( dd, pbCodes[i - pBitsFirst[nVarsIn]] );
    }

    // perform the composition
    bResult = Cudd_bddVectorCompose( dd, bFlexGlo, pbFuncs );  Cudd_Ref( bResult );

    // deref the codes
    Vmx_VarMapEncodeDeref( dd, pMan->pVmxG, pbCodes );
    FREE( pbFuncs );

    Cudd_Deref( bResult );
    return bResult;
}


/**Function*************************************************************

  Synopsis    [Adds the EXDCs to the global functions of the COs.]

  Description [The global functions of the COs should be computed by now.
  This function adds EXDC to each i-set of the global MDDs of COs, 
  and saves the resulting COs in the spec MDDs of the primary network.
  If EXDC is not given, it simply saves the global functions in the
  global S functions.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fmm_UtilsPrepareSpec( Ntk_Network_t * pNet )
{
    DdManager * dd;
    DdNode * bExdc;
    DdNode ** pbFuncs, ** pbFuncsS;
    Ntk_Node_t * pNode, * pNodeExdc;

    // create spec for each CO
    dd = Ntk_NetworkReadDdGlo( pNet );
    Ntk_NetworkForEachCo( pNet, pNode )
    {
        // copy functions from Glo into GloS
        pbFuncs = Ntk_NodeReadFuncGlob( pNode );
        pbFuncsS = Ntk_NodeReadFuncGlobS( pNode );
        assert( pbFuncs[0] );
        assert( pbFuncsS[0] == NULL );
        Ntk_NodeGlobalCopyFuncs( pbFuncsS, pbFuncs, pNode->nValues );
        Ntk_NodeGlobalRefFuncs( pbFuncsS, pNode->nValues );

        // get hold of EXDC node for this output
        if ( pNet->pNetExdc )
            pNodeExdc = Ntk_NetworkFindNodeByName( pNet->pNetExdc, pNode->pName );
        else
            pNodeExdc = NULL;
        if ( pNodeExdc )
        {
            // add the functions from pNodeExdc
            bExdc = (Ntk_NodeReadFuncGlob(pNodeExdc))[1]; 
            Ntk_NodeGlobalAddToFuncs( dd, pbFuncsS, bExdc, pNode->nValues );
        }
    }
}


/**Function*************************************************************

  Synopsis    [Compute the domain of the relation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Fmm_UtilsRelationDomainCompute( DdManager * dd, DdNode * bRel, Vmx_VarMap_t * pVmx )
{
    DdNode * bCubeOut, * bDomain;
    // make sure that the global flexibility is well defined
    bCubeOut = Vmx_VarMapCharCubeOutput( dd, pVmx );        Cudd_Ref( bCubeOut );
    bDomain  = Cudd_bddExistAbstract( dd, bRel, bCubeOut ); Cudd_Ref( bDomain );
    Cudd_RecursiveDeref( dd, bCubeOut );
    Cudd_Deref( bDomain );
    return bDomain;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the relation domain is 1.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fmm_UtilsRelationDomainCheck( DdManager * dd, DdNode * bRel, Vmx_VarMap_t * pVmx )
{
    DdNode * bDomain;
    bool fRetValue = 1;
    bDomain = Fmm_UtilsRelationDomainCompute( dd, bRel, pVmx ); Cudd_Ref( bDomain );
    if ( bDomain != b1 )
    {
        fRetValue = 0;
        printf( "Global flexibility of node is not well defined!\n" );
    }
    Cudd_RecursiveDeref( dd, bDomain );
    return fRetValue;
}


/**Function*************************************************************

  Synopsis    [Returns 1 if the relation domain is 1.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Fmm_UtilsRelationGlobalCompute( DdManager * dd, Ntk_Node_t * pPivot, Vmx_VarMap_t * pVmxG )
{
    DdNode ** pbFuncs, ** pbCodes;
    DdNode * bFlex;
    pbFuncs = Ntk_NodeReadFuncGlob( pPivot );
    pbCodes = Vmx_VarMapEncodeVar( dd, pVmxG, Ntk_NetworkReadCiNum(pPivot->pNet) );
    bFlex = Ntk_NodeGlobalConvolveFuncs( dd, pbFuncs, pbCodes, pPivot->nValues );
    Cudd_Ref( bFlex );
    Vmx_VarMapEncodeDeref( dd, pVmxG, pbCodes );
    Cudd_Deref( bFlex );
    return bFlex;
}

////////////////////////////////////////////////////////////////////////
///                         END OF FILE                              ///
////////////////////////////////////////////////////////////////////////

