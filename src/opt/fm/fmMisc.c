/**CFile****************************************************************

  FileName    [fmMisc.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Miscellaneous utilities used by the flexibility manager.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: fmMisc.c,v 1.3 2004/04/08 05:05:07 alanmi Exp $]

***********************************************************************/

#include "fmInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void       fmUtilsDerefNetwork_rec( DdManager * dd, Sh_Network_t * pNet, Sh_Node_t * pNode );
static int        fmUtilsVarCompareByWeight( int * ptrX, int * ptrY );
static double *   s_pWeights = NULL;

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Derives an interleaved var ordering for the strashed network.]

  Description [This procedure orders the leaves, the special nodes (if present),
  and the roots. This procedure assumes that the nodes are linked into the special
  linked list and that the data fields are clean. As a byproduce, collects all
  the nodes corresponding to value in pVmx into an array.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vmx_VarMap_t * Fm_UtilsCreateVarOrdering( Vmx_VarMap_t * pVmxGlo, Sh_Network_t * pNet )
{
    Vmx_VarMap_t * pVmxGloNew;
    Vm_VarMap_t * pVm;
    Sh_Node_t * pNode;
    double * pWeights;
    int * pVarOrder, * pVarOrderInv, * pValues;
    int nVarsIn, nVarsOut, nVars;
    int nValuesIn, nValuesOut, nValues;
    int i, v, iValue;

    // get the params
    pVm        = Vmx_VarMapReadVm( pVmxGlo );
    nVarsIn    = Vm_VarMapReadVarsInNum( pVm );
    nVarsOut   = Vm_VarMapReadVarsOutNum( pVm );
    nVars      = nVarsIn + nVarsOut;
    nValuesIn  = Vm_VarMapReadValuesInNum( pVm );
    nValuesOut = Vm_VarMapReadValuesOutNum( pVm );
    nValues    = nValuesIn + nValuesOut;
    pValues    = Vm_VarMapReadValuesArray( pVm );

    // the number of inputs and special is the same as the number of input values
    assert( nValuesIn  == pNet->nInputs );

    // mark the nodes
    for ( i = 0; i < pNet->nInputs; i++ )
        pNet->pMan->pVars[i]->pData2 = 1;

    // go through the node and set their numbers in the interleaved ordering
    i = 1;
    Sh_NetworkForEachNodeSpecial( pNet, pNode )
        if ( pNode->pData2 )
            pNode->pData = i++;

    // find the average weight of each MV var
    pVarOrder    = ALLOC( int,    nVars );
    pVarOrderInv = ALLOC( int,    nVars );
    pWeights     = ALLOC( double, nVars );
    // collect the weights of these nodes
    iValue  = 0;
    for ( i = 0; i < nVarsIn; i++ )
    {
        pWeights[i] = 0.0;
        for ( v = 0; v < pValues[i]; v++ )
            pWeights[i] += pNet->pMan->pVars[iValue++]->pData;
        pWeights[i] /= pValues[i];
    }
    assert( iValue == nValuesIn );

    // clean the input data fields
    for ( i = 0; i < pNet->nInputs; i++ )
       pNet->pMan->pVars[i]->pData = pNet->pMan->pVars[i]->pData2 = 0;


    // sort the vars in the increasing order of weights
    for ( i = 0; i < nVarsIn; i++ )
    {
        pVarOrder[i] = i;
        // if weight is 0, it means the PI is unreachable from POs, order it at the end
        if ( pWeights[i] == 0.0 )
            pWeights[i] = 1000000.0;
    }
    s_pWeights = pWeights;
    qsort( (void *)pVarOrder, nVarsIn,
        sizeof(int),(int (*)(const void *, const void *))fmUtilsVarCompareByWeight);
    s_pWeights = NULL;
    

//Vmx_VarMapPrint( pMan->pVmxGlo );
//for ( i = 0; i < Vm_VarMapReadVarsNum( Vmx_VarMapReadVm(pMan->pVmxGlo) ); i++ )
//printf( "Var = %2d. Order = %2d\n", i, pVarOrder[i] );
    // inverse this order
    for ( i = 0; i < nVarsIn; i++ )
        pVarOrderInv[ pVarOrder[i] ] = i;
    // set the remaining vars to be the same
    for ( i = nVarsIn; i < nVars; i++ )
        pVarOrderInv[i] = i;
    // update the variable map given the order
    pVmxGloNew = Vmx_VarMapCreateOrdered( pVmxGlo, pVarOrderInv );
//Vmx_VarMapPrint( pMan->pVmxGlo );

    FREE( pVarOrderInv );
    FREE( pVarOrder );
    FREE( pWeights );

    return pVmxGloNew;
}

/**Function*************************************************************

  Synopsis    [Compares the vars by their number.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int fmUtilsVarCompareByWeight( int * ptrX, int * ptrY )
{
    if ( s_pWeights[*ptrX] < s_pWeights[*ptrY] )
        return 1;
    else if ( s_pWeights[*ptrX] > s_pWeights[*ptrY] )
        return -1;
    return 0;
} 

/**Function*************************************************************

  Synopsis    [Assigns the elementary BDDs to the leaves of the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fm_UtilsAssignLeaves( DdManager * dd, DdNode * pbVars[], Sh_Network_t * pNet, Vmx_VarMap_t * pVmx, bool fOutputsOnly )
{
    Vm_VarMap_t * pVm;
    DdNode ** pbCodes;
    int i;

    pVm = Vmx_VarMapReadVm( pVmx );
    assert( pNet->nInputs      == Vm_VarMapReadValuesInNum(pVm) );
    assert( pNet->nOutputsCore == Vm_VarMapReadValuesOutNum(pVm) );

    // encode the var map
    pbCodes = Vmx_VarMapEncodeMapUsingVars( dd, pbVars, pVmx );

    // set the input code into Glo
    if ( !fOutputsOnly )
    {
        for ( i = 0; i < pNet->nInputs; i++ )
        {
            Cudd_Ref( pbCodes[i] );
            Fm_DataSetNodeGlo( pNet->pMan->pVars[i], pbCodes[i] );
        }

        // set the output codes into GloZ
        for ( i = 0; i < pNet->nOutputsCore; i++ )
        {
            Cudd_Ref( pbCodes[pNet->nInputs+i] );
            Fm_DataSetNodeGloZ( pNet->ppOutputsCore[i], pbCodes[pNet->nInputs+i] );
        }
    }
    else
    {

        // set the output codes into Glo
        for ( i = 0; i < pNet->nOutputsCore; i++ )
        {
            Cudd_Ref( pbCodes[pNet->nInputs+i] );
            Fm_DataSetNodeGlo( pNet->ppOutputsCore[i], pbCodes[pNet->nInputs+i] );
        }
    }

    // deref the codes
    Vmx_VarMapEncodeDeref( dd, pVmx, pbCodes );   // refs are taken
}

/**Function*************************************************************

  Synopsis    [Assigns the elementary BDDs to the leaves of the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fm_UtilsAssignLeavesSet( DdManager * dd, DdNode * pbVars[], Sh_Network_t * pNet, Vmx_VarMap_t * pVmx, bool fOutputsOnly )
{
    Vm_VarMap_t * pVm;
    DdNode ** pbCodes;
    int i;

    pVm = Vmx_VarMapReadVm( pVmx );
    assert( pNet->nInputs      == Vm_VarMapReadValuesInNum(pVm) );
    assert( pNet->nOutputsCore == Vm_VarMapReadVarsOutNum(pVm) );

    // encode the var map
    pbCodes = Vmx_VarMapEncodeMapUsingVars( dd, pbVars, pVmx );

    // set the input code into Glo
    if ( !fOutputsOnly )
    {
        for ( i = 0; i < pNet->nInputs; i++ )
        {
            Cudd_Ref( pbCodes[i] );
            Fm_DataSetNodeGlo( pNet->pMan->pVars[i], pbCodes[i] );
        }

        // set the output codes into GloZ
        for ( i = 0; i < pNet->nOutputsCore; i++ )
        {
            Cudd_Ref( pbCodes[pNet->nInputs + 2*i + 1] );
            assert( !Cudd_IsComplement(pbCodes[pNet->nInputs + 2*i + 1]) );
            Fm_DataSetNodeGloZ( pNet->ppOutputsCore[i], pbCodes[pNet->nInputs + 2*i + 1] );
        }
    }
    else
    {
        // set the output codes into GloZ
        for ( i = 0; i < pNet->nOutputsCore; i++ )
        {
            Cudd_Ref( pbCodes[pNet->nInputs + 2*i + 1] );
            assert( !Cudd_IsComplement(pbCodes[pNet->nInputs + 2*i + 1]) );
            Fm_DataSetNodeGlo( pNet->ppOutputsCore[i], pbCodes[pNet->nInputs + 2*i + 1] );
        }
    }

    // deref the codes
    Vmx_VarMapEncodeDeref( dd, pVmx, pbCodes );   // refs are taken
}

/**Function*************************************************************

  Synopsis    [Dereferences the global BDDs of the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fm_UtilsDerefNetwork( DdManager * dd, Sh_Network_t * pNet )
{
    Sh_Node_t * pNode;
    int i;
    // set the traversal ID for this DFS ordering
    Sh_ManagerIncrementTravId( pNet->pMan );
    // set the trav ID and deref the leaves
    for ( i = 0; i < pNet->nInputs; i++ )
    {
        Fm_DataDerefNodeGlo( dd, pNet->pMan->pVars[i] );
        // mark the node as visited
        Sh_NodeSetTravIdCurrent( pNet->pMan, pNet->pMan->pVars[i] );
    }
    // dereference specials
    for ( i = 0; i < pNet->nSpecials; i++ )
        Fm_DataDerefNodeGlo( dd, pNet->ppSpecials[i] );
    // dereference outupts
    for ( i = 0; i < pNet->nOutputsCore; i++ )
        Fm_DataDerefNodeGlo( dd, pNet->ppOutputsCore[i] );

    // recursively call for the roots
//    for ( i = 0; i < pNet->nOutputs; i++ )
//        fmUtilsDerefNetwork_rec( dd, pNet, Sh_Regular(pNet->ppOutputs[i]) );

    // dereference nodes in the list
    Sh_NetworkForEachNodeSpecial( pNet, pNode )
        Fm_DataDerefNodeGlo( dd, pNode );
}

/**Function*************************************************************

  Synopsis    [Dereferences the global BDDs of the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void fmUtilsDerefNetwork_rec( DdManager * dd, Sh_Network_t * pNet, Sh_Node_t * pNode )
{
    // check if the node is visited
    if ( Sh_NodeIsTravIdCurrent(pNet->pMan, pNode) )
        return;
    assert( shNodeIsAnd(pNode) );
    // mark the node as visited
    Sh_NodeSetTravIdCurrent( pNet->pMan, pNode );
    // solve the problem for the "cofactors"
    fmUtilsDerefNetwork_rec( dd, pNet, Sh_Regular(pNode->pOne) );
    fmUtilsDerefNetwork_rec( dd, pNet, Sh_Regular(pNode->pTwo) );
    // deref the node
    Fm_DataDerefNodeGlo( dd, pNode );
}


/**Function*************************************************************

  Synopsis    [Resizes the BDD manager to fit for the given encoded map.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fm_UtilsResizeManager( DdManager * dd, Vmx_VarMap_t * pVmx )
{
    int nBits, i;
    nBits = Vmx_VarMapReadBitsNum(pVmx);
    if ( dd->size < nBits )
        for ( i = dd->size; i < nBits; i++ )
            Cudd_bddNewVar( dd );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


