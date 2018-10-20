/**CFile****************************************************************

  FileName    [satSolverInter.c]

  PackageName [A C version of SAT solver MINISAT, originally developed 
  in C++ by Niklas Een and Niklas Sorensson, Chalmers University of 
  Technology, Sweden: http://www.cs.chalmers.se/~een/Satzoo.]

  Synopsis    [Procedures for interpolant computation.]

  Author      [Alan Mishchenko <alanmi@eecs.berkeley.edu>]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - January 1, 2004.]

  Revision    [$Id: satSolverInter.c,v 1.1 2005/07/08 01:01:29 alanmi Exp $]

***********************************************************************/

#include "satInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

extern Sat_Resol_t * Sat_ResolReadR1( Sat_Resol_t * pR );
extern Sat_Resol_t * Sat_ResolReadR2( Sat_Resol_t * pR );
extern bool          Sat_ResolReadF1( Sat_Resol_t * pR );         
extern bool          Sat_ResolReadF2( Sat_Resol_t * pR );        
extern int           Sat_ResolReadVar( Sat_Resol_t * pR );        
extern int           Sat_ResolReadNum( Sat_Resol_t * pR );        
extern void          Sat_ResolSetNum( Sat_Resol_t * pR, int Num );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Sat_SolverComputeInterpolant( Sat_Solver_t * p, 
    Sat_ClauseVec_t * vProof, 
    char * pMan,
    char * (* pProcConst0)(char * pMan), 
    char * (* pProcConst1)(char * pMan), 
    char * (* pProcVar)(char * pMan, int Var),
    char * (* pProcAnd)(char * pMan, char * pArg1, char * pArg2),
    char * (* pProcOr )(char * pMan, char * pArg1, char * pArg2),
    char * (* pProcNot)(char * pArg), 
    void   (* pProcRef)(char * pArg), 
    void   (* pProcRecDeref)(char * pMan, char * pArg) )
{
    Sat_Resol_t ** pResols;
    Sat_Resol_t * pR, * pR1, * pR2;
    Sat_Clause_t ** ppClauses;
    Sat_Clause_t * pC;
    char * pRes, * pResV, * pRes1, * pRes2, * pResT;
    char ** pData, ** pDataClo;
    int nLits, * pLits;
    int nResols, nClauses, i, k;

    // precompute the root clauses
    nClauses  = Sat_ClauseVecReadSize( p->vClauses );
    ppClauses = Sat_ClauseVecReadArray( p->vClauses );
    pDataClo  = ALLOC( char *, nClauses );
    memset( pDataClo, 0, sizeof(char *) * nClauses );
    for ( i = 0; i < nClauses; i++ )
    {
        pC = ppClauses[i];
        if ( Sat_ClauseReadTypeA(pC) )
        {
            // go through literals
            nLits = Sat_ClauseReadSize( pC );
            pLits = Sat_ClauseReadLits( pC );
            pRes = pProcConst0( pMan );     pProcRef( pRes );
            for ( k = 0; k < nLits; k++ )
            {
                if ( p->pVarTypeA[SAT_LIT2VAR(pLits[k])] )
                    continue;
                pResV = pProcVar( pMan, SAT_LIT2VAR(pLits[k]) );  
                if ( SAT_LITSIGN(pLits[k]) )
                    pResV = pProcNot( pResV );
                pRes = pProcOr( pMan, pResT = pRes, pResV );   pProcRef( pRes );
                pProcRecDeref( pMan, pResT );
            }
        }
        else
        {
            pRes = pProcConst1( pMan );  pProcRef( pRes );
        }
        pDataClo[i] = pRes; // takes ref
    }

    // compute the interpolant for each resolvent
    nResols = Sat_ClauseVecReadSize( vProof );
    pResols = (Sat_Resol_t **)Sat_ClauseVecReadArray( vProof );
    // start the array of clause numbers
    pData = ALLOC( char *, nResols );
    memset( pData, 0, sizeof(char *) * nResols );
    for ( i = 0; i < nResols; i++ )
    {
        pR  = pResols[i];
        pR1 = Sat_ResolReadR1( pR );
        pR2 = Sat_ResolReadR2( pR );
        // construct interpolant for the first branch
        if ( Sat_ResolReadF1(pR) )
            pRes1 = pDataClo[Sat_ClauseReadNum((Sat_Clause_t *)pR1)];
        else
            pRes1 = pData[Sat_ResolReadNum(pR1)];
        // construct interpolant for the second branch
        if ( Sat_ResolReadF2(pR) )
            pRes2 = pDataClo[Sat_ClauseReadNum((Sat_Clause_t *)pR2)];
        else
            pRes2 = pData[Sat_ResolReadNum(pR2)];
        // get the result
        if ( p->pVarTypeA[Sat_ResolReadVar(pR)] )
            pData[i] = pProcOr( pMan, pRes1, pRes2 );
        else
            pData[i] = pProcAnd( pMan, pRes1, pRes2 );
        pProcRef( pData[i] ); // takes ref
        Sat_ResolSetNum( pR, i );
    }

    // deref intermediate results
    pRes = pData[nResols - 1];
    for ( i = 0; i < nResols - 1; i++ )
        pProcRecDeref( pMan, pData[i] );
    FREE( pData );
    // deref intermediate results
    for ( i = 0; i < nClauses; i++ )
        pProcRecDeref( pMan, pDataClo[i] );
    FREE( pDataClo );
    return pRes; // returns referenced result!
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


