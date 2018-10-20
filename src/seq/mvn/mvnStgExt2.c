/**CFile****************************************************************

  FileName    [mvnExtract.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Sequential manipulation on the level of the MV network.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: mvnStgExt2.c,v 1.2 2004/04/08 04:48:27 alanmi Exp $]

***********************************************************************/

#include "mvnInt.h"
#include "au.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int            Mvn_NetworkWriteSTG( Ntk_Network_t * pNet, char * pFileName, int nStatesLimit, int fUseFsm, int fVerbose );
static void           Mvn_NetworkAssignAutomatonNames( Ntk_Network_t * pNet, Au_Auto_t * pAut );
static DdNode *       Mvn_NetworkGetVarsIoNs( Ntk_Network_t * pNet );
static DdNode *       Mvn_NetworkGetVarsNs( Ntk_Network_t * pNet );
static int *          Mvn_NetworkGetMvCsVars( Ntk_Network_t * pNet );
static int *          Mvn_NetworkGetMvNsVars( Ntk_Network_t * pNet );
static Vmx_VarMap_t * Mvn_NetworkCreatePermutedVmx( Ntk_Network_t * pNet );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Extracts STG from the network and writes it into a KISS file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mvn_NetworkStgExtract2( Ntk_Network_t * pNet, char * pFileName, int nStatesLimit, int fUseFsm, int fVerbose )
{
    FILE * pOutput;
    Ntk_Node_t * pNode;
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

    // create the STG
    if ( Mvn_NetworkWriteSTG( pNet, pFileName, nStatesLimit, fUseFsm, fVerbose ) )
        RetValue = 1;
    return RetValue;
}


/**Function*************************************************************

  Synopsis    [Extract the STG from the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mvn_NetworkWriteSTG( Ntk_Network_t * pNet, char * pFileName, int nStatesLimit, int fUseFsm, int fVerbose )
{
    ProgressBar * pProgress;
    Au_Auto_t * pAut;
    Au_State_t * pState, * pS;
    Au_Trans_t * pTrans;
    Ntk_Node_t * pNode;
    DdManager * dd;
    Extra_ImageTree_t * pTreeT, * pTreeO;
    st_table * tReachStates;
    Vmx_VarMap_t * pVmxExt, * pVmxNew;
    DdNode * bInitState, * bCube;
    DdNode * bTemp, * bRelationTr, * bRelationIo;
    DdNode ** pbPartsT, ** pbPartsO;
    DdNode ** pVarsBinIoNs, ** pVarsBinNs;
    DdNode * bVarsBinIoNs, * bVarsCs, * bVarsNs;
    DdNode * bStateNs, * bStateCs, * bCond, * bDomain;
    int nPartsT, nPartsO, nLatches, iVar, s;
    int nVarsBinIoNs, nVarsPIO;
    int RetValue = 0, * pEntry;
    DdNode ** pbCodes; 
    int * pCodes, * pVarsCs, * pVarsNs;
    int * pValuesFirst;

    // compute the global functions
    Ntk_NetworkGlobalComputeCo( pNet, 1, 0, 0 );
    dd = Ntk_NetworkReadDdGlo( pNet );

    // create the extended variable map for all COs 
    pVmxExt = Ntk_NetworkGlobalCreateVmxCo( pNet, 0 );
    Ntk_NetworkSetVmxGlo( pNet, pVmxExt );

    // get the permuted variable map 
    pVmxNew = Mvn_NetworkCreatePermutedVmx( pNet );

    // get parameters
    nLatches = Ntk_NetworkReadLatchNum( pNet );
    nVarsPIO = Ntk_NetworkReadCiNum(pNet) + Ntk_NetworkReadCoNum(pNet) - 2 * nLatches;

    // create the latch partitions
    nPartsT  = nLatches;
    pbPartsT = Ntk_NetworkDerivePartitions( pNet, 1, 0 );

    // create the output partitions
    nPartsO  = Ntk_NetworkReadCoNum( pNet ) - nLatches;
    pbPartsO = Ntk_NetworkDerivePartitions( pNet, 0, 0 );

    // save the BDD manager
    Ntk_NetworkWriteDdGlo( pNet, NULL );
    // dereference the global latch functions
    Ntk_NetworkGlobalDeref( pNet );
    // return the manager back where it belongs
    Ntk_NetworkSetDdGlo( pNet, dd );
/*
printf( "BDD nodes in the T partitiones = %d.\n", Cudd_SharingSize( pbPartsT, nPartsT ) );
printf( "BDD nodes in the O partitiones = %d.\n", Cudd_SharingSize( pbPartsO, nPartsO ) );
    // reorder the BDDs of partitions one more time
    Cudd_ReduceHeap( dd, CUDD_REORDER_SYMM_SIFT, 1 );
printf( "BDD nodes in the T partitiones = %d.\n", Cudd_SharingSize( pbPartsT, nPartsT ) );
printf( "BDD nodes in the O partitiones = %d.\n", Cudd_SharingSize( pbPartsO, nPartsO ) );
*/
    // introduce ZDD variables
    Cudd_zddVarsFromBddVars( dd, 2 );

    // get the initial state and set up the permutation CS <-> NS
    bInitState = Ntk_NetworkComputeInitState( pNet, 0 );     Cudd_Ref( bInitState );

    // get the binary NS and IO variables (not to be quantified in the image computation)
    bVarsBinIoNs = Mvn_NetworkGetVarsIoNs( pNet );        Cudd_Ref( bVarsBinIoNs );
    pVarsBinIoNs = Extra_bddComputeVarArray( dd, bVarsBinIoNs, &nVarsBinIoNs ); 
    Cudd_RecursiveDeref( dd, bVarsBinIoNs );

    // get only NS variables 
    bVarsNs    = Mvn_NetworkGetVarsNs( pNet );            Cudd_Ref( bVarsNs );
    pVarsBinNs = Extra_bddComputeVarArray( dd, bVarsNs, NULL ); 
    // get the cube of CS vars
    bVarsCs = Cudd_bddVarMap( dd, bVarsNs );              Cudd_Ref( bVarsCs );

    // start the image computation for the transitions and for the outputs
    pTreeT = Extra_bddImageStart( dd, bVarsCs, nPartsT, pbPartsT, nVarsBinIoNs, pVarsBinIoNs, fVerbose );
    pTreeO = Extra_bddImageStart( dd, bVarsCs, nPartsO, pbPartsO, nVarsBinIoNs, pVarsBinIoNs, fVerbose );
    FREE( pVarsBinIoNs );
    FREE( pVarsBinNs );
    Ntk_NodeGlobalDerefFuncs( dd, pbPartsT, nPartsT );
    Ntk_NodeGlobalDerefFuncs( dd, pbPartsO, nPartsO );
    FREE( pbPartsT );
    FREE( pbPartsO );
    Cudd_RecursiveDeref( dd, bVarsCs );

    // start the new automaton
    pAut = Au_AutoAlloc();
    pAut->pMan     = Ntk_NetworkReadMan(pNet);
    pAut->nInputs  = nVarsPIO;
