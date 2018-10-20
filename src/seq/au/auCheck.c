/**CFile****************************************************************

  FileName    [auCheck.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Checking equivalence/containment of the automata behaviors.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: auCheck.c,v 1.1 2004/02/19 03:06:44 alanmi Exp $]

***********************************************************************/

#include "auInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define PROD_STATE_HASH(s1,s2)   ((((unsigned)s1) << 16) | (((unsigned)s2) & 0xffff))

typedef struct AutCheckDataStruct Au_CheckData_t;
struct AutCheckDataStruct
{
    Au_Auto_t *  pAut1;
    Au_Auto_t *  pAut2;
    bool          f1Contains2;
    bool          f2Contains1;
    int           nError12;
    int           nError21;
    int           nErrorAlloc;
    int           nTrace;
    int *         pTrace1;
    int *         pTrace2;
    int *         pError12_1;
    int *         pError12_2;
    int *         pError21_1;
    int *         pError21_2;
    st_table *    tStates;
    Mvc_Data_t *  pData;
};

static bool Au_AutoCheckProduct_rec( Au_CheckData_t * p, int s1, int s2 );
static void Au_AutoCheckPrintTrace( FILE * pOut, Au_CheckData_t * p, int fFirst );
static bool Au_AutoCheckProduct12_rec( Au_Auto_t * pAut1, Au_Auto_t * pAut2, int s1, int s2, 
    st_table * tStates, Mvc_Data_t * pData );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Checks the containment of the automata.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Au_AutoCheck( FILE * pOut, Au_Auto_t * pAut1, Au_Auto_t * pAut2, bool fError )
{
    Au_CheckData_t Data, * p = &Data;
    Vm_VarMap_t * pVm;
    Mvc_Data_t * pData;
    Mvc_Cover_t * pCover;

    // check if automata are empty
    if ( pAut1->nStates == 0 )
    {
        fprintf( pOut, "Automaton \"%s\" has no states; checking skipped.\n", pAut1->pName );
        return;
    }
    if ( pAut2->nStates == 0 )
    {
        fprintf( pOut, "Automaton \"%s\" has no states; checking skipped.\n", pAut2->pName );
        return;
    }

    // make sure they have the same number of inputs
    // make sure that the ordering of input names are identical
    if ( !Au_AutoProductCheckInputs( pAut1, pAut2 ) )
        return;

    // check if the automata are deterministic
    if ( Au_AutoCheckNd( stdout, pAut1, pAut1->nInputs, 0 ) > 0 )
    {
        printf( "Automaton \"%s\" is non-deterministic; checking cannot be performed.\n", pAut1->pName );
        return;
    }
    if ( Au_AutoCheckNd( stdout, pAut2, pAut2->nInputs, 0 ) > 0 )
    {
        printf( "Automaton \"%s\" is non-deterministic; checking cannot be performed.\n", pAut2->pName );
        return;
    }

    // check if the automata are complete
    if ( Au_AutoComplete( pAut1, pAut1->nInputs, 0, 0 ) )
        printf( "Warning: Automaton \"%s\" is completed before checking.\n", pAut1->pName );
    if ( Au_AutoComplete( pAut2, pAut2->nInputs, 0, 0 ) )
        printf( "Warning: Automaton \"%s\" is completed before checking.\n", pAut2->pName );

    // check capacity constraints
    if ( pAut1->nStates >= (1<<16) || pAut2->nStates >= (1<<16) )
    {
        fprintf( pOut, "Currently can only check automata with less than %d state.\n", (1<<16) );
        return;
    }

    // create the MVC data to check the intersection of conditions
    pCover = pAut1->pStates[0]->pHead->pCond;
    pVm    = Vm_VarMapCreateBinary( Fnc_ManagerReadManVm(pAut1->pMan), pAut1->nInputs, 0 );
    pData  = Mvc_CoverDataAlloc( pVm, pCover );

    // initialize the data structure
    memset( p, 0, sizeof(Au_CheckData_t) );
    p->pAut1       = pAut1;
    p->pAut2       = pAut2;
    p->f1Contains2 = 1;
    p->f2Contains1 = 1;
    p->nErrorAlloc = 20000;
    p->pTrace1     = ALLOC( int, p->nErrorAlloc );
    p->pTrace2     = ALLOC( int, p->nErrorAlloc );
    p->pError12_1  = ALLOC( int, p->nErrorAlloc );
    p->pError12_2  = ALLOC( int, p->nErrorAlloc );
    p->pError21_1  = ALLOC( int, p->nErrorAlloc );
    p->pError21_2  = ALLOC( int, p->nErrorAlloc );
    p->tStates     = st_init_table(st_numcmp,st_numhash);
    p->pData       = pData;

    // save the initial state as the current state for the traversal
    p->pTrace1[0] = 0;
    p->pTrace2[0] = 0;
    p->nTrace = 1;
    // traverse the product machine and detect non-containment
    Au_AutoCheckProduct_rec( p, 0, 0 );

    // analize the results of traversal
    if ( p->f1Contains2 && p->f2Contains1 )
        fprintf( pOut, "\"%s\" and \"%s\" are sequentially equivalent\n", pAut1->pName, pAut2->pName );
    else if ( p->f1Contains2 )
        fprintf( pOut, "The behavior of \"%s\" contains the behavior of \"%s\".\n", pAut1->pName, pAut2->pName );
    else if ( p->f2Contains1 )
        fprintf( pOut, "The behavior of \"%s\" is contained in the behavior of \"%s\".\n", pAut1->pName, pAut2->pName );
    else 
        fprintf( pOut, "There is no behavior containment among \"%s\" and \"%s\".\n", pAut1->pName, pAut2->pName );

    if ( fError )
    {
        if ( !p->f1Contains2 )
        {
            fprintf( pOut, "The behavior of \"%s\" does not contain the behavior of \"%s\".\n", pAut1->pName, pAut2->pName );
            fprintf( pOut, "Error trace:\n" );
            Au_AutoCheckPrintTrace( pOut, p, 1 );
        }
        if ( !p->f2Contains1 )
        {
            fprintf( pOut, "The behavior of \"%s\" is not contained in the behavior of \"%s\".\n", pAut1->pName, pAut2->pName );
            fprintf( pOut, "Error trace:\n" );
            Au_AutoCheckPrintTrace( pOut, p, 0 );
        }
    }

    FREE( p->pTrace1 );
    FREE( p->pTrace2 );
    FREE( p->pError12_1 );
    FREE( p->pError12_2 );
    FREE( p->pError21_1 );
    FREE( p->pError21_2 );
    st_free_table( p->tStates );
    // remove the MVC data
    Mvc_CoverDataFree( pData, pCover );
}

