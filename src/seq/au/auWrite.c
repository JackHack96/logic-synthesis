/**CFile****************************************************************

  FileName    [auWrite.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Write the automaton into a KISS file.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: auWrite.c,v 1.1 2004/02/19 03:06:49 alanmi Exp $]

***********************************************************************/

#include "auInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Au_AutoWriteKiss( Au_Auto_t * pAut, char * FileName )
{
    FILE * pFile;
    Mvc_Cube_t * pCube;
    Au_Trans_t * pTrans;
    int Value0, Value1, i, v;
    int nCubes, nLength;

    // count the number of cubes in all transitions
    nCubes = Au_AutoCountProducts( pAut );

    pFile = fopen( FileName, "w" );
    fprintf( pFile, ".i %d\n", pAut->nInputs );
    fprintf( pFile, ".o %d\n", pAut->nOutputs );
    fprintf( pFile, ".s %d\n", pAut->nStates );
    fprintf( pFile, ".p %d\n", nCubes );

    fprintf( pFile, ".ilb" );
    for ( i = 0; i < pAut->nInputs; i++ )
        fprintf( pFile, " %s", pAut->ppNamesInput[i] );
    fprintf( pFile, "\n" );

    fprintf( pFile, ".ob" );
    fprintf( pFile, "\n" );

    fprintf( pFile, ".accepting" );
    for ( i = 0; i < pAut->nStates; i++ )
        if ( pAut->pStates[i]->fAcc )
            fprintf( pFile, " %s", pAut->pStates[i]->pName );
    fprintf( pFile, "\n" );

    // determine the length of the longest state name
    nLength = 0;
    for ( i = 0; i < pAut->nStates; i++ )
        if ( nLength < (int)strlen(pAut->pStates[i]->pName) )
            nLength = strlen(pAut->pStates[i]->pName);

    // write the table
    for ( i = 0; i < pAut->nStates; i++ )
    {
        Au_StateForEachTransition( pAut->pStates[i], pTrans )
        {
            Mvc_CoverForEachCube( pTrans->pCond, pCube )
            {
                for ( v = 0; v < pAut->nInputs; v++ )
                {
                    Value0 = Mvc_CubeBitValue( pCube, 2*v );
                    Value1 = Mvc_CubeBitValue( pCube, 2*v + 1 );
                    if ( Value0 && Value1 )
                        fprintf( pFile, "-" );
                    else if ( Value0 && !Value1 )
                        fprintf( pFile, "0" );
                    else if ( !Value0 && Value1 )
                        fprintf( pFile, "1" );
                    else
                    {
                        assert( 0 );
                    }
                }
                // print the current and the next states
                fprintf( pFile, " %*s", nLength, pAut->pStates[pTrans->StateCur]->pName );
                fprintf( pFile, " %*s", nLength, pAut->pStates[pTrans->StateNext]->pName );
                // print the accepting status of the state
//                fprintf( pFile, "  %d", pAut->pStates[pTrans->StateCur]->fAcc );
                fprintf( pFile, "\n" );
            }
        }
    }
    fprintf( pFile, ".e" );
    fprintf( pFile, "\n" );
    fclose( pFile );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


