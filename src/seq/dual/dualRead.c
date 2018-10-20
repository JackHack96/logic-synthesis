/**CFile****************************************************************

  FileName    [dualRead.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Reads L language formulas in symbolic representation.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - August 18, 2003.]

  Revision    [$Id: dualRead.c,v 1.5 2004/02/19 03:06:53 alanmi Exp $]

***********************************************************************/

#include "dualInt.h"
#include "ioInt.h"
#include "parse.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

/*

The spec is the list of timed Boolean constraints. The Boolean operations
currently supported in the decreasing order of precedence:
    !      (not)
     &     (and)
    <+>    (exor)
     +     (or)
    <=>    (equivalence)
     =>    (forward implication)
    <=     (backward implication)
No sign among the variables means "&". The round parantheses can be used 
to override the precendence. The square brackets set the expression inside 
them to be one tick before. So "p" means "p(t)", while "[p]" means "p(t-1)".
Each constraint should be specified from the new line.

An example of the spec represented in language L. The automaton that accepts 
all string, in which the occurrences of "1" are separated from each other by 
the even (non-zero) number of zeros:
        p <=> [!p & !x]
        x  => [ p & !x]
Here x is the input and p is the parameter, which is 1 if the automaton
received an odd number of 0's before but not including the current moment.

The corresponding automaton is:
.i 1
.o 0
.s 3
.p 4
.accepting 2
1 1 2
0 2 3
0 3 2
0 3 1

The initial state is the state "1", in which 1's are produced. The accepting
state is the state "2", before which 1's are produced. It is better to consider
the automaton without the accepting state, in which case substrings of
the required string is also accepted.

*/

