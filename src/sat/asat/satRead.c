/**CFile****************************************************************

  FileName    [satRead.c]

  PackageName [A C version of SAT solver MINISAT, originally developed 
  in C++ by Niklas Een and Niklas Sorensson, Chalmers University of 
  Technology, Sweden: http://www.cs.chalmers.se/~een/Satzoo.]

  Synopsis    [The reader of the CNF formula in DIMACS format.]

  Author      [Alan Mishchenko <alanmi@eecs.berkeley.edu>]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - January 1, 2004.]

  Revision    [$Id: satRead.c,v 1.1 2005/07/08 01:01:28 alanmi Exp $]

***********************************************************************/

#include "satInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static char * Sat_FileRead( FILE * pFile );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Read the file into the internal buffer.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Sat_FileRead( FILE * pFile )
{
    int nFileSize;
    char * pBuffer;
    // get the file size, in bytes
    fseek( pFile, 0, SEEK_END );  
    nFileSize = ftell( pFile );  
    // move the file current reading position to the beginning
    rewind( pFile ); 
    // load the contents of the file into memory
    pBuffer = ALLOC( char, nFileSize + 3 );
    fread( pBuffer, nFileSize, 1, pFile );
    // terminate the string with '\0'
    pBuffer[ nFileSize + 0] = '\n';
    pBuffer[ nFileSize + 1] = '\0';
    return pBuffer;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static void Sat_ReadWhitespace( char ** pIn ) 
{
    while ((**pIn >= 9 && **pIn <= 13) || **pIn == 32)
        (*pIn)++; 
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static void Sat_ReadNotWhitespace( char ** pIn ) 
{
    while ( !((**pIn >= 9 && **pIn <= 13) || **pIn == 32) )
        (*pIn)++; 
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static void skipLine( char ** pIn ) 
{
    while ( 1 )
    {
        if (**pIn == 0) 
            return;
        if (**pIn == '\n') 
        { 
            (*pIn)++; 
            return; 
        }
        (*pIn)++; 
    } 
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static int Sat_ReadInt( char ** pIn ) 
{
    int     val = 0;
    bool    neg = 0;

    Sat_ReadWhitespace( pIn );
    if ( **pIn == '-' ) 
        neg = 1, 
        (*pIn)++;
    else if ( **pIn == '+' ) 
        (*pIn)++;
    if ( **pIn < '0' || **pIn > '9' ) 
        fprintf(stderr, "PARSE ERROR! Unexpected char: %c\n", **pIn), 
        exit(1);
    while ( **pIn >= '0' && **pIn <= '9' )
        val = val*10 + (**pIn - '0'),
        (*pIn)++;
    return neg ? -val : val; 
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static void Sat_ReadClause( char ** pIn, Sat_Solver_t * p, Sat_IntVec_t * pLits ) 
{
    int nVars = Sat_SolverReadVarNum( p );
    int parsed_lit, var, sign;

    Sat_IntVecClear( pLits );
    while ( 1 )
    {
        parsed_lit = Sat_ReadInt(pIn);
        if ( parsed_lit == 0 ) 
            break;
        var = abs(parsed_lit) - 1;
        sign = (parsed_lit > 0);
        if ( var >= nVars )
        {
            printf( "Variable %d is larger than the number of allocated variables (%d).\n", var+1, nVars );
            exit(1);
        }
        Sat_IntVecPush( pLits, SAT_VAR2LIT(var, !sign) );
    }
}
 
/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static bool Sat_ReadDimacs( char * pText, Sat_Solver_t ** pS, bool fVerbose, bool fProof ) 
{
    Sat_Solver_t * p;
    Sat_IntVec_t * pLits;
    char * pIn = pText;
    int nVars, nClas;
    while ( 1 )
    {
        Sat_ReadWhitespace( &pIn );
        if ( *pIn == 0 )
            break;
        else if ( *pIn == 'c' )
            skipLine( &pIn );
        else if ( *pIn == 'p' )
        {
            pIn++;
            Sat_ReadWhitespace( &pIn );
            Sat_ReadNotWhitespace( &pIn );

            nVars = Sat_ReadInt( &pIn );
            nClas = Sat_ReadInt( &pIn );
            skipLine( &pIn );
            // start the solver
            p = Sat_SolverAlloc( nVars, 1, 1, 1, 1, 0 ); 
            Sat_SolverClean( p, nVars );
            Sat_SolverSetVerbosity( p, fVerbose );
            Sat_SolverSetProofWriting( p, fProof );
            // allocate the vector
            pLits = Sat_IntVecAlloc( nVars );
        }
        else
        {
            if ( p == NULL )
            {
                printf( "There is no parameter line.\n" );
                exit(1);
            }
            Sat_ReadClause( &pIn, p, pLits );
            if ( !Sat_SolverAddClause( p, pLits ) )
                return 0;
        }
    }
    Sat_IntVecFree( pLits );
    *pS = p;
    return Sat_SolverSimplifyDB( p );
}

/**Function*************************************************************

  Synopsis    [Starts the solver and reads the DIMAC file.]

  Description [Returns FALSE upon immediate conflict.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Sat_SolverParseDimacs( FILE * pFile, Sat_Solver_t ** p, int fVerbose, bool fProof )
{
    char * pText;
    bool Value;
    pText = Sat_FileRead( pFile );
    Value = Sat_ReadDimacs( pText, p, fVerbose, fProof );
    free( pText );
    return Value;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


