/**CFile****************************************************************

  FileName    [fmbsSat.c]

  PackageName [Binary flexibility manager.]

  Synopsis    [Interface to the SAT computation.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 3.0. Started - September 27, 2003.]

  Revision    [$Id: fmbsSat.c,v 1.2 2004/10/18 01:39:23 alanmi Exp $]

***********************************************************************/

#include "fmbsInt.h"
 
////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int      Fmbs_ComputeSatSolutions( Fmbs_Manager_t * p, Mvr_Relation_t * pMvr, Ntk_Node_t * pPivot, int fVerbose );
static DdNode * Fmbs_DeriveBddFromTruth( Fmbs_Manager_t * p, int fDontCare );
static Mvc_Cover_t * Fmbs_DeriveMvcForCare( Fmbs_Manager_t * p, Mvr_Relation_t * pMvr );
static bool Fmbs_EstimateFfSize( Fmbs_Manager_t * p );

////////////////////////////////////////////////////////////////////////
///                     EXPORTED FUNCTIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvr_Relation_t * Fmbs_ManagerComputeLocalDcNode( Fmbs_Manager_t * p, Ntk_Node_t * pPivot, Ntk_Node_t * ppFanins[], int nFanins )
{
    DdNode * bFlex;
    Sh_Node_t * gConst1;
    Mvr_Manager_t * pMvrMan;
    Mvr_Relation_t * pMvrLoc;
    Cvr_Cover_t * pCvr;
    int nSolut, nSimul, clk;
    int nLevelTfi, nLevelTfo;
    assert( pPivot->Type == MV_NODE_INT );

    // get the window
clk = clock();
    if ( p->pPars->nLevelTfi == -1 || p->pPars->nLevelTfo == -1 )
        p->pWnd  = Wn_WindowCreateFromNetwork( p->pNet );
    else        
    {
        nLevelTfi = p->pPars->nLevelTfi;
        nLevelTfo = p->pPars->nLevelTfo;
        p->pWnd = Wn_WindowDeriveForNode( pPivot, nLevelTfi, nLevelTfo );
//printf( "Starting %d x %d.\n", Wn_WindowReadLeafNum(p->pWnd), Wn_WindowReadRootNum(p->pWnd) );
        // rescale the window down until it fits into these limites
        while ( !Fmbs_EstimateFfSize( p ) )
        {
            Wn_WindowDelete( p->pWnd );
            // get the new parameters
            nLevelTfi = (nLevelTfi > 0)? nLevelTfi - 1 : nLevelTfi;
            nLevelTfo = (nLevelTfo > 0)? nLevelTfo - 1 : nLevelTfo;
            // get the new window
            p->pWnd = Wn_WindowDeriveForNode( pPivot, nLevelTfi, nLevelTfo );
            if ( nLevelTfi == 0 && nLevelTfo == 0 )
                break;
        }

    }
p->timeWnd += clock() - clk;

if ( Fmbs_ManagerReadVerbose(p) )
printf( "Wnd = %2d x %2d x %3d ", 
       Wn_WindowReadLeafNum(p->pWnd),  
       Wn_WindowReadRootNum(p->pWnd),
       Wn_WindowReadNodeNum(p->pWnd) );


    // get the local BDD manager
    pMvrMan = Ntk_NetworkReadManMvr( p->pNet );
    p->pNet = pPivot->pNet;
    p->dd = Mvr_ManagerReadDdLoc(pMvrMan);
    pCvr = Ntk_NodeGetFuncCvr(pPivot);
    pMvrLoc = Fnc_FunctionDeriveMvrFromCvr( pMvrMan,
        Ntk_NetworkReadManVmx( p->pNet ), pCvr );

    if ( Wn_WindowReadLevelsTfi(p->pWnd) == 0 && Wn_WindowReadLevelsTfo(p->pWnd) == 0 )
    {
if ( Fmbs_ManagerReadVerbose(p) )
        printf( "Empty window for node %s.\n", Ntk_NodeGetNamePrintable(pPivot) );
        Wn_WindowDelete( p->pWnd );
        p->pWnd = NULL;
        return pMvrLoc;
    }

    // check the number of fanins
    p->nFanins = nFanins;
    if ( nFanins > 8 )
    {
        if ( Fmbs_ManagerReadVerbose(p) )
            printf( "The number of fanins of node %s (%d) is more than 8.\n", Ntk_NodeGetNamePrintable(pPivot), nFanins );
        Wn_WindowDelete( p->pWnd );
        p->pWnd = NULL;
        return pMvrLoc;
    }


clk = clock();
    // derive the strashed network
    p->pShNet = Wn_WindowStrashBinaryMiter( p->pShMan, p->pWnd );
    gConst1 = Sh_ManagerReadConst1( Sh_NetworkReadManager(p->pShNet) );
p->timeStr += clock() - clk;
//Sh_NetworkShow( p->pShNet );

//Wn_WindowShow( p->pWnd );
    Wn_WindowDelete( p->pWnd );
    p->pWnd = NULL;

    // the output is constant 0 -- the complete don't-care
    if ( Sh_NetworkReadOutput( p->pShNet, p->nFanins ) == Sh_Not(gConst1) ) 
    {
        if ( Fmbs_ManagerReadVerbose(p) )
        printf( "Constant 0 output for node %s.\n", Ntk_NodeGetNamePrintable(pPivot) );
        Sh_NetworkFree( p->pShNet );
        p->pShNet = NULL;
        Mvr_RelationAddDontCare( pMvrLoc, p->dd->one );
if ( Fmbs_ManagerReadVerbose(p) )
printf( "\n" );
        return pMvrLoc;
    }

    // check if the network is too large
    p->nNodes = Sh_NetworkCollectInternal( p->pShNet ) + Sh_NetworkReadInputNum( p->pShNet );
//printf( "%d ", p->nNodes );

//    if ( p->nNodes > 40 && p->nNodes < 70 )
//        Sh_NetworkShow( p->pShNet, "cdc_aig" );

clk = clock();
//if ( Sh_NetworkReadInputNum( p->pShNet ) < 20 )
//    Fmbs_Computation( p );
//else
//    printf( "\n" );
p->timeAig += clock() - clk;


if ( Fmbs_ManagerReadVerbose(p) )
printf( " ANDs = %3d ", p->nNodes );
    if ( p->nNodes > Fmbs_ManagerReadNodesMax(p) )
    {
        if ( Fmbs_ManagerReadVerbose(p) )
        printf( "The network has more than 500 nodes (%d). Sat solver is not used.\n", p->nNodes );
        Sh_NetworkFree( p->pShNet );
        p->pShNet = NULL;
        return pMvrLoc;
    }

    // if there are no internal nodes, this is the complete care set
    if ( Sh_NetworkReadNodeNum( p->pShNet ) == 0 ) 
    {
if ( Fmbs_ManagerReadVerbose(p) )
        printf( "The strashed network for node %s has no nodes.\n", Ntk_NodeGetNamePrintable(pPivot) );
        Sh_NetworkFree( p->pShNet );
        p->pShNet = NULL;
        return pMvrLoc;
    }

clk = clock();
    // compute the care set using simulation
    nSimul = Fmbs_Simulation( p );
    assert( nSimul <= (1<<p->nFanins) );
p->timeSim += clock() - clk;

if ( Fmbs_ManagerReadVerbose(p) )
printf( "%5s: Mint = %3d Sim = %3d", 
       Ntk_NodeGetNamePrintable(pPivot), (1<<p->nFanins), nSimul );


    if ( nSimul == (1<<p->nFanins) )
    { // the node's care set is complete => there is no don't-care
        Sh_NetworkFree( p->pShNet );
        p->pShNet = NULL;
if ( Fmbs_ManagerReadVerbose(p) )
printf( "\n" );
        return pMvrLoc;
    }


    // get the SAT solutions
clk = clock();
    nSolut = Fmbs_ComputeSatSolutions( p, pMvrLoc, pPivot, Fmbs_ManagerReadVerbose(p) );
    Sh_NetworkFree( p->pShNet );
    p->pShNet = NULL;
p->timeSat += clock() - clk;

    {
        int i, Counter;
        Counter = 0;
        for ( i = 0; i < (1<<p->nFanins); i++ )
            Counter += (p->pCareSet[i] == 0);
if ( Fmbs_ManagerReadVerbose(p) )
printf( " Sat = %3d  DCs = %3d\n", nSolut, Counter );
    }

    // compute the BDD for the don't-care
    bFlex = Fmbs_DeriveBddFromTruth( p, 1 );   Cudd_Ref( bFlex );
    Mvr_RelationAddDontCare( pMvrLoc, bFlex );
    Cudd_RecursiveDeref( p->dd, bFlex );
    return pMvrLoc;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fmbs_ComputeSatSolutions( Fmbs_Manager_t * p, Mvr_Relation_t * pMvr, Ntk_Node_t * pPivot, int fVerbose )
{
    Sat_IntVec_t * vProj, * vLits;
    Sh_Node_t * gNode, * gNode1, * gNode2;
    Sh_Node_t ** pgNodes, ** pgOutputs;
    int nNodesInt, fComp1, fComp2, Var, Var1, Var2, clk;
    int RetValue, nOutputs, i, iVar, Value;
    Mvc_Cover_t * pMvc;
    Mvc_Cube_t * pCube;
    int fCompls[100];
    unsigned Mask;
    int nSols, * pSols;
    int iMint;
    Sh_Node_t * gConst1;

    // clean the solver
    Sat_SolverClean( p->pSat, p->nNodes );

    gConst1 = Sh_ManagerReadConst1( Sh_NetworkReadManager(p->pShNet) );
//    return 0;
    vProj = Sat_IntVecAlloc( 100 );
    vLits = Sat_IntVecAlloc( 10 );

    // assign numbers to the AND-gates
    RetValue = Sh_NetworkSetNumbers( p->pShNet );

    // get the internal AND array
    pgNodes = Sh_NetworkReadNodes( p->pShNet );
    nNodesInt = Sh_NetworkReadNodeNum( p->pShNet );
    for ( i = 0; i < nNodesInt; i++ )
    {
        gNode = pgNodes[i];
        assert( !Sh_NodeIsVar(gNode) );

        gNode1 = Sh_NodeReadOne(gNode);
        gNode2 = Sh_NodeReadTwo(gNode);

        fComp1 = Sh_IsComplement( gNode1 );
        fComp2 = Sh_IsComplement( gNode2 );

        Var = Sh_NodeReadData( gNode );// + 1;
        Var1 = Sh_NodeReadData( Sh_Regular( gNode1 ) );// + 1;
        Var2 = Sh_NodeReadData( Sh_Regular( gNode2 ) );// + 1;

//        if ( fComp1 )
//            Var1 = -Var1;
//        if ( fComp2 )
//            Var2 = -Var2;

        // suppose the AND-gate is A * B = C
        // write !A => !C   or   A + !C
//        fprintf( pFile, "%d %d 0%c", Var1, -Var, 10 );
        Sat_IntVecClear( vLits );
        Sat_IntVecPush( vLits, SAT_VAR2LIT(Var1, fComp1) );
        Sat_IntVecPush( vLits, SAT_VAR2LIT(Var,  1) );
        RetValue = Sat_SolverAddClause( p->pSat, vLits );
        assert( RetValue );

        // write !B => !C   or   B + !C
//        fprintf( pFile, "%d %d 0%c", Var2, -Var, 10 );
        Sat_IntVecClear( vLits );
        Sat_IntVecPush( vLits, SAT_VAR2LIT(Var2, fComp2) );
        Sat_IntVecPush( vLits, SAT_VAR2LIT(Var,  1) );
        RetValue = Sat_SolverAddClause( p->pSat, vLits );
        assert( RetValue );

        // write A & B => C   or   !A + !B + C
//        fprintf( pFile, "%d %d %d 0%c", -Var1, -Var2, Var, 10 );
        Sat_IntVecClear( vLits );
        Sat_IntVecPush( vLits, SAT_VAR2LIT(Var1, !fComp1) );
        Sat_IntVecPush( vLits, SAT_VAR2LIT(Var2, !fComp2) );
        Sat_IntVecPush( vLits, SAT_VAR2LIT(Var, 0) );
        RetValue = Sat_SolverAddClause( p->pSat, vLits );
        assert( RetValue );
    }
//    free( pgNodes );

    // get the outputs
    pgOutputs = Sh_NetworkReadOutputs( p->pShNet );
    nOutputs    = Sh_NetworkReadOutputNum( p->pShNet );

    // add the clause corresponding to the Miter if it is not a constant)
    assert( pgOutputs[nOutputs-1] != Sh_Not(gConst1) );
    if ( pgOutputs[nOutputs-1] != gConst1 ) 
    {
        // check the last output
        fComp1 = Sh_IsComplement( pgOutputs[nOutputs-1] );
        Var1   = Sh_NodeReadData( Sh_Regular(pgOutputs[nOutputs-1]) );
        // add the last clause
        Sat_IntVecClear( vLits );
        Sat_IntVecPush( vLits, SAT_VAR2LIT(Var1, fComp1) );
        RetValue = Sat_SolverAddClause( p->pSat, vLits );
        assert( RetValue );
    }
    else
    {
        if ( fVerbose )
        printf( " SDC only " );
//        return 0;
    }

    // collect the projection variables
    Sat_IntVecClear( vProj );
    Mask = 0;  // mask to store info about complementations
    for ( i = 0; i < nOutputs - 1; i++ )
    {
        Var1 = Sh_NodeReadData( Sh_Regular(pgOutputs[i]) );
        Sat_IntVecPush( vProj, Var1 );
        fCompls[i] = Sh_IsComplement(pgOutputs[i]);
        if ( fCompls[i] )
            Mask |= (1<<i);
//printf( "Va =%2d C=%d   ", Var1+1, fCompls[i] );
    }
//if (Mask == 0 )
//printf( "Empty mask" );

    // add the clauses corresponding to the care set
    pMvc = Fmbs_DeriveMvcForCare( p, pMvr );
//Mvc_CoverPrintBinary( pMvc );
    Mvc_CoverForEachCube( pMvc, pCube )
    {
        Sat_IntVecClear( vLits );
        Mvc_CubeForEachVarValue( pMvc, pCube, iVar, Value )
        {
            assert( Value != 0 );
            if ( Value == 3 )
                continue;
            Var1 = Sat_IntVecReadEntry(vProj, iVar);
            assert( Var1 >= 0 );
            if ( (Value == 1) ^ fCompls[iVar] )
                Sat_IntVecPush( vLits, SAT_VAR2LIT(Var1, 0) );
            else
                Sat_IntVecPush( vLits, SAT_VAR2LIT(Var1, 1) );
        }
        assert( Sat_IntVecReadEntry(vLits, 0) >= 0 );
        RetValue = Sat_SolverAddClause( p->pSat, vLits );
        if ( RetValue == 0 )
        {
            // this happens when the output points to one of the local inputs
            // how it is possible?
//            printf( "1 " );
            return 0;

            Sh_NetworkShow( p->pShNet, "temp_aig" );
            printf( "assertion\n" );
        }
//        assert( RetValue );
    }
    Mvc_CoverFree( pMvc );


    RetValue = Sat_SolverSimplifyDB( p->pSat );
//    assert( RetValue );
    if ( RetValue == 0 )
    {
//        printf( "2 " );
        Sat_IntVecFree( vProj );
        Sat_IntVecFree( vLits );
        return 0;
    }


//Sat_SolverWriteDimacs( p->pSat, "start.cnf" );

clk = clock();
    RetValue = Sat_SolverSolve( p->pSat, NULL, vProj, -1 );
//    Sat_SolverPrintStats( p->pSat, (double)(clock()-clk)/CLOCKS_PER_SEC );
//    printf( RetValue ? "\nSATISFIABLE\n" : "\nUNSATISFIABLE\n" );
    p->timeSat += clock() - clk;

//Sat_SolverWriteDimacs( p->pSat, "stop.cnf" );

    // get the solutions from the solver
    nSols = Sat_SolverReadSolutions( p->pSat );
    pSols = Sat_SolverReadSolutionsArray( p->pSat );
/*
Extra_PrintBinary( stdout, &Mask, p->nFanins );
fprintf( stdout, "\n" );

    // add the solutions to the care set
    for ( i = 0; i < nSols; i++ )
    {
        Extra_PrintBinary( stdout, pSols + i, p->nFanins );
        fprintf( stdout, "   " );
        iMint = (pSols[i] ^ Mask);
        Extra_PrintBinary( stdout, &iMint, p->nFanins );
        fprintf( stdout, "   " );
        fprintf( stdout, "   %d", p->pCareSet[(pSols[i] ^ Mask)] );
        fprintf( stdout, "\n" );
    }
*/
    for ( i = 0; i < nSols; i++ )
    {
        iMint = (pSols[i] ^ Mask);
        p->pCareSet[iMint] = 1;
    }

    Sat_IntVecFree( vProj );
//printf( "a" );  fflush( stdout );
    Sat_IntVecFree( vLits );
//printf( "b" );  fflush( stdout );
    return nSols;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Fmbs_DeriveBddFromTruth( Fmbs_Manager_t * p, int fDontCare )
{
    DdManager * dd = p->dd;
    DdNode * bFlex, * bMint, * bTemp;
    int nMints = (1 << p->nFanins);
    int m;
    bFlex = b0;   Cudd_Ref( bFlex );
    for ( m = 0; m < nMints; m++ )
    {
        // skip the care set minterms
        if ( p->pCareSet[m] == fDontCare )
            continue;
        bMint = Extra_bddBitsToCube2( dd, m, p->nFanins, dd->vars ); Cudd_Ref( bMint );
        bFlex = Cudd_bddOr( dd, bTemp = bFlex, bMint );             Cudd_Ref( bFlex );
        Cudd_RecursiveDeref( dd, bTemp );
        Cudd_RecursiveDeref( dd, bMint );
    }
    Cudd_Deref( bFlex );
    return bFlex;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvc_Cover_t * Fmbs_DeriveMvcForCare( Fmbs_Manager_t * p, Mvr_Relation_t * pMvr )
{
    DdManager * dd = p->dd;
    DdNode * bFunc;
    Mvc_Cover_t * pMvc;
    // derive the cover for the condition
    bFunc = Fmbs_DeriveBddFromTruth( p, 0 );   Cudd_Ref( bFunc );
//PRB( dd, bFunc );

    pMvc = Fnc_FunctionDeriveSopFromMddSpecial( Ntk_NetworkReadManMvc( p->pNet ), 
            dd, bFunc, bFunc, Mvr_RelationReadVmx(pMvr), p->nFanins );
    Cudd_RecursiveDeref( dd, bFunc );
    return pMvc;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Fmbs_EstimateFfSize( Fmbs_Manager_t * p )
{
    Ntk_Node_t * pNode;
    int nFFlits, nFfflitsLimit;
    // count the number of literals in the FFs of the window
    nFFlits = 0;
    nFfflitsLimit = Fmbs_ManagerReadNodesMax(p);
    Ntk_NetworkForEachNodeSpecial( Wn_WindowReadNet(p->pWnd), pNode )
    {
        nFFlits += Cvr_CoverReadLitFacNum( Ntk_NodeGetFuncCvr(pNode) );
        if ( nFFlits > nFfflitsLimit )
            return 0;
    }
    return 1;
}


////////////////////////////////////////////////////////////////////////
///                         END OF FILE                              ///
////////////////////////////////////////////////////////////////////////