//    pAut->nOutputs = Ntk_NetworkReadCoNum(pNet) - nLatches;
    pAut->nOutputs = 0;
    pAut->ppNamesInput = ALLOC( char *, pAut->nInputs );
    pAut->ppNamesOutput = NULL;
    // copy the input names
    iVar = 0;
    Ntk_NetworkForEachCi( pNet, pNode )
    {
        if ( pNode->Subtype != MV_BELONGS_TO_LATCH )
            pAut->ppNamesInput[iVar++] = util_strsav( pNode->pName );
    }
    Ntk_NetworkForEachCo( pNet, pNode )
    {
        if ( pNode->Subtype != MV_BELONGS_TO_LATCH )
            pAut->ppNamesInput[iVar++] = util_strsav( pNode->pName );
    }
    assert( iVar == pAut->nInputs );

    pAut->nStates      = 0;
    pAut->nStatesAlloc = 1000;
    pAut->pStates      = ALLOC( Au_State_t *, pAut->nStatesAlloc );

    // create the initial state
    pState = Au_AutoStateAlloc();
    pState->bSubset = bInitState;  // takes ref   Cudd_Ref( pState->bSubset );
    pAut->pStates[ pAut->nStates++ ] = pState;

    // start the table to collect the reachable states
    tReachStates = st_init_table(st_ptrcmp,st_ptrhash);
    st_insert( tReachStates, (char *)pState->bSubset, (char *)0 );

    // create the cubes of inputs, outputs, CS, and NS
    pbCodes = Vmx_VarMapEncodeMap( dd, pVmxExt );
    pCodes = ALLOC( int, nLatches );
    pVarsCs = Mvn_NetworkGetMvCsVars( pNet );
    pVarsNs = Mvn_NetworkGetMvNsVars( pNet );
    pValuesFirst = Vm_VarMapReadValuesFirstArray( Vmx_VarMapReadVm( pVmxExt ) );

    // explore the reachable states
    if ( !fVerbose )
        pProgress = Extra_ProgressBarStart( stdout, 1000 );
    for ( s = 0; s < pAut->nStates; s++ )
    {
        if ( s == nStatesLimit )
        {
            if ( !fVerbose )
                Extra_ProgressBarStop( pProgress );
            RetValue = 1;
            goto FINISH;
        }

        // get the subset
        pState = pAut->pStates[s]; 
        // get the transitions for this state
        bRelationTr = Extra_bddImageCompute( pTreeT, pState->bSubset ); 
        if ( bRelationTr == NULL )
            goto FINISH;
        Cudd_Ref( bRelationTr );
//printf( "%d  : ", s );
//PRB( dd, bRelationTr );
        // get the outputs for this state
        bRelationIo = Extra_bddImageCompute( pTreeO, pState->bSubset ); 
        if ( bRelationIo == NULL )
            goto FINISH;
        Cudd_Ref( bRelationIo );
        // extract the subsets reachable by certain inputs
        while ( bRelationTr != b0 )
        {
            // get one state in terms of NS vars
//            bStateNs = Extra_bddGetOneMinterm( dd, bRelationTr, bVarsNs );   Cudd_Ref( bStateNs );
            bCube = Extra_bddGetOneMinterm( dd, bRelationTr, bVarsNs );        Cudd_Ref( bCube );
            // map it into the CS vars
//            bStateCs = Cudd_bddVarMap( dd, bStateNs );                       Cudd_Ref( bStateCs );

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
                    pAut->pStates  = REALLOC( Au_State_t *, pAut->pStates,  2 * pAut->nStatesAlloc );
                    pAut->nStatesAlloc *= 2;
                }

                pS = Au_AutoStateAlloc();
                pS->bSubset = bStateCs;   Cudd_Ref( bStateCs );
                pAut->pStates[pAut->nStates] = pS;
                *pEntry = pAut->nStates;
                pAut->nStates++;
            }
            Cudd_RecursiveDeref( dd, bStateCs );

            // find the input condition for this transition
            bCond = Cudd_bddAndAbstract( dd, bRelationTr, bStateNs, bVarsNs ); Cudd_Ref( bCond );
            assert( bCond != b0 );
