/**CFile****************************************************************

  FileName    [mioFunc.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [File reading/writing for technology mapping.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 8, 2003.]

  Revision    [$Id: mioFunc.c,v 1.5 2005/01/23 06:59:45 alanmi Exp $]

***********************************************************************/

#include "mioInt.h"
#include "parse.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// these symbols (and no other) can appear in the formulas
#define MIO_SYMB_AND    '*'
#define MIO_SYMB_OR     '+'
#define MIO_SYMB_NOT    '!'
#define MIO_SYMB_AFTNOT '\''
#define MIO_SYMB_OPEN   '('
#define MIO_SYMB_CLOSE  ')'

static int Mio_GateParseFormula( Mio_Gate_t * pGate );
static int Mio_GateCollectNames( char * pFormula, char * pPinNames[] );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Deriving the functionality of the gates.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mio_LibraryParseFormulas( Mio_Library_t * pLib )
{
    Mio_Gate_t * pGate;

    // count the gates
    pLib->nGates = 0;
    Mio_LibraryForEachGate( pLib, pGate )
        pLib->nGates++;        

    // start a temporary BDD manager
    pLib->dd = Cudd_Init( 10, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
    // introduce ZDD variables
    Cudd_zddVarsFromBddVars( pLib->dd, 2 );

    // for each gate, derive its function
    Mio_LibraryForEachGate( pLib, pGate )
        if ( Mio_GateParseFormula( pGate ) )
            return 1;
    return 0;
}


/**Function*************************************************************

  Synopsis    [Deriving the functionality of the gates.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mio_GateParseFormula( Mio_Gate_t * pGate )
{
    DdManager * dd = pGate->pLib->dd;
    Fnc_Manager_t * pMan = pGate->pLib->pMan; 
    Vmx_VarMap_t * pVmx;
    char * pPinNames[100];
    char * pPinNamesCopy[100];
    Mio_Pin_t * pPin, ** ppPin;
    int nPins, iPin, i;

    // set the maximum delay of the gate; count pins
    pGate->dDelayMax = 0.0;
    nPins = 0;
    Mio_GateForEachPin( pGate, pPin )
    {
        // set the maximum delay of the gate
        if ( pGate->dDelayMax < pPin->dDelayBlockMax )
            pGate->dDelayMax = pPin->dDelayBlockMax;
        // count the pin
        nPins++;
    }

    // check for the gate with const function
    if ( nPins == 0 )
    {
        if ( strcmp( pGate->pForm, MIO_STRING_CONST0 ) == 0 )
            pGate->bFunc = b0;
        else if ( strcmp( pGate->pForm, MIO_STRING_CONST1 ) == 0 )
            pGate->bFunc = b1;
        else
        {
            printf( "Cannot read formula \"%s\" of gate \"%s\".\n", pGate->pForm, pGate->pName );
            return 1;
        }
        Cudd_Ref( pGate->bFunc );
        return 0;
    }

    // collect the names as they appear in the formula
    nPins = Mio_GateCollectNames( pGate->pForm, pPinNames );
    if ( nPins == 0 )
    {
        printf( "Cannot read formula \"%s\" of gate \"%s\".\n", pGate->pForm, pGate->pName );
        return 1;
    }

    // set the number of inputs
    pGate->nInputs = nPins;

    // consider the case when all the pins have identical pin info
    if ( strcmp( pGate->pPins->pName, "*" ) == 0 )
    {
        // get the topmost (generic) pin
        pPin = pGate->pPins;
        FREE( pPin->pName );

        // create individual pins from the generic pin
        ppPin = &pPin->pNext;
        for ( i = 1; i < nPins; i++ )
        {
            // get the new pin
            *ppPin = Mio_PinDup( pPin );
            // set its name
            (*ppPin)->pName = pPinNames[i];
            // prepare the next place in the list
            ppPin = &((*ppPin)->pNext);
        }
        *ppPin = NULL;

        // set the name of the topmost pin
        pPin->pName = pPinNames[0];
    }
    else
    {
        // reorder the variable names to appear the save way as the pins
        iPin = 0;
        Mio_GateForEachPin( pGate, pPin )
        {
            // find the pin with the name pPin->pName
            for ( i = 0; i < nPins; i++ )
            {
                if ( pPinNames[i] && strcmp( pPinNames[i], pPin->pName ) == 0 )
                {
                    // free pPinNames[i] because it is already available as pPin->pName
                    // setting pPinNames[i] to NULL is useful to make sure that
                    // this name is not assigned to two pins in the list
                    FREE( pPinNames[i] );
                    pPinNamesCopy[iPin++] = pPin->pName;
                    break;
                }
                if ( i == nPins )
                {
                    printf( "Cannot find pin name \"%s\" in the formula \"%s\" of gate \"%s\".\n", 
                        pPin->pName, pGate->pForm, pGate->pName );
                    return 1;
                }
            }
        }

        // check for the remaining names
        for ( i = 0; i < nPins; i++ )
            if ( pPinNames[i] )
            {
                printf( "Name \"%s\" appears in the formula \"%s\" of gate \"%s\" but there is no such pin.\n", 
                    pPinNames[i], pGate->pForm, pGate->pName );
                return 1;
            }

        // copy the names back
        memcpy( pPinNames, pPinNamesCopy, nPins * sizeof(char *) );
    }

    // expand the manager if necessary
    if ( dd->size < nPins )
    {
        Cudd_Quit( dd );
        dd = Cudd_Init( nPins + 10, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
        Cudd_zddVarsFromBddVars( dd, 2 );
    }

    // derive the formula as the BDD
    pGate->bFunc = Parse_FormulaParser( stdout, pGate->pForm, nPins, 0, pPinNames, dd, dd->vars );
    Cudd_Ref( pGate->bFunc );

    // derive the cover (SOP)
    pVmx = Vmx_VarMapCreateBinary( Fnc_ManagerReadManVm(pMan), Fnc_ManagerReadManVmx(pMan), nPins, 0 );
    pGate->pMvc  = Fnc_FunctionDeriveSopFromMddSpecial( Fnc_ManagerReadManMvc(pMan), dd, pGate->bFunc, pGate->bFunc, pVmx, nPins );
    pGate->pMvcC = Fnc_FunctionDeriveSopFromMddSpecial( Fnc_ManagerReadManMvc(pMan), dd, Cudd_Not(pGate->bFunc), Cudd_Not(pGate->bFunc), pVmx, nPins );
    return 0;
}

/**Function*************************************************************

  Synopsis    [Collect the pin names in the formula.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mio_GateCollectNames( char * pFormula, char * pPinNames[] )
{
    char Buffer[1000];
    char * pTemp;
    int nPins, i;

    // save the formula as it was
    strcpy( Buffer, pFormula );

    // remove the non-name symbols
    for ( pTemp = Buffer; *pTemp; pTemp++ )
        if ( *pTemp == MIO_SYMB_AND  || *pTemp == MIO_SYMB_OR || *pTemp == MIO_SYMB_NOT
          || *pTemp == MIO_SYMB_OPEN || *pTemp == MIO_SYMB_CLOSE || *pTemp == MIO_SYMB_AFTNOT )
            *pTemp = ' ';

    // save the names
    nPins = 0;
    pTemp = strtok( Buffer, " " );
    while ( pTemp )
    {
        for ( i = 0; i < nPins; i++ )
            if ( strcmp( pTemp, pPinNames[i] ) == 0 )
                break;
        if ( i == nPins )
        { // cannot find this name; save it
            pPinNames[nPins++] = util_strsav(pTemp);
        }
        // get the next name
        pTemp = strtok( NULL, " " );
    }
    return nPins;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