/**Function*************************************************************

  Synopsis    [Performs the search of product automaton for non-equivalences.]

  Description [Return 1 if both containment checks have failed. Returns 0
  if they are equaivalent or one containment holds.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Au_AutoCheckProduct_rec( Au_CheckData_t * p, int s1, int s2 )
{
    Au_State_t * pS1, * pS2;
    Au_Trans_t * pT1, * pT2;
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
    Hash = PROD_STATE_HASH(s1, s2);
    if ( st_find_or_add( p->tStates, (char *)Hash, NULL ) )
        return 0;

    // check if this state allows us to make some conclusions
    pS1 = p->pAut1->pStates[s1];
    pS2 = p->pAut2->pStates[s2];
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
    Au_StateForEachTransition( pS1, pT1 )
    {
        Au_StateForEachTransition( pS2, pT2 )
        {
            // check if the conditions overlap
            if ( Mvc_CoverIsIntersecting( p->pData, pT1->pCond, pT2->pCond ) )
            {
                // save this state as the current state
                p->pTrace1[ p->nTrace ] = pT1->StateNext;
                p->pTrace2[ p->nTrace ] = pT2->StateNext;
                p->nTrace++;
                // perform the traversal starting from these states
                RetValue = Au_AutoCheckProduct_rec( p, pT1->StateNext, pT2->StateNext );
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
void Au_AutoCheckPrintTrace( FILE * pOut, Au_CheckData_t * p, int fFirst )
{
    Mvc_Cube_t * pCube;
    Au_State_t * pS1, * pS2;
    Au_Trans_t * pT1, * pT2;
    Mvc_Cover_t * pMvc;
    int * pErrorTrace1;
    int * pErrorTrace2;
    int nErrorTrace;
    int i, v, Value0, Value1;

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
        pS1 = p->pAut1->pStates[pErrorTrace1[i]];
        pS2 = p->pAut2->pStates[pErrorTrace2[i]];

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
        Au_StateForEachTransition( pS1, pT1 )
            if ( pT1->StateNext == pErrorTrace1[i+1] )
                break;
        Au_StateForEachTransition( pS2, pT2 )
            if ( pT2->StateNext == pErrorTrace2[i+1] )
                break;
        assert( pT1 && pT2 );

        // get the intersection of the conditions
        pMvc = Mvc_CoverBooleanAnd( p->pData, pT1->pCond, pT2->pCond );
        // one cube from the condition is anough for the error trace
        pCube = Mvc_CoverReadCubeHead(pMvc);
        for ( v = 0; v < p->pAut1->nInputs; v++ )
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
        fprintf( pOut, "\n" );
        Mvc_CoverFree( pMvc );
    }
}



/**Function*************************************************************

  Synopsis    [Checks the containment of Aut1 in Aut2.]

  Description [Returns 0 if Aut2 contains Aut1; 1 otherwise.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Au_AutoCheckProduct12( Au_Auto_t * pAut1, Au_Auto_t * pAut2 )
{
    Mvc_Cover_t * pCover;
    Vm_VarMap_t * pVm;
    Mvc_Data_t * pData;
    st_table * tStates;
    int RetValue;

    // create the MVC data to check the intersection of conditions
    pCover = pAut1->pStates[0]->pHead->pCond;
    pVm    = Vm_VarMapCreateBinary( Fnc_ManagerReadManVm(pAut1->pMan), pAut1->nInputs, 0 );
    pData  = Mvc_CoverDataAlloc( pVm, pCover );
    tStates = st_init_table(st_numcmp,st_numhash);

    // make sure they have the same number of inputs
    // make sure that the ordering of input names are identical
    if ( !Au_AutoProductCheckInputs( pAut1, pAut2 ) )
        return 1;

    RetValue = Au_AutoCheckProduct12_rec( pAut1, pAut2, 0, 0, tStates, pData );

    // remove the MVC data
    Mvc_CoverDataFree( pData, pCover );
    st_free_table( tStates );

    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Checks the containment of Aut1 in Aut2.]

  Description [Returns 0 if Aut2 contains Aut1; 1 otherwise.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Au_AutoCheckProduct12_rec( Au_Auto_t * pAut1, Au_Auto_t * pAut2, int s1, int s2, 
    st_table * tStates, Mvc_Data_t * pData )
{
    Au_State_t * pS1, * pS2;
    Au_Trans_t * pT1, * pT2;
    unsigned Hash;

    // check if this state has been reached
    Hash = PROD_STATE_HASH(s1, s2);
    if ( st_find_or_add( tStates, (char *)Hash, NULL ) )
        return 0;

    // check if this state allows us to make some conclusions
    pS1 = pAut1->pStates[s1];
    pS2 = pAut2->pStates[s2];
    if ( pS1->fAcc && !pS2->fAcc )
        return 1;

    // try all the matching transitions from the two states
    Au_StateForEachTransition( pS1, pT1 )
    {
        Au_StateForEachTransition( pS2, pT2 )
        {
            // check if the conditions overlap
            if ( Mvc_CoverIsIntersecting( pData, pT1->pCond, pT2->pCond ) )
            {
                if ( Au_AutoCheckProduct12_rec( pAut1, pAut2, pT1->StateNext, pT2->StateNext, tStates, pData ) )
                    return 1;
            }
        }
    }
    return 0;
}



/**Function*************************************************************

  Synopsis    [Returns the number of non-deterministic states.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Au_AutoCheckNd( FILE * pOut, Au_Auto_t * pAut, int nInputs, int fVerbose )
{
    Vm_VarMap_t * pVm;
    Mvc_Data_t * pData;
    Mvc_Cover_t * pCover;
    Au_State_t * pS;
    Au_Trans_t * pT1, * pT2;
    int nStatesNd, fStateIsNd, s;

    if ( pAut->nStates == 0 )
    {
        printf( "Trying to check for non-determinism an automaton with no states.\n" );
        return 0;
    }

    // create the MVC data to check the intersection of conditions
    pCover = pAut->pStates[0]->pHead->pCond;
    pVm    = Vm_VarMapCreateBinary( Fnc_ManagerReadManVm(pAut->pMan), nInputs, 0 );
    pData  = Mvc_CoverDataAlloc( pVm, pCover );


    // try all the matching transitions from the two states
    nStatesNd = 0;
    for ( s = 0; s < pAut->nStates; s++ )
    {
        fStateIsNd = 0;
        pS = pAut->pStates[s];
        Au_StateForEachTransition( pS, pT1 )
        {
            Au_StateForEachTransitionStart( pT1->pNext, pT2 )
            {
                // check if the conditions overlap
                if ( Mvc_CoverIsIntersecting( pData, pT1->pCond, pT2->pCond ) )
                {
                    if ( fVerbose )
                    {
                        printf( "ND state %s: Transitions to states %s and %s overlap.\n",
                            pS->pName, 
                            pAut->pStates[pT1->StateNext]->pName, 
                            pAut->pStates[pT2->StateNext]->pName );
                    }
                    fStateIsNd = 1;
                    break;
                }
            }
            if ( fStateIsNd )
                break;
        }
        // increment the counter of ND states
        nStatesNd += fStateIsNd;
    }

    // remove the MVC data
    Mvc_CoverDataFree( pData, pCover );
    return nStatesNd;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