//printf( "%d -> %d:  ", s, *pEntry );
//PRB( dd, bCond );

            // get the input/output domain
            bDomain = Cudd_bddAnd( dd, bRelationIo, bCond );  Cudd_Ref( bDomain );
            Cudd_RecursiveDeref( dd, bCond );

            // add the transition
            pTrans = Au_AutoTransAlloc();
            pTrans->pCond = Fnc_FunctionDeriveSopFromMddSpecial( 
                Ntk_NetworkReadManMvc(pNet), dd, bDomain, bDomain, pVmxNew, nVarsPIO );
            Cudd_RecursiveDeref( dd, bDomain );

            pTrans->StateCur  = s;
            pTrans->StateNext = *pEntry;
            // add the transition
            Au_AutoTransAdd( pState, pTrans );


            bRelationTr = Cudd_bddAnd( dd, bTemp = bRelationTr, Cudd_Not( bStateNs ) ); Cudd_Ref( bRelationTr );
            Cudd_RecursiveDeref( dd, bTemp );
            Cudd_RecursiveDeref( dd, bStateNs );
        }
        Cudd_RecursiveDeref( dd, bRelationTr );
        Cudd_RecursiveDeref( dd, bRelationIo );

        if ( !fVerbose )
            if ( s && s % 100 == 0 )
            {
                char Buffer[100];
                sprintf( Buffer, "%d states", s );
                Extra_ProgressBarUpdate( pProgress, 1000 * s / pAut->nStates, Buffer );
            }
    }
    if ( !fVerbose )
        Extra_ProgressBarStop( pProgress );

    Mvn_NetworkAssignAutomatonNames( pNet, pAut );

    fprintf( stdout, "The STG with %d states and %d transitions is written to file \"%s\".\n", 
        pAut->nStates, Au_AutoCountTransitions(pAut), pFileName );

    // write the automaton into the file
    if ( fUseFsm )
        Au_AutoWriteKissFsm( pAut, pFileName, Ntk_NetworkReadCiNum(pNet) - nLatches );
    else
        Au_AutoWriteKiss( pAut, pFileName );