static int Dual_SpecReadDots( Dual_Spec_t * p );
static void Dual_SpecReadVarAndRank( Dual_Spec_t * p );
static char * Dual_SpecReadNextLine( char * pStr, char * pEnd );
static int Dual_SpecReadIsLineEmpty( char * pStr );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Read the spec represented in L language.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Dual_Spec_t * Dual_SpecRead( Mv_Frame_t * pMvsis, char * FileName )
{
    DdManager * dd;
    Dual_Spec_t * p;
    int i, nBddVars;
//    char * pFormula;
    DdNode * bTemp;
    DdNode * bFormula;
    char * pTemp, * pEnd;
    int fReorder = 1;

    // read the file
    p = Dual_SpecAlloc();
    p->pOutput = Mv_FrameReadOut(pMvsis);
    p->pMan = Mv_FrameReadMan(pMvsis);
    p->dd = dd = Cudd_Init( 0, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
    p->pBuffer = Io_ReadFileFileContents( FileName, &p->FileSize );
    Io_ReadFileRemoveComments( p->pBuffer, NULL, NULL );
    pEnd = p->pBuffer + p->FileSize - 1;
    // split the file into parts
    if ( !Dual_SpecReadDots( p ) )
        return NULL;
    if ( p->pDotModel == NULL )
        p->pDotModel = util_strsav( FileName );

    // determine the number of vars and the rank
    Dual_SpecReadVarAndRank( p );

    // extend the manager if necessary
    nBddVars = p->nVars * (p->nRank + 1);
    if ( dd->size <= nBddVars )
    {
        for ( i = dd->size; i < nBddVars; i++ )
            Cudd_bddNewVar(dd);
    }
    // assign the elementary vars
    p->pbVars = ALLOC( DdNode *, p->nVars );
    for ( i = 0; i < p->nVars; i++ )
        p->pbVars[i] = dd->vars[ p->nVars * p->nRank + i ];

    // read the initial condition if given
//    pFormula = ALLOC( char, p->FileSize );
    if ( p->pDotInit )
    {
        assert( strncmp( p->pDotInit, ".init", 5 ) == 0 );
        for ( pTemp = p->pDotInit; pTemp && *pTemp != ' ' && *pTemp != '\t'; pTemp++ );
//        sprintf( pFormula, "(%s)", pTemp );
        p->bCondInit = Parse_FormulaParser( p->pOutput, pTemp, p->nVars, p->nRank, 
            p->ppVarNames, dd, p->pbVars ); 
        if ( p->bCondInit == NULL )
            return NULL;
        Cudd_Ref( p->bCondInit );
    }
    else
    { // make all the states initial
        p->bCondInit = b1;  Cudd_Ref( p->bCondInit );
        printf( "The initial condition is not given; assuming all states initial.\n" );
    }

    assert( dd->size == (p->nRank + 1) * p->nVars );
    if ( fReorder ) 
    {
        // create the tree
        Cudd_MakeTreeNode( dd, 0, dd->size, MTR_FIXED );
        Cudd_MakeTreeNode( dd, 0, p->nRank * p->nVars, MTR_DEFAULT );
        Cudd_MakeTreeNode( dd, p->nRank * p->nVars, p->nVars, MTR_DEFAULT );
        //Mtr_PrintGroups( dd->tree, 0 );
        Cudd_AutodynEnable(dd, CUDD_REORDER_SIFT);
    }

    // perform parsing of the formula
    p->bSpec = b1;   Cudd_Ref( b1 );
    for ( pTemp = p->pDotStart; pTemp; pTemp = Dual_SpecReadNextLine(pTemp, pEnd) )
    {
        if ( Dual_SpecReadIsLineEmpty(pTemp) )
            continue;
        if ( pTemp[ strlen(pTemp) - 1 ] == '\r' )
            pTemp[ strlen(pTemp) - 1 ] = 0;

printf( "Parsing formula   %s\n", pTemp );

//        sprintf( pFormula, "(%s)", pTemp );
        bFormula = Parse_FormulaParser( p->pOutput, pTemp, p->nVars, p->nRank, 
            p->ppVarNames, dd, p->pbVars ); 
        if ( bFormula == NULL )
        {
            Cudd_RecursiveDeref( dd, p->bSpec );
            return NULL;
        }
        Cudd_Ref( bFormula );

        p->bSpec = Cudd_bddAnd( dd, bTemp = p->bSpec, bFormula );   Cudd_Ref( p->bSpec );
        Cudd_RecursiveDeref( dd, bTemp );
        Cudd_RecursiveDeref( dd, bFormula );
    }
//    FREE( pFormula );
 
    if ( fReorder ) 
    {
        printf( "BDD size before reordering = %d. ", Cudd_DagSize(p->bSpec) );
        Cudd_ReduceHeap( dd, CUDD_REORDER_SIFT, 10 );
        Cudd_AutodynDisable(dd);
        printf( "BDD size after reordering = %d.\n", Cudd_DagSize(p->bSpec) );
        //for ( i = 0; i < dd->size; i++ )
        //    printf( " %d", dd->invperm[i] );
        //printf( "\n" );
    }

    // quit
    FREE( p->pBuffer );
    p->pBuffer = NULL;
    return p;
}

/**Function*************************************************************

  Synopsis    [Read the header.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dual_SpecReadDots( Dual_Spec_t * p )
{
    char * pBuffer = p->pBuffer;
    char * pCur, * pCurMax;
    for ( pCur = p->pBuffer; *pCur; pCur++ )
        if ( *pCur == '.' )
        {
            if ( pCur[1] == 'm' && pCur[2] == 'o' && pCur[3] == 'd' && pCur[4] == 'e' && pCur[5] == 'l' )
            {
                if ( p->pDotModel )
                {
                    fprintf( p->pOutput, "The L language file contains multiple .model lines.\n" );
                    return 0;
                }
                p->pDotModel = pCur;
            }
            else if ( pCur[1] == 'i' && pCur[2] == 'n' && pCur[3] == 'p' && pCur[4] == 'u' && pCur[5] == 't' )
            {
                if ( p->pDotIns )
                {
                    fprintf( p->pOutput, "The L language file contains multiple .inputs lines.\n" );
                    return 0;
                }
                p->pDotIns = pCur;
            }
            else if ( pCur[1] == 'p' && pCur[2] == 'a' && pCur[3] == 'r' && pCur[4] == 'a' && pCur[5] == 'm' )
            {
                if ( p->pDotPars )
                {
                    fprintf( p->pOutput, "The L language file contains multiple .parameters lines.\n" );
                    return 0;
                }
                p->pDotPars = pCur;
            }
            else if ( pCur[1] == 'o' && pCur[2] == 'u' && pCur[3] == 't' && pCur[4] == 'p' && pCur[5] == 'u' && pCur[6] == 't' )
            {
                if ( p->pDotOuts )
                {
                    fprintf( p->pOutput, "The L language file contains multiple .outputs lines.\n" );
                    return 0;
                }
                p->pDotOuts = pCur;
            }
            else if ( pCur[1] == 'i' && pCur[2] == 'n' && pCur[3] == 'i' && pCur[4] == 't' )
            {
                if ( p->pDotInit )
                {
                    fprintf( p->pOutput, "The L language file contains multiple .initial lines.\n" );
                    return 0;
                }
                p->pDotInit = pCur;
            }
        }
    if ( p->pDotIns == NULL )
    {
        fprintf( p->pOutput, "The KISS file does not contain .inputs line.\n" );
        return 0;
    }
    if ( p->pDotPars == NULL )
    {
        fprintf( p->pOutput, "The KISS file does not contain .parameters line.\n" );
        return 0;
    }
    if ( p->pDotOuts == NULL )
    {
        fprintf( p->pOutput, "The KISS file does not contain .outputs line.\n" );
        return 0;
    }
    // find the beginning of the table
    pCurMax = p->pBuffer;
    if ( p->pDotModel && pCurMax < p->pDotModel )
        pCurMax = p->pDotModel;
    if ( p->pDotIns   && pCurMax < p->pDotIns )
        pCurMax = p->pDotIns;
    if ( p->pDotPars  && pCurMax < p->pDotPars )
        pCurMax = p->pDotPars;
    if ( p->pDotOuts  && pCurMax < p->pDotOuts )
        pCurMax = p->pDotOuts;
    if ( p->pDotInit  && pCurMax < p->pDotInit )
        pCurMax = p->pDotInit;
    // go to the next new line symbol
    for ( ; *pCurMax; pCurMax++ )
        if ( *pCurMax == '\n' )
        {
            p->pDotStart = pCurMax + 1;
            break;
        }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Count the number of variables and their rank.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dual_SpecReadVarAndRank( Dual_Spec_t * p )
{
    char * pTemp;
    int nParams, nParamsMax;

    // replace end of lines by zeros
    for ( pTemp = p->pBuffer; *pTemp; pTemp++ )
        if ( *pTemp == '\n' )
            *pTemp = 0;

    // alloc room for variables
    p->nVarsAlloc = 100;
    p->ppVarNames = ALLOC( char *, p->nVarsAlloc );


    // read the inputs
    p->nVars = 0;
    p->nVarsIn = 0;
    pTemp = strtok( p->pDotIns, " \t" );
    assert( strncmp( pTemp, ".inputs", 6 ) == 0 );
    while ( pTemp = strtok( NULL, " \t\n\r" ) )
    {
        p->ppVarNames[p->nVars++] = util_strsav(pTemp);
        if ( p->nVarsAlloc <= p->nVars )
        {
            p->ppVarNames  = REALLOC( char *, p->ppVarNames,  2 * p->nVarsAlloc );
            p->nVarsAlloc *= 2;
        }
        p->nVarsIn++;
    }

    // read the parameters
    p->nVarsPar = 0;
    pTemp = strtok( p->pDotPars, " \t" );
    assert( strncmp( pTemp, ".parameters", 6 ) == 0 );
    while ( pTemp = strtok( NULL, " \t\n\r" ) )
    {
        p->ppVarNames[p->nVars++] = util_strsav(pTemp);
        if ( p->nVarsAlloc <= p->nVars )
        {
            p->ppVarNames  = REALLOC( char *, p->ppVarNames,  2 * p->nVarsAlloc );
            p->nVarsAlloc *= 2;
        }
        p->nVarsPar++;
    }

    // read the outputs
    p->nVarsOut = 0;
    pTemp = strtok( p->pDotOuts, " \t" );
    assert( strncmp( pTemp, ".outputs", 6 ) == 0 );
    while ( pTemp = strtok( NULL, " \t\n\r" ) )
    {
        p->ppVarNames[p->nVars++] = util_strsav(pTemp);
        if ( p->nVarsAlloc <= p->nVars )
        {
            p->ppVarNames  = REALLOC( char *, p->ppVarNames,  2 * p->nVarsAlloc );
            p->nVarsAlloc *= 2;
        }
        p->nVarsOut++;
    }

    // determine the rank
    nParamsMax = -1;
    nParams = 0;
    for ( pTemp = p->pDotStart; pTemp < p->pDotStart + p->FileSize; pTemp++ )
    {
        if ( *pTemp == '[' )
            nParams++;
        else if ( *pTemp == ']' )
            nParams--;
        if ( nParamsMax < nParams )
            nParamsMax = nParams;
    }
    p->nRank = nParamsMax;
}

/**Function*************************************************************

  Synopsis    [Returns the pointer to the next non-empty line in the file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Dual_SpecReadNextLine( char * p, char * pEnd )
{
    // scroll to the end of this line
    while ( *p )
        p++;
    // scroll to the end of the zeros
    while ( *p == 0 )
        p++;
    if ( p < pEnd )
        return p;
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the line contains only transparent chars.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dual_SpecReadIsLineEmpty( char * p )
{
    for ( ; *p; p++ )
        if ( *p != ' ' && *p != '\n' && *p != '\t' && *p != '\r' )
            return 0;
    return 1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


