/**CFile****************************************************************

  FileName    [fmbUtils.c]

  PackageName [Binary flexibility manager.]

  Synopsis    [Various utilities.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 3.0. Started - September 27, 2003.]

  Revision    [$Id: fmbUtils.c,v 1.1 2003/11/18 18:55:10 alanmi Exp $]

***********************************************************************/

#include "fmbInt.h"


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     EXPORTED FUNCTIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Computes the number of nodes in the shared BDD of the POs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fmb_UtilsNumSharedBddNodes( Fmb_Manager_t * pMan )
{
	Ntk_Node_t * pNode;
	DdNode ** pbOutputs;
    int nOutputsAlloc;
	int Counter, RetValue;

    // allocate storage for the PO BDDs
    nOutputsAlloc = 1000;
	pbOutputs = ALLOC( DdNode *, nOutputsAlloc );
	// start the DFS search from the POs of this slice
	Counter = 0;
    Ntk_NetworkForEachNode( pMan->pNet, pNode )
    {
        pbOutputs[Counter++] = Ntk_NodeReadFuncBinGlo(pNode);
        if ( Counter == nOutputsAlloc )
        {
            pbOutputs = REALLOC( DdNode *, pbOutputs, nOutputsAlloc * 2 );
            nOutputsAlloc *= 2;
        }
    }
	RetValue = Cudd_SharingSize( pbOutputs, Counter );
	FREE( pbOutputs );
	return RetValue;

}

/**Function*************************************************************

  Synopsis    [Compute the domain of the relation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Fmb_UtilsRelationDomainCompute( DdManager * dd, DdNode * bRel, Vmx_VarMap_t * pVmx )
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
int Fmb_UtilsRelationDomainCheck( DdManager * dd, DdNode * bRel, Vmx_VarMap_t * pVmx )
{
    DdNode * bDomain;
    bool fRetValue = 1;
    bDomain = Fmb_UtilsRelationDomainCompute( dd, bRel, pVmx ); Cudd_Ref( bDomain );
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
DdNode * Fmb_UtilsRelationGlobalCompute( Fmb_Manager_t * p, Ntk_Node_t * pPivots[], int nPivots, Vmx_VarMap_t * pVmxG )
{
    DdManager * dd = p->ddGlo;
    DdNode * bFunc, * bVar, * bTemp;
    DdNode * bRel, * bComp;
    int nInputs, i;

    nInputs = Vm_VarMapReadVarsInNum( Vmx_VarMapReadVm(pVmxG) );
    bRel = b1;   Cudd_Ref( bRel );
    for ( i = 0; i < nPivots; i++ )
    {
        bFunc = Ntk_NodeReadFuncBinGlo( pPivots[i] );
        bVar  = p->pbVars0[nInputs + i];
        bComp = Cudd_bddXnor( dd, bFunc, bVar );         Cudd_Ref( bComp );
        bRel  = Cudd_bddAnd( dd, bTemp = bRel, bComp );  Cudd_Ref( bRel );
        Cudd_RecursiveDeref( dd, bTemp );
        Cudd_RecursiveDeref( dd, bComp );
    }
    Cudd_Deref( bRel );
    return bRel;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the relation domain is 1.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Node_t * Fmb_UtilsComputeGlobal( Fmb_Manager_t * p )
{
    Ntk_Node_t * pNodeGlo, * pNode;
    int i;

    // create the interleaved ordering of nodes in the window
    Wn_WindowInterleave( p->pWnd );

    // create the node with the fanins ordered the same way
    // create the array of values of the fanins
    pNodeGlo = Ntk_NodeCreate( p->pNet, NULL, MV_NODE_NONE, -1 ); 
    i = 0;
    Ntk_NetworkForEachNodeSpecial( p->pNet, pNode )
        if ( Ntk_NodeReadLevel( pNode ) )  // window leaf
        {
//PRB( p->ddGlo, p->pbVars0[i] );

            // set its global function
            Ntk_NodeWriteFuncBinGlo( pNode, p->pbVars0[i++] );
            Ntk_NodeWriteFuncDd( pNode, p->ddGlo ); 
            // add fanin to the global node
            Ntk_NodeAddFanin( pNodeGlo, pNode );
        }

    // set the global parameters of the network
//    Ntk_NetworkSetDdGlo( pNet, p->ddGlo );
//    Ntk_NetworkSetVmxGlo( pNet, p->pVmxG );

    // compute global functions of the network
    Wn_WindowDfs( p->pWnd );
    Ntk_NetworkForEachNodeSpecial( p->pNet, pNode )
        if ( Ntk_NodeGlobalComputeBinary( p->ddGlo, pNode, 0 ) )
            return NULL; // timeout

    return pNodeGlo;
}

/**Function*************************************************************

  Synopsis    [Composes the special node functions.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int * Fmb_UtilsCreatePermMapRel( Fmb_Manager_t * pMan )
{
    DdManager * dd = pMan->ddGlo;
    int * pPermute, * pBitOrder, * pBitsFirst;
    int i, iVar, nBits;
    int nVarsIn;

    // allocate the permutation map
    pPermute = ALLOC( int, dd->size );
    for ( i = 0; i < dd->size; i++ )
        pPermute[i] = -1;

    // get the parameters
    pBitsFirst = Vmx_VarMapReadBitsFirst( pMan->pVmxL );
    nVarsIn    = Vm_VarMapReadVarsInNum( Vmx_VarMapReadVm(pMan->pVmxL) );

    // add the input variables
    iVar = 0;
    for ( i = 0; i < pBitsFirst[nVarsIn]; i++ )
//        pPermute[pMan->nVars0 + i] = iVar++;
//        pPermute[dd->size/2 + i] = iVar++;
        pPermute[pMan->pbVars1[i]->index] = iVar++;

    // get the parameters
    pBitOrder  = Vmx_VarMapReadBitsOrder( pMan->pVmxG );
    pBitsFirst = Vmx_VarMapReadBitsFirst( pMan->pVmxG );
    nVarsIn    = Vm_VarMapReadVarsInNum( Vmx_VarMapReadVm(pMan->pVmxG) );
    nBits      = Vmx_VarMapReadBitsNum( pMan->pVmxG );

    // add the output variables
    for ( i = pBitsFirst[nVarsIn]; i < nBits; i++ )
        pPermute[pBitOrder[i]] = iVar++;
    return pPermute;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if at least one of the fanins of node has GloZ.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Fmb_UtilsHasZFanin( Ntk_Node_t * pNode )
{
    Ntk_Node_t * pFanin;
    Ntk_Pin_t * pPin;
//    assert( Ntk_NodeReadFuncBinGloZ(pNode) == NULL );
    if ( Ntk_NodeReadFuncBinGloZ(pNode) ) // pivot node
        return 0;
    Ntk_NodeForEachFanin( pNode, pPin, pFanin )
        if ( Ntk_NodeReadFuncBinGloZ(pFanin) )
            return 1;
    return 0;
}

////////////////////////////////////////////////////////////////////////
///                         END OF FILE                              ///
////////////////////////////////////////////////////////////////////////


