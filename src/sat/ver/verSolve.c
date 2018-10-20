/**CFile****************************************************************

  FileName    [verSolve.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Procedures employing SAT solver for verification.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: verSolve.c,v 1.6 2004/10/06 03:33:55 alanmi Exp $]

***********************************************************************/

#include "verInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static Sat_Solver_t * Ver_NetworkProveClasses( Ver_Manager_t * p, int iSplitLimit );
static void Ver_NetworkSplitClass( Sat_Solver_t * pSat, Sh_Node_t * pCand, Sh_Node_t ** ppRest, int iSplitLimit );
static int Ver_NetworkProveNodes( Sat_Solver_t * pSat, Sh_Node_t * one, Sh_Node_t * two );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Prove or disprove the equivalence classes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ver_NetworkEquivCheckSat( Ver_Manager_t * p )
{
    Sat_Solver_t * pSat;
    int RetValue;

    // set up the SAT problem
    pSat = Ver_NetworkProveClasses( p, 5 );  // We don't really need to identify all internal equiv nodes
    // assert the miter
    Ver_NetworkAddOutputOne( pSat, p->pShNet );

    // reduce the data base and solve
    if ( Sat_SolverSimplifyDB( pSat ) == 0 )
    {
//printf( "DB simplification returned proof.\n" );
        RetValue = 1;
    }
    else
    {
//printf( "Solving the problem...\n" );
        RetValue = (Sat_SolverSolve( pSat, NULL, NULL, -1 ) == SAT_FALSE);
    }
    Sat_SolverFree( pSat );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Prove or disprove the equivalence classes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ver_NetworkSweepSat( Ver_Manager_t * p, int iSplitLimit )
{
    Sat_Solver_t * pSat;
    pSat = Ver_NetworkProveClasses( p, iSplitLimit );
    Sat_SolverFree( pSat );
}

/**Function*************************************************************

  Synopsis    [Prove or disprove the equivalence classes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

/***** Old Code / New Code at bottom of file ****************

Sat_Solver_t * Ver_NetworkProveClasses( Ver_Manager_t * p )
{
    ProgressBar * pProgress;
    Sat_Solver_t * pSat;
    Sat_IntVec_t * vProj;
    int fComp1, fComp2, fComp, Var1, Var2, k, c;
    int RetValue, RetValue1, RetValue2;
    int nFalse00 = 0, nFalse01 = 0, nFalse10 = 0;
    int fSkipProof = 0;

    // clean the solver
    pSat = Sat_SolverAlloc( p->pShNet->nNodes + p->pShMan->nVars, 1, 1, 1, 1, 0 );
    Sat_SolverClean( pSat, p->pShNet->nNodes + p->pShMan->nVars );

    // add the clauses for the AI-graph
    Vec_NetworkAddClauses( pSat, p->pShNet );
//Sat_SolverWriteDimacs( pSat, "start.cnf" );
    if ( fSkipProof )
        return pSat;

    // go through the equivalence classes
    pProgress = Extra_ProgressBarStart( stdout, p->nClasses );
    vProj = Sat_IntVecAlloc( 10 );
    k = 0;
    for ( c = 0; c < p->nClasses; c++ )
    {
        assert( p->ppClasses[c]->pOrder );

        // get the first two vars
        Var1 = p->ppClasses[c]->pData;
        Var2 = p->ppClasses[c]->pOrder->pData; 
        fComp1 = p->ppClasses[c]->fMark;
        fComp2 = p->ppClasses[c]->pOrder->fMark;
        fComp  = ( fComp1 != fComp2 ); // they have different phase

        // A = 0; B = 1     OR     A = 0; B = 0 
        Sat_IntVecClear( vProj );
        Sat_IntVecPush( vProj, SAT_VAR2LIT(Var1, 1) );
        Sat_IntVecPush( vProj, SAT_VAR2LIT(Var2, fComp) );
        // solve under assumptions
        RetValue1 = Sat_SolverSolve( pSat, vProj, NULL, -1 );
        if ( RetValue1 == SAT_FALSE )
        {
            // add the clause
            Sat_IntVecClear( vProj );
            Sat_IntVecPush( vProj, SAT_VAR2LIT(Var1, 0) );
            Sat_IntVecPush( vProj, SAT_VAR2LIT(Var2, !fComp) );
            RetValue = Sat_SolverAddClause( pSat, vProj );
            assert( RetValue );
        }

        // A = 1; B = 0     OR     A = 1; B = 1 
        Sat_IntVecClear( vProj );
        Sat_IntVecPush( vProj, SAT_VAR2LIT(Var1, 0) );
        Sat_IntVecPush( vProj, SAT_VAR2LIT(Var2, !fComp) );
        // solve under assumptions
        RetValue2 = Sat_SolverSolve( pSat, vProj, NULL, -1 );
        if ( RetValue2 == SAT_FALSE )
        {
            // add the clause
            Sat_IntVecClear( vProj );
            Sat_IntVecPush( vProj, SAT_VAR2LIT(Var1, 1) );
            Sat_IntVecPush( vProj, SAT_VAR2LIT(Var2, fComp) );
            RetValue = Sat_SolverAddClause( pSat, vProj );
            assert( RetValue );
        }
        // check if we learned the equivalence
        if ( RetValue1 == SAT_FALSE && RetValue2 == SAT_FALSE )
            p->ppClasses[k++] = p->ppClasses[c];
        else if ( RetValue1 != SAT_FALSE && RetValue2 == SAT_FALSE )
            nFalse01++;
        else if ( RetValue1 == SAT_FALSE && RetValue2 != SAT_FALSE )
            nFalse10++;
        else if ( RetValue1 != SAT_FALSE && RetValue2 != SAT_FALSE )
            nFalse00++;
        if ( c % 20 == 0 )
            Extra_ProgressBarUpdate( pProgress, c, NULL );
    }
    Extra_ProgressBarStop( pProgress );
    p->nClasses = k;
    Sat_IntVecFree( vProj );
//printf( "Proof = %5d. Impl1 = %5d. Impl2 = %5d. None = %5d.\n", 
//       p->nClasses, nFalse01, nFalse10, nFalse00 );
    return pSat;
}

*/

/**Function*************************************************************

  Synopsis    [Prove or disprove the equivalence classes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Vec_NetworkAddClauses( Sat_Solver_t * pSat, Sh_Network_t * pNet )
{
    Sat_IntVec_t * vLits;
    Sh_Node_t * pShNode, * pShNode1, * pShNode2;
    Sh_Node_t ** ppShNodes;
    int nNodesInt, fComp1, fComp2, Var, Var1, Var2;
    int RetValue, Counter, i;

    // get the internal AND array
    ppShNodes = Sh_NetworkReadNodes( pNet );
    nNodesInt = Sh_NetworkReadNodeNum( pNet );
    // assign numbers to the AND-gates
//    Sh_NetworkSetNumbers( pNet );
    Counter = 0;
    for ( i = 0; i < pNet->nInputs; i++ )
        pNet->pMan->pVars[i]->pData = Counter++;
    for ( i = 0; i < nNodesInt; i++ )
        ppShNodes[i]->pData = Counter++;
    assert( Counter == Sat_SolverReadVarNum(pSat) );

    // add the clauses
    vLits = Sat_IntVecAlloc( 10 );
    for ( i = 0; i < nNodesInt; i++ )
    {
        pShNode = ppShNodes[i];
        assert( !Sh_NodeIsVar(pShNode) );

        pShNode1 = Sh_NodeReadOne(pShNode);
        pShNode2 = Sh_NodeReadTwo(pShNode);

        fComp1 = Sh_IsComplement( pShNode1 );
        fComp2 = Sh_IsComplement( pShNode2 );

        Var = Sh_NodeReadData( pShNode );// + 1;
        Var1 = Sh_NodeReadData( Sh_Regular( pShNode1 ) );// + 1;
        Var2 = Sh_NodeReadData( Sh_Regular( pShNode2 ) );// + 1;

        // suppose the AND-gate is A * B = C
        // write !A => !C   or   A + !C
//        fprintf( pFile, "%d %d 0%c", Var1, -Var, 10 );
        Sat_IntVecClear( vLits );
        Sat_IntVecPush( vLits, SAT_VAR2LIT(Var1, fComp1) );
        Sat_IntVecPush( vLits, SAT_VAR2LIT(Var,  1) );
        RetValue = Sat_SolverAddClause( pSat, vLits );
        assert( RetValue );

        // write !B => !C   or   B + !C
//        fprintf( pFile, "%d %d 0%c", Var2, -Var, 10 );
        Sat_IntVecClear( vLits );
        Sat_IntVecPush( vLits, SAT_VAR2LIT(Var2, fComp2) );
        Sat_IntVecPush( vLits, SAT_VAR2LIT(Var,  1) );
        RetValue = Sat_SolverAddClause( pSat, vLits );
        assert( RetValue );

        // write A & B => C   or   !A + !B + C
//        fprintf( pFile, "%d %d %d 0%c", -Var1, -Var2, Var, 10 );
        Sat_IntVecClear( vLits );
        Sat_IntVecPush( vLits, SAT_VAR2LIT(Var1, !fComp1) );
        Sat_IntVecPush( vLits, SAT_VAR2LIT(Var2, !fComp2) );
        Sat_IntVecPush( vLits, SAT_VAR2LIT(Var, 0) );
        RetValue = Sat_SolverAddClause( pSat, vLits );
        assert( RetValue );
    }
    Sat_IntVecFree( vLits );
}

/**Function*************************************************************

  Synopsis    [Add the assertion that the of the network is always one.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ver_NetworkAddOutputOne( Sat_Solver_t * pSat, Sh_Network_t * pShNet )
{
    Sat_IntVec_t * vLits;
    Sh_Node_t ** ppShOutputs;
    int nOutputs, RetValue, Var1, fComp1;

    // get the outputs
    ppShOutputs = Sh_NetworkReadOutputs( pShNet );
    nOutputs    = Sh_NetworkReadOutputNum( pShNet );
    assert( nOutputs == 1 );
    // check the last output
    fComp1 = Sh_IsComplement( ppShOutputs[0] );
    Var1   = Sh_NodeReadData( Sh_Regular(ppShOutputs[0]) );

    // add the last clause
    vLits = Sat_IntVecAlloc( 10 );
    Sat_IntVecPush( vLits, SAT_VAR2LIT(Var1, fComp1) );
    RetValue = Sat_SolverAddClause( pSat, vLits );
    assert( RetValue );
    Sat_IntVecFree( vLits );
}

/**Function*************************************************************

  Synopsis    [Prove or disprove the equivalence classes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sat_Solver_t * Ver_NetworkProveClasses( Ver_Manager_t * p, int iSplitLimit )
{
    Sh_Node_t * pRest;
    Sat_Solver_t * pSat;
    ProgressBar * pProgress;
    int i;

    pProgress = Extra_ProgressBarStart( stdout, 100 );

    // clean the solver
    pSat = Sat_SolverAlloc( p->pShNet->nNodes + p->pShMan->nVars, 1, 1, 1, 1, 0 );
    Sat_SolverClean( pSat, p->pShNet->nNodes + p->pShMan->nVars );

    // add the clauses for the AI-graph
    Vec_NetworkAddClauses( pSat, p->pShNet );
    //Sat_SolverWriteDimacs( pSat, "start.cnf" );

    i = 0;
    while ( i < p->nClasses )
    {
        assert ( p->ppClasses[i]->pOrder );

        Ver_NetworkSplitClass ( pSat, p->ppClasses[i], &pRest, iSplitLimit );

        if (( pRest != 0 ) && ( pRest->pOrder != 0 ))
        {
            p->ppClasses[ p->nClasses++ ] = pRest;
            fflush( stdout );
        }

        assert ( p->nClasses <= p->nClassesAlloc );

        if ( i % 20 == 0 )
            Extra_ProgressBarUpdate( pProgress, (int) (i * 100.0 / p->nClasses) , NULL );

        i++;
    }

    Extra_ProgressBarStop( pProgress );

    return pSat;
}

/**Function*************************************************************

  Synopsis    [Splits the candidate class pCand into two parts; the first part
               is the list of all nodes SAT-equivalent to pCand, and the rest which is
               returned in pRest; pRest may be 0]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ver_NetworkSplitClass( Sat_Solver_t * pSat, Sh_Node_t * pCand, Sh_Node_t ** ppRest, int iSplitLimit )
{
    Sh_Node_t * pEquiv[1000];
    Sh_Node_t * pNotEq[1000];
    Sh_Node_t * pTemp;
    int i = 0, j = 0, x;
    int count = 0;

    assert ( pCand->pOrder != 0 );

    pEquiv[i] = 0;
    pNotEq[j] = 0;

    for ( pTemp = pCand->pOrder; pTemp; pTemp = pTemp->pOrder )
    {
        if ( Ver_NetworkProveNodes( pSat, pCand, pTemp ) )
        {
            pEquiv[i++] = pTemp;
            assert ( i < 1000 );
            pEquiv[i] = 0;
        }
        else
        {
            pNotEq[j++] = pTemp;
            assert ( j < 1000 );
            pNotEq[j] = 0;
        }
        
        if ( (iSplitLimit >= 0) && (count++ > iSplitLimit) )
            break;
    }

    // Make them into lists
    
    for ( x = 0; x < i; x++ )
    {
        pEquiv[x]->pOrder = pEquiv[x + 1];
    }

    for ( x = 0; x < j; x++ )
    {
        pNotEq[x]->pOrder = pNotEq[x + 1];
    }

    *ppRest       = pNotEq[0];
    pCand->pOrder = pEquiv[0];
}

/**Function*************************************************************

  Synopsis    [Prove or disprove the equivalence of two nodes; returns 1 
               if they are equiv, 0 otherwise]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ver_NetworkProveNodes( Sat_Solver_t * pSat, Sh_Node_t * one, Sh_Node_t * two )
{
    int fComp1, fComp2, fComp, Var1, Var2;
    int RetValue, RetValue1, RetValue2;
    Sat_IntVec_t * vProj;

    vProj = Sat_IntVecAlloc( 10 );

    // get the first two vars
    Var1 = one->pData;
    Var2 = two->pData; 
    fComp1 = one->fMark;
    fComp2 = two->fMark;
    fComp  = ( fComp1 != fComp2 ); // they have different phase

    // A = 0; B = 1     OR     A = 0; B = 0 
    Sat_IntVecClear( vProj );
    Sat_IntVecPush( vProj, SAT_VAR2LIT(Var1, 1) );
    Sat_IntVecPush( vProj, SAT_VAR2LIT(Var2, fComp) );
    // solve under assumptions
    RetValue1 = Sat_SolverSolve( pSat, vProj, NULL, -1 );
    if ( RetValue1 == SAT_FALSE )
    {
        // add the clause
        Sat_IntVecClear( vProj );
        Sat_IntVecPush( vProj, SAT_VAR2LIT(Var1, 0) );
        Sat_IntVecPush( vProj, SAT_VAR2LIT(Var2, !fComp) );
        RetValue = Sat_SolverAddClause( pSat, vProj );
        assert( RetValue );
    }

    // A = 1; B = 0     OR     A = 1; B = 1 
    Sat_IntVecClear( vProj );
    Sat_IntVecPush( vProj, SAT_VAR2LIT(Var1, 0) );
    Sat_IntVecPush( vProj, SAT_VAR2LIT(Var2, !fComp) );
    // solve under assumptions
    RetValue2 = Sat_SolverSolve( pSat, vProj, NULL, -1 );
    if ( RetValue2 == SAT_FALSE )
    {
        // add the clause
        Sat_IntVecClear( vProj );
        Sat_IntVecPush( vProj, SAT_VAR2LIT(Var1, 1) );
        Sat_IntVecPush( vProj, SAT_VAR2LIT(Var2, fComp) );
        RetValue = Sat_SolverAddClause( pSat, vProj );
        assert( RetValue );
    }

    Sat_IntVecFree( vProj );

    return RetValue1 == SAT_FALSE && RetValue2 == SAT_FALSE;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