FINISH:
    // undo the image tree
    Extra_bddImageTreeDelete( pTreeT );
    Extra_bddImageTreeDelete( pTreeO );
    Cudd_RecursiveDeref( dd, bVarsNs ); 
    st_free_table( tReachStates );
    Au_AutoFree( pAut );
    Vmx_VarMapEncodeDeref( dd, pVmxExt, pbCodes );
    FREE( pVarsCs );
    FREE( pVarsNs );
    FREE( pCodes );
    // remove the manager
    if ( pNet->pNetExdc )
        Ntk_NetworkWriteDdGlo( pNet->pNetExdc, NULL );
    Ntk_NetworkWriteDdGlo( pNet, NULL );
    if ( RetValue == 0 )
        Extra_StopManager( dd );
    else
    {
        printf( "The limit on the number of states (%d) is reached; STG extraction aborted.\n", nStatesLimit );
        Cudd_Quit( dd );
    }
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Creates the cube of IO and NS vars.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvn_NetworkAssignAutomatonNames( Ntk_Network_t * pNet, Au_Auto_t * pAut )
{
    Au_State_t * pState;
    char Buffer[500], Digit[10];
    int * pVarsCs;
    int * pCodes;
    int nDigits, nLatches, s, i;
    int fUseLongNames = 1;
    DdManager * dd;
    Vmx_VarMap_t * pVmxExt;

    dd = Ntk_NetworkReadDdGlo( pNet );
    pVmxExt = Ntk_NetworkReadVmxGlo( pNet );

    // get the number of digits in the state number
    nDigits = Extra_Base10Log( pAut->nStates );
    nLatches = Ntk_NetworkReadLatchNum( pNet );

    pVarsCs = NULL;
    pCodes = NULL;
    if ( fUseLongNames )
    {
        pVarsCs = Mvn_NetworkGetMvCsVars( pNet );
        pCodes = ALLOC( int, nLatches );
    }

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
//printf( "Name %s   ", Buffer );
//PRB( dd, pState->bSubset );
        }
        else
        {
            sprintf( Buffer, "s%0*d", nDigits, s );
            pState->pName = util_strsav( Buffer );
        }
        // deref the subset
        Cudd_RecursiveDeref( dd, pState->bSubset );
    }
    FREE( pVarsCs );
    FREE( pCodes );
}

