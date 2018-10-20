/**CFile****************************************************************

  FileName    [satSolverProof.c]

  PackageName [A C version of SAT solver MINISAT, originally developed 
  in C++ by Niklas Een and Niklas Sorensson, Chalmers University of 
  Technology, Sweden: http://www.cs.chalmers.se/~een/Satzoo.]

  Synopsis    [Generates the unsatisfiability proofs.]

  Author      [Alan Mishchenko <alanmi@eecs.berkeley.edu>]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - January 1, 2004.]

  Revision    [$Id: satSolverProof.c,v 1.1 2005/07/08 01:01:29 alanmi Exp $]

***********************************************************************/

#include "satInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// data structure to store one node of the resolution DAG
struct Sat_Resol_t_
{
    Sat_Resol_t *       pR1;         // the father
    Sat_Resol_t *       pR2;         // the mother
    int                 Var;         // the pivot
    unsigned            fC1  :  1;   // indicates that the father is a clause
    unsigned            fC2  :  1;   // indicates that the mother is a clause
    unsigned            fMark:  1;   // flag marking visit to the given node
    unsigned            Num  : 29;   // the number of this node
    unsigned            uTruth[2];   // the truth table
    Sat_Clause_t *      pC;          // storage for the learned clause
};

static Sat_Resol_t * Sat_SolverDeriveClauseProof_rec( Sat_Solver_t * p, Sat_Clause_t * pC, int LitUIP );
static Sat_Resol_t * Sat_SolverAllocProofNode( Sat_Solver_t * p, Sat_Resol_t * pR1, Sat_Clause_t * pC1, Sat_Resol_t * pR2, Sat_Clause_t * pC2, int Var );
static void Sat_SolverCollect0LevelLits( Sat_Solver_t * p, Sat_Clause_t * pC );
static void Sat_SolverCollectProof_rec( Sat_Solver_t * p, Sat_Resol_t * pNode, 
    Sat_ClauseVec_t * vProof, int * pClausesCore, int * pClausesLearned );
static unsigned Sat_SolverCollectProofTruth_rec( Sat_Solver_t * p, Sat_Resol_t * pNode, Sat_ClauseVec_t * vProof );

static void Extra_PrintBinary( FILE * pFile, unsigned Sign[], int nBits );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Access to the resolution node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sat_Resol_t *  Sat_ResolReadR1( Sat_Resol_t * pR )              {  return pR->pR1;  }
Sat_Resol_t *  Sat_ResolReadR2( Sat_Resol_t * pR )              {  return pR->pR2;  }
bool           Sat_ResolReadF1( Sat_Resol_t * pR )              {  return pR->fC1; }
bool           Sat_ResolReadF2( Sat_Resol_t * pR )              {  return pR->fC2; }
int            Sat_ResolReadVar( Sat_Resol_t * pR )             {  return pR->Var;  }
int            Sat_ResolReadNum( Sat_Resol_t * pR )             {  return pR->Num;  }
unsigned       Sat_ResolReadTruth( Sat_Resol_t * pR )           {  return pR->uTruth[0]; }

