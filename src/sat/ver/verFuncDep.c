/**CFile****************************************************************

  FileName    [verFuncDep.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Explores functional dependency of latch excitation functions.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: verFuncDep.c,v 1.2 2004/10/06 03:33:55 alanmi Exp $]

***********************************************************************/

#include "verInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void           Ver_RemovePrimaryOutputs( Ntk_Network_t * pNet, Sh_Network_t * pShNet );
static Sh_Network_t * Ver_TransferNetwork( Sh_Network_t * pShNet );
static Sh_Node_t *    Ver_FuncDepDeriveMiter( Sh_Network_t * pShNet1, Sh_Network_t * pShNet2, int * pSort, int nSort, int iCur );
static Sh_Node_t *    Ver_FuncDepDeriveMiter2( Sh_Network_t * pShNet1, Sh_Network_t * pShNet2, int * pPres, int iCur );
static void           Ver_FunctionalDependencyCheck( Ntk_Network_t * pNet, Sh_Network_t * pShNet1, Sh_Network_t * pShNet2 );
static void           Ver_FunctionalDependencyCheck2( Sh_Network_t * pShNet1, Sh_Network_t * pShNet2 );
static void           Ver_FindLatchDependency( Ntk_Network_t * pNet, Mvc_Cover_t * pMvc, int iLatch, Extra_IntVec_t * pVec );
static Mvc_Cover_t *  Ver_ComputeLatchSupports( Ntk_Network_t * pNet );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Explores functional dependency of the next state functions of the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ver_FunctionalDependency( Ntk_Network_t * pNet )
{
    Sh_Network_t * pShNet1, * pShNet2;
    Sh_Manager_t * pShMan;

    if ( !Ntk_NetworkIsBinary(pNet) )
    {
        printf( "Currently can only work with binary networks, but the network is not binary.\n" );
        return;
    }
    if ( !Ntk_NetworkReadLatchNum(pNet) )
    {
        printf( "Currently only work with latch next state funtions, but there are no latched in the newtork.\n" );
        return;
    }

    // derive the next state functions:
    pShMan = Sh_ManagerCreate( 2 * Ntk_NetworkReadCiNum(pNet), 100000, 1 );
    pShNet1 = Ver_NetworkStrash( pShMan, pNet, 0 );
    // remove the primary output functions
    Ver_RemovePrimaryOutputs( pNet, pShNet1 );
    // transfer the functions into a different variable range
    pShNet2 = Ver_TransferNetwork( pShNet1 );

    // compute the functional dependency
    Ver_FunctionalDependencyCheck( pNet, pShNet1, pShNet2 );

    Sh_NetworkFree( pShNet1 );
    Sh_NetworkFree( pShNet2 );
    Sh_ManagerFree( pShMan, 1 );   
}

