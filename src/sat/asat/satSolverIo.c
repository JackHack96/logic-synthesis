/**CFile****************************************************************

  FileName    [satSolverIo.c]

  PackageName [A C version of SAT solver MINISAT, originally developed 
  in C++ by Niklas Een and Niklas Sorensson, Chalmers University of 
  Technology, Sweden: http://www.cs.chalmers.se/~een/Satzoo.]

  Synopsis    [Input/output of CNFs.]

  Author      [Alan Mishchenko <alanmi@eecs.berkeley.edu>]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - January 1, 2004.]

  Revision    [$Id: satSolverIo.c,v 1.1 2005/07/08 01:01:29 alanmi Exp $]

***********************************************************************/

#include "satInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static char * Sat_TimeStamp();

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sat_SolverPrintAssignment( Sat_Solver_t * p )
{
    int i;
    printf( "Current assignments are: \n" );
    for ( i = 0; i < p->nVars; i++ )
        printf( "%d", i % 10 );
    printf( "\n" );
    for ( i = 0; i < p->nVars; i++ )
        if ( p->pAssigns[i] == SAT_VAR_UNASSIGNED )
            printf( "." );
        else
        {
            assert( i == SAT_LIT2VAR(p->pAssigns[i]) );
            if ( SAT_LITSIGN(p->pAssigns[i]) )
                printf( "0" );
            else 
                printf( "1" );
        }
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sat_SolverPrintClauses( Sat_Solver_t * p )
{
    Sat_Clause_t ** pClauses;
    int nClauses, i;

    printf( "Original clauses: \n" );
    nClauses = Sat_ClauseVecReadSize( p->vClauses );
    pClauses = Sat_ClauseVecReadArray( p->vClauses );
    for ( i = 0; i < nClauses; i++ )
    {
        printf( "%3d: ", i );
        Sat_ClausePrint( pClauses[i] );
    }

    printf( "Learned clauses: \n" );
    nClauses = Sat_ClauseVecReadSize( p->vLearned );
    pClauses = Sat_ClauseVecReadArray( p->vLearned );
    for ( i = 0; i < nClauses; i++ )
    {
        printf( "%3d: ", i );
        Sat_ClausePrint( pClauses[i] );
    }

    printf( "Variable activity: \n" );
    for ( i = 0; i < p->nVars; i++ )
        printf( "%3d : %.4f\n", i, p->pdActivity[i] );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sat_SolverWriteDimacs( Sat_Solver_t * p, char * pFileName )
{
    FILE * pFile;
    Sat_Clause_t ** pClauses;
    int nClauses, i;

    nClauses = Sat_ClauseVecReadSize(p->vClauses) + Sat_ClauseVecReadSize(p->vLearned);
    for ( i = 0; i < p->nVars; i++ )
        nClauses += ( p->pLevel[i] == 0 );

    pFile = fopen( pFileName, "wb" );
    fprintf( pFile, "c Produced by Sat_SolverWriteDimacs() on %s\n", Sat_TimeStamp() );
    fprintf( pFile, "p cnf %d %d\n", p->nVars, nClauses );

    nClauses = Sat_ClauseVecReadSize( p->vClauses );
    pClauses = Sat_ClauseVecReadArray( p->vClauses );
    for ( i = 0; i < nClauses; i++ )
        Sat_ClauseWriteDimacs( pFile, pClauses[i], 1 );

    nClauses = Sat_ClauseVecReadSize( p->vLearned );
    pClauses = Sat_ClauseVecReadArray( p->vLearned );
    for ( i = 0; i < nClauses; i++ )
        Sat_ClauseWriteDimacs( pFile, pClauses[i], 1 );

    // write zero-level assertions
    for ( i = 0; i < p->nVars; i++ )
        if ( p->pLevel[i] == 0 )
            fprintf( pFile, "%s%d 0\n", ((p->pAssigns[i]&1)? "-": ""), i + 1 );

    fprintf( pFile, "\n" );
    fclose( pFile );
}


/**Function*************************************************************

  Synopsis    [Returns the time stamp.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Sat_TimeStamp()
{
    static char Buffer[100];
	time_t ltime;
	char * TimeStamp;
    // get the current time
	time( &ltime );
	TimeStamp = asctime( localtime( &ltime ) );
	TimeStamp[ strlen(TimeStamp) - 1 ] = 0;
    strcpy( Buffer, TimeStamp );
    return Buffer;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


