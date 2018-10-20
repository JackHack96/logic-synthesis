/**CFile****************************************************************

  FileName    [autiCheck.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Checking procedures.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - January 1, 2004.]

  Revision    [$Id: autiCheck.c,v 1.2 2005/06/02 03:34:20 alanmi Exp $]

***********************************************************************/

#include "autiInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define PROD_STATE_HASH(s1,s2)   ((((unsigned)s1) << 16) | (((unsigned)s2) & 0xffff))

typedef struct AutCheckDataStruct Aut_CheckData_t;
struct AutCheckDataStruct
{
    Aut_Auto_t *  pAut1;
    Aut_Auto_t *  pAut2;
    bool          f1Contains2;
    bool          f2Contains1;
    int           nError12;
    int           nError21;
    int           nErrorAlloc;
    int           nTrace;
    Aut_State_t ** pTrace1;
    Aut_State_t ** pTrace2;
    Aut_State_t ** pError12_1;
    Aut_State_t ** pError12_2;
    Aut_State_t ** pError21_1;
    Aut_State_t ** pError21_2;
    st_table *    tStates;
};

static bool Auti_AutoCheckProduct_rec( Aut_CheckData_t * p, Aut_State_t * pS1, Aut_State_t * pS2 );
static void Auti_AutoCheckPrintTrace( FILE * pOut, Aut_CheckData_t * p, int fFirst );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Checks the containment of the automata.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Auti_AutoCheck( FILE * pOut, Aut_Auto_t * pAut1, Aut_Auto_t * pAut2, bool fError )
{
    Aut_CheckData_t Data, * p = &Data;

    // check if automata are empty
    if ( pAut1->nStates == 0 )
    {
        fprintf( pOut, "Automaton 1 has no states; checking skipped.\n", pAut1->pName );
        return;
    }
    if ( pAut2->nStates == 0 )
    {
        fprintf( pOut, "Automaton 2 has no states; checking skipped.\n", pAut2->pName );
        return;
    }

    // make sure they have the same number of inputs
    // make sure that the ordering of input names are identical
    if ( !Auti_AutoProductCheckInputs( pAut1, pAut2 ) )
        return;

    // check if the automata are deterministic
    if ( Auti_AutoCheckIsNd( stdout, pAut1, pAut1->nVars, 0 ) > 0 )
    {
        printf( "Automaton 1 is non-deterministic; checking cannot be performed.\n", pAut1->pName );
        return;
    }
    if ( Auti_AutoCheckIsNd( stdout, pAut2, pAut2->nVars, 0 ) > 0 )
    {
        printf( "Automaton 2 is non-deterministic; checking cannot be performed.\n", pAut2->pName );
        return;
    }

    // check if the automata are complete
    if ( Auti_AutoComplete( pAut1, pAut1->nVars, 0 ) )
        printf( "Warning: Automaton 1 is completed before checking.\n", pAut1->pName );
    if ( Auti_AutoComplete( pAut2, pAut2->nVars, 0 ) )
        printf( "Warning: Automaton 2 is completed before checking.\n", pAut2->pName );

    // check capacity constraints
    if ( pAut1->nStates >= (1<<16) || pAut2->nStates >= (1<<16) )
    {
        fprintf( pOut, "Currently can only check automata with less than %d state.\n", (1<<16) );
        return;
    }

    // initialize the data structure
    memset( p, 0, sizeof(Aut_CheckData_t) );
    p->pAut1       = pAut1;
    p->pAut2       = pAut2;
    p->f1Contains2 = 1;
    p->f2Contains1 = 1;
    p->nErrorAlloc = 2000;
    p->pTrace1     = ALLOC( Aut_State_t *, p->nErrorAlloc );
    p->pTrace2     = ALLOC( Aut_State_t *, p->nErrorAlloc );
    p->pError12_1  = ALLOC( Aut_State_t *, p->nErrorAlloc );
    p->pError12_2  = ALLOC( Aut_State_t *, p->nErrorAlloc );
    p->pError21_1  = ALLOC( Aut_State_t *, p->nErrorAlloc );
    p->pError21_2  = ALLOC( Aut_State_t *, p->nErrorAlloc );
    p->tStates     = st_init_table(st_numcmp,st_numhash);

    // assign the state numbers
    Aut_AutoAssignNumbers( pAut1 );
    Aut_AutoAssignNumbers( pAut2 );

    // save the initial state as the current state for the traversal
    p->pTrace1[0] = pAut1->pInit[0];
    p->pTrace2[0] = pAut2->pInit[0];
    p->nTrace = 1;
    // traverse the product machine and detect non-containment
    Auti_AutoCheckProduct_rec( p, pAut1->pInit[0], pAut2->pInit[0] );

    // analize the results of traversal
    if ( p->f1Contains2 && p->f2Contains1 )
        fprintf( pOut, "Automata are sequentially equivalent\n", pAut1->pName, pAut2->pName );
    else if ( p->f1Contains2 )
        fprintf( pOut, "The behavior of automaton 1 contains the behavior of automaton 2.\n", pAut1->pName, pAut2->pName );
    else if ( p->f2Contains1 )
        fprintf( pOut, "The behavior of automaton 1 is contained in the behavior of automaton 2.\n", pAut1->pName, pAut2->pName );
    else 
        fprintf( pOut, "There is no behavior containment between the automata.\n", pAut1->pName, pAut2->pName );

    if ( fError )
    {
        if ( !p->f1Contains2 )
        {
            fprintf( pOut, "The behavior of automaton 1 does not contain the behavior of automaton 2.\n", pAut1->pName, pAut2->pName );
            fprintf( pOut, "Error trace:\n" );
            Auti_AutoCheckPrintTrace( pOut, p, 1 );
        }
        if ( !p->f2Contains1 )
        {
            fprintf( pOut, "The behavior of automaton 1 is not contained in the behavior of automaton 2.\n", pAut1->pName, pAut2->pName );
            fprintf( pOut, "Error trace:\n" );
            Auti_AutoCheckPrintTrace( pOut, p, 0 );
        }
    }

    FREE( p->pTrace1 );
    FREE( p->pTrace2 );
    FREE( p->pError12_1 );
    FREE( p->pError12_2 );
    FREE( p->pError21_1 );
    FREE( p->pError21_2 );
    st_free_table( p->tStates );
}

