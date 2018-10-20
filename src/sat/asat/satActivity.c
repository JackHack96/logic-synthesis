/**CFile****************************************************************

  FileName    [satActivity.c]

  PackageName [A C version of SAT solver MINISAT, originally developed 
  in C++ by Niklas Een and Niklas Sorensson, Chalmers University of 
  Technology, Sweden: http://www.cs.chalmers.se/~een/Satzoo.]

  Synopsis    [Procedures controlling activity of variables and clauses.]

  Author      [Alan Mishchenko <alanmi@eecs.berkeley.edu>]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - January 1, 2004.]

  Revision    [$Id: satActivity.c,v 1.1 2005/07/08 01:01:26 alanmi Exp $]

***********************************************************************/

#include "satInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sat_SolverVarBumpActivity( Sat_Solver_t * p, Sat_Lit_t Lit )
{
    Sat_Var_t Var;
    if ( p->dVarDecay < 0 ) // (negative decay means static variable order -- don't bump)
        return;
    Var = SAT_LIT2VAR(Lit);
    if ( (p->pdActivity[Var] += p->dVarInc) > 1e100 )
        Sat_SolverVarRescaleActivity( p );
    {
        int clk = clock();
        extern int timeSelect;
    Sat_OrderUpdate( p->pOrder, Var );
    timeSelect += clock() - clk;
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sat_SolverVarDecayActivity( Sat_Solver_t * p )
{
    if ( p->dVarDecay >= 0 )
        p->dVarInc *= p->dVarDecay;
}

/**Function*************************************************************

  Synopsis    [Divide all variable activities by 1e100.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sat_SolverVarRescaleActivity( Sat_Solver_t * p )
{
    int i;
    for ( i = 0; i < p->nVars; i++ )
        p->pdActivity[i] *= 1e-100;
    p->dVarInc *= 1e-100;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sat_SolverClaBumpActivity( Sat_Solver_t * p, Sat_Clause_t * pC )
{
    float Activ;
    Activ = Sat_ClauseReadActivity(pC);
    if ( Activ + p->dClaInc > 1e20 )
    {
        Sat_SolverClaRescaleActivity( p );
        Activ = Sat_ClauseReadActivity( pC );
    }
    Sat_ClauseWriteActivity( pC, Activ + (float)p->dClaInc );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sat_SolverClaDecayActivity( Sat_Solver_t * p )
{
    p->dClaInc *= p->dClaDecay;
}

/**Function*************************************************************

  Synopsis    [Divide all constraint activities by 1e20.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sat_SolverClaRescaleActivity( Sat_Solver_t * p )
{
    Sat_Clause_t ** pLearned;
    int nLearned, i;
    float Activ;
    nLearned = Sat_ClauseVecReadSize( p->vLearned );
    pLearned = Sat_ClauseVecReadArray( p->vLearned );
    for ( i = 0; i < nLearned; i++ )
    {
        Activ = Sat_ClauseReadActivity( pLearned[i] );
        Sat_ClauseWriteActivity( pLearned[i], Activ * (float)1e-20 );
    }
    p->dClaInc *= 1e-20;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


