/**CFile****************************************************************

  FileName    [fmGlobal.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Computation of global BDD for the trashed networks.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: fmGlobal.c,v 1.3 2003/05/27 23:15:33 alanmi Exp $]

***********************************************************************/

#include "fmInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void            fmGlobaComputeFunctions( DdManager * dd, Sh_Network_t * pNet );
static Mva_FuncSet_t * fmGlobalSaveRootFunctions( DdManager * dd, Sh_Network_t * pNet, Vmx_VarMap_t * pVmx );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Computes the global BDDs of outputs of one network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mva_FuncSet_t * Fm_GlobalCompute( DdManager * dd, Fmw_Manager_t * pMan, Sh_Network_t * pNet )
{
    Mva_FuncSet_t * pRes;

    // collect nodes in the interleaved DFS order
    pNet->nNodes = Sh_NetworkInterleaveNodes( pNet );;
    // clean the network's data fields (assumes the nodes in the list)
    Sh_NetworkCleanData( pNet );

    // create the encoded global variable map
    pMan->pVmxLoc = NULL;
    pMan->pVmxGlo = Vmx_VarMapLookup( pMan->pManVmx, pNet->pVmL, -1, NULL );

    // order the variables 
//    Fm_UtilsCreateVarOrdering( pMan, pNet );

    // resize the manager
    Fm_UtilsResizeManager( dd, pMan->pVmxGlo );

    // assign the elementary BDDs to the leaves
//    Fm_UtilsAssignLeaves( dd, dd->vars, pNet, pMan->pVmxGlo );

    // derive the global BDDs for all nodes in the network
    fmGlobaComputeFunctions( dd, pNet );
    // save the global BDDs in this structure
    pRes = fmGlobalSaveRootFunctions( dd, pNet, pMan->pVmxGlo );

    // deref global BDDs in the nodes
    Fm_UtilsDerefNetwork( dd, pNet );
    return pRes;
}

/**Function*************************************************************

  Synopsis    [Computes the global BDDs of outputs of two network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fm_GlobalComputeTwo( DdManager * dd, Fmw_Manager_t * pMan, Sh_Network_t * pNet1, Sh_Network_t * pNet2,
    Mva_FuncSet_t ** ppRes1, Mva_FuncSet_t ** ppRes2 )
{
    // should take only one network with doubled output space
    // should deal with outputs, which strash into the same AND/INV vertices
}

/**Function*************************************************************

  Synopsis    [Constructs the global BDDs for all nodes in the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void fmGlobaComputeFunctions( DdManager * dd, Sh_Network_t * pNet )
{
    Sh_Node_t * pNode;
    DdNode * bFunc1, * bFunc2, * bFunc;

    // go through the internal nodes in the DFS order
    Sh_NetworkForEachNodeSpecial( pNet, pNode )
    {
        if ( Sh_NodeIsVar(pNode) )
        {   // make sure the elementary BDD is set
            assert( pNode->pData );
            continue;
        }
        // this is the internal node
        // make sure the BDD is not computed
        assert( pNode->pData == 0 );

        // compute the global BDD
        bFunc1 = Fm_DataGetNodeGlo( pNode->pOne );
        bFunc2 = Fm_DataGetNodeGlo( pNode->pTwo );
        // get the result
        bFunc = Cudd_bddAnd( dd, bFunc1, bFunc2 );  Cudd_Ref( bFunc );
        // set it at the node
        Fm_DataSetNodeGlo( pNode, bFunc ); // takes ref

        // ADD HERE THE CASE WITH SPECIAL NODES
    }
}

/**Function*************************************************************

  Synopsis    [Saves the global BDDs of the roots of the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mva_FuncSet_t * fmGlobalSaveRootFunctions( DdManager * dd, Sh_Network_t * pNet, Vmx_VarMap_t * pVmx )
{
    Mva_FuncSet_t * pRes;
    Mva_Func_t * pFunc;
    Sh_Node_t * pNode;
    int nVarsIn, nVarsOut;
    int * pValuesOut;
    int iValue, i, v;

    // get the MV var map
    nVarsIn    = Vm_VarMapReadVarsInNum( pNet->pVmL );
    nVarsOut   = Vm_VarMapReadVarsInNum( pNet->pVmR );
    pValuesOut = Vm_VarMapReadValuesArray( pNet->pVmR );

    // create the resulting set of global BDDs
    pRes = Mva_FuncSetAlloc( pVmx, nVarsOut );

    // create the global BDDs
    iValue = 0;
    for ( i = 0; i < nVarsOut; i++ )
    {
        pFunc = Mva_FuncAlloc( dd, pValuesOut[i] );
        for ( v = 0; v < pFunc->nIsets; v++ )
        {
            pNode = pNet->ppOutputs[iValue++]; 
            pFunc->pbFuncs[v] = (DdNode *)Sh_Regular(pNode)->pData;
            if ( Sh_IsComplement(pNode) )
                pFunc->pbFuncs[v] = Cudd_Not( pFunc->pbFuncs[v] );
            Cudd_Ref( pFunc->pbFuncs[v] );
        }
        Mva_FuncSetAdd( pRes, i, pFunc );
    }
    assert( iValue == Vm_VarMapReadValuesInNum( pNet->pVmR ) );
    return pRes;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