/**Function*************************************************************

  Synopsis    [Creates the cube of IO and NS vars.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Mvn_NetworkGetVarsIoNs( Ntk_Network_t * pNet )
{
    DdManager * dd;
    Vmx_VarMap_t * pVmxExt;
    DdNode * bCubeIn, * bCubeCs, * bCubeOut, * bCubeNs;
    DdNode * bCube, * bTemp;
    Ntk_Node_t * pNode;
    int i;

    dd      = Ntk_NetworkReadDdGlo( pNet );
    pVmxExt = Ntk_NetworkReadVmxGlo( pNet );

    // PIs and CS
    bCubeIn = b1;   Cudd_Ref( bCubeIn );
    bCubeCs = b1;   Cudd_Ref( bCubeCs );
    i = 0;
    Ntk_NetworkForEachCi( pNet, pNode )
    {
        bCube = Vmx_VarMapCharCube( dd, pVmxExt, i );       Cudd_Ref( bCube );
        if ( pNode->Subtype == MV_BELONGS_TO_LATCH )
        {
            bCubeCs = Cudd_bddAnd( dd, bTemp = bCubeCs, bCube );  Cudd_Ref( bCubeCs );
        }
        else
        {
            bCubeIn = Cudd_bddAnd( dd, bTemp = bCubeIn, bCube );  Cudd_Ref( bCubeIn );
        }
        Cudd_RecursiveDeref( dd, bTemp ); 
        Cudd_RecursiveDeref( dd, bCube ); 
        i++;
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

    bCube = Cudd_bddAnd( dd, bCubeIn, bCubeOut );         Cudd_Ref( bCube );
    bCube = Cudd_bddAnd( dd, bTemp = bCube, bCubeNs );    Cudd_Ref( bCube );
    Cudd_RecursiveDeref( dd, bTemp ); 

    Cudd_RecursiveDeref( dd, bCubeIn ); 
    Cudd_RecursiveDeref( dd, bCubeOut ); 
    Cudd_RecursiveDeref( dd, bCubeCs ); 
    Cudd_RecursiveDeref( dd, bCubeNs ); 

    Cudd_Deref( bCube );
    return bCube;
}

/**Function*************************************************************

  Synopsis    [Creates the cube of only NS vars.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Mvn_NetworkGetVarsNs( Ntk_Network_t * pNet )
{
    DdManager * dd;
    Vmx_VarMap_t * pVmxExt;
    DdNode * bCubeIn, * bCubeCs, * bCubeOut, * bCubeNs;
    DdNode * bCube, * bTemp;
    Ntk_Node_t * pNode;
    int i;

    dd      = Ntk_NetworkReadDdGlo( pNet );
    pVmxExt = Ntk_NetworkReadVmxGlo( pNet );

    // PIs and CS
    bCubeIn = b1;   Cudd_Ref( bCubeIn );
    bCubeCs = b1;   Cudd_Ref( bCubeCs );
    i = 0;
    Ntk_NetworkForEachCi( pNet, pNode )
    {
        bCube = Vmx_VarMapCharCube( dd, pVmxExt, i );       Cudd_Ref( bCube );
        if ( pNode->Subtype == MV_BELONGS_TO_LATCH )
        {
            bCubeCs = Cudd_bddAnd( dd, bTemp = bCubeCs, bCube );  Cudd_Ref( bCubeCs );
        }
        else
        {
            bCubeIn = Cudd_bddAnd( dd, bTemp = bCubeIn, bCube );  Cudd_Ref( bCubeIn );
        }
        Cudd_RecursiveDeref( dd, bTemp ); 
        Cudd_RecursiveDeref( dd, bCube ); 
        i++;
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

    Cudd_RecursiveDeref( dd, bCubeIn ); 
    Cudd_RecursiveDeref( dd, bCubeOut ); 
    Cudd_RecursiveDeref( dd, bCubeCs ); 

    Cudd_Deref( bCubeNs );
    return bCubeNs;
}

/**Function*************************************************************

  Synopsis    [Creates the cube of only NS vars.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int * Mvn_NetworkGetMvCsVars( Ntk_Network_t * pNet )
{
    DdManager * dd;
    Vmx_VarMap_t * pVmxExt;
    int * pVarsCs;
    Ntk_Node_t * pNode;
    int i, iVar;

    dd      = Ntk_NetworkReadDdGlo( pNet );
    pVmxExt = Ntk_NetworkReadVmxGlo( pNet );

    pVarsCs = ALLOC( int, Ntk_NetworkReadCiNum( pNet ) );

    i = iVar = 0;
    Ntk_NetworkForEachCi( pNet, pNode )
    {
        if ( pNode->Subtype == MV_BELONGS_TO_LATCH )
        {
//printf( "Node %d <%s>\n", i, Ntk_NodeGetNamePrintable(pNode) );
            pVarsCs[iVar++] = i;
        }
        i++;
    }
    return pVarsCs;
}

/**Function*************************************************************

  Synopsis    [Creates the cube of only NS vars.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int * Mvn_NetworkGetMvNsVars( Ntk_Network_t * pNet )
{
    DdManager * dd;
    Vmx_VarMap_t * pVmxExt;
    int * pVarsNs;
    Ntk_Node_t * pNode;
    int i, iVar, nVarsIO;

    nVarsIO = Ntk_NetworkReadCiNum( pNet );

    dd      = Ntk_NetworkReadDdGlo( pNet );
    pVmxExt = Ntk_NetworkReadVmxGlo( pNet );

    pVarsNs = ALLOC( int, Ntk_NetworkReadCiNum( pNet ) );

    i = iVar = 0;
    Ntk_NetworkForEachCo( pNet, pNode )
    {
        if ( pNode->Subtype == MV_BELONGS_TO_LATCH )
            pVarsNs[iVar++] = nVarsIO + i;
        i++;
    }
    return pVarsNs;
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
Vmx_VarMap_t * Mvn_NetworkCreatePermutedVmx( Ntk_Network_t * pNet )
{
    Vmx_VarMap_t * pVmxExt;
    Vmx_VarMap_t * pVmxNew;
    Ntk_Node_t * pNode;
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
    i = 0;
    Ntk_NetworkForEachCi( pNet, pNode )
    {
        if ( pNode->Subtype == MV_BELONGS_TO_LATCH )
            pPermute[iVarLIO++] = i;
        else
            pPermute[iVarPIO++] = i;
        i++;
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


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