/**Function*************************************************************

  Synopsis    [Performs the search of product automaton for non-equivalences.]

  Description [Return 1 if both containment checks have failed. Returns 0
  if they are equaivalent or one containment holds.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Auti_AutoCheckProduct_rec( Aut_CheckData_t * p, Aut_State_t * pS1, Aut_State_t * pS2 )
{
    Aut_Trans_t * pT1, * pT2;
    int RetValue, s;
    unsigned Hash;

    if ( p->nTrace >= p->nErrorAlloc )
    {
        printf( "Cannot compute the product machine with very long state sequences.\n" );
        p->nError12 = 0;
        p->nError21 = 0;
        return 1;
    }

    // check if this state has been reached
    Hash = PROD_STATE_HASH(pS1->uData, pS2->uData);
    if ( st_find_or_add( p->tStates, (char *)Hash, NULL ) )
        return 0;

    // check if this state allows us to make some conclusions
    if ( p->f1Contains2 && !pS1->fAcc && pS2->fAcc )
    {
        p->f1Contains2 = 0;
        for ( s = 0; s < p->nTrace; s++ )
        {
            p->pError12_1[s] = p->pTrace1[s];
            p->pError12_2[s] = p->pTrace2[s];
        }
        p->nError12 = p->nTrace;
    }
    if ( p->f2Contains1 && pS1->fAcc && !pS2->fAcc )
    {
        p->f2Contains1 = 0;
        for ( s = 0; s < p->nTrace; s++ )
        {
            p->pError21_1[s] = p->pTrace1[s];
            p->pError21_2[s] = p->pTrace2[s];
        }
        p->nError21 = p->nTrace;
    }

    if ( !p->f2Contains1 && !p->f1Contains2 )
        return 1;

    // try all the matching transitions from the two states
    Aut_StateForEachTransitionFrom_int( pS1, pT1 )
    {
        Aut_StateForEachTransitionFrom_int( pS2, pT2 )
        {
            // check if the conditions overlap
            if ( Aut_AutoTransOverlap( pT1, pT2 ) )
            {
                // save this state as the current state
                p->pTrace1[ p->nTrace ] = pT1->pTo;
                p->pTrace2[ p->nTrace ] = pT2->pTo;
                p->nTrace++;
                // perform the traversal starting from these states
                RetValue = Auti_AutoCheckProduct_rec( p, pT1->pTo, pT2->pTo );
                p->nTrace--;
                if ( RetValue == 1 )
                    return 1;
            }
        }
    }
    return 0;
}



/**Function*************************************************************

  Synopsis    [Checks the containment of the automata.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Auti_AutoCheckPrintTrace( FILE * pOut, Aut_CheckData_t * p, int fFirst )
{
//    Mvc_Cube_t * pCube;
    Aut_State_t * pS1, * pS2;
    Aut_Trans_t * pT1, * pT2;
//    Mvc_Cover_t * pMvc;
    Aut_State_t ** pErrorTrace1;
    Aut_State_t ** pErrorTrace2;
    int nErrorTrace;
//    int v, Value0, Value1;
    int i;

    if ( fFirst )
    {
        pErrorTrace1 = p->pError12_1;
        pErrorTrace2 = p->pError12_2;
        nErrorTrace  = p->nError12;
    }
    else
    {
        pErrorTrace1 = p->pError21_1;
        pErrorTrace2 = p->pError21_2;
        nErrorTrace  = p->nError21;
    }

    fprintf( pOut, "Trans  " );
    fprintf( pOut, "  %15s    ",  p->pAut1->pName );
    fprintf( pOut, "  %15s    ",  p->pAut2->pName );
    fprintf( pOut, "   Condition\n" );

    for ( i = 0; i < nErrorTrace; i++ )
    {
        // get the states
        pS1 = pErrorTrace1[i];
        pS2 = pErrorTrace2[i];

        // print the states and their acceptance status
        fprintf( pOut, "%4d",     i + 1 );
        fprintf( pOut, "  %15s %s",  pS1->pName, (pS1->fAcc? "(*)": "   ") );
        fprintf( pOut, "  %15s %s",  pS2->pName, (pS2->fAcc? "(*)": "   ") );
        fprintf( pOut, "       " );
        if ( i == nErrorTrace - 1 )
        {
            fprintf( pOut, "\n" );
            break;
        }

        // find the transition to the next state in the error trace
        pT1 = pT2 = NULL;
        Aut_StateForEachTransitionFrom( pS1, pT1 )
            if ( pT1->pTo == pErrorTrace1[i+1] )
                break;
        Aut_StateForEachTransitionFrom( pS2, pT2 )
            if ( pT2->pTo == pErrorTrace2[i+1] )
                break;
        assert( pT1 && pT2 );
/*
        // get the intersection of the conditions
        pMvc = Mvc_CoverBooleanAnd( p->pData, pT1->pCond, pT2->pCond );
        // one cube from the condition is anough for the error trace
        pCube = Mvc_CoverReadCubeHead(pMvc);
        for ( v = 0; v < p->pAut1->nVars; v++ )
        {
            Value0 = Mvc_CubeBitValue( pCube, 2*v );
            Value1 = Mvc_CubeBitValue( pCube, 2*v + 1 );
            if ( Value0 && Value1 )
                fprintf( pOut, "-" );
            else if ( Value0 && !Value1 )
                fprintf( pOut, "0" );
            else if ( !Value0 && Value1 )
                fprintf( pOut, "1" );
            else
            {
                assert( 0 );
            }
        }
*/
        fprintf( pOut, "\n" );
