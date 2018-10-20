/**CFile****************************************************************

  FileName    [mvnExtract.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Sequential manipulation on the level of the MV network.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: mvnStgExt.c,v 1.5 2004/04/08 04:48:26 alanmi Exp $]

***********************************************************************/

#include "mvnInt.h"
#include "aut.h"
#include "aio.h"
#include "ioInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int            Mvn_NetworkWriteSTG( Ntk_Network_t * pNet, char * pFileName, int nStatesLimit, int fUseFsm, int fVerbose, int fWriteAut );
static void           Mvn_NetworkAssignAutomatonNames( Ntk_Network_t * pNet, Aut_Auto_t * pAut );
static DdNode *       Mvn_NetworkGetBddCubeIoNs( Ntk_Network_t * pNet );
static DdNode *       Mvn_NetworkGetBddCubeNs( Ntk_Network_t * pNet );
static int *          Mvn_NetworkGetMvCsVars( Ntk_Network_t * pNet );
static int *          Mvn_NetworkGetMvNsVars( Ntk_Network_t * pNet );
static Vmx_VarMap_t * Mvn_NetworkCreatePermutedVmx( Ntk_Network_t * pNet );
static Aut_Var_t *    Mvn_NetworkGetVar( Ntk_Node_t * pNode );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Extract the STG from the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mvn_NetworkStgExtract( Ntk_Network_t * pNet, char * pFileName, int nStatesLimit, int fUseFsm, int fWriteAut, int fVerbose )
{
    ProgressBar * pProgress;
    Aut_Auto_t * pAut;
    Aut_State_t * pState, * pS;
    Aut_Trans_t * pTrans;
    Aut_Var_t ** ppVars, ** ppVarsL;
    Aut_Man_t * pMan;
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
    DdNode * bSubset;
    int nPartsT, nPartsO, nLatches, iVar, iVarL;
    int nVarsBinIoNs, nVarsPIO;
    int RetValue = 0;
    Aut_State_t ** pEntry;
    DdNode ** pbCodes; 
    int * pCodes, * pVarsCs, * pVarsNs;
//    int * pVarsPres;
//    int nVarsRem, nVarsAll, i;
    int * pValuesFirst;
    int iState;
//    st_table * tName2Var = NULL;

    // compute the global functions
    Ntk_NetworkGlobalComputeCo( pNet, 1, 0, 0 );
    dd = Ntk_NetworkReadDdGlo( pNet );

    // create the extended variable map for all COs 
    pVmxExt = Ntk_NetworkGlobalCreateVmxCo( pNet, 0 );
    Ntk_NetworkSetVmxGlo( pNet, pVmxExt );

    // get the permuted variable map 
    pVmxNew = Mvn_NetworkCreatePermutedVmx( pNet );
/*
    // cut off the useless vars
    nVarsAll = Ntk_NetworkReadCiNum(pNet) + Ntk_NetworkReadCoNum(pNet);
    nVarsRem = nVarsAll - 2 * Ntk_NetworkReadLatchNum(pNet);
    pVarsPres = ALLOC( int, nVarsAll );
    for ( i = 0; i < nVarsRem; i++ )
        pVarsPres[i] = 1;
    for (      ; i < nVarsAll; i++ )
        pVarsPres[i] = 0;
    pVmxNew = Vmx_VarMapCreateReduced( pVmxNew, pVarsPres );
    FREE( pVarsPres );
*/
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
    bVarsBinIoNs = Mvn_NetworkGetBddCubeIoNs( pNet );        Cudd_Ref( bVarsBinIoNs );
    pVarsBinIoNs = Extra_bddComputeVarArray( dd, bVarsBinIoNs, &nVarsBinIoNs ); 
    Cudd_RecursiveDeref( dd, bVarsBinIoNs );

    // get only NS variables 
    bVarsNs    = Mvn_NetworkGetBddCubeNs( pNet );            Cudd_Ref( bVarsNs );
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
    pMan = Aut_ManCreate( dd );
    pAut = Aut_AutoAlloc( pVmxNew, pMan );
    // set the network name
    Aut_AutoSetName( pAut, Extra_FileNameGeneric(pNet->pName) );

    // copy the input names
    iVar = 0;
    iVarL = 0;
    ppVars = Aut_AutoReadVars( pAut );
    ppVarsL = ALLOC( Aut_Var_t *, Ntk_NetworkReadLatchNum(pNet) );
    Ntk_NetworkForEachCi( pNet, pNode )
    {
        if ( pNode->Subtype != MV_BELONGS_TO_LATCH )
            ppVars[iVar++] = Mvn_NetworkGetVar( pNode );
        else
            ppVarsL[iVarL++] = Mvn_NetworkGetVar( pNode );
    }
    Ntk_NetworkForEachCo( pNet, pNode )
    {
        if ( pNode->Subtype != MV_BELONGS_TO_LATCH )
            ppVars[iVar++] = Mvn_NetworkGetVar( pNode );
    }
    Aut_AutoSetVarNum( pAut, iVar );
    Aut_AutoSetVarLNum( pAut, iVarL );
    Aut_AutoSetVarsL( pAut, ppVarsL );

    // create the initial state
    pState = Aut_StateAlloc( pAut );
    Aut_StateSetAcc( pState, 1 );
    Aut_StateSetData( pState, (char *)bInitState ); // takes ref 
    Aut_AutoStateAddLast( pState );
    // set the initial state
    Aut_AutoSetInitNum( pAut, 1 );
    Aut_AutoSetInit( pAut, pState );

    // start the table to collect the reachable states
    tReachStates = st_init_table(st_ptrcmp,st_ptrhash);
    st_insert( tReachStates, (char *)bInitState, (char *)pState );

    // create the cubes of inputs, outputs, CS, and NS
    pbCodes = Vmx_VarMapEncodeMap( dd, pVmxExt );
    pCodes = ALLOC( int, nLatches );
    pVarsCs = Mvn_NetworkGetMvCsVars( pNet );
    pVarsNs = Mvn_NetworkGetMvNsVars( pNet );
    pValuesFirst = Vm_VarMapReadValuesFirstArray( Vmx_VarMapReadVm( pVmxExt ) );

    // explore the reachable states
    if ( !fVerbose )
        pProgress = Extra_ProgressBarStart( stdout, 1000 );
    iState = 0;
    Aut_AutoForEachState( pAut, pState )
    {
        if ( iState++ == nStatesLimit )
        {
            if ( !fVerbose )
                Extra_ProgressBarStop( pProgress );
            RetValue = 1;
            goto FINISH;
        }

        // get the subset
        bSubset = (DdNode *)Aut_StateReadData( pState );
        // get the transitions for this state
        bRelationTr = Extra_bddImageCompute( pTreeT, bSubset ); 
        if ( bRelationTr == NULL )
            goto FINISH;
        Cudd_Ref( bRelationTr );
//printf( "%d  : ", s );
//PRB( dd, bRelationTr );
        // get the outputs for this state
        bRelationIo = Extra_bddImageCompute( pTreeO, bSubset ); 
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
                pS = Aut_StateAlloc( pAut );
                Aut_StateSetAcc( pS, 1 );
                Aut_StateSetData( pS, (char *)bStateCs );   Cudd_Ref( bStateCs );
                Aut_AutoStateAddLast( pS );
                *pEntry = pS;
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

            // create a new transition
            pTrans = Aut_TransAlloc( pAut );
            Aut_TransSetStateFrom( pTrans, pState );
            Aut_TransSetStateTo  ( pTrans, *pEntry );
            Aut_TransSetCond     ( pTrans, bDomain );    // takes ref
            // add the transition
            Aut_AutoAddTransition( pTrans );

            bRelationTr = Cudd_bddAnd( dd, bTemp = bRelationTr, Cudd_Not( bStateNs ) ); Cudd_Ref( bRelationTr );
            Cudd_RecursiveDeref( dd, bTemp );
            Cudd_RecursiveDeref( dd, bStateNs );
        }
        Cudd_RecursiveDeref( dd, bRelationTr );
        Cudd_RecursiveDeref( dd, bRelationIo );

        if ( !fVerbose )
            if ( iState && iState % 100 == 0 )
            {
                char Buffer[100];
                sprintf( Buffer, "%d states", iState );
                Extra_ProgressBarUpdate( pProgress, 1000 * iState / Aut_AutoReadStateNum(pAut), Buffer );
            }
    }
    if ( !fVerbose )
        Extra_ProgressBarStop( pProgress );

    Mvn_NetworkAssignAutomatonNames( pNet, pAut );

    fprintf( stdout, "The STG with %d states and %d transitions is written to file \"%s\".\n", 
        Aut_AutoReadStateNum(pAut), Aut_AutoCountTransitions(pAut), pFileName );

    // write the automaton into the file
    Aio_AutoWrite( pAut, pFileName, fWriteAut, fUseFsm );

