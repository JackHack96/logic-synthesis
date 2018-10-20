/**CFile****************************************************************

  FileName    [satClause.c]

  PackageName [A C version of SAT solver MINISAT, originally developed 
  in C++ by Niklas Een and Niklas Sorensson, Chalmers University of 
  Technology, Sweden: http://www.cs.chalmers.se/~een/Satzoo.]

  Synopsis    [Procedures working with SAT clauses.]

  Author      [Alan Mishchenko <alanmi@eecs.berkeley.edu>]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - January 1, 2004.]

  Revision    [$Id: satClause.c,v 1.1 2005/07/08 01:01:27 alanmi Exp $]

***********************************************************************/

#include "satInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

struct Sat_Clause_t_
{
    int           Num;              // unique number of the clause
    unsigned      fLearned   :  1;  // 1 if the clause is learned
    unsigned      fMark      :  1;  // used to mark visited clauses during proof recording
    unsigned      fTypeA     :  1;  // used to mark clauses belonging to A for interpolant computation
    unsigned      nSize      : 14;  // the number of literals in the clause
    unsigned      nSizeAlloc : 15;  // the number of bytes allocated for the clause
    Sat_Lit_t     pData[0];
};

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Creates a new clause.]

  Description [Returns FALSE if top-level conflict detected (must be handled); 
  TRUE otherwise. 'pClause_out' may be set to NULL if clause is already 
  satisfied by the top-level assignment.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Sat_ClauseCreate( Sat_Solver_t * p, Sat_IntVec_t * vLits, bool fLearned, Sat_Clause_t ** pClause_out )
{
    int * pAssigns = Sat_SolverReadAssignsArray(p);
    Sat_ClauseVec_t ** pvWatched;
    Sat_Clause_t * pC;
    int * pLits;
    int nLits, i, j;
    int nBytes;
    Sat_Var_t Var;
    bool Sign;

    *pClause_out = NULL;

    nLits = Sat_IntVecReadSize(vLits);
    pLits = Sat_IntVecReadArray(vLits);

    if ( !fLearned ) 
    {
        int * pSeen = Sat_SolverReadSeenArray( p );
        int nSeenId;
        assert( Sat_SolverReadDecisionLevel(p) == 0 );
        // sorting literals makes the code trace-equivalent 
        // with to the original C++ solver
        Sat_IntVecSort( vLits, 0 );
        // increment the counter of seen twice
        nSeenId = Sat_SolverIncrementSeenId( p );
        nSeenId = Sat_SolverIncrementSeenId( p );
        // nSeenId - 1 stands for negative
        // nSeenId     stands for positive
        // Remove false literals
        for ( i = j = 0; i < nLits; i++ ) {
            // get the corresponding variable
            Var  = SAT_LIT2VAR(pLits[i]);
            Sign = SAT_LITSIGN(pLits[i]); // Sign=0 for positive
            // check if we already saw this variable in the this clause
            if ( pSeen[Var] >= nSeenId - 1 )
            {
                if ( (pSeen[Var] != nSeenId) == Sign ) // the same lit
                    continue;
                return 1; // two opposite polarity lits -- don't add the clause
            }
            // mark the variable as seen
            pSeen[Var] = nSeenId - !Sign;

            // analize the value of this literal
            if ( pAssigns[Var] != SAT_VAR_UNASSIGNED )
            {
                if ( pAssigns[Var] == pLits[i] )
                    return 1;  // the clause is always true -- don't add anything
                // the literal has no impact - skip it
                continue;
            }
            // otherwise, add this literal to the clause
            pLits[j++] = pLits[i];
        }
        Sat_IntVecShrink( vLits, j );
        nLits = j;
    }
    // 'vLits' is now the (possibly) reduced vector of literals.
    if ( nLits == 0 )
        return 0;
    if ( nLits == 1 )
        return Sat_SolverEnqueue( p, pLits[0], NULL );

    // Allocate clause:
//    nBytes = sizeof(unsigned)*(nLits + 1 + (int)fLearned);
    nBytes = sizeof(unsigned)*(nLits + 2 + (int)fLearned);
#ifdef USE_SYSTEM_MEMORY_MANAGEMENT
    pC = (Sat_Clause_t *)ALLOC( char, nBytes );
#else
    pC = (Sat_Clause_t *)Sat_MmStepEntryFetch( Sat_SolverReadMem(p), nBytes );
#endif
    pC->Num        = p->nClauses++;
    pC->fTypeA     = 0;
    pC->fMark      = 0;
    pC->fLearned   = fLearned;
    pC->nSize      = nLits;
    pC->nSizeAlloc = nBytes;
    memcpy( pC->pData, pLits, sizeof(int)*nLits );
    // make room for the resolution information
//    if ( p->fProof )
    if ( Sat_ClauseVecReadSize(p->vResols) <= pC->Num )
    {
        Sat_ClauseVecPush( p->vResols,     NULL );
        Sat_ClauseVecPush( p->vResolsTemp, NULL );
    }

    // For learnt clauses only:
    if ( fLearned )
    {
        int * pLevel = Sat_SolverReadDecisionLevelArray( p );
        int iLevelMax, iLevelCur, iLitMax;

        // Put the second watch on the literal with highest decision level:
        iLitMax = 1;
        iLevelMax = pLevel[ SAT_LIT2VAR(pLits[1]) ];
        for ( i = 2; i < nLits; i++ )
        {
            iLevelCur = pLevel[ SAT_LIT2VAR(pLits[i]) ];
            assert( iLevelCur != -1 );
            if ( iLevelMax < iLevelCur )
            // this is very strange - shouldn't it be???
            // if ( iLevelMax > iLevelCur )
                iLevelMax = iLevelCur, iLitMax = i;
        }
        pC->pData[1]       = pLits[iLitMax];
        pC->pData[iLitMax] = pLits[1];

        // Bumping:
        // (newly learnt clauses should be considered active)
        Sat_ClauseWriteActivity( pC, 0.0 );
        Sat_SolverClaBumpActivity( p, pC ); 
//        if ( nLits < 20 )
        for ( i = 0; i < nLits; i++ )
            Sat_SolverVarBumpActivity( p, pLits[i] );
    }

    // Store clause:
    pvWatched = Sat_SolverReadWatchedArray( p );
    Sat_ClauseVecPush( pvWatched[ SAT_LITNOT(pC->pData[0]) ], pC );
    Sat_ClauseVecPush( pvWatched[ SAT_LITNOT(pC->pData[1]) ], pC );
    *pClause_out = pC;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Creates a fake 1-lit clause.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sat_Clause_t * Sat_ClauseCreateFake( Sat_Solver_t * p, Sat_IntVec_t * vLits )
{
    Sat_Clause_t * pC;
    Sat_Var_t Var;
    Sat_Lit_t Lit;
    int nLits = 1;
    bool fLearned = 1;
    int nBytes;

    assert( p->fProof );
    assert( Sat_IntVecReadSize(vLits) == 1 );
    Lit = Sat_IntVecReadEntry( vLits, 0 );

printf( "Creating fake %d.\n", Lit );

    nBytes = sizeof(unsigned)*(nLits + 2 + (int)fLearned);
#ifdef USE_SYSTEM_MEMORY_MANAGEMENT
    pC = (Sat_Clause_t *)ALLOC( char, nBytes );
#else
    pC = (Sat_Clause_t *)Sat_MmStepEntryFetch( Sat_SolverReadMem(p), nBytes );
#endif
    pC->Num        = p->nClauses++;
    pC->fTypeA     = 0;
    pC->fMark      = 0;
    pC->fLearned   = fLearned;
    pC->nSize      = nLits;
    pC->nSizeAlloc = nBytes;
    pC->pData[0]   = Lit;
    // make room for the resolution information
//    if ( p->fProof )
    if ( Sat_ClauseVecReadSize(p->vResols) <= pC->Num )
    {
        Sat_ClauseVecPush( p->vResols,     NULL );
        Sat_ClauseVecPush( p->vResolsTemp, NULL );
    }
    // set the new clause into the special place
    Var = SAT_LIT2VAR(Lit);
    assert( p->pResols[Var] == NULL );
    p->pResols[Var] = pC;
    return pC;
}

/**Function*************************************************************

  Synopsis    [Creates a fake 1-lit clause.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sat_Clause_t * Sat_ClauseCreateFakeLit( Sat_Solver_t * p, Sat_Lit_t Lit )
{
    Sat_Clause_t * pC;
    Sat_Var_t Var;
    int nLits = 1;
    bool fLearned = 1;
    int nBytes;

    assert( p->fProof );

printf( "Creating fake %d.\n", Lit );

    nBytes = sizeof(unsigned)*(nLits + 2 + (int)fLearned);
#ifdef USE_SYSTEM_MEMORY_MANAGEMENT
    pC = (Sat_Clause_t *)ALLOC( char, nBytes );
#else
    pC = (Sat_Clause_t *)Sat_MmStepEntryFetch( Sat_SolverReadMem(p), nBytes );
#endif
    pC->Num        = p->nClauses++;
    pC->fTypeA     = 0;
    pC->fMark      = 0;
    pC->fLearned   = fLearned;
    pC->nSize      = nLits;
    pC->nSizeAlloc = nBytes;
    pC->pData[0]   = Lit;
    // make room for the resolution information
//    if ( p->fProof )
    if ( Sat_ClauseVecReadSize(p->vResols) <= pC->Num )
    {
        Sat_ClauseVecPush( p->vResols,     NULL );
        Sat_ClauseVecPush( p->vResolsTemp, NULL );
    }
    // set the new clause into the special place
    Var = SAT_LIT2VAR(Lit);
    assert( p->pResols[Var] == NULL );
    p->pResols[Var] = pC;
    return pC;
}

/**Function*************************************************************

  Synopsis    [Deallocates the clause.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sat_ClauseFree( Sat_Solver_t * p, Sat_Clause_t * pC, bool fRemoveWatched )
{
    if ( fRemoveWatched )
    {
        Sat_Lit_t Lit;
        Sat_ClauseVec_t ** pvWatched;
        pvWatched = Sat_SolverReadWatchedArray( p );
        Lit = SAT_LITNOT( pC->pData[0] );
        Sat_ClauseRemoveWatch( pvWatched[Lit], pC );
        Lit = SAT_LITNOT( pC->pData[1] );
        Sat_ClauseRemoveWatch( pvWatched[Lit], pC ); 
    }

#ifdef USE_SYSTEM_MEMORY_MANAGEMENT
    free( pC );
#else
    Sat_MmStepEntryRecycle( Sat_SolverReadMem(p), (char *)pC, pC->nSizeAlloc );
#endif

}

/**Function*************************************************************

  Synopsis    [Access the data field of the clause.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool  Sat_ClauseReadLearned( Sat_Clause_t * pC )  {  return pC->fLearned; }
int   Sat_ClauseReadSize( Sat_Clause_t * pC )     {  return pC->nSize;    }
int * Sat_ClauseReadLits( Sat_Clause_t * pC )     {  return pC->pData;    }
bool  Sat_ClauseReadMark( Sat_Clause_t * pC )     {  return pC->fMark;    }
int   Sat_ClauseReadNum( Sat_Clause_t * pC )      {  return pC->Num;      }
bool  Sat_ClauseReadTypeA( Sat_Clause_t * pC )    {  return pC->fTypeA;    }

void  Sat_ClauseSetMark( Sat_Clause_t * pC, bool fMark )   {  pC->fMark = fMark;   }
void  Sat_ClauseSetNum( Sat_Clause_t * pC, int Num )       {  pC->Num = Num;       }
void  Sat_ClauseSetTypeA( Sat_Clause_t * pC, bool fTypeA ) {  pC->fTypeA = fTypeA; }

/**Function*************************************************************

  Synopsis    [Checks whether the learned clause is locked.]

  Description [The clause may be locked if it is the reason of a
  recent conflict. Such clause cannot be removed from the database.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Sat_ClauseIsLocked( Sat_Solver_t * p, Sat_Clause_t * pC )
{
    Sat_Clause_t ** pReasons = Sat_SolverReadReasonArray( p );
    return (bool)(pReasons[SAT_LIT2VAR(pC->pData[0])] == pC);
}

/**Function*************************************************************

  Synopsis    [Reads the activity of the given clause.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Sat_ClauseReadActivity( Sat_Clause_t * pC )
{
    return *((float *)(pC->pData + pC->nSize));
}

/**Function*************************************************************

  Synopsis    [Sets the activity of the clause.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sat_ClauseWriteActivity( Sat_Clause_t * pC, float Num )
{
    *((float *)(pC->pData + pC->nSize)) = Num;
}

/**Function*************************************************************

  Synopsis    [Propages the assignment.]

  Description [The literal that has become true (Lit) is given to this 
  procedure. The array of current variable assignments is given for
  efficiency. The output literal (pLit_out) can be the second watched
  literal (if TRUE is returned) or the conflict literal (if FALSE is 
  returned). This messy interface is used to improve performance.
  This procedure accounts for ~50% of the runtime of the solver.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Sat_ClausePropagate( Sat_Clause_t * pC, Sat_Lit_t Lit, int * pAssigns, Sat_Lit_t * pLit_out )
{
    // make sure the false literal is pC->pData[1]
    Sat_Lit_t LitF = SAT_LITNOT(Lit);
    if ( pC->pData[0] == LitF )
         pC->pData[0] = pC->pData[1], pC->pData[1] = LitF;
    assert( pC->pData[1] == LitF );
    // if the 0-th watch is true, clause is already satisfied
    if ( pAssigns[SAT_LIT2VAR(pC->pData[0])] == pC->pData[0] )
        return 1; 
    // look for a new watch
    if ( pC->nSize > 2 )
    {
        int i;
        for ( i = 2; i < (int)pC->nSize; i++ )
            if ( pAssigns[SAT_LIT2VAR(pC->pData[i])] != SAT_LITNOT(pC->pData[i]) )
            {
                pC->pData[1] = pC->pData[i], pC->pData[i] = LitF;
                *pLit_out = SAT_LITNOT(pC->pData[1]);
                return 1; 
            }
    }
    // clause is unit under assignment
    *pLit_out = pC->pData[0];
    return 0;
}

/**Function*************************************************************

  Synopsis    [Simplifies the clause.]

  Description [Assumes everything has been propagated! (esp. watches 
  in clauses are NOT unsatisfied)]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Sat_ClauseSimplify( Sat_Clause_t * pC, int * pAssigns )
{
    Sat_Var_t Var;
    int i, j;
    for ( i = j = 0; i < (int)pC->nSize; i++ )
    {
        Var = SAT_LIT2VAR(pC->pData[i]);
        if ( pAssigns[Var] == SAT_VAR_UNASSIGNED )
        {
            pC->pData[j++] = pC->pData[i];
            continue;
        }
        if ( pAssigns[Var] == pC->pData[i] )
            return 1;
        // otherwise, the value of the literal is false
        // make sure, this literal is not watched
        assert( i >= 2 );
    }
    // if the size has changed, update it and move activity
    if ( j < (int)pC->nSize )
    {
        float Activ = Sat_ClauseReadActivity(pC);
        pC->nSize = j;
        Sat_ClauseWriteActivity(pC, Activ);
    }
    return 0;
}

/**Function*************************************************************

  Synopsis    [Computes reason of conflict in the given clause.]

  Description [If the literal is unassigned, finds the reason by 
  complementing literals in the given cluase (pC). If the literal is
  assigned, makes sure that this literal is the first one in the clause
  and computes the complement of all other literals in the clause.
  Returns the reason in the given array (vLits_out). If the clause is
  learned, bumps its activity.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sat_ClauseCalcReason( Sat_Solver_t * p, Sat_Clause_t * pC, Sat_Lit_t Lit, Sat_IntVec_t * vLits_out )
{
    int i;
    // clear the reason
    Sat_IntVecClear( vLits_out );
    assert( Lit == SAT_LIT_UNASSIGNED || Lit == pC->pData[0] );
    for ( i = (Lit != SAT_LIT_UNASSIGNED); i < (int)pC->nSize; i++ )
    {
        assert( Sat_SolverReadAssignsArray(p)[SAT_LIT2VAR(pC->pData[i])] == SAT_LITNOT(pC->pData[i]) );
        Sat_IntVecPush( vLits_out, SAT_LITNOT(pC->pData[i]) );
    }
    if ( pC->fLearned ) 
        Sat_SolverClaBumpActivity( p, pC );
}

/**Function*************************************************************

  Synopsis    [Removes the given clause from the watched list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sat_ClauseRemoveWatch( Sat_ClauseVec_t * vClauses, Sat_Clause_t * pC )
{
    Sat_Clause_t ** pClauses;
    int nClauses, i;
    nClauses = Sat_ClauseVecReadSize( vClauses );
    pClauses = Sat_ClauseVecReadArray( vClauses );
    for ( i = 0; pClauses[i] != pC; i++ )
        assert( i < nClauses );
    for (      ; i < nClauses - 1; i++ )
        pClauses[i] = pClauses[i+1];
    Sat_ClauseVecPop( vClauses );
}

/**Function*************************************************************

  Synopsis    [Prints the given clause.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sat_ClausePrint( Sat_Clause_t * pC )
{
    int i;
    if ( pC == NULL )
        printf( "NULL pointer" );
    else 
    {
        if ( pC->fLearned )
            printf( "Act = %.4f  ", Sat_ClauseReadActivity(pC) );
        for ( i = 0; i < (int)pC->nSize; i++ )
            printf( " %s%d", ((pC->pData[i]&1)? "-": ""),  pC->pData[i]/2 + 1 );
    }
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    [Writes the given clause in a file in DIMACS format.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sat_ClauseWriteDimacs( FILE * pFile, Sat_Clause_t * pC, bool fIncrement )
{
    int i;
    for ( i = 0; i < (int)pC->nSize; i++ )
        fprintf( pFile, "%s%d ", ((pC->pData[i]&1)? "-": ""),  pC->pData[i]/2 + (int)(fIncrement>0) );
    if ( fIncrement )
        fprintf( pFile, "0" );
    fprintf( pFile, "\n" );
}

/**Function*************************************************************

  Synopsis    [Prints the given clause.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sat_ClausePrintSymbols( Sat_Clause_t * pC )
{
    int i;
    if ( pC == NULL )
        printf( "NULL pointer" );
    else 
    {
//        if ( pC->fLearned )
//            printf( "Act = %.4f  ", Sat_ClauseReadActivity(pC) );
        for ( i = 0; i < (int)pC->nSize; i++ )
            printf(" "L_LIT, L_lit(pC->pData[i]));
    }
    printf( "\n" );
}


/**Function*************************************************************

  Synopsis    [Prints the given clause.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned Sat_ClauseComputeTruth( Sat_Solver_t * p, Sat_Clause_t * pC )
{
    int i, iVar, iGlobal, Size;
    unsigned uTruth;

    Size = Sat_IntVecReadSize(p->vVarMap);
    uTruth = 0;
    for ( i = 0; i < (int)pC->nSize; i++ )
    {
        iVar = SAT_LIT2VAR(pC->pData[i]);
        if ( iVar >= Size )
            return ~((unsigned)0);
        // get the number of the global variable
        iGlobal = Sat_IntVecReadEntry(p->vVarMap, iVar);
        if ( iGlobal == -1 )
            continue;
        assert( iGlobal < 6 );
        if ( SAT_LITSIGN(pC->pData[i]) ) // negative
            uTruth |= ~p->uTruths[iGlobal][0];
        else
            uTruth |= p->uTruths[iGlobal][0];
    }
//    if ( uTruth == 0 )
//        return ~((unsigned)0);
    return uTruth;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