//        Mvc_CoverFree( pMvc );
    }
}


/**Function*************************************************************

  Synopsis    [Returns the number of non-deterministic states.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Auti_AutoCheckIsNd( FILE * pOut, Aut_Auto_t * pAut, int nInputs, int fVerbose )
{
    Aut_State_t * pS;
    Aut_Trans_t * pT1, * pT2;
    int nStatesNd, fStateIsNd;
    DdManager * dd = pAut->pMan->dd;
    DdNode * bCube, * bCondT1, * bCondT2;

    if ( pAut->nStates == 0 )
    {
        printf( "Trying to check for non-determinism an automaton with no states.\n" );
        return 0;
    }

    bCube = Aut_AutoGetCubeOutput( pAut, nInputs );  Cudd_Ref( bCube );

    // try all the matching transitions from the two states
    nStatesNd = 0;
    Aut_AutoForEachState_int( pAut, pS )
    {
        fStateIsNd = 0;
        Aut_StateForEachTransitionFrom_int( pS, pT1 )
        {
            bCondT1 = Cudd_bddExistAbstract( dd, pT1->bCond, bCube ); Cudd_Ref( bCondT1 );
            Aut_StateForEachTransitionFromStart_int( pT1->pNextFrom, pT2 )
            {
                bCondT2 = Cudd_bddExistAbstract( dd, pT2->bCond, bCube ); Cudd_Ref( bCondT2 );
                // check if the conditions overlap
                if ( !Cudd_bddLeq( dd, bCondT1, Cudd_Not(bCondT2) ) )
                {
                    Cudd_RecursiveDeref( dd, bCondT2 );
                    if ( fVerbose )
                    {
                        printf( "ND state %s: Transitions to states %s and %s overlap.\n",
                            pS->pName, pT1->pTo->pName, pT2->pTo->pName );
                    }
                    fStateIsNd = 1;
                    break;
                }
                else
                    Cudd_RecursiveDeref( dd, bCondT2 );
            }
            Cudd_RecursiveDeref( dd, bCondT1 );
            if ( fStateIsNd )
                break;
        }
        // increment the counter of ND states
        nStatesNd += fStateIsNd;
    }
    Cudd_RecursiveDeref( dd, bCube );
    return nStatesNd;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


