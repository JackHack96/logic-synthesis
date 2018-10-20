/**CFile****************************************************************

  FileName    [aigSat.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [AND-INV graph package with built in sat_sweep.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 8, 2003.]

  Revision    [$Id: aigSat.c,v 1.3 2004/10/18 02:18:08 satrajit Exp $]

***********************************************************************/

#include "aigInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int  Aig_NodeCountPis( Sat_IntVec_t * vVars, int nVarsPi );
static int  Aig_NodeCountSuppVars( Aig_Man_t * p, Aig_Node_t * pNode );
static void Aig_NodesPrintSupps( Aig_Man_t * p, Aig_Node_t * p1, Aig_Node_t * p2 );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Add the clauses corresponding to one AND gate.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_NodeAddClauses( Aig_Man_t * p, Aig_Node_t * pNode, int fVerbose )
{
    Sat_IntVec_t * vLits = p->vProj;
    Aig_Node_t * pNode1, * pNode2;
    int fComp1, fComp2, RetValue, nVars, Var, Var1, Var2;

    assert( Aig_NodeIsAnd( pNode ) );
    nVars = Sat_SolverReadVarNum(p->pSat);

    // get the predecessor nodes
    pNode1 = Aig_NodeReadOne( pNode );
    pNode2 = Aig_NodeReadTwo( pNode );

    // get the complemented attributes of the nodes
    fComp1 = Aig_IsComplement( pNode1 );
    fComp2 = Aig_IsComplement( pNode2 );

    // determine the variable numbers
    Var = pNode->Num;
    Var1 = Aig_Regular(pNode1)->Num;
    Var2 = Aig_Regular(pNode2)->Num;
    // check that the variables are in the SAT manager
    assert( Var  < nVars ); 
    assert( Var1 < nVars );
    assert( Var2 < nVars );

    if ( fVerbose )
        printf( "   Clause %2d%s * %2d%s = %2d.\n", 
            Var1, fComp1? "\'": " ",   Var2, fComp2? "\'": " ", Var );

    // suppose the AND-gate is A * B = C
    // write !A => !C   or   A + !C
//  fprintf( pFile, "%d %d 0%c", Var1, -Var, 10 );
    Sat_IntVecClear( vLits );
    Sat_IntVecPush( vLits, SAT_VAR2LIT(Var1, fComp1) );
    Sat_IntVecPush( vLits, SAT_VAR2LIT(Var,  1) );
    RetValue = Sat_SolverAddClause( p->pSat, vLits );
    assert( RetValue );

    // write !B => !C   or   B + !C
//  fprintf( pFile, "%d %d 0%c", Var2, -Var, 10 );
    Sat_IntVecClear( vLits );
    Sat_IntVecPush( vLits, SAT_VAR2LIT(Var2, fComp2) );
    Sat_IntVecPush( vLits, SAT_VAR2LIT(Var,  1) );
    RetValue = Sat_SolverAddClause( p->pSat, vLits );
    assert( RetValue );

    // write A & B => C   or   !A + !B + C
//  fprintf( pFile, "%d %d %d 0%c", -Var1, -Var2, Var, 10 );
    Sat_IntVecClear( vLits );
    Sat_IntVecPush( vLits, SAT_VAR2LIT(Var1, !fComp1) );
    Sat_IntVecPush( vLits, SAT_VAR2LIT(Var2, !fComp2) );
    Sat_IntVecPush( vLits, SAT_VAR2LIT(Var, 0) );
    RetValue = Sat_SolverAddClause( p->pSat, vLits );
    assert( RetValue );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Checks whether two nodes are functinally equivalent.]

  Description [The flag (fComp) tells whether the nodes to be checked
  are in the opposite polarity. The second flag (fSkipZeros) tells whether
  the checking should be performed if the simulation vectors are zeros.
  Returns 1 if the nodes are equivalent; 0 othewise.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_NodeIsEquivalent( Aig_Man_t * p, Aig_Node_t * p1, Aig_Node_t * p2, int fComp, int fSkipZero, int fVerbose )
{
    Sat_IntVec_t * vProj = p->vProj;
    int RetValue, RetValue1, RetValue2;//, RetValue;
    int Var1, Var2;
    int clk;

    // make sure the nodes are not complemented
    assert( !Aig_IsComplement(p1) );
    assert( !Aig_IsComplement(p2) );

    // if the simulation vector looks like constant 0, do not check!
    // this saves a lot of runtime while degrading the results only very little
    if ( fSkipZero && p1->pSims->uTests[0] == 0 )
    {
        p->nSatZeros++;
        return 0; // return non-equivalent, although in fact we do not know...
    }
//    Sat_SolverPrintClauses( p->pSat );

    // collect the TFI cones of the two nodes (the numbers are in p->vVarsInt)
clk = clock();
    Aig_DfsForTwo( p, p1, p2 );
p->timeTrav += clock() - clk;
//printf( "(%3d)%3d ", Aig_NodeCountPis(p->vVarsInt, p->nInputs), Sat_IntVecReadSize(p->vVarsInt) );

    // get the first two variables
    Var1  = p1->Num;
    Var2  = p2->Num; 

    // A = 0; B = 1     OR     A = 0; B = 0 
    Sat_IntVecClear( vProj );
    Sat_IntVecPush( vProj, SAT_VAR2LIT(Var1, 1) );
    Sat_IntVecPush( vProj, SAT_VAR2LIT(Var2, fComp) );
    // solve under assumptions
    // prepare the solver to run incrementally on these variables
    Sat_SolverPrepare( p->pSat, p->vVarsInt );
    // run the solver
clk = clock();
    RetValue1 = Sat_SolverSolve( p->pSat, vProj, NULL, -1 );
p->timeSat += clock() - clk;
    if ( RetValue1 == SAT_FALSE )
    {
        // add the clause
        Sat_IntVecClear( vProj );
        Sat_IntVecPush( vProj, SAT_VAR2LIT(Var1, 0) );
        Sat_IntVecPush( vProj, SAT_VAR2LIT(Var2, !fComp) );
        RetValue = Sat_SolverAddClause( p->pSat, vProj );
        assert( RetValue );
        p->time1 += clock() - clk;
    }
    else
        p->time2 += clock() - clk;

    if ( RetValue1 != SAT_FALSE )
    {
        p->nSatCounter++;
        return 0;
    }

    // A = 1; B = 0     OR     A = 1; B = 1 
    Sat_IntVecClear( vProj );
    Sat_IntVecPush( vProj, SAT_VAR2LIT(Var1, 0) );
    Sat_IntVecPush( vProj, SAT_VAR2LIT(Var2, !fComp) );
    // solve under assumptions
    // prepare the solver to run incrementally on these variables
    Sat_SolverPrepare( p->pSat, p->vVarsInt );
    // run the solver
clk = clock();
    RetValue2 = Sat_SolverSolve( p->pSat, vProj, NULL, -1 );
p->timeSat += clock() - clk;
    if ( RetValue2 == SAT_FALSE )
    {
        // add the clause
        Sat_IntVecClear( vProj );
        Sat_IntVecPush( vProj, SAT_VAR2LIT(Var1, 1) );
        Sat_IntVecPush( vProj, SAT_VAR2LIT(Var2, fComp) );
        RetValue = Sat_SolverAddClause( p->pSat, vProj );
        assert( RetValue );
        p->time1 += clock() - clk;
    }
    else
        p->time2 += clock() - clk;

    RetValue = (RetValue1 == SAT_FALSE && RetValue2 == SAT_FALSE);
    if ( RetValue )
        p->nSatProof++;
    else
        p->nSatCounter++;
//printf( "%s\n", RetValue? "proof": "counter example" );
    return RetValue;
}


/**Function*************************************************************

  Synopsis    [Counts the number of PIs in the array of nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_NodeCountPis( Sat_IntVec_t * vVars, int nVarsPi )
{
    int i, Counter, Size, * pArray;
    Counter = 0;
    Size = Sat_IntVecReadSize(vVars);
    pArray = Sat_IntVecReadArray(vVars);
    for ( i = 0; i < Size; i++ )
        if ( pArray[i] <= nVarsPi )
            Counter++;
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Counts the number of support variables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_NodeCountSuppVars( Aig_Man_t * p, Aig_Node_t * pNode )
{
    int i, Count;
    Count = 0;
    for ( i = 0; i < p->nInputs; i++ )
        Count += Aig_NodeHasVar( pNode, i );
    return Count;
}

/**Function*************************************************************

  Synopsis    [Prints the supports of two nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_NodesPrintSupps( Aig_Man_t * p, Aig_Node_t * p1, Aig_Node_t * p2 )
{
    int i;

    printf( "%d & %d ", Aig_NodeCountSuppVars(p,p1), Aig_NodeCountSuppVars(p,p2) );

    for ( i = 0; i < p->nSuppWords; i++ )
        p1->pSupp[i] ^= p2->pSupp[i];

    printf( " = %d   ", Aig_NodeCountSuppVars(p,p1) );

    for ( i = 0; i < p->nSuppWords; i++ )
        p1->pSupp[i] ^= p2->pSupp[i];
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