FINISH:
    // undo the image tree
    Extra_bddImageTreeDelete( pTreeT );
    Extra_bddImageTreeDelete( pTreeO );
    Cudd_RecursiveDeref( dd, bVarsNs ); 
    st_free_table( tReachStates );
    Vmx_VarMapEncodeDeref( dd, pVmxExt, pbCodes );
    FREE( pVarsCs );
    FREE( pVarsNs );
    FREE( pCodes );
    // remove the manager
    if ( pNet->pNetExdc )
        Ntk_NetworkWriteDdGlo( pNet->pNetExdc, NULL );
    Ntk_NetworkWriteDdGlo( pNet, NULL );
    Aut_AutoFree( pAut );
/*
    if ( RetValue == 0 )
        Extra_StopManager( dd );
    else
    {
        printf( "The limit on the number of states (%d) is reached; STG extraction aborted.\n", nStatesLimit );
        Cudd_Quit( dd );
    }
*/
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Creates the cube of IO and NS vars.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvn_NetworkAssignAutomatonNames( Ntk_Network_t * pNet, Aut_Auto_t * pAut )
{
    Aut_State_t * pState;
    char Buffer[500], Digit[10];
    int * pVarsCs;
    int * pCodes;
    int nDigits, nLatches, i;
    int fUseLongNames = 1;
    DdManager * dd;
    DdNode * bSubset;
    Vmx_VarMap_t * pVmxExt;
    int iState;

    dd = Ntk_NetworkReadDdGlo( pNet );
    pVmxExt = Ntk_NetworkReadVmxGlo( pNet );

    // get the number of digits in the state number
    nDigits = Extra_Base10Log( Aut_AutoReadStateNum(pAut) );
    nLatches = Ntk_NetworkReadLatchNum( pNet );

    pVarsCs = NULL;
    pCodes = NULL;
    if ( fUseLongNames )
    {
        pVarsCs = Mvn_NetworkGetMvCsVars( pNet );
        pCodes = ALLOC( int, nLatches );
    }

    // generate the state names and acceptance conditions
    iState = 0;
    Aut_AutoForEachState( pAut, pState )
    {
        // print simple state names
        bSubset = (DdNode *) Aut_StateReadData( pState );
        if ( fUseLongNames )
        {
            // decode the cube
            Vmx_VarMapDecodeCube( dd, bSubset, pVmxExt, pVarsCs, pCodes, nLatches );
            // print symbolic state name
            Buffer[0] = 0;
            for ( i = 0; i < nLatches; i++ )
            {
                sprintf( Digit, "%d", pCodes[i] );
                strcat( Buffer, Digit );
            }
        }
        else
            sprintf( Buffer, "s%0*d", nDigits, iState );
        // set the state name
        Aut_StateSetName( pState, util_strsav(Buffer) );
        Aut_AutoStateAddToTable( pState );
        // deref the subset
        Cudd_RecursiveDeref( dd, bSubset );
        iState++;
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
DdNode * Mvn_NetworkGetBddCubeIoNs( Ntk_Network_t * pNet )
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
DdNode * Mvn_NetworkGetBddCubeNs( Ntk_Network_t * pNet )
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

/**Function*************************************************************

  Synopsis    [Derive the variable.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aut_Var_t * Mvn_NetworkGetVar( Ntk_Node_t * pNode )
{
    Aut_Var_t * pVar;
    Io_Var_t * pIoVar;
    char ** pValueNames;
    int i;
    st_table * tName2Var = (st_table *)pNode->pNet->pData;

    pVar = Aut_VarAlloc();
    Aut_VarSetName( pVar, util_strsav( pNode->pName ) );
    Aut_VarSetValueNum( pVar, pNode->nValues );
    if ( tName2Var && st_lookup( tName2Var, pNode->pName, (char **)&pIoVar ) )
    {
        assert( pNode->nValues == pIoVar->nValues );
        pValueNames = ALLOC( char *, pIoVar->nValues );
        for ( i = 0; i < pIoVar->nValues; i++ )
            pValueNames[i] = util_strsav( pIoVar->pValueNames[i] );
        Aut_VarSetValueNames( pVar, pValueNames );
    }
    return pVar;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