/**Function*************************************************************

  Synopsis    [Setting the resolution node numbers.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sat_ResolSetNum( Sat_Resol_t * pR, int Num )
{
    pR->Num = Num;
}


/**Function*************************************************************

  Synopsis    [Derives the resolution proof for the learned clause.]

  Description [The new clause has been just learned. Its first literal
  contains the 1-UIP of the current decision level]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sat_Resol_t * Sat_SolverDeriveClauseProof( Sat_Solver_t * p, Sat_Clause_t * pC, int LitUIP )
{
    Sat_Resol_t * pRes, * pRes0;
    Sat_Var_t Var;
    Sat_Clause_t ** ppClauses;
    int nClauses, i;

printf( "Adding proof:  " );
Sat_ClauseWriteDimacs( stdout, pC, 1 );


    // collect the nodes of the implication graph in p->vTemp0
    // only the nodes on the reason side are collected
    Sat_ClauseVecClear( p->vTemp0 );
    Sat_ClauseVecClear( p->vTemp1 );
    pRes = Sat_SolverDeriveClauseProof_rec( p, pC, LitUIP );

    // unmark them
    nClauses  = Sat_ClauseVecReadSize( p->vTemp0 );
    ppClauses = Sat_ClauseVecReadArray( p->vTemp0 );
    for ( i = 0; i < nClauses; i++ )
    {
        assert( Sat_ClauseReadMark( ppClauses[i] ) );
        Sat_ClauseSetMark( ppClauses[i], 0 );
        // clean the temporary resolvent
        Sat_SolverSetResolTemp( p, ppClauses[i], NULL );
    }

    // add zero-level literals
    nClauses = Sat_ClauseVecReadSize( p->vTemp1 );
    if ( nClauses )
    {
        ppClauses = Sat_ClauseVecReadArray( p->vTemp1 );
        for ( i = 0; i < nClauses; i++ )
        {
            pRes0 = Sat_SolverReadResol( p, ppClauses[i] );
            assert( pRes0 );
            Var  = SAT_LIT2VAR( *Sat_ClauseReadLits( ppClauses[i] ) );
            pRes =  Sat_SolverAllocProofNode( p, pRes, NULL, pRes0, NULL, Var );
        }
        // unmark them
        for ( i = 0; i < nClauses; i++ )
        {
            assert( Sat_ClauseReadMark( ppClauses[i] ) );
            Sat_ClauseSetMark( ppClauses[i], 0 );
        }
    }
    return pRes;
}

/**Function*************************************************************

  Synopsis    [Collects nodes of the implication graph in the DFS order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sat_Resol_t * Sat_SolverDeriveClauseProof_rec( Sat_Solver_t * p, Sat_Clause_t * pC, int LitUIP )
{
    Sat_Resol_t * pR1, * pR = NULL;
    Sat_Resol_t * pRside, * pRdir;
    Sat_Clause_t * pCside, * pCdir;
    Sat_Clause_t * pCReason;
    int nLevelCur, nLits, * pLits, i;
    Sat_Var_t Var;

    // if this clause is visited, return its temporary resolvent
    if ( Sat_ClauseReadMark( pC ) )
        return Sat_SolverReadResolTemp( p, pC );
    // mark this clause as visited
    Sat_ClauseSetMark( pC, 1 );
    Sat_ClauseVecPush( p->vTemp0, pC );
//Sat_ClauseWriteDimacs( stdout, pC, 0 );

    // go through the literals of the clause
    nLevelCur = Sat_IntVecReadSize(p->vTrailLim);
    nLits = Sat_ClauseReadSize( pC );
    pLits = Sat_ClauseReadLits( pC );
    for ( i = 0; i < nLits; i++ )
    {
        // get the reason clause
        Var = SAT_LIT2VAR(pLits[i]);
        pCReason = p->pReasons[Var];
        // skip the implied literal
        if ( pCReason == pC )
        {
//            assert( Sat_ClauseReadNum(pCReason) < p->nClauses );
            continue;
        }
        // skip the decision literal
        if ( pCReason == NULL )
            continue;
//        assert( Sat_ClauseReadNum(pCReason) < p->nClauses );

        // skip the node from the other decision levels
        // do not skip the node from Level 0, which has a reason
        // this node is derived by propagating 0-level unit clauses
        // (instead of this hack, procedure Sat_SolverCollect0LevelLits() can be used)
        if ( p->pLevel[Var] < nLevelCur && !(p->pLevel[Var] == 0 && pCReason) )
            continue;
        // skip the UIP
        if ( pLits[i] == LitUIP )
            continue;

        // call recursively for this clause and add it in the end
        pR1 = Sat_SolverDeriveClauseProof_rec( p, pCReason, LitUIP );

        // if no resolution was performed, use the clause itself
        // otherwise, get its permanent resolvent or the clause itself
        if ( pR1 )
        {
            pRside = pR1;
            pCside = NULL;
        }
        else if ( Sat_ClauseReadLearned(pCReason) )
        {
            pRside = Sat_SolverReadResol( p, pCReason );
            pCside = NULL;
            Sat_SolverCollect0LevelLits( p, pCReason );
        }
        else 
        {
            pRside = NULL;
            pCside = pCReason;
            Sat_SolverCollect0LevelLits( p, pCReason );
//printf( "Resolve with " ); 
//Sat_ClauseWriteDimacs( stdout, pCside, 0 );
        }
        // set the current clause reason
        if ( pR )
        {
            pRdir = pR;
            pCdir = NULL;
        }
        else if ( Sat_ClauseReadLearned(pC) )
        {
            pRdir = Sat_SolverReadResol( p, pC );
            pCdir = NULL;
            Sat_SolverCollect0LevelLits( p, pC );
        }
        else
        {
            pRdir = NULL;
            pCdir = pC;
            Sat_SolverCollect0LevelLits( p, pC );
//printf( "Resolve with " ); 
//Sat_ClauseWriteDimacs( stdout, pCdir, 0 );
        }
//printf( "variable = %d\n\n", Var );
        // create a new resolution node
        pR = Sat_SolverAllocProofNode( p, pRdir, pCdir, pRside, pCside, Var );
    }
    // set the temporary resolvent of this clause
    Sat_SolverSetResolTemp( p, pC, pR );
    return pR;
}

/**Function*************************************************************

  Synopsis    [Allocate one resolution node.]

  Description [This procedure collects unmarked 0-level 1-lit clauses]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sat_SolverCollect0LevelLits( Sat_Solver_t * p, Sat_Clause_t * pC )
{
    int nLits, * pLits, i;
    Sat_Var_t Var;
    nLits = Sat_ClauseReadSize( pC );
    pLits = Sat_ClauseReadLits( pC );
    for ( i = 0; i < nLits; i++ )
    {
        Var = SAT_LIT2VAR( pLits[i] );
//        if ( Var == 16 )
//        {
//            int i = 0;
//        }
        if ( p->pLevel[Var] != 0 )
            continue;

        if ( p->pResols[Var] == NULL )
            continue;
        if ( Sat_ClauseReadMark( p->pResols[Var] ) )
            continue;
        // mark this clause as visited
        Sat_ClauseSetMark( p->pResols[Var], 1 );
        Sat_ClauseVecPush( p->vTemp1, p->pResols[Var] );
    }
}

/**Function*************************************************************

  Synopsis    [Find resolution for the 0-level implication.]

  Description [Given one clause, this procedure finds the resolution 
  on the 0 level.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sat_Resol_t * Sat_SolverResolveOneClause( Sat_Solver_t * p, Sat_Clause_t * pC, int Lit )
{
    Sat_Resol_t * pRes = NULL;
    Sat_Resol_t * p0Res, * pRdir;
    Sat_Clause_t * p0Lev, * pCdir;
    int nLits, * pLits, i;
    Sat_Var_t Var;

    // check that the decision level is indeed 0
    assert( Sat_SolverReadDecisionLevel(p) == 0 );
    // make sure that all the literals in the clause, except "Lit"
    // have resolution pointer on the 0 level
    nLits = Sat_ClauseReadSize( pC );
    pLits = Sat_ClauseReadLits( pC );
    for ( i = 0; i < nLits; i++ )
    {
        if ( pLits[i] == Lit )
            continue;
        // get the 0-level clause for this literal
        Var = SAT_LIT2VAR(pLits[i]);
        p0Lev = p->pResols[Var];
        assert( p0Lev );
        // get the resolvent of this clause
        p0Res = Sat_SolverReadResol( p, p0Lev );
        if ( pRes )
        {
            pRdir = pRes;
            pCdir = NULL;
        }
        else if ( Sat_ClauseReadLearned(pC) )
        {
            pRdir = Sat_SolverReadResol( p, pC );
            pCdir = NULL;
        }
        else
        {
            pRdir = NULL;
            pCdir = pC;
        }
        pRes = Sat_SolverAllocProofNode( p, pRdir, pCdir, p0Res, NULL, Var );
    }
    return pRes;
}

/**Function*************************************************************

  Synopsis    [Allocate one resolution node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sat_Resol_t * Sat_SolverAllocProofNode( Sat_Solver_t * p, 
    Sat_Resol_t * pR1, Sat_Clause_t * pC1, Sat_Resol_t * pR2, Sat_Clause_t * pC2, int Var )
{
    Sat_Resol_t * pR;
    // create the resolution node
#ifdef USE_SYSTEM_MEMORY_MANAGEMENT
    pR = (Sat_Resol_t *)ALLOC( char, sizeof(Sat_Resol_t) );
#else
//    pR = (Sat_Resol_t *)Sat_MmStepEntryFetch( Sat_SolverReadMem(p), sizeof(Sat_Resol_t) );
    if ( p->pMemProof == NULL )
        p->pMemProof = Sat_MmFixedStart( sizeof(Sat_Resol_t), 10000, 100 );
    pR = (Sat_Resol_t *)Sat_MmFixedEntryFetch( p->pMemProof );
#endif
    pR->Var = Var;
    pR->pC = NULL;
    pR->Num = 999;
    pR->fMark = 0;
    // set the mother
    assert( !pR1 ^ !pC1 );
    if ( pR1 )
    {
        pR->pR1 = pR1;
        pR->fC1 = 0;
    }
    else
    {
        pR->pR1 = (Sat_Resol_t *)pC1;
        pR->fC1 = 1;
    }
    // set the father
    assert( !pR2 ^ !pC2 );
    if ( pR2 )
    {
        pR->pR2 = pR2;
        pR->fC2 = 0;
    }
    else
    {
        pR->pR2 = (Sat_Resol_t *)pC2;
        pR->fC2 = 1;
    }
    return pR;
}



/**Function*************************************************************

  Synopsis    [Frees the resolution tree.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sat_SolverFreeResolTree_rec( Sat_Solver_t * p, Sat_Resol_t * pR )
{
//    if ( !pR->fC1 && !pR->pR1->pC )
//        Sat_SolverFreeResolTree_rec( p, pR->pR1 );
//    if ( !pR->fC2 && !pR->pR2->pC )
//        Sat_SolverFreeResolTree_rec( p, pR->pR2 );
/*
#ifdef USE_SYSTEM_MEMORY_MANAGEMENT
    free( pR );
#else
    Sat_MmStepEntryRecycle( Sat_SolverReadMem(p), (char *)pR, sizeof(Sat_Resol_t) );
#endif
*/
}

