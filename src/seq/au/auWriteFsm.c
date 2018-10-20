/**CFile****************************************************************

  FileName    [auWriteFsm.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Procedures to convert the automaton into an FSM.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: auWriteFsm.c,v 1.1 2004/02/19 03:06:49 alanmi Exp $]

***********************************************************************/

#include "auInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////


/**Function*************************************************************

  Synopsis    [Writes the automaton as an FSM.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Au_AutoWriteKissFsm( Au_Auto_t * pAut, char * FileName, int nInputs )
{
    FILE * pFile;
    Mvc_Cube_t * pCube;
    Au_Trans_t * pTrans;
    int Value0, Value1, i, v;
    int nCubes, nLength;

    if ( nInputs < 1 )
    {
        fprintf( stdout, "Cannot write the FSM without inputs.\n" );
        return;
    }
    if ( nInputs >= pAut->nInputs )
    {
        fprintf( stdout, "Cannot write the FSM without outputs.\n" );
        return;
    }

    // check the presence of non-accepting states
    for ( i = 0; i < pAut->nStates; i++ )
        if ( !pAut->pStates[i]->fAcc )
        {
            fprintf( stdout, "Warning: The non-accepting states of the automaton are also written as the FSM states.\n" );
            break;
        }



    // count the number of cubes in all transitions
    nCubes = 0;
    for ( i = 0; i < pAut->nStates; i++ )
        Au_StateForEachTransition( pAut->pStates[i], pTrans )
            nCubes += Mvc_CoverReadCubeNum( pTrans->pCond );

    pFile = fopen( FileName, "w" );
    fprintf( pFile, ".i %d\n", nInputs );
    fprintf( pFile, ".o %d\n", pAut->nInputs - nInputs );
    fprintf( pFile, ".s %d\n", pAut->nStates );
    fprintf( pFile, ".p %d\n", nCubes );

    fprintf( pFile, ".ilb" );
    for ( i = 0; i < nInputs; i++ )
        fprintf( pFile, " %s", pAut->ppNamesInput[i] );
    fprintf( pFile, "\n" );

    fprintf( pFile, ".ob" );
    for ( ; i < pAut->nInputs; i++ )
        fprintf( pFile, " %s", pAut->ppNamesInput[i] );
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
//Mvc_CoverPrint( pTrans->pCond );
            Mvc_CoverForEachCube( pTrans->pCond, pCube )
            {
                for ( v = 0; v < nInputs; v++ )
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
                fprintf( pFile, " " );
                for ( v = nInputs; v < pAut->nInputs; v++ )
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


