/**CFile****************************************************************

  FileName    [ntkiUnreach.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Computation of the unreachable states of the sequential circuit.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ntkiUnreach.c,v 1.3 2004/04/08 05:05:05 alanmi Exp $]

***********************************************************************/

#include "ntkiInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void      Ntk_NetworkVarsAndResetValues( Ntk_Network_t * pNet, int * pVarsCs, int * pVarsNs, int * pCodes, int fLatchOnly );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Computes the unreachable states.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Ntk_NetworkComputeUnreach( Ntk_Network_t * pNet, int fVerbose )
{
    DdManager * dd;
    Extra_ImageTree_t * pTree;
    Vmx_VarMap_t * pVmxExt;
    DdNode * bInitState, * bFront, * bNext;
    DdNode * bReached, * bTemp;
    DdNode ** pbParts, ** pbVarsNs;
    DdNode * bVarsCs, * bVarsNs;
    int nVarsMvIn, nVarsBinNs;
    int nParts, nIters;

    // compute the global latch excitation functions
    Ntk_NetworkGlobalComputeCo( pNet, 1, 1, 0 );
    dd = Ntk_NetworkReadDdGlo( pNet );

    // create the extended variable map for latches only
    pVmxExt = Ntk_NetworkGlobalCreateVmxCo( pNet, 1 );
    Ntk_NetworkSetVmxGlo( pNet, pVmxExt );

    // create the partitions
    nParts = Ntk_NetworkReadLatchNum( pNet );
    pbParts = Ntk_NetworkDerivePartitions( pNet, 1, 1 );

    // save the BDD manager
    Ntk_NetworkWriteDdGlo( pNet, NULL );

    // dereference the global latch excitation functions
    Ntk_NetworkGlobalDeref( pNet );

    // return the manager back where it belongs
    Ntk_NetworkSetDdGlo( pNet, dd );

    // reorder the partions

//    Cudd_AutodynEnable( dd, CUDD_REORDER_SYMM_SIFT );

    // get the initial state and set up the permutation CS <-> NS
    bInitState = Ntk_NetworkComputeInitState( pNet, 1 );   Cudd_Ref( bInitState );

    // get the NS variables (not to be quantified)
    nVarsMvIn  = Ntk_NetworkReadCiNum( pNet );
    nVarsBinNs = Vmx_VarMapBitsNumRange( pVmxExt, nVarsMvIn, nParts );
    pbVarsNs   = Vmx_VarMapBinVarsRange( dd, pVmxExt, nVarsMvIn, nParts );

    // for scheduing, use the vector of CS variables 
    // (because the initial state may not depend on all of them)
    bVarsNs = Cudd_bddComputeCube( dd, pbVarsNs, NULL, nVarsBinNs );   Cudd_Ref( bVarsNs );
    bVarsCs = Cudd_bddVarMap( dd, bVarsNs );                           Cudd_Ref( bVarsCs );
    Cudd_RecursiveDeref( dd, bVarsNs );

    // set up the image computation
    pTree = Extra_bddImageStart( dd, bVarsCs, nParts, pbParts, nVarsBinNs, pbVarsNs, fVerbose );
    Cudd_RecursiveDeref( dd, bVarsCs );
    FREE( pbVarsNs );


    // perform reachability analisys
    bReached = bInitState;              Cudd_Ref( bInitState );
    bFront   = bInitState;              Cudd_Ref( bInitState ); 
    nIters = 0;
    while ( 1 )
    {
        nIters++;
        // compute the next states
        bNext = Extra_bddImageCompute( pTree, bFront );         Cudd_Ref( bNext );
        Cudd_RecursiveDeref( dd, bFront );
        // remap these states into the current state vars
        bNext = Cudd_bddVarMap( dd, bTemp = bNext );            Cudd_Ref( bNext );
        Cudd_RecursiveDeref( dd, bTemp );
//PRB( dd, bNext );
        // check if there are any new states
        if ( Cudd_bddLeq( dd, bNext, bReached ) )
        {
            Cudd_RecursiveDeref( dd, bNext );
            break;
        }
        // get the new states
        bFront = Cudd_bddAnd( dd, bNext, Cudd_Not(bReached) );  Cudd_Ref( bFront );
        // minimize the new states with the reached states
//        bFront = Cudd_bddRestrict( dd, bTemp = bFront, Cudd_Not(bReached) ); Cudd_Ref( bFront );
//        Cudd_RecursiveDeref( dd, bTemp );
        // add to the reached states
        bReached = Cudd_bddOr( dd, bTemp = bReached, bNext );  Cudd_Ref( bReached );
        Cudd_RecursiveDeref( dd, bTemp );
        Cudd_RecursiveDeref( dd, bNext );

        if ( nIters % 50 )
            fprintf( stdout, "%d iterations\r", nIters );
    }
    fprintf( stdout, "Reachability analysis completed in %d iterations.\n", nIters );
//PRB( dd, bReached );
    printf( "The number of minterms in the reachable state set = %d.\n", (int)Cudd_CountMinterm(dd, bReached, nVarsBinNs) ); 

    // deref the partitions
    Ntk_NodeGlobalDerefFuncs( dd, pbParts, nParts );
    FREE( pbParts );
    Cudd_RecursiveDeref( dd, bInitState );
    // undo the image tree
    Extra_bddImageTreeDelete( pTree );

//    printf( "The BDD size of the reachable states = %d.\n", Cudd_DagSize( bReached ) );
    Cudd_ReduceHeap( dd, CUDD_REORDER_SYMM_SIFT, 1 );
//    printf( "The BDD size of the reachable states = %d.\n", Cudd_DagSize( bReached ) );

    Cudd_Deref( bReached );
    return Cudd_Not( bReached );
}

