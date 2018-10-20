/**CFile****************************************************************

  FileName    [auSupp.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Various procedures working with IO variables of automata.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: auSupport.c,v 1.1 2004/02/19 03:06:48 alanmi Exp $]

***********************************************************************/

#include "auInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Puts the automaton on a different support.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Au_AutoSupport( Au_Auto_t * pAut, char * pInputOrder )
{
    Au_Trans_t * pTrans;
    Mvc_Cover_t * pMvc;
    char * pInputNames[100];
    char * pTemp;
    int pVarsRem[100];
    int nVarsRem;
    int nInputNames, i, k;

    // parse the input order
    pTemp = strtok( pInputOrder, "," );
    nInputNames = 0;
    while ( pTemp )
    {
        pInputNames[nInputNames++] = pTemp;
        pTemp = strtok( NULL, "," );
    }

    // create the permutation
    nVarsRem = 0;
    for ( k = 0; k < nInputNames; k++ )
    {
        for ( i = 0; i < pAut->nInputs; i++ )
            if ( strcmp( pInputNames[k], pAut->ppNamesInput[i] ) == 0 )
            { // this var is found
                pVarsRem[ nVarsRem++ ] = 2*i;
                pVarsRem[ nVarsRem++ ] = 2*i + 1;
                break;
            }
        if ( i == pAut->nInputs )
        {
            pVarsRem[ nVarsRem++ ] = -1;
            pVarsRem[ nVarsRem++ ] = -1;
        }
    }

    // update the inputs
    for ( i = 0; i < pAut->nInputs; i++ )
        FREE( pAut->ppNamesInput[i] );
    FREE( pAut->ppNamesInput );

    pAut->nInputs = nInputNames;
    pAut->ppNamesInput = ALLOC( char *, nInputNames );
    for ( i = 0; i < nInputNames; i++ )
        pAut->ppNamesInput[i] = util_strsav( pInputNames[i] );

    // remap all the conditions
    for ( i = 0; i < pAut->nStates; i++ )
    {
        Au_StateForEachTransition( pAut->pStates[i], pTrans )
        {
            pTrans->pCond = Mvc_CoverRemap( pMvc = pTrans->pCond, pVarsRem, nVarsRem );
            Mvc_CoverContain( pTrans->pCond );
            Mvc_CoverFree( pMvc );
        }
    }
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Au_AutoRaise( Au_Auto_t * pAut, char * pInputOrder )
{
    Au_Trans_t * pTrans;
    Mvc_Cover_t * pMvc;
    char * pInputNames[100];
    char * pTemp;
    int pVarsRem[100];
    int nVarsRem;
    int nInputNames, i, k;

    // parse the input order
    pTemp = strtok( pInputOrder, "," );
    nInputNames = 0;
    while ( pTemp )
    {
        pInputNames[nInputNames++] = pTemp;
        pTemp = strtok( NULL, "," );
    }

    // make sure that every old var is encountered in the input order
    for ( i = 0; i < pAut->nInputs; i++ )
    {
        for ( k = 0; k < nInputNames; k++ )
            if ( strcmp( pAut->ppNamesInput[i], pInputNames[k] ) == 0 )
                break;
        if ( k == nInputNames )
        {
            printf( "Cannot find the input variable \"%s\" in the given order.\n", pAut->ppNamesInput[i] );
            return;
        }
    }

    // create the permutation
    nVarsRem = 0;
    for ( k = 0; k < nInputNames; k++ )
    {
        for ( i = 0; i < pAut->nInputs; i++ )
            if ( strcmp( pInputNames[k], pAut->ppNamesInput[i] ) == 0 )
            { // this var is found
                pVarsRem[ nVarsRem++ ] = 2*i;
                pVarsRem[ nVarsRem++ ] = 2*i + 1;
                break;
            }
        if ( i == pAut->nInputs )
        {
            pVarsRem[ nVarsRem++ ] = -1;
            pVarsRem[ nVarsRem++ ] = -1;
        }
    }

    // update the inputs
    for ( i = 0; i < pAut->nInputs; i++ )
        FREE( pAut->ppNamesInput[i] );
    FREE( pAut->ppNamesInput );

    pAut->nInputs = nInputNames;
    pAut->ppNamesInput = ALLOC( char *, nInputNames );
    for ( i = 0; i < nInputNames; i++ )
        pAut->ppNamesInput[i] = util_strsav( pInputNames[i] );

    // remap all the conditions
    for ( i = 0; i < pAut->nStates; i++ )
    {
        Au_StateForEachTransition( pAut->pStates[i], pTrans )
        {
            pTrans->pCond = Mvc_CoverRemap( pMvc = pTrans->pCond, pVarsRem, nVarsRem );
            Mvc_CoverFree( pMvc );
        }
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Au_AutoLower( Au_Auto_t * pAut, char * pInputOrder )
{
    Au_Trans_t * pTrans;
    Mvc_Cover_t * pMvc;
    char * pInputNames[100];
    char * pTemp;
    int pVarsRem[100];
    int nVarsRem;
    int nInputNames, i, k;

    // parse the input order
    pTemp = strtok( pInputOrder, "," );
    nInputNames = 0;
    while ( pTemp )
    {
        pInputNames[nInputNames++] = pTemp;
        pTemp = strtok( NULL, "," );
    }

    // make sure that every old var is encountered in the input order
    for ( k = 0; k < nInputNames; k++ )
    {
        for ( i = 0; i < pAut->nInputs; i++ )
            if ( strcmp( pInputNames[k], pAut->ppNamesInput[i] ) == 0 )
                break;
        if ( i == pAut->nInputs )
        {
            printf( "Cannot find the variable \"%s\" among the inputs of the automaton.\n", pInputNames[k] );
            return;
        }
    }

    // create the permutation
    nVarsRem = 0;
    for ( k = 0; k < nInputNames; k++ )
    {
        for ( i = 0; i < pAut->nInputs; i++ )
            if ( strcmp( pInputNames[k], pAut->ppNamesInput[i] ) == 0 )
            { // this var is found
                pVarsRem[ nVarsRem++ ] = 2*i;
                pVarsRem[ nVarsRem++ ] = 2*i + 1;
                break;
            }
    }

    // update the inputs
    for ( i = 0; i < pAut->nInputs; i++ )
        FREE( pAut->ppNamesInput[i] );
    FREE( pAut->ppNamesInput );

    pAut->nInputs = nInputNames;
    pAut->ppNamesInput = ALLOC( char *, nInputNames );
    for ( i = 0; i < nInputNames; i++ )
        pAut->ppNamesInput[i] = util_strsav( pInputNames[i] );

    // remap all the conditions
    for ( i = 0; i < pAut->nStates; i++ )
    {
        Au_StateForEachTransition( pAut->pStates[i], pTrans )
        {
            pTrans->pCond = Mvc_CoverRemap( pMvc = pTrans->pCond, pVarsRem, nVarsRem );
            Mvc_CoverFree( pMvc );
        }
    }
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