/**Function*************************************************************

  Synopsis    [Leaves only next state functions in the strashed network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ver_RemovePrimaryOutputs( Ntk_Network_t * pNet, Sh_Network_t * pShNet )
{
    Ntk_Node_t * pNode;
    Sh_Manager_t * pShMan;
    Sh_Node_t ** pgRoots;
    int nRoots, i, k;

    pShMan  = Sh_NetworkReadManager( pShNet );
    pgRoots = Sh_NetworkReadOutputs( pShNet );
    nRoots  = Sh_NetworkReadOutputNum( pShNet );
    i = k = 0;
    Ntk_NetworkForEachCo( pNet, pNode )
    {
        if ( pNode->Subtype == MV_BELONGS_TO_NET )
        {
            Sh_RecursiveDeref( pShMan, pgRoots[i++] );
            continue;
        }
        pgRoots[k++] = pgRoots[i++];
    }
    assert( i == nRoots );
    assert( k == Ntk_NetworkReadLatchNum(pNet) );
    Sh_NetworkSetOutputNum( pShNet, k );
}

/**Function*************************************************************

  Synopsis    [Transfers the outputs to depend on another set of PI variables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sh_Network_t * Ver_TransferNetwork( Sh_Network_t * pShNet )
{
    Sh_Manager_t * pShMan;
    Sh_Network_t * pShNetNew;
    Sh_Node_t ** pgRoots, ** pgVars;
    Sh_Node_t * gTemp;
    int nRoots, nVars, i;

    pShNetNew = Sh_NetworkDup( pShNet );
    pShMan  = Sh_NetworkReadManager( pShNetNew );
    pgRoots = Sh_NetworkReadOutputs( pShNetNew );
    nRoots  = Sh_NetworkReadOutputNum( pShNetNew );

    pgVars = Sh_ManagerReadVars( pShMan );
    nVars = Sh_ManagerReadVarsNum( pShMan ) / 2;

    for ( i = 0; i < nRoots; i++ )
    {
        pgRoots[i] = Sh_NodeVarReplace( pShMan, gTemp = pgRoots[i], pgVars, pgVars + nVars, nVars ); 
        shRef( pgRoots[i] );
        Sh_RecursiveDeref( pShMan, gTemp );
    }
    return pShNetNew;
}

/**Function*************************************************************

  Synopsis    [Derives the Miter cone to assert the independene of the given output.]

  Description [The cone is derived as follows. The given output is independent
  iff there exist input minterms x1 and x2 s.t. [F(X1) == F(x2)] & [g(X1) != g(x2)].]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sh_Node_t * Ver_FuncDepDeriveMiter( Sh_Network_t * pShNet1, Sh_Network_t * pShNet2, int * pSort, int nSort, int iCur )
{
    Sh_Manager_t * pShMan;
    Sh_Node_t ** pgRoots1, ** pgRoots2;
    Sh_Node_t * gRes, * gExor, * gTemp;
    int nRoots1, nRoots2, i;

    pShMan   = Sh_NetworkReadManager( pShNet1 );

    pgRoots1 = Sh_NetworkReadOutputs( pShNet1 );
    pgRoots2 = Sh_NetworkReadOutputs( pShNet2 );
    nRoots1  = Sh_NetworkReadOutputNum( pShNet1 );
    nRoots2  = Sh_NetworkReadOutputNum( pShNet2 );
    assert( nRoots1 == nRoots2 );
    assert( iCur < nRoots1 );

    gRes = Sh_NodeExor( pShMan, pgRoots1[iCur], pgRoots2[iCur] );   shRef( gRes );
    for ( i = 0; i < nSort; i++ )
    {
        assert( pSort[i] != iCur );
        gExor = Sh_NodeExor( pShMan, pgRoots1[pSort[i]], pgRoots2[pSort[i]] );   shRef( gExor );
        gRes  = Sh_NodeAnd( pShMan, gTemp = gRes, Sh_Not(gExor) ); shRef( gRes );
        Sh_RecursiveDeref( pShMan, gTemp );
        Sh_RecursiveDeref( pShMan, gExor );
    }
    Sh_Deref( gRes );
    return gRes;
}

/**Function*************************************************************

  Synopsis    [Finds a minimal set of independent functions.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ver_FunctionalDependencyCheck( Ntk_Network_t * pNet, Sh_Network_t * pShNet1, Sh_Network_t * pShNet2 )
{
    ProgressBar * pProgress;
    Sat_Solver_t * pSat;
    Sh_Manager_t * pShMan;
    Sh_Network_t * pShNet;
    Sh_Node_t * gNode;
    int * pSort, nSort;
    int nRoots, nVars, i, k;
    int nAndGates, nAndGatesMax;
    int RetValue;
    Mvc_Cover_t * pMvc;
    Extra_IntVec_t * pVec = Extra_IntVecAlloc( 100 );

    // get supports of the latches
    pMvc = Ver_ComputeLatchSupports( pNet );

    // count the max number of AND gates
    pShMan = Sh_NetworkReadManager( pShNet1 );
    nRoots = Sh_NetworkReadOutputNum( pShNet1 );
    nVars  = Sh_ManagerReadVarsNum( pShMan ) / 2;
//    nAndGatesMax = 2 * Sh_NetworkCollectInternal( pShNet1 ) + 2 * nVars + 10 * nRoots;
    nAndGatesMax = 1000;
    // start the solver
    pSat = Sat_SolverAlloc( nAndGatesMax, 1, 1, 1, 1, 0 );

    // try adding other functions
    pProgress = Extra_ProgressBarStart( stdout, nRoots );
    for ( i = 0; i < nRoots; i++ )
    {
        // find the dependency candidates
        Ver_FindLatchDependency( pNet, pMvc, i, pVec );
        pSort = Extra_IntVecReadArray( pVec );
        nSort = Extra_IntVecReadSize( pVec );
        if ( nSort > 32 )
            nSort = 32;
        // check until we come to independency
        k = 0;
        do {
            // get the miter of the corresponding functions
            gNode = Ver_FuncDepDeriveMiter( pShNet1, pShNet2, pSort, nSort - k, i );  shRef( gNode );
            if ( Sh_Regular(gNode) == pShMan->pConst1 )
            {
                RetValue = (gNode == Sh_Not(pShMan->pConst1));
                Sh_RecursiveDeref( pShMan, gNode );
            }
            else
            {
                // convert into a network
                pShNet = Sh_NetworkCreateFromNode( pShMan, gNode );
                Sh_RecursiveDeref( pShMan, gNode );
    //    Sh_NetworkShow( pShNet, "net" );

                // count the internal nodes
                nAndGates = Sh_NetworkCollectInternal( pShNet ) + 2 * nVars;
                if ( nAndGates > nAndGatesMax )
                {
                    Sat_SolverFree( pSat );
                    pSat = Sat_SolverAlloc( 2 * nAndGates, 1, 1, 1, 1, 0 );
                    nAndGatesMax = 2 * nAndGates;
                }

    //printf( "%d ", nAndGates );
                Sat_SolverClean( pSat, nAndGates );

                // set up the problem
                Vec_NetworkAddClauses( pSat, pShNet );
                Ver_NetworkAddOutputOne( pSat, pShNet );
                Sh_NetworkFree( pShNet );
Sat_SolverWriteDimacs( pSat, "fd.cnf" );

                // solve the problem
                // reduce the data base and solve
                RetValue = 0;
                if ( Sat_SolverSimplifyDB( pSat ) == 0 )
                {
    //   printf( "DB simplification returned proof.\n" );
                    RetValue = 1;
                }
                else
                {
                    int i;
    //   printf( "Solving the problem...\n" );
                    RetValue = (Sat_SolverSolve( pSat, NULL, NULL, -1 ) == SAT_FALSE);
                    i = 0;
                }
                // RetValue = 1, that is UNSAT, means functionally dependent
//                if ( RetValue == 0 ) // independent
//                    pPres[i] = 1;
            }
            k++;
        } while ( RetValue && nSort - k > 1 );
//        printf( "Latch %3d : Initial = %2d.  Final = %2d.\n", i, nSort, ((k==1)? -1 : nSort-k+1-RetValue) );

        if ( i % 5 == 0 )
            Extra_ProgressBarUpdate( pProgress, i, NULL );
    }
    Extra_ProgressBarStop( pProgress );
    Sat_SolverFree( pSat );
    Mvc_CoverFree( pMvc );

    // count the number of independent function
//    Counter = 0;
//    for ( i = 0; i < nRoots; i++ )
//        Counter += pPres[i];
//    printf( "Total number of functions = %4d. Independent set = %d.\n", nRoots, Counter );
    Extra_IntVecFree( pVec );
}

/**Function*************************************************************

  Synopsis    [Collect information about structural supports of NS functions.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvc_Cover_t * Ver_ComputeLatchSupports( Ntk_Network_t * pNet )
{
    Mvc_Manager_t * pMvcMan;
    Mvc_Cover_t * pMvc;
    Mvc_Cube_t * pCube;
    Ntk_Node_t * pNode, * pFanin;
    Ntk_Pin_t * pPin;
    int nLatches, i;
    // starting the cover
    pMvcMan = Ntk_NetworkReadManMvc(pNet);
    nLatches = Ntk_NetworkReadLatchNum(pNet);
    pMvc = Mvc_CoverAlloc( pMvcMan, nLatches );
    // set the PI node supports
    i = 0;
    Ntk_NetworkForEachCi( pNet, pNode )
    {
        pCube = Mvc_CubeAlloc( pMvc );
        Mvc_CubeBitClean( pCube );
        Mvc_CubeBitInsert( pCube, i );
        i++;
        pNode->pData = (char *)pCube;
    }
    // attach a cube to each of the nodes
    Ntk_NetworkDfs( pNet, 1 );
    Ntk_NetworkForEachNodeSpecial( pNet, pNode )
    {
        if ( pNode->Type == MV_NODE_CI )
            continue;
//        if ( Ntk_NodeReadFaninNum(pNode) == 0 )
//            continue;
        pCube = Mvc_CubeAlloc( pMvc );
        Mvc_CubeBitClean( pCube );
        Ntk_NodeForEachFanin( pNode, pPin, pFanin )
        {
//            if ( Ntk_NodeReadFaninNum(pFanin) == 0 )
//                continue;
            Mvc_CubeBitOr( pCube, pCube, (Mvc_Cube_t *)pFanin->pData );
        }
        pNode->pData = (char *)pCube;
    }
    // collect the CO cubes
    i = 0;
    Ntk_NetworkForEachCo( pNet, pNode )
    {
        if ( pNode->Subtype == MV_BELONGS_TO_NET )
            continue;
        pCube = (Mvc_Cube_t *)pNode->pData;
        pNode->pData = NULL;
        Mvc_CoverAddCubeTail( pMvc, pCube );
        Mvc_CubeSetSize( pCube, i );
        i++;
    }
    // recycle other cubes
    Ntk_NetworkForEachNodeSpecial( pNet, pNode )
    {
        if ( pNode->pData == NULL )
            continue;
//        if ( Ntk_NodeReadFaninNum(pNode) == 0 )
//            continue;
//        Mvc_CubeFree( pMvc, (Mvc_Cube_t *)pNode->pData );
    }
    // create array of cubes
    Mvc_CoverList2Array( pMvc );
    Mvc_CoverAllocateMask( pMvc );
    return pMvc;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ver_FindLatchDependency( Ntk_Network_t * pNet, Mvc_Cover_t * pMvc, int iLatch, Extra_IntVec_t * pVec )
{
    Mvc_Cube_t * pCubeMask, * pCubeThis, * pCubeOther;
    int nOnes, Res, i;
    int * pSort, nSort;

    Extra_IntVecClear( pVec );
    
    // find all the latches related to the given one
    pCubeMask = pMvc->pMask;
    pCubeThis = pMvc->pCubes[iLatch];
    Mvc_CoverForEachCube( pMvc, pCubeOther )
    {
        if ( pCubeThis == pCubeOther )
            continue;
        Mvc_CubeBitAnd( pCubeMask, pCubeThis, pCubeOther );
        Mvc_CubeBitEmpty( Res, pCubeMask );
        if ( Res )
            continue;
        nOnes = Mvc_CoverGetCubeSize( pCubeMask );
        Extra_IntVecPush( pVec, ((nOnes << 16) | Mvc_CubeReadSize(pCubeOther)) );
    }
    Extra_IntVecSort( pVec );
    pSort = Extra_IntVecReadArray( pVec );
    nSort = Extra_IntVecReadSize( pVec );
    // remove extra info
    for ( i = 0; i < nSort; i++ )
    {
//        printf( "Overlap = %2d.  Latch = %2d.\n", (pSort[i] >> 16), (pSort[i] & 0xffff)  );
        pSort[i] = (pSort[i] & 0xffff);
    }
    // reverse order
    for ( i = 0; i < nSort/2; i++ )
    {
        Res = pSort[nSort-1-i];
        pSort[nSort-1-i] = pSort[i];
        pSort[i] = Res;
    }
}






/**Function*************************************************************

  Synopsis    [Derives the Miter cone to assert the independene of the given output.]

  Description [The cone is derived as follows. The given output is independent
  iff there exist input minterms x1 and x2 s.t. [F(X1) == F(x2)] & [g(X1) != g(x2)].]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sh_Node_t * Ver_FuncDepDeriveMiter2( Sh_Network_t * pShNet1, Sh_Network_t * pShNet2, int * pPres, int iCur )
{
    Sh_Manager_t * pShMan;
    Sh_Node_t ** pgRoots1, ** pgRoots2;
    Sh_Node_t * gRes, * gExor, * gTemp;
    int nRoots1, nRoots2, i;

    pShMan   = Sh_NetworkReadManager( pShNet1 );

    pgRoots1 = Sh_NetworkReadOutputs( pShNet1 );
    pgRoots2 = Sh_NetworkReadOutputs( pShNet2 );
    nRoots1  = Sh_NetworkReadOutputNum( pShNet1 );
    nRoots2  = Sh_NetworkReadOutputNum( pShNet2 );
    assert( nRoots1 == nRoots2 );
    assert( pPres[iCur] == 0 );
    assert( iCur < nRoots1 );

    gRes = Sh_NodeExor( pShMan, pgRoots1[iCur], pgRoots2[iCur] );   shRef( gRes );
    for ( i = 0; i < nRoots1; i++ )
    {
        if ( pPres[i] == 0 )
            continue;
        gExor = Sh_NodeExor( pShMan, pgRoots1[i], pgRoots2[i] );   shRef( gExor );
        gRes  = Sh_NodeAnd( pShMan, gTemp = gRes, Sh_Not(gExor) ); shRef( gRes );
        Sh_RecursiveDeref( pShMan, gTemp );
        Sh_RecursiveDeref( pShMan, gExor );
    }
    Sh_Deref( gRes );
    return gRes;
}

/**Function*************************************************************

  Synopsis    [Finds a minimal set of independent functions.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ver_FunctionalDependencyCheck2( Sh_Network_t * pShNet1, Sh_Network_t * pShNet2 )
{
    ProgressBar * pProgress;
    Sat_Solver_t * pSat;
    Sh_Manager_t * pShMan;
    Sh_Network_t * pShNet;
    Sh_Node_t * gNode;
    int * pPres;
    int nRoots, nVars, i;
    int nAndGates, nAndGatesMax;
    int RetValue;
    int Counter;

    // count the max number of AND gates
    pShMan = Sh_NetworkReadManager( pShNet1 );
    nRoots = Sh_NetworkReadOutputNum( pShNet1 );
    nVars  = Sh_ManagerReadVarsNum( pShMan ) / 2;
    nAndGatesMax = 2 * Sh_NetworkCollectInternal( pShNet1 ) + 2 * nVars + 10 * nRoots;
    // start the solver
    pSat = Sat_SolverAlloc( nAndGatesMax, 1, 1, 1, 1, 0 );

    // add the first function
    pPres = ALLOC( int, nRoots );
    memset( pPres, 0, sizeof(int) * nRoots );
    pPres[0] = 1;

    // try adding other functions
    pProgress = Extra_ProgressBarStart( stdout, nRoots );
    for ( i = 1; i < nRoots; i++ )
    {
        // get the miter of the corresponding functions
        gNode = Ver_FuncDepDeriveMiter2( pShNet1, pShNet2, pPres, i );  shRef( gNode );
        if ( Sh_Regular(gNode) == pShMan->pConst1 )
            RetValue = (gNode == Sh_Not(pShMan->pConst1));
        else
        {
            // convert into a network
            pShNet = Sh_NetworkCreateFromNode( pShMan, gNode );
            Sh_RecursiveDeref( pShMan, gNode );
//    Sh_NetworkShow( pShNet, "net" );

            // count the internal nodes
            nAndGates = Sh_NetworkCollectInternal( pShNet ) + 2 * nVars;
            assert( nAndGates <= nAndGatesMax );
//printf( "%d ", nAndGates );
            Sat_SolverClean( pSat, nAndGates );
            // set up the problem
            Vec_NetworkAddClauses( pSat, pShNet );
            Ver_NetworkAddOutputOne( pSat, pShNet );
            Sh_NetworkFree( pShNet );

            // solve the problem
            // reduce the data base and solve
            RetValue = 0;
            if ( Sat_SolverSimplifyDB( pSat ) == 0 )
            {
//   printf( "DB simplification returned proof.\n" );
                RetValue = 1;
            }
            else
            {
//   printf( "Solving the problem...\n" );
                RetValue = (Sat_SolverSolve( pSat, NULL, NULL, -1 ) == SAT_FALSE);
            }
        }
        // RetValue = 1, that is UNSAT, means functionally dependent
        if ( RetValue == 0 ) // independent
            pPres[i] = 1;
//        else
//            printf( "Next state function %2d is dependent on previous functions\n", i );
        if ( i % 5 == 0 )
            Extra_ProgressBarUpdate( pProgress, i, NULL );
    }
    Extra_ProgressBarStop( pProgress );
    Sat_SolverFree( pSat );

    // count the number of independent function
    Counter = 0;
    for ( i = 0; i < nRoots; i++ )
        Counter += pPres[i];
    printf( "Total number of functions = %4d. Independent set = %d.\n", nRoots, Counter );
    FREE( pPres );
}



////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


