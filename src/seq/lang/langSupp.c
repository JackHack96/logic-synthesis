/**CFile****************************************************************

  FileName    [langSupp.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Implicit language solving flow.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: langSupp.c,v 1.4 2004/02/19 03:06:55 alanmi Exp $]

***********************************************************************/

#include "langInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Puts the automaton on a different support.]

  Description [Returns 1 if successful; 0 otherwise.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Lang_AutoSupport( Lang_Auto_t * pAut, char * pInputOrder )
{
    Vm_VarMap_t * pVm;
    DdManager * dd = pAut->dd;
    DdNode * bCube, * bCubeTot, * bTemp;
    Vm_VarMap_t * pVmNew;
    Vmx_VarMap_t * pVmxNew;
    int * pValuesNew, * pVarsRem;
    char ** pInputNames, * pTemp;
    int nInputNames, nVars, i, k;
    int nValues1, nValues2, nBitsAdd;
    int Length1, Length2;

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
        pTemp   = strtok( pInputNames[i], "=" );
        Length2 = strlen( pTemp );
        if ( Length1 == Length2 ) // the same string
            pValuesNew[i] = 2;
        else
        {
            pTemp = strtok( NULL, "=" );
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
    // add the current state and next state variables
    pVm = Vmx_VarMapReadVm( pAut->pVmx );
    for ( i = 0; i < 2 * pAut->nLatches; i++ )
        pValuesNew[nInputNames+i] = Vm_VarMapReadValues( pVm, pAut->nInputs + i );

    // check whether it is true that the values of the variables coincide
    for ( i = 0; i < pAut->nInputs; i++ )
    {
        for ( k = 0; k < nInputNames; k++ )
            if ( strcmp( pInputNames[k], pAut->ppNamesInput[i] ) == 0 )
                break;
        if ( k != nInputNames ) // found in the order
        {
            nValues1 = Vm_VarMapReadValues( Vmx_VarMapReadVm(pAut->pVmx), i );
            nValues2 = pValuesNew[k];
            if ( nValues1 != nValues2 )
            {
                printf( "The number of values of variables \"%s\" in the automaton (%d)\n", pAut->ppNamesInput[i], nValues1 );
                printf( "differs from the number of its values in the input order (%d)\n", nValues2 );
                FREE( pInputNames );
                FREE( pValuesNew );
                FREE( pVarsRem );
                return 0;
            }
        }
    }

    // go through the automaton inputs and find those that do not appear in the order
    bCubeTot = b1;  Cudd_Ref( bCubeTot );
    for ( i = 0; i < pAut->nInputs; i++ )
    {
        for ( k = 0; k < nInputNames; k++ )
            if ( strcmp( pInputNames[k], pAut->ppNamesInput[i] ) == 0 )
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

    // remove the varibles in the cube from the relation
    pAut->bRel = Cudd_bddExistAbstract( dd, bTemp = pAut->bRel, bCubeTot );  Cudd_Ref( pAut->bRel );
    Cudd_RecursiveDeref( dd, bTemp );
    Cudd_RecursiveDeref( dd, bCubeTot );


    // create the tranformation array for the new variable map
    nBitsAdd = 0;
    for ( k = 0; k < nInputNames; k++ )
    {
        for ( i = 0; i < pAut->nInputs; i++ )
            if ( strcmp( pInputNames[k], pAut->ppNamesInput[i] ) == 0 )
                break;
        if ( i != pAut->nInputs ) // var is found
            pVarsRem[k] = i;
        else // var is not found
        {
            pVarsRem[k] = -1;
            nBitsAdd += Extra_Base2Log(pValuesNew[i]);
        }
    }
    // add the latch variables
    for ( i = 0; i < 2 * pAut->nLatches; i++ )
        pVarsRem[nInputNames+i] = pAut->nInputs + i;

    // expand the manager
    for ( i = 0; i < nBitsAdd; i++ )
        Cudd_bddNewVar( pAut->dd );

    // create the new VM
    pVmNew  = Vm_VarMapLookup( Fnc_ManagerReadManVm(pAut->pMan), nInputNames, 2 * pAut->nLatches, pValuesNew );
    pVmxNew = Vmx_VarMapCreateExpanded( pAut->pVmx, pVmNew, pVarsRem );
    pAut->pVmx = pVmxNew;

    // update the inputs of the old automaton
    for ( i = 0; i < pAut->nInputs; i++ )
        FREE( pAut->ppNamesInput[i] );
    FREE( pAut->ppNamesInput );

    // set the new inputs
    pAut->nInputs = nInputNames;
    pAut->ppNamesInput = ALLOC( char *, nInputNames );
    for ( i = 0; i < nInputNames; i++ )
        pAut->ppNamesInput[i] = util_strsav( pInputNames[i] );

    FREE( pInputNames );
    FREE( pValuesNew );
    FREE( pVarsRem );

    Lang_AutoVerifySupport( pAut );
    return 1;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


