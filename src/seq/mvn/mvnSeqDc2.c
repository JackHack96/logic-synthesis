/**CFile****************************************************************

  FileName    [mvnSeqDcOld.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Extracting sequential DCs of the network.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: mvnSeqDc2.c,v 1.2 2004/04/08 04:48:26 alanmi Exp $]

***********************************************************************/

#include "mvnInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static Ntk_Network_t * Ntk_NetworkCreateExdc( Ntk_Network_t * pNet, DdNode * bUnreach );
static Vmx_VarMap_t * Mvn_NetworkCreatePermutedVmxCs( Ntk_Network_t * pNet, Ntk_Node_t * pNodeGlo );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////


/**Function*************************************************************

  Synopsis    [Extracts sequential DCs of the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mvn_NetworkExtractSeqDcs( Ntk_Network_t * pNet )
{
    DdManager * dd;
    DdNode * bTR, * bUnreach;
    Vmx_VarMap_t * pVmxGlo;

    // remove EXDC network if present
    if ( pNet->pNetExdc )
    {
        Ntk_NetworkDelete( pNet->pNetExdc );
        pNet->pNetExdc = NULL; 
    }

    // free the old global manager if necessary
    Ntk_NetworkSetDdGlo( pNet, NULL );
    // compute the global functions
    Ntk_NetworkGlobalCompute( pNet, 1, 1, 0, 0 );
    // create the global variable map
    pVmxGlo = Ntk_NetworkGlobalCreateVmxCo( pNet, 0 );
    Ntk_NetworkSetVmxGlo( pNet, pVmxGlo );

    // compute the global relation of the network
    bTR = Ntk_NetworkGlobalRelation( pNet, 1, 1 );   Cudd_Ref( bTR );

    // compute the unreachable states
    bUnreach = Ntk_NetworkComputeUnreachable( pNet, bTR ); Cudd_Ref( bUnreach );
    // allocate ZDD variables
    dd = Ntk_NetworkReadDdGlo( pNet );
    Cudd_zddVarsFromBddVars( dd, 2 );
    // create the EXDC network representing the unreachable states
    pNet->pNetExdc = Ntk_NetworkCreateExdc( pNet, bUnreach );

    // deref the relation
    Cudd_RecursiveDeref( dd, bUnreach );
    Cudd_RecursiveDeref( dd, bTR );
    // undo the global functions and the global manager
    Ntk_NetworkGlobalDeref( pNet );
    return 0;
}

/**Function*************************************************************

  Synopsis    [Computes the set of unreachable states.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Ntk_NetworkComputeUnreachable( Ntk_Network_t * pNet, DdNode * bTR )
{
    Ntk_Node_t * pNodeGlo; 
    DdNode * bCubeIn, * bCubeOut, * bCubeCs, * bCubeNs;
//    DdNode * bCubeInCs;
    DdNode * bTrans, * bCurrent, * bNext, * bReached, * bTemp;
    DdNode ** pbCodes;
    DdManager * dd;
    int * pCodes, * pVarsCs, * pVarsNs;
    int * pValuesFirst;
    Vmx_VarMap_t * pVmxExt;
    int nIters;
    int nVarsBinNs;

    pNodeGlo = Ntk_NodeCreateFromNetwork( pNet, NULL );

    // create the codes
    dd = Ntk_NetworkReadDdGlo( pNet );
    pVmxExt = Ntk_NetworkReadVmxGlo( pNet );

    // collect the CS and NS var numbers and the reset values
    pVarsCs = ALLOC( int, dd->size );
    pVarsNs = ALLOC( int, dd->size );
    pCodes  = ALLOC( int, dd->size );
    Mvn_NetworkVarsAndResetValues( pNet, pNodeGlo, pVarsCs, pVarsNs, pCodes );

    // get the initial state
    pbCodes = Vmx_VarMapEncodeMap( dd, pVmxExt );
    pValuesFirst = Vm_VarMapReadValuesFirstArray( Vmx_VarMapReadVm(pVmxExt) );
    bCurrent = Mvn_ExtractGetCube( dd, pbCodes, pValuesFirst, pVarsCs, pCodes, Ntk_NetworkReadLatchNum(pNet) ); 
    Cudd_Ref( bCurrent );
    Vmx_VarMapEncodeDeref( dd, pVmxExt, pbCodes );

    // create the variable map
    Mvn_NetworkSetupVarMap( dd, pVmxExt, pVarsCs, pVarsNs, Ntk_NetworkReadLatchNum(pNet) );

    // get the cube of PI and CS variables
//    bCubeInCs = Vmx_VarMapCharCubeInput( dd, pVmxExt );  Cudd_Ref( bCubeInCs );

    // get the encoded cubes
    Mvn_NetworkCreateVarCubes( pNet, pNodeGlo, 
        &bCubeIn, &bCubeCs, &bCubeOut, &bCubeNs );

    // perform reachability analisys
    bReached = bCurrent;   Cudd_Ref( bCurrent );
//    bTrans   = bTR;  Cudd_Ref( bTrans );
    // quantify the input
    bTrans   = Cudd_bddExistAbstract( dd, bTR, bCubeIn );  Cudd_Ref( bTrans );
    nIters = 0;
    while ( 1 )
    {
        nIters++;
        // compute the next states
//        bNext = Cudd_bddAndAbstract( dd, bTrans, bCurrent, bCubeInCs ); Cudd_Ref( bNext );
        bNext = Cudd_bddAndAbstract( dd, bTrans, bCurrent, bCubeCs ); Cudd_Ref( bNext );
        Cudd_RecursiveDeref( dd, bCurrent );
        // remap these states into the current state vars
        bNext = Cudd_bddVarMap( dd, bTemp = bNext );               Cudd_Ref( bNext );
        Cudd_RecursiveDeref( dd, bTemp );

        if ( nIters < 3 )
        {
//PRB( dd, bNext );
        }
        // check if there are any new states
        if ( Cudd_bddLeq( dd, bNext, bReached ) )
            break;
        // get the new states
        bCurrent = Cudd_bddAnd( dd, bNext, Cudd_Not(bReached) ); Cudd_Ref( bCurrent );
        // minimize the new states with the reached states
//        bCurrent = Cudd_bddConstrain( dd, bTemp = bCurrent, Cudd_Not(bReached) ); Cudd_Ref( bCurrent );
//        Cudd_RecursiveDeref( dd, bTemp );
        // add to the reached states
        bReached = Cudd_bddOr( dd, bTemp = bReached, bNext );  Cudd_Ref( bReached );
        Cudd_RecursiveDeref( dd, bTemp );
        Cudd_RecursiveDeref( dd, bNext );
//PRB( dd, bReached );
        // minimize the transition relation
//        bTrans = Cudd_bddConstrain( dd, bTemp = bTrans, Cudd_Not(bReached) ); Cudd_Ref( bTrans );
//        Cudd_RecursiveDeref( dd, bTemp );
    }
    Cudd_RecursiveDeref( dd, bTrans );
    Cudd_RecursiveDeref( dd, bNext );
//    Cudd_RecursiveDeref( dd, bCubeInCs ); 
    fprintf( stdout, "Reachability analysis completed in %d iterations.\n", nIters );

    nVarsBinNs = Cudd_SupportSize( dd, bCubeCs );
    printf( "The number of minterms in the reachable state set = %d.\n", (int)Cudd_CountMinterm(dd, bReached, nVarsBinNs) ); 

//PRB( dd, bReached );

    Ntk_NodeDelete( pNodeGlo );

    FREE( pVarsCs );
    FREE( pVarsNs );
    FREE( pCodes );

    Cudd_RecursiveDeref( dd, bCubeIn ); 
    Cudd_RecursiveDeref( dd, bCubeOut ); 
    Cudd_RecursiveDeref( dd, bCubeCs ); 
    Cudd_RecursiveDeref( dd, bCubeNs ); 

    Cudd_Deref( bReached );
    return Cudd_Not( bReached );
}

/**Function*************************************************************

  Synopsis    [Creates the EXDC network.]

  Description [The set of unreachable states in given in terms of 
  CS variables.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Network_t * Ntk_NetworkCreateExdc( Ntk_Network_t * pNet, DdNode * bUnreach )
{
    Ntk_Node_t * pNodeGlo; 
    Ntk_Node_t * pFanin, * pNodeNew, * pNodeDc, * pNodeCo;
    Ntk_Pin_t * pPin;
    Ntk_Network_t * pNetNew;
    Vmx_VarMap_t * pVmxNew;
    Vm_VarMap_t * pVmDc;
    Mvc_Cover_t * pMvc, ** pIsets;
    Cvr_Cover_t * pCvr;
    DdManager * dd;
    char * pName;
    int nLatches;

    pNodeGlo = Ntk_NodeCreateFromNetwork( pNet, NULL );

    // start the network
    pNetNew = Ntk_NetworkAlloc( Ntk_NetworkReadMvsis(pNet) );
    // register the name 
    pNetNew->pName = Ntk_NetworkRegisterNewName( pNetNew, "exdc" );
    // get the internal node
    pNodeDc = Ntk_NodeCreate( pNetNew, NULL, MV_NODE_INT, 2 );
    Ntk_NodeForEachFanin( pNodeGlo, pPin, pFanin )
    {
        if ( pFanin->Subtype != MV_BELONGS_TO_LATCH )
            continue;
        pNodeNew = Ntk_NodeDup( pNetNew, pFanin );
        // clean the subtype - make it a normal PI
        Ntk_NodeSetSubtype( pNodeNew, MV_BELONGS_TO_NET );
        // add the new node to the network
        Ntk_NetworkAddNode( pNetNew, pNodeNew, 1 );
        // add the new node as a fanin to the DC node
        Ntk_NodeAddFanin( pNodeDc, pNodeNew );
    }

    // get the var map of the new node
    pVmDc = Ntk_NodeAssignVm( pNodeDc ); 

    // get the cover
    dd = Ntk_NetworkReadDdGlo( pNet );
    nLatches = Ntk_NetworkReadLatchNum(pNet);
    pVmxNew = Mvn_NetworkCreatePermutedVmxCs( pNet, pNodeGlo );
    pMvc = Fnc_FunctionDeriveSopFromMddSpecial( 
        Ntk_NetworkReadManMvc(pNet), dd, bUnreach, bUnreach, pVmxNew, nLatches );

    // get the Cvr of the new node
    pIsets = ALLOC( Mvc_Cover_t *, 2 );
    pIsets[0] = NULL;
    pIsets[1] = pMvc;
    pCvr = Cvr_CoverCreate( pVmDc, pIsets );


    // set the functionality of the node
    Ntk_NodeWriteFuncVm( pNodeDc, pVmDc );
    Ntk_NodeWriteFuncCvr( pNodeDc, pCvr );

    // make the node minimum-base
    Ntk_NodeMakeMinimumBase( pNodeDc );
    // add the new node to the network
    Ntk_NetworkAddNode( pNetNew, pNodeDc, 1 );

    // create the CO nodes of the EXDC network
    Ntk_NetworkForEachCo( pNet, pNodeCo )
    {
        // create the buffer
        pNodeNew = Ntk_NodeCreateOneInputBinary( pNetNew, pNodeDc, 0 );
        // add the node to the network
        Ntk_NetworkAddNode( pNetNew, pNodeNew, 1 );
        // set the name
        pName = Ntk_NetworkRegisterNewName(pNetNew, Ntk_NodeReadName(pNodeCo));
        Ntk_NodeAssignName( pNodeNew, pName );
        // create the CO node
        Ntk_NetworkAddNodeCo( pNetNew, pNodeNew, 1 );
    }

    Ntk_NodeDelete( pNodeGlo );

    // put IDs into a proper order
//    Ntk_NetworkReassignIds( pNetNew );
    Ntk_NetworkOrderFanins( pNetNew );

    // check the network
    if ( !Ntk_NetworkCheck( pNetNew ) )
       fprintf( Ntk_NetworkReadMvsisOut(pNetNew), "Ntk_NetworkCreateExdc(): Network check has failed.\n" );
    return pNetNew;
}


/**Function*************************************************************

  Synopsis    [Creates permuted extended variable map.]

  Description [The resulting map contains the CS variables
  of the original relation, followed by the PI and NS variables.
  The resulting map is useful to derive the SOP in terms of CS vars
  by the call to Fnc_FunctionDeriveSopFromMddSpecial().]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vmx_VarMap_t * Mvn_NetworkCreatePermutedVmxCs( Ntk_Network_t * pNet, Ntk_Node_t * pNodeGlo )
{
    Vmx_VarMap_t * pVmxExt;
    Vmx_VarMap_t * pVmxNew;
    Ntk_Node_t * pFanin, * pNode;
    Ntk_Pin_t * pPin;
    int nVarsTot, nVarsPIO, nVarsPI, nVarsPO, nLatches;
    int iVarCS, iVarOther, i;
    int * pPermute;

    // the number of variables
    nVarsTot = Ntk_NetworkReadCiNum(pNet) + Ntk_NetworkReadCoNum(pNet);
    nLatches = Ntk_NetworkReadLatchNum(pNet);
    nVarsPI  = Ntk_NetworkReadCiNum(pNet) - nLatches;
    nVarsPO  = Ntk_NetworkReadCoNum(pNet) - nLatches;
    nVarsPIO = nVarsPI + nVarsPO;

    // create the variable map, which contains the PI and PO variables
    // listed first, followed by other variables
    iVarCS    = 0;
    iVarOther = nLatches;
    pPermute = ALLOC( int, nVarsTot );
    Ntk_NodeForEachFaninWithIndex( pNodeGlo, pPin, pFanin, i )
    {
        if ( pFanin->Subtype == MV_BELONGS_TO_LATCH )
            pPermute[iVarCS++] = i;
        else
            pPermute[iVarOther++] = i;
    }
    Ntk_NetworkForEachCo( pNet, pNode )
    {
        pPermute[iVarOther++] = i++;
    }
    assert( iVarCS == nLatches );
    assert( iVarOther == nVarsTot );

    pVmxExt = Ntk_NetworkReadVmxGlo( pNet );
    pVmxNew = Vmx_VarMapCreatePermuted( pVmxExt, pPermute );
    FREE( pPermute );

    return pVmxNew;
}

/**Function*************************************************************

  Synopsis    [Set up the permutation variable map.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvn_NetworkSetupVarMap( DdManager * dd, Vmx_VarMap_t * pVmx, int * pVarsCs, int * pVarsNs, int nVars )
{
    DdNode ** pbVarsX, ** pbVarsY;
    int * pBits, * pBitsFirst, * pBitsOrder;
    int nBddVars, i, b;;

    // set the variable mapping for Cudd_bddVarMap()
    pbVarsX = ALLOC( DdNode *, dd->size );
    pbVarsY = ALLOC( DdNode *, dd->size );
    pBits = Vmx_VarMapReadBits( pVmx );
    pBitsFirst = Vmx_VarMapReadBitsFirst( pVmx );
    pBitsOrder = Vmx_VarMapReadBitsOrder( pVmx );
    nBddVars = 0;
    for ( i = 0; i < nVars; i++ )
    {
        assert( pBits[pVarsCs[i]] == pBits[pVarsNs[i]] );
        for ( b = 0; b < pBits[pVarsCs[i]]; b++ )
        {
            pbVarsX[nBddVars] = dd->vars[ pBitsOrder[pBitsFirst[pVarsCs[i]] + b] ];
            pbVarsY[nBddVars] = dd->vars[ pBitsOrder[pBitsFirst[pVarsNs[i]] + b] ];
            nBddVars++;
        }
    }
    Cudd_SetVarMap( dd, pbVarsX, pbVarsY, nBddVars );
    FREE( pbVarsX );
    FREE( pbVarsY );
}


/**Function*************************************************************

  Synopsis    [Creates the cube of the values of the given vars.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvn_NetworkCreateVarCubes( Ntk_Network_t * pNet, Ntk_Node_t * pNodeGlo,
    DdNode ** pbCubeIn, DdNode ** pbCubeCs, DdNode ** pbCubeOut, DdNode ** pbCubeNs )
{
    DdManager * dd;
    Vmx_VarMap_t * pVmxExt;
    DdNode * bCubeIn, * bCubeCs, * bCubeOut, * bCubeNs, * bCube, * bTemp;
    Ntk_Node_t * pFanin, * pNode;
    Ntk_Pin_t * pPin;
    int i;

    dd      = Ntk_NetworkReadDdGlo( pNet );
    pVmxExt = Ntk_NetworkReadVmxGlo( pNet );

    // PIs and CS
    bCubeIn = b1;   Cudd_Ref( bCubeIn );
    bCubeCs = b1;   Cudd_Ref( bCubeCs );
    Ntk_NodeForEachFaninWithIndex( pNodeGlo, pPin, pFanin, i )
    {
        bCube = Vmx_VarMapCharCube( dd, pVmxExt, i );       Cudd_Ref( bCube );
        if ( pFanin->Subtype == MV_BELONGS_TO_LATCH )
        {
            bCubeCs = Cudd_bddAnd( dd, bTemp = bCubeCs, bCube );  Cudd_Ref( bCubeCs );
        }
        else
        {
            bCubeIn = Cudd_bddAnd( dd, bTemp = bCubeIn, bCube );  Cudd_Ref( bCubeIn );
        }
        Cudd_RecursiveDeref( dd, bTemp ); 
        Cudd_RecursiveDeref( dd, bCube ); 
    }

    // POs and NS
    bCubeOut = b1;   Cudd_Ref( bCubeOut );
    bCubeNs  = b1;   Cudd_Ref( bCubeNs );
    Ntk_NetworkForEachCo( pNet, pNode )
    {
        bCube = Vmx_VarMapCharCube( dd, pVmxExt, i );  Cudd_Ref( bCube );
        if ( pNode->Subtype == MV_BELONGS_TO_LATCH )
        {
            bCubeNs = Cudd_bddAnd( dd, bTemp = bCubeNs, bCube );   Cudd_Ref( bCubeNs );
        }
        else
        {
            bCubeOut = Cudd_bddAnd( dd, bTemp = bCubeOut, bCube ); Cudd_Ref( bCubeOut );
        }
        Cudd_RecursiveDeref( dd, bTemp ); 
        Cudd_RecursiveDeref( dd, bCube ); 
        i++;
    }

    *pbCubeIn  = bCubeIn;
    *pbCubeOut = bCubeOut;
    *pbCubeCs  = bCubeCs;
    *pbCubeNs  = bCubeNs;
}


/**Function*************************************************************

  Synopsis    [Collects CS and NS variables and latch reset values.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvn_NetworkVarsAndResetValues( Ntk_Network_t * pNet, Ntk_Node_t * pNodeGlo, 
     int * pVarsCs, int * pVarsNs, int * pCodes )
{
    Ntk_Latch_t * pLatch;
    Ntk_Node_t * pFanin, * pLatchInput;
    Ntk_Pin_t * pPin;
    int nVarsIn, iVar, i;

    nVarsIn = Ntk_NodeReadFaninNum( pNodeGlo );

    iVar = 0;
    Ntk_NodeForEachFaninWithIndex( pNodeGlo, pPin, pFanin, i )
    {
        if ( pFanin->Subtype == MV_BELONGS_TO_LATCH )
        {
            pVarsCs[iVar] = i;
            // find the corresponding latch
            Ntk_NetworkForEachLatch( pNet, pLatch )
                if ( pFanin == Ntk_LatchReadOutput( pLatch ) )
                    break;
            assert( pLatch );
            // find the corresponding CS var
            pLatchInput = Ntk_LatchReadInput( pLatch );
            pVarsNs[iVar] = nVarsIn + Ntk_NetworkReadCoIndex( pNet, pLatchInput );
            assert( pVarsNs[iVar] >= nVarsIn );

            // get the reset value of this latch
            pCodes[iVar++] = Ntk_LatchReadReset( pLatch );
        }
    }
    assert( iVar == Ntk_NetworkReadLatchNum(pNet) );
}


/**Function*************************************************************

  Synopsis    [Creates the cube of the values of the given vars.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Mvn_ExtractGetCube( DdManager * dd, DdNode * pbCodes[], int * pValuesFirst, 
    int pVars[], int pCodes[], int nVars )
{
    DdNode * bCube, * bValue, * bTemp;
    int i;
    bCube = b1;   Cudd_Ref( bCube );
    for ( i = 0; i < nVars; i++ )
    {
        bValue = pbCodes[ pValuesFirst[pVars[i]] + pCodes[i] ];
//printf( "Var = %d. Value = %d.  ", pVars[i], pCodes[i] );
//PRB( dd, bValue );

        bCube = Cudd_bddAnd( dd, bTemp = bCube, bValue );   Cudd_Ref( bCube );
        Cudd_RecursiveDeref( dd, bTemp ); 
    }
//printf( "\n" );
    Cudd_Deref( bCube );
    return bCube;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