/**Function*************************************************************

  Synopsis    [Frees the resolution tree.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sat_SolverSetResolClause( Sat_Resol_t * pR, Sat_Clause_t * pC )
{
    pR->pC = pC;
}



/**Function*************************************************************

  Synopsis    [Write the current proof into a file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sat_ClauseVec_t * Sat_SolverCollectProof( Sat_Solver_t * p )
{
    Sat_ClauseVec_t * vProof;
    Sat_Clause_t ** pClauses;
    int nClausesCore    = 0;
    int nClausesLearned = 0;
    int nClauses, i;

    // clean mark in the problem clauses
    nClauses = Sat_ClauseVecReadSize( p->vClauses );
    pClauses = Sat_ClauseVecReadArray( p->vClauses );
    // start the array of clause numbers
    for ( i = 0; i < nClauses; i++ )
        Sat_ClauseSetMark( pClauses[i], 0 );

    vProof = Sat_ClauseVecAlloc( 1000 );
    Sat_SolverCollectProof_rec( p, p->pRoot, vProof, &nClausesCore, &nClausesLearned );

    printf( "Original: Clauses = %5d.  Learned = %5d. \n", 
        Sat_ClauseVecReadSize( p->vClauses ),
        Sat_ClauseVecReadSize( p->vLearned ) );
    printf( "Proof:    Clauses = %5d.  Learned = %5d.  Resolv = %5d.\n", 
        nClausesCore, nClausesLearned, Sat_ClauseVecReadSize(vProof) );
    return vProof;
}

/**Function*************************************************************

  Synopsis    [Write the proof at the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sat_SolverCollectProof_rec( Sat_Solver_t * p, Sat_Resol_t * pNode, 
    Sat_ClauseVec_t * vProof, int * pClausesCore, int * pClausesLearned )
{
    Sat_Clause_t * pClause;

    if ( pNode->fC1 )
    {
        pClause = (Sat_Clause_t *)pNode->pR1;
        if ( !Sat_ClauseReadMark(pClause) )
        {
            Sat_ClauseSetMark( pClause, 1 );
            (*pClausesCore)++;
        }
    }
    else 
        Sat_SolverCollectProof_rec( p, pNode->pR1, vProof, pClausesCore, pClausesLearned );

    if ( pNode->fC2 )
    {
        pClause = (Sat_Clause_t *)pNode->pR2;
        if ( !Sat_ClauseReadMark(pClause) )
        {
            Sat_ClauseSetMark( pClause, 1 );
            (*pClausesCore)++;
        }
    }
    else
        Sat_SolverCollectProof_rec( p, pNode->pR2, vProof, pClausesCore, pClausesLearned );

    if ( pNode->pC && Sat_ClauseReadSize(pNode->pC) > 1 )
        (*pClausesLearned)++;

    Sat_ClauseVecPush( vProof, (Sat_Clause_t *)pNode );
}




/**Function*************************************************************

  Synopsis    [Write the current proof into a file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned Sat_SolverCollectProofTruth( Sat_Solver_t * p )
{
    Sat_ClauseVec_t * vProof;
    Sat_Clause_t ** pClauses;
    Sat_Resol_t * pNode;
    int nClauses, i;
    unsigned uTruth;

 
    // collect the proof into the array and compute the truth table
    vProof = Sat_ClauseVecAlloc( 1000 );
    uTruth = Sat_SolverCollectProofTruth_rec( p, p->pRoot, vProof );

    // clean the proof
    nClauses = Sat_ClauseVecReadSize( vProof );
    pClauses = Sat_ClauseVecReadArray( vProof );
    for ( i = 0; i < nClauses; i++ )
    {
        pNode = (Sat_Resol_t *)pClauses[i];
        pNode->fMark = 0;
    }
    Sat_ClauseVecFree( vProof );

    return uTruth;
}

/**Function*************************************************************

  Synopsis    [Write the proof at the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned Sat_SolverCollectProofTruth_rec( Sat_Solver_t * p, Sat_Resol_t * pNode, Sat_ClauseVec_t * vProof )
{
    unsigned uTruth1, uTruth2;

    if ( pNode->fMark ) 
        return pNode->uTruth[0];

    if ( pNode->fC1 )
        uTruth1 = Sat_ClauseComputeTruth( p, (Sat_Clause_t *)pNode->pR1 );
    else 
        uTruth1 = Sat_SolverCollectProofTruth_rec( p, pNode->pR1, vProof );

    if ( pNode->fC2 )
        uTruth2 = Sat_ClauseComputeTruth( p, (Sat_Clause_t *)pNode->pR2 );
    else
        uTruth2 = Sat_SolverCollectProofTruth_rec( p, pNode->pR2, vProof );

    // compute the truth table of the node
    if ( pNode->Var < Sat_IntVecReadSize(p->vVarMap) && 
         Sat_IntVecReadEntry(p->vVarMap, pNode->Var) == -1 )  // local variable of A
         pNode->uTruth[0] = uTruth1 | uTruth2;
    else
         pNode->uTruth[0] = uTruth1 & uTruth2;

    assert( pNode->fMark == 0 );
    pNode->fMark = 1;

    Sat_ClauseVecPush( vProof, (Sat_Clause_t *)pNode );
    return pNode->uTruth[0];
}




/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sat_SolverPrintProof( Sat_Solver_t * p, Sat_ClauseVec_t * pProof, char * pFileName )
{
    FILE * pFile;
    Sat_Clause_t ** pClauses;
    Sat_Resol_t ** pResols;
    Sat_Resol_t * pR;
    Sat_Clause_t * pC;
    int nClauses, nResols, i;
    int Num1, Num2, Num, Counter;
    int * pNumbers, ClauseNum;
    unsigned uTruth, uTruth1, uTruth2;
    int fPrint = 1;

    // clean mark in the problem clauses
    nClauses = Sat_ClauseVecReadSize( p->vClauses );
    pClauses = Sat_ClauseVecReadArray( p->vClauses );
    // start the array of clause numbers
    pNumbers = ALLOC( int, nClauses );
    for ( i = 0; i < nClauses; i++ )
    {
        Sat_ClauseSetMark( pClauses[i], 0 );
        pNumbers[i] = -1;
    }

    // go through the resolution nodes and print them
    pFile = fopen( pFileName, "w" );
    Counter = 0;
    nResols = Sat_ClauseVecReadSize( pProof );
    pResols = (Sat_Resol_t **)Sat_ClauseVecReadArray( pProof );
    for ( i = 0; i < nResols; i++ )
    {
        pR = pResols[i];
        // get the number of the first clause
        if ( pR->fC1 )
        {
            // get the clause and its number
            pC = (Sat_Clause_t *)pR->pR1;
            ClauseNum = Sat_ClauseReadNum(pC);
            assert( !Sat_ClauseReadLearned(pC) );
            // get the number of the clause line, or print the clause
            if ( Sat_ClauseReadMark( pC ) )
            {
                Num1 = pNumbers[ClauseNum];
                assert( Num1 != -1 );
            }
            else
            {
                Num1 = Counter++;
                // print the clause
                fprintf( pFile, "%5d  =  ", Num1 );
                Sat_ClauseWriteDimacs( pFile, pC, 1 );
                // set the number of this clause
                assert( pNumbers[ClauseNum] == -1 );
                pNumbers[ClauseNum] = Num1;
                // mark the clause
                Sat_ClauseSetMark( pC, 1 );
            }


            if ( fPrint )
            {
                uTruth1 = Sat_ClauseComputeTruth( p, pC );
                Extra_PrintBinary( pFile, &uTruth1, 8 ); fprintf( pFile, "\n" ); fflush( pFile );
            }
        }
        else
        {
            Num1 = pR->pR1->Num;


            if ( fPrint )
                uTruth1 = pR->pR1->uTruth[0];
        }
        // get the number of the second clause
        if ( pR->fC2 )
        {
            // get the clause and its number
            pC = (Sat_Clause_t *)pR->pR2;
            ClauseNum = Sat_ClauseReadNum(pC);
            assert( !Sat_ClauseReadLearned(pC) );
            // get the number of the clause line, or print the clause
            if ( Sat_ClauseReadMark( pC ) )
            {
                Num2 = pNumbers[ClauseNum];
                assert( Num2 != -1 );
            }
            else
            {
                Num2 = Counter++;
                // print the clause
                fprintf( pFile, "%5d  =  ", Num2 );
                Sat_ClauseWriteDimacs( pFile, pC, 1 );
                // set the number of this clause
                assert( pNumbers[ClauseNum] == -1 );
                pNumbers[ClauseNum] = Num2;
                // mark the clause
                Sat_ClauseSetMark( pC, 1 );
            }


            if ( fPrint )
            {
                uTruth2 = Sat_ClauseComputeTruth( p, pC );
                Extra_PrintBinary( pFile, &uTruth2, 8 ); fprintf( pFile, "\n" ); fflush( pFile );
            }
        }
        else
        {
            Num2 = pR->pR2->Num;


            if ( fPrint )
                uTruth2 = pR->pR2->uTruth[0];
        }
        // print the given node
        Num = Counter++;
        if ( pR->pC == NULL )
            fprintf( pFile, "%5d  :  %d %d  %d\n", Num, Num1, Num2, pR->Var+1 );
        else
        {
            fprintf( pFile, "%5d  :  %d %d  %d   ", Num, Num1, Num2, pR->Var+1 );
            Sat_ClauseWriteDimacs( pFile, pR->pC, 1 );
        }
        pR->Num = Num;

      if ( fPrint )
      {
        // compute the truth table of the node
        if ( pR->Var < Sat_IntVecReadSize(p->vVarMap) && 
             Sat_IntVecReadEntry(p->vVarMap, pR->Var) == -1 )  // local variable of A
             uTruth = uTruth1 | uTruth2;
        else
             uTruth = uTruth1 & uTruth2;
//        assert( pR->uTruth[0] == uTruth );
        pR->uTruth[0] = uTruth;

        Extra_PrintBinary( pFile, &uTruth, 8 ); fprintf( pFile, "\n" ); fflush( pFile );
      }
    }
    free( pNumbers );
    fclose( pFile );
}



/**Function*************************************************************

  Synopsis    [Derive the resolvent of two clauses.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sat_IntVec_t * Sat_SolverVerifyResolvent( Sat_IntVec_t * vC1, Sat_IntVec_t * vC2, int iVar )
{
    Sat_IntVec_t * vC;
    int nEntries1, nEntries2;
    int * pEntries1, * pEntries2;
    int * pArray, nSize;
    int iVar1, iVar2, i, k;

    nEntries1 = Sat_IntVecReadSize( vC1 );
    pEntries1 = Sat_IntVecReadArray( vC1 );
    iVar1 = -1;
    for ( i = 0; i < nEntries1; i++ )
        if ( SAT_LIT2VAR(pEntries1[i]) == iVar )
        {
            iVar1 = i; 
            break;
        }

    nEntries2 = Sat_IntVecReadSize( vC2 );
    pEntries2 = Sat_IntVecReadArray( vC2 );
    iVar2 = -1;
    for ( i = 0; i < nEntries2; i++ )
        if ( SAT_LIT2VAR(pEntries2[i]) == iVar )
        {
            iVar2 = i;
            break;
        }

    if ( iVar1 == -1 )
    {
        printf( "Error1\n" );
        assert( 0 );
        return NULL;
    }
    if ( iVar2 == -1 )
    {
        printf( "Error2\n" );
        assert( 0 );
        return NULL;
    }
    if ( pEntries1[iVar1] == pEntries2[iVar2] )
    {
        printf( "Error3\n" );
        assert( 0 );
        return NULL;
    }

    // collect the entries
    pArray = ALLOC( int, nEntries1 + nEntries2 - 2 );
    nSize = 0;
    for ( i = 0; i < nEntries1; i++ )
        if ( i != iVar1 )
            pArray[nSize++] = pEntries1[i];
    for ( i = 0; i < nEntries2; i++ )
        if ( i != iVar2 )
            pArray[nSize++] = pEntries2[i];
    assert( nSize == nEntries1 + nEntries2 - 2 );

    // create the vector
    vC = Sat_IntVecAllocArray( pArray, nSize );

    // sort the literals
    Sat_IntVecSort( vC, 0 );
    nEntries1 = Sat_IntVecReadSize( vC );
    pEntries1 = Sat_IntVecReadArray( vC );

    // compact the identical entries
    if ( nEntries1 > 1 )
    {
        k = 1;
        for ( i = 1; i < nEntries1; i++ )
            if ( pEntries1[i-1] != pEntries1[i] )
                pEntries1[k++] = pEntries1[i];
        Sat_IntVecShrink( vC, k );
        nEntries1 = k;
    }

    // make sure there is no duplicated entries
    for ( i = 1; i < nEntries1; i++ )
        if ( SAT_LIT2VAR(pEntries1[i-1]) == SAT_LIT2VAR(pEntries1[i]) )
        {
            printf( "Error4\n" );
            assert( 0 );
            return NULL;
        }
    return vC;
}

/**Function*************************************************************

  Synopsis    [Verify the proof.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sat_SolverVerifyProof( char * pFileName, char * pFileNameTrace )
{
    FILE * pFileIn, * pFileOut;
    Sat_IntVec_t * vArray;
    Sat_IntVec_t * vClause, * vClause1, * vClause2;
    char * pToken, * Buffer;
    int * pLits;
    int nLits, nClauses, Temp;
    int iClause1, iClause2, iVar, i;
    int nEntries, * pEntries;

    // open the files
    pFileIn = fopen( pFileName, "r" );
    if ( pFileIn == NULL )
    {
        printf( "Cannot open the input file \"%s\" with the proof.\n", pFileName );
        return;
    }
    pFileOut = fopen( pFileNameTrace, "w" );
    if ( pFileOut == NULL )
    {
        fclose( pFileIn );
        printf( "Cannot open the output file \"%s\" for the trace.\n", pFileNameTrace );
        return;
    }

    Buffer = ALLOC( char, 10000 );
    pLits = ALLOC( int, 10000 );
    vArray = Sat_IntVecAlloc( 100 );
    nClauses = 0;
    while ( fgets( Buffer, 1999, pFileIn ) )
    {
        Buffer[ strlen(Buffer) - 1 ] = 0;
        fprintf( pFileOut, "%-30s ", Buffer );

        pToken = strtok( Buffer, " " );
        Temp   = atoi( pToken );
        assert( Temp == nClauses );
        // determine the type
        pToken = strtok( NULL, " " );
        if ( pToken[0] == ':' )
        {
            // read the clause numbers
            pToken = strtok( NULL, " " );
            iClause1 = atoi( pToken );
            pToken = strtok( NULL, " " );
            iClause2 = atoi( pToken );
            pToken = strtok( NULL, " " );
            iVar = atoi( pToken );
            // generate resolvent
            vClause1 = (Sat_IntVec_t *)Sat_IntVecReadEntry( vArray, iClause1 );
            vClause2 = (Sat_IntVec_t *)Sat_IntVecReadEntry( vArray, iClause2 );
            vClause = Sat_SolverVerifyResolvent( vClause1, vClause2, iVar-1 );
        }
        else
        {
            assert( pToken[0] == '=' );
            // save the literals
            nLits = 0;
            while ( pToken = strtok( NULL, " " ) )
            {
                Temp = atoi( pToken );
                if ( Temp == 0 )
                    break;
                if ( Temp < 0 )
                    pLits[nLits++] = SAT_VAR2LIT(-Temp-1, 1);
                else
                    pLits[nLits++] = SAT_VAR2LIT(Temp-1, 0);
            }
            // create the array of literals
            vClause = Sat_IntVecAllocArrayCopy( pLits, nLits );
        }
        // print the resulting clause
        nEntries = Sat_IntVecReadSize( vClause );
        pEntries = Sat_IntVecReadArray( vClause );
        fprintf( pFileOut, "%5d : ", nClauses );
        for ( i = 0; i < nEntries; i++ )
//            fprintf( pFileOut, " %d", pEntries[i] );
            fprintf( pFileOut, " %s%d", ((pEntries[i]&1)? "-": ""),  pEntries[i]/2 + 1 );
        fprintf( pFileOut, "\n" );

        Sat_IntVecPush( vArray, (int)vClause );
        nClauses++;
    }
    nEntries = Sat_IntVecReadSize( vClause );
    if ( nEntries == 0 )
    {
        printf( "Verification successful.\n" );
        fprintf( pFileOut, "Verification successful.\n" );
    }
    else
    {
        printf( "Verification failed.\n" );
        fprintf( pFileOut, "Verification failed.\n" );
    }
    fclose( pFileIn );
    fclose( pFileOut );

    nEntries = Sat_IntVecReadSize( vArray );
    pEntries = Sat_IntVecReadArray( vArray );
    for ( i = 0; i < nEntries; i++ )
        Sat_IntVecFree( (Sat_IntVec_t *)pEntries[i] );
    Sat_IntVecFree( vArray );
    FREE( Buffer );
    FREE( pLits );
}

/**Function*************************************************************

  Synopsis    [Prints the bit string.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_PrintBinary( FILE * pFile, unsigned Sign[], int nBits )
{
    int Remainder, nWords;
    int w, i;

    Remainder = (nBits%(sizeof(unsigned)*8));
    nWords    = (nBits/(sizeof(unsigned)*8)) + (Remainder>0);

    for ( w = nWords-1; w >= 0; w-- )
        for ( i = ((w == nWords-1 && Remainder)? Remainder-1: 31); i >= 0; i-- )
            fprintf( pFile, "%c", '0' + (int)((Sign[w] & (1<<i)) > 0) );

//  fprintf( pFile, "\n" );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


