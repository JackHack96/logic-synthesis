/**CFile****************************************************************

  FileName    [autSupp.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Various procedures working with IO variables of automata.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: autiSupp.c,v 1.2 2005/06/02 03:34:20 alanmi Exp $]

***********************************************************************/

#include "autiInt.h"

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
int Auti_AutoSupport( Aut_Auto_t * pAut, char * pInputOrder )
{
    Aut_State_t * pState;
    Aut_Trans_t * pTrans;
    Vm_VarMap_t * pVm;
    DdManager * dd = Aut_AutoReadDd(pAut);
    DdNode * bCube, * bCubeTot, * bTemp;
    Vm_VarMap_t * pVmNew;
    Vmx_VarMap_t * pVmxNew;
    int * pValuesNew, * pVarsRem;
    char ** pInputNames, * pTemp;
    int nInputNames, nVars, i, k;
    int nValues1, nValues2, nBitsAdd;
    int Length1, Length2;
    int nVarsOld;
    Aut_Var_t ** pVarsOld;

    if ( pAut->nStates == 0 )
    {
        printf( "Trying to change support of an automaton with no states.\n" );
        return 1;
    }


    // count the number of variables in the input order
    nVars = 1;
    for ( pTemp = pInputOrder; *pTemp; pTemp++ )
        if ( *pTemp == ',' )
            nVars++;

    // allocate storage for variable associated info
    pInputNames = ALLOC( char *, nVars + 100 );
    pValuesNew  = ALLOC( int , nVars + 100 );
    pVarsRem    = ALLOC( int , nVars + 100 );


    // parse the input order
    pTemp = strtok( pInputOrder, "," );
    nInputNames = 0;
    while ( pTemp )
    {
        pInputNames[nInputNames++] = pTemp;
        pTemp = strtok( NULL, "," );
    }

    // extract the number of values for each var
    for ( i = 0; i < nInputNames; i++ )
    {
        Length1 = strlen( pInputNames[i] );
        pTemp   = strtok( pInputNames[i], "(" );
        Length2 = strlen( pTemp );
        if ( Length1 == Length2 ) // the same string
            pValuesNew[i] = 2;
        else
        {
            pTemp = strtok( NULL, ")" );
            pValuesNew[i] = atoi(pTemp);
            if ( pValuesNew[i] < 2 || pValuesNew[i] > 32 )
            {
                printf( "The number of values of variables \"%s\" is bad (%d)\n", 
                    pInputNames[i], pValuesNew[i] );
                FREE( pInputNames );
                FREE( pValuesNew );
                FREE( pVarsRem );
                return 0;
            }
        }        
    }

    // check whether it is true that the values of the variables coincide
    pVm = Vmx_VarMapReadVm( pAut->pVmx );
    for ( i = 0; i < pAut->nVars; i++ )
    {
        for ( k = 0; k < nInputNames; k++ )
            if ( strcmp( pInputNames[k], pAut->pVars[i]->pName ) == 0 )
                break;
        if ( k != nInputNames ) // found in the order
        {
            nValues1 = Vm_VarMapReadValues( pVm, i );
            nValues2 = pValuesNew[k];
            if ( nValues1 != nValues2 )
            {
                printf( "The number of values of variable \"%s\" in the automaton (%d)\n", pAut->pVars[i]->pName, nValues1 );
                printf( "differs from the number of its values in the input order (%d)\n", nValues2 );
                FREE( pInputNames );
                FREE( pValuesNew );
                FREE( pVarsRem );
                return 0;
            }
        }
    }

    // go through the automaton inputs and find those that do not appear in the new order
    bCubeTot = b1;  Cudd_Ref( bCubeTot );
    for ( i = 0; i < pAut->nVars; i++ )
    {
        for ( k = 0; k < nInputNames; k++ )
            if ( strcmp( pInputNames[k], pAut->pVars[i]->pName ) == 0 )
                break;
        if ( k == nInputNames ) // NOT found in the order
        { 
            // add this variable to the quantification cube
            bCube = Vmx_VarMapCharCube( dd, pAut->pVmx, i );         Cudd_Ref( bCube );
            bCubeTot = Cudd_bddAnd( dd, bTemp = bCubeTot, bCube );   Cudd_Ref( bCubeTot );
            Cudd_RecursiveDeref( dd, bTemp );
            Cudd_RecursiveDeref( dd, bCube );
        }
    }

    // remove the varibles in the cube from the conditions
    Aut_AutoDerefSumCond( pAut );
    Aut_AutoDerefSumCondI( pAut );

    // go through the transitions and reduce the condition
    Aut_AutoForEachState_int( pAut, pState )
        Aut_StateForEachTransitionFrom_int( pState, pTrans )
        {
            pTrans->bCond = Cudd_bddExistAbstract( dd, bTemp = pTrans->bCond, bCubeTot );  
            Cudd_Ref( pTrans->bCond );
            Cudd_RecursiveDeref( dd, bTemp );
        }
    Cudd_RecursiveDeref( dd, bCubeTot );

 
    // create the tranformation array for the new variable map
    nBitsAdd = 0;
    for ( k = 0; k < nInputNames; k++ )
    {
        for ( i = 0; i < pAut->nVars; i++ )
            if ( strcmp( pInputNames[k], pAut->pVars[i]->pName ) == 0 )
                break;
        if ( i != pAut->nVars ) // var is found
            pVarsRem[k] = i;
        else // var is not found
        {
            pVarsRem[k] = -1;
            nBitsAdd += Extra_Base2Log(pValuesNew[k]);
        }
    }
    // expand the manager
    for ( i = 0; i < nBitsAdd; i++ )
        Cudd_bddNewVar( dd );

    // create the new VM
    pVmNew  = Vm_VarMapLookup( Vm_VarMapReadMan(pVm), nInputNames, 0, pValuesNew );
    pVmxNew = Vmx_VarMapCreateExpanded( pAut->pVmx, pVmNew, pVarsRem );
    pAut->pVmx = pVmxNew;

    // update the inputs of the old automaton
    // save the old vars
    nVarsOld = pAut->nVars;
    pVarsOld = pAut->pVars;

    // set the new inputs
    pAut->nVars = nInputNames;
    pAut->pVars = ALLOC( Aut_Var_t *, nInputNames );
    for ( k = 0; k < nInputNames; k++ )
    {
        if ( pVarsRem[k] >= 0 )
        {
            assert( pVarsRem[k] < nVarsOld );
            pAut->pVars[k] = Aut_VarDup( pVarsOld[ pVarsRem[k] ] );
            assert( pAut->pVars[k]->nValues == pValuesNew[k] );
            assert( strcmp( pAut->pVars[k]->pName, pInputNames[k] ) == 0 );
        }
        else
        {
            pAut->pVars[k] = Aut_VarAlloc();
            pAut->pVars[k]->pName = util_strsav( pInputNames[k] );
            pAut->pVars[k]->nValues = pValuesNew[k];
        }
    }

    // remove the old vars
    for ( i = 0; i < nVarsOld; i++ )
        Aut_VarFree( pVarsOld[i] );
    FREE( pVarsOld );

    FREE( pInputNames );
    FREE( pValuesNew );
    FREE( pVarsRem );
    return 1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