/**Function*************************************************************

  Synopsis    [Derive the partitions corresponding to latches.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode ** Ntk_NetworkDerivePartitions( Ntk_Network_t * pNet, int fLatchOnly, int fMapIsLatchOnly )
{
    DdManager * dd;
    Vmx_VarMap_t * pVmxExt;
    Ntk_Node_t * pNode;
    DdNode ** pbCodes, ** pbCodesOne;
    DdNode ** pbParts, ** pbFuncs;
    int iPart, iCoNum, nVarsMvIn, nParts;
    int * pValuesFirst;

    // allocate room for partitions
    nParts = Ntk_NetworkReadCoNum( pNet );
    pbParts = ALLOC( DdNode *, nParts );

    // get the global map with latches
    pVmxExt = Ntk_NetworkReadVmxGlo( pNet );
    nVarsMvIn = Ntk_NetworkReadCiNum( pNet );
    pValuesFirst = Vm_VarMapReadValuesFirstArray( Vmx_VarMapReadVm(pVmxExt) );

    // encode the map
    dd = Ntk_NetworkReadDdGlo( pNet );
    pbCodes = Vmx_VarMapEncodeMap( dd, pVmxExt );

    // create partitions
    iPart = 0;
    iCoNum = 0;
    Ntk_NetworkForEachCo( pNet, pNode )
    {
        if (  fLatchOnly && pNode->Subtype != MV_BELONGS_TO_LATCH ||
             !fLatchOnly && pNode->Subtype == MV_BELONGS_TO_LATCH )
        {
            iCoNum++;
            continue;
        }
        // get the global functions
        pbFuncs = Ntk_NodeReadFuncGlob( pNode );
        pbCodesOne = pbCodes + pValuesFirst[nVarsMvIn + (fMapIsLatchOnly?iPart:iCoNum)];
        pbParts[iPart] = Ntk_NodeGlobalConvolveFuncs( dd, pbFuncs, pbCodesOne, pNode->nValues ); Cudd_Ref( pbParts[iPart] );
        iPart++;
        iCoNum++;
    }
    if ( fLatchOnly )
    {
        assert( iPart == Ntk_NetworkReadLatchNum( pNet ) );
    }
    else
    {
        assert( iPart == Ntk_NetworkReadCoNum( pNet ) - Ntk_NetworkReadLatchNum( pNet ) );
    }

    Vmx_VarMapEncodeDeref( dd, pVmxExt, pbCodes );
    return pbParts;
}

/**Function*************************************************************

  Synopsis    [Computes the code of the initial state.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Ntk_NetworkComputeInitState( Ntk_Network_t * pNet, int fLatchOnly )
{
    DdManager * dd;
    DdNode * bInitState;
    Vmx_VarMap_t * pVmxExt;
    int * pCodes, * pVarsCs, * pVarsNs;

    // create the codes
    dd = Ntk_NetworkReadDdGlo( pNet );
    pVmxExt = Ntk_NetworkReadVmxGlo( pNet );

    // collect the CS and NS var numbers and the reset values
    pVarsCs = ALLOC( int, dd->size );
    pVarsNs = ALLOC( int, dd->size );
    pCodes  = ALLOC( int, dd->size );
    Ntk_NetworkVarsAndResetValues( pNet, pVarsCs, pVarsNs, pCodes, fLatchOnly );

    // create the variable map
    Vmx_VarMapRemapSetup( dd, pVmxExt, pVarsCs, pVarsNs, Ntk_NetworkReadLatchNum(pNet) );

    // get the initial state
    bInitState = Vmx_VarMapCodeCube( dd, pVmxExt, pVarsCs, pCodes, Ntk_NetworkReadLatchNum(pNet) ); 
    FREE( pVarsCs );
    FREE( pVarsNs );
    FREE( pCodes );
    return bInitState;
}


/**Function*************************************************************

  Synopsis    [Collects CS and NS variables and latch reset values.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkVarsAndResetValues( Ntk_Network_t * pNet, 
    int * pVarsCs, int * pVarsNs, int * pCodes, int fLatchOnly )
{
    Ntk_Latch_t * pLatch;
    Ntk_Node_t * pNode, * pLatchInput, * pNodeCo;
    int nVarsIn, iLatch, iLatchNum, iVar;

    // get the number of CI
    nVarsIn = Ntk_NetworkReadCiNum( pNet );

    // go through all the input variables
    iVar = iLatch = 0;
    Ntk_NetworkForEachCi( pNet, pNode )
    {
        // check if it is the latch
        if ( pNode->Subtype != MV_BELONGS_TO_LATCH )
        {
            iVar++;
            continue;
        }

        // write down the number of the CS var
        pVarsCs[iLatch] = iVar;
        // find the corresponding latch
        Ntk_NetworkForEachLatch( pNet, pLatch )
            if ( pNode == Ntk_LatchReadOutput( pLatch ) )
                break;
        assert( pLatch );
        // get the corresponding latch input
        pLatchInput = Ntk_LatchReadInput( pLatch );
        // write doewn the number of the NS var
//            pVarsNs[iLatch] = nVarsIn + Ntk_NetworkReadCoIndex( pNet, pLatchInput );
//            assert( pVarsNs[iLatch] >= nVarsIn );
        // find the number of this latch in the CO list
        iLatchNum = 0;
        Ntk_NetworkForEachCo( pNet, pNodeCo )
        {
            if ( pNodeCo == pLatchInput )
                break;
            else if ( !fLatchOnly || pNodeCo->Subtype == MV_BELONGS_TO_LATCH )
                iLatchNum++;
        }
        assert( iLatchNum >= 0 );
        pVarsNs[iLatch] = nVarsIn + iLatchNum;
//printf( "Pair %3d: Replacing %2d <-> %2d.\n", iLatch, iVar, nVarsIn + iLatchNum );

        // get the reset value of this latch
        pCodes[iLatch] = Ntk_LatchReadReset( pLatch );
        iLatch++;

        iVar++;
    }
    assert( iLatch == Ntk_NetworkReadLatchNum(pNet) );
}




////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


