/**CFile****************************************************************

  FileName    [mvnExtract.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Sequential manipulation on the level of the MV network.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: mvnStgExtOld.c,v 1.1 2003/12/26 22:08:57 alanmi Exp $]

***********************************************************************/

#include "mvnInt.h"
#include "aut.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int Mvn_NetworkWriteSTG( Ntk_Network_t * pNet, Ntk_Node_t * pNodeGlo, DdNode * bRelTrans, DdNode * bRelOuts, char * pFileName, int nStatesLimit );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Extracts STG from the network and writes it into a KISS file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mvn_NetworkStgExtract( Ntk_Network_t * pNet, char * pFileName, int nStatesLimit, int fUseReach )
{
    FILE * pOutput;
    Ntk_Node_t * pNode, * pNodeGlo;
    DdNode * bRelTrans, * bRelOuts;
//    DdNode * bUnreach, * bTemp;
    DdManager * dd;
    int RetValue = 0;

    // make sure the network has only binary inputs/outputs
    pOutput = Ntk_NetworkReadMvsisOut(pNet);
    Ntk_NetworkForEachCi( pNet, pNode )
        if ( pNode->Subtype != MV_BELONGS_TO_LATCH )
            if ( pNode->nValues != 2 )
            {
                fprintf( pOutput, "Can only extract STG from networks with binary I/O; latches can be multi-valued.\n" );
                return 1;
            }
    Ntk_NetworkForEachCo( pNet, pNode )
        if ( pNode->Subtype != MV_BELONGS_TO_LATCH )
            if ( pNode->nValues != 2 )
            {
                fprintf( pOutput, "Can only extract STG from networks with binary I/O; latches can be multi-valued.\n" );
                return 1;
            }

    // free the old global manager if necessary
    Ntk_NetworkSetDdGlo( pNet, NULL );
    // compute the global functions
//    pNodeGlo = Ntk_NetworkGlobalCompute( pNet, NULL, 1, 1, 0, 0 );
    // create the global variable map
//    Ntk_NetworkGlobalVarMap( pNet, 0 );
    // compute the global relation of the network
    bRelTrans = Ntk_NetworkGlobalRelation( pNet, 1, 1 );   Cudd_Ref( bRelTrans );
    bRelOuts  = Ntk_NetworkGlobalRelation( pNet, 0, 1 );   Cudd_Ref( bRelOuts );

    // restrict the EXDC to the reachable states
    dd = Ntk_NetworkReadDdGlo( pNet );
/*
    if ( fUseReach )
    {
        bUnreach = Ntk_NetworkComputeUnreachable( pNet, pNodeGlo, bRelTrans ); Cudd_Ref( bUnreach );
PRN( bUnreach );
PRN( bRelTrans );
//        bRelTrans = Cudd_bddAnd( dd, bTemp = bRelTrans, Cudd_Not(bUnreach) );  Cudd_Ref( bRelTrans );
        bRelTrans = Cudd_bddMinimize( dd, bTemp = bRelTrans, Cudd_Not(bUnreach) );  Cudd_Ref( bRelTrans );
        Cudd_RecursiveDeref( dd, bTemp );
PRN( bRelTrans );
PRN( bRelOuts );
//        bRelOuts = Cudd_bddAnd( dd, bTemp = bRelOuts, Cudd_Not(bUnreach) );    Cudd_Ref( bRelOuts );
        bRelOuts = Cudd_bddMinimize( dd, bTemp = bRelOuts, Cudd_Not(bUnreach) );    Cudd_Ref( bRelOuts );
        Cudd_RecursiveDeref( dd, bTemp );
        Cudd_RecursiveDeref( dd, bUnreach );
PRN( bRelOuts );
    }
*/

    // allocate ZDD variables
    Cudd_zddVarsFromBddVars( dd, 2 );

    // create the STG
    if ( Mvn_NetworkWriteSTG( pNet, pNodeGlo, bRelTrans, bRelOuts, pFileName, nStatesLimit ) )
        RetValue = 1;

    // deref the relation
    Cudd_RecursiveDeref( dd, bRelTrans );
    Cudd_RecursiveDeref( dd, bRelOuts );
    // delete the global node
    Ntk_NodeDelete( pNodeGlo );
    // undo the global functions and the global manager
    Ntk_NetworkGlobalDeref( pNet );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mvn_NetworkWriteSTG( Ntk_Network_t * pNet, Ntk_Node_t * pNodeGlo, DdNode * bRelTrans, DdNode * bRelOuts, char * pFileName, int nStatesLimit )
{
    ProgressBar * pProgress;
    DdManager * dd;
    Vmx_VarMap_t * pVmxExt;
    Ntk_Pin_t * pPin;
    Ntk_Node_t * pNode, * pFanin;
    DdNode * bCubeIn, * bCubeOut, * bCubeCs, * bCubeNs;
    DdNode * bCube, * bTemp, * bStateCs, * bStateNs;
    DdNode * bTrans, * bCond, * bDomain, * bRelIO;
    DdNode ** pbCodes;
    st_table * tReachStates;
    int * pValuesFirst, * pEntry;
    int * pCodes, * pVarsCs, * pVarsNs;
    int nVarsIn, nVarsPIO, nVarsPI, nVarsPO, nLatches;
    int i, s, iVar;

    Aut_Auto_t * pAut;
    Aut_State_t * pState, * pS;
    Aut_Trans_t * pTrans;
    Vmx_VarMap_t * pVmxNew;
    int nDigits;
    char Buffer[100], Digit[10];
    int fUseLongNames = 1;
    int RetValue = 0;


    // the parameters of the global relation
    dd = Ntk_NetworkReadDdGlo( pNet );
    pVmxExt      = Ntk_NetworkReadVmxGlo( pNet );
    nVarsIn      = Ntk_NodeReadFaninNum( pNodeGlo );
    pValuesFirst = Vm_VarMapReadValuesFirstArray( Vmx_VarMapReadVm(pVmxExt) );

    nLatches     = Ntk_NetworkReadLatchNum(pNet);
    nVarsPI      = Ntk_NetworkReadCiNum(pNet) - nLatches;
    nVarsPO      = Ntk_NetworkReadCoNum(pNet) - nLatches;
    nVarsPIO     = nVarsPI + nVarsPO;

    // get the permuted variable map 
    pVmxNew = Mvn_NetworkCreatePermutedVmx( pNet, pNodeGlo );

    // get the encoded cubes
    Mvn_NetworkCreateVarCubes( pNet, pNodeGlo, 
        &bCubeIn, &bCubeCs, &bCubeOut, &bCubeNs );

//PRB( dd, bCubeIn );
//PRB( dd, bCubeCs );
//PRB( dd, bCubeOut );
//PRB( dd, bCubeNs );

    // collect the CS and NS var numbers and the reset values
    pVarsCs = ALLOC( int, dd->size );
    pVarsNs = ALLOC( int, dd->size );
    pCodes  = ALLOC( int, dd->size );
    Mvn_NetworkVarsAndResetValues( pNet, pNodeGlo, pVarsCs, pVarsNs, pCodes );

    // create the cubes of inputs, outputs, CS, and NS
    pbCodes = Vmx_VarMapEncodeMap( dd, pVmxExt );

    //////////////////////////////////////////////////
    // make sure, the reset values are set correctly!!!
    //////////////////////////////////////////////////

    // start the new automaton
    pAut = Aut_AutoAlloc();
    pAut->pMan     = Ntk_NetworkReadMan(pNet);
    pAut->nInputs  = nVarsPIO;
    pAut->nOutputs = 0;
    pAut->ppNamesInput = ALLOC( char *, pAut->nInputs );
    pAut->ppNamesOutput = ALLOC( char *, pAut->nOutputs );
    // copy the input names
    iVar = 0;
    Ntk_NodeForEachFaninWithIndex( pNodeGlo, pPin, pFanin, i )
    {
        if ( pFanin->Subtype != MV_BELONGS_TO_LATCH )
            pAut->ppNamesInput[iVar++] = util_strsav( pFanin->pName );
    }
    Ntk_NetworkForEachCo( pNet, pNode )
    {
        if ( pNode->Subtype != MV_BELONGS_TO_LATCH )
            pAut->ppNamesInput[iVar++] = util_strsav( pNode->pName );
        i++;
    }
    assert( iVar == nVarsPIO );
    pAut->nStates      = 0;
    pAut->nStatesAlloc = 1000;
    pAut->pStates      = ALLOC( Aut_State_t *, pAut->nStatesAlloc );

    // create the initial state
    pState = Aut_AutoStateAlloc();
    pState->bSubset = Mvn_ExtractGetCube( dd, pbCodes, pValuesFirst, pVarsCs, pCodes, nLatches ); 
    Cudd_Ref( pState->bSubset );
    pAut->pStates[ pAut->nStates++ ] = pState;

    // start the table to collect the reachable states
    tReachStates = st_init_table(st_ptrcmp,st_ptrhash);
    st_insert( tReachStates, (char *)pState->bSubset, (char *)0 );


    pProgress = Extra_ProgressBarStart( stdout, 1000 );

    // explore the reachable states
    for ( s = 0; s < pAut->nStates; s++ )
    {
        if ( s == nStatesLimit )
        {
            RetValue = 1;
            goto FINISH;
        }

        // get the subset
        pState = pAut->pStates[s]; 
        // get the transitions for this state
        bTrans = Cudd_bddAndAbstract( dd, bRelTrans, pState->bSubset, bCubeCs ); Cudd_Ref( bTrans );
//printf( "%d  : ", s );
//PRB( dd, bTrans );
        // get the outputs for this state
        bRelIO = Cudd_bddAndAbstract( dd, bRelOuts, pState->bSubset, bCubeCs ); Cudd_Ref( bRelIO );
        // extract the subsets reachable by certain inputs
        while ( bTrans != b0 )
        {
            // get the cube in terms of PI and NS vars
            bCube = Extra_bddGetOneCube( dd, bTrans );                    Cudd_Ref( bCube );
            assert( Cudd_bddLeq( dd, bCube, bTrans ) );
            // get the cube in terms of NS vars
            bCube = Cudd_bddExistAbstract( dd, bTemp = bCube, bCubeIn );  Cudd_Ref( bCube );
            Cudd_RecursiveDeref( dd, bTemp );

            // find the codes of one next state
            Vmx_VarMapDecodeCube( dd, bCube, pVmxExt, pVarsNs, pCodes, nLatches );
            Cudd_RecursiveDeref( dd, bCube );

            // get the CS and NS cubes for this state
            bStateCs = Mvn_ExtractGetCube( dd, pbCodes, pValuesFirst, pVarsCs, pCodes, nLatches ); Cudd_Ref( bStateCs );
            bStateNs = Mvn_ExtractGetCube( dd, pbCodes, pValuesFirst, pVarsNs, pCodes, nLatches ); Cudd_Ref( bStateNs );

            // check whether this current state is already reached
            if ( !st_find_or_add( tReachStates, (char *)bStateCs, (char ***)&pEntry ) )
            { // does not exist - add it to storage

                // realloc storage for states if necessary
                if ( pAut->nStatesAlloc <= pAut->nStates )
                {
                    pAut->pStates  = REALLOC( Aut_State_t *, pAut->pStates,  2 * pAut->nStatesAlloc );
                    pAut->nStatesAlloc *= 2;
                }

                pS = Aut_AutoStateAlloc();
                pS->bSubset = bStateCs;   Cudd_Ref( bStateCs );
                pAut->pStates[pAut->nStates] = pS;
                *pEntry = pAut->nStates;
                pAut->nStates++;
            }
            Cudd_RecursiveDeref( dd, bStateCs );

            // find the input condition for this transition
            bCond = Cudd_bddAndAbstract( dd, bTrans, bStateNs, bCubeNs ); Cudd_Ref( bCond );
            assert( bCond != b0 );
//printf( "%d -> %d:  ", s, *pEntry );
//PRB( dd, bCond );

            // get the input/output domain
            bDomain = Cudd_bddAnd( dd, bRelIO, bCond );  Cudd_Ref( bDomain );
            Cudd_RecursiveDeref( dd, bCond );

            // add the transition
            pTrans = Aut_AutoTransAlloc();
            pTrans->pCond = Fnc_FunctionDeriveSopFromMddSpecial( 
                Ntk_NetworkReadManMvc(pNet), dd, bDomain, bDomain, pVmxNew, nVarsPIO );
            Cudd_RecursiveDeref( dd, bDomain );

            pTrans->StateCur  = s;
            pTrans->StateNext = *pEntry;
            // add the transition
            Aut_AutoTransAdd( pState, pTrans );


            bTrans = Cudd_bddAnd( dd, bTemp = bTrans, Cudd_Not( bStateNs ) ); Cudd_Ref( bTrans );
            Cudd_RecursiveDeref( dd, bTemp );
            Cudd_RecursiveDeref( dd, bStateNs );
        }
        Cudd_RecursiveDeref( dd, bTrans );
        Cudd_RecursiveDeref( dd, bRelIO );

        if ( s > 20 && (s % 20 == 0) )
            Extra_ProgressBarUpdate( pProgress, 1000 * s / pAut->nStates );
    }
    Extra_ProgressBarStop( pProgress );

        // get the number of digits in the state number
	for ( nDigits = 0, i = pAut->nStates - 1;  i;  i /= 10,  nDigits++ );

    // generate the state names and acceptable conditions
    for ( s = 0; s < pAut->nStates; s++ )
    {
        // get the state
        pState = pAut->pStates[s];
        // set the state as accepting
        pState->fAcc = 1;
        // print simple state names
        if ( fUseLongNames )
        {
            // decode the cube
            Vmx_VarMapDecodeCube( dd, pState->bSubset, pVmxExt, pVarsCs, pCodes, nLatches );
            // print symbolic state name
            Buffer[0] = 0;
            for ( i = 0; i < nLatches; i++ )
            {
                sprintf( Digit, "%d", pCodes[i] );
                strcat( Buffer, Digit );
            }
            pState->pName = util_strsav( Buffer );
        }
        else
        {
            sprintf( Buffer, "s%0*d", nDigits, s );
            pState->pName = util_strsav( Buffer );
        }
        // deref the subset
        Cudd_RecursiveDeref( dd, pState->bSubset );
    }

    fprintf( stdout, "The extracted STG has %d states and %d transitions.\n", 
        pAut->nStates, Aut_AutoCountTransitions(pAut) );

    // write the automaton into the file
    Aut_AutoWriteKissFsm( pAut, pFileName, nVarsPI );

FINISH:

    Aut_AutoFree( pAut );

    Cudd_RecursiveDeref( dd, bCubeIn ); 
    Cudd_RecursiveDeref( dd, bCubeOut ); 
    Cudd_RecursiveDeref( dd, bCubeCs ); 
    Cudd_RecursiveDeref( dd, bCubeNs ); 

    st_free_table( tReachStates );
    Vmx_VarMapEncodeDeref( dd, pVmxExt, pbCodes );
    FREE( pVarsCs );
    FREE( pVarsNs );
    FREE( pCodes );

    return RetValue;
}


/**Function*************************************************************

  Synopsis    [Creates permuted extended variable map.]

  Description [The resulting map contains the PI and PO variables
  of the original relation, followed by the CS and NS variables.
  The resulting map is useful to derive the PI/PO conditions of
  the transitions by the call to Fnc_FunctionDeriveSopFromMddSpecial().]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vmx_VarMap_t * Mvn_NetworkCreatePermutedVmx( Ntk_Network_t * pNet, Ntk_Node_t * pNodeGlo )
{
    Vmx_VarMap_t * pVmxExt;
    Vmx_VarMap_t * pVmxNew;
    Ntk_Node_t * pFanin, * pNode;
    Ntk_Pin_t * pPin;
    int nVarsTot, nVarsPIO, nVarsPI, nVarsPO, nLatches;
    int iVarPIO, iVarLIO, i;
    int * pPermute;

    // the number of variables
    nVarsTot = Ntk_NetworkReadCiNum(pNet) + Ntk_NetworkReadCoNum(pNet);
    nLatches = Ntk_NetworkReadLatchNum(pNet);
    nVarsPI  = Ntk_NetworkReadCiNum(pNet) - nLatches;
    nVarsPO  = Ntk_NetworkReadCoNum(pNet) - nLatches;
    nVarsPIO = nVarsPI + nVarsPO;

    // create the variable map, which contains the PI and PO variables
    // listed first, followed by other variables
    iVarPIO = 0;
    iVarLIO = nVarsPIO;
    pPermute = ALLOC( int, nVarsTot );
    Ntk_NodeForEachFaninWithIndex( pNodeGlo, pPin, pFanin, i )
    {
        if ( pFanin->Subtype == MV_BELONGS_TO_LATCH )
            pPermute[iVarLIO++] = i;
        else
            pPermute[iVarPIO++] = i;
    }
    Ntk_NetworkForEachCo( pNet, pNode )
    {
        if ( pNode->Subtype == MV_BELONGS_TO_LATCH )
            pPermute[iVarLIO++] = i;
        else
            pPermute[iVarPIO++] = i;
        i++;
    }
    assert( iVarPIO == nVarsPIO );
    assert( iVarLIO == nVarsTot );

    pVmxExt = Ntk_NetworkReadVmxGlo( pNet );
    pVmxNew = Vmx_VarMapCreatePermuted( pVmxExt, pPermute );
    FREE( pPermute );

    return pVmxNew;
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


