/**CFile****************************************************************

  FileName    [mvcPrint.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Printing cubes and covers.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: mvcPrint.c,v 1.9 2005/01/23 07:11:44 alanmi Exp $]

***********************************************************************/

#include "mvc.h"
#include "vm.h"
#include "vmInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void Mvc_CubePrintBinary( Mvc_Cover_t * pCover, Mvc_Cube_t * pCube );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvc_CoverPrint( Mvc_Cover_t * pCover )
{
    Mvc_Cube_t * pCube;
    int i;
    // print general statistics
    printf( "The cover contains %d cubes (%d bits and %d words)\n", 
        pCover->lCubes.nItems, pCover->nBits, pCover->nWords );
    // iterate through the cubes
    Mvc_CoverForEachCube( pCover, pCube )
        Mvc_CubePrint( pCover, pCube );

    if ( pCover->pLits )
    {
        for ( i = 0; i < pCover->nBits; i++ )
            printf( " %d", pCover->pLits[i] );
        printf( "\n" ); 
    }
    printf( "End of cover printout\n" ); 
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvc_CubePrint( Mvc_Cover_t * pCover, Mvc_Cube_t * pCube )
{
    int iBit, Value;
    // iterate through the literals
//    printf( "Size = %2d   ", Mvc_CubeReadSize(pCube) );
    Mvc_CubeForEachBit( pCover, pCube, iBit, Value )
        printf( "%c", '0' + Value );
    printf( "\n" );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvc_CoverPrintBinary( Mvc_Cover_t * pCover )
{
    Mvc_Cube_t * pCube;
    int i;
    // print general statistics
    printf( "The cover contains %d cubes (%d bits and %d words)\n", 
        pCover->lCubes.nItems, pCover->nBits, pCover->nWords );
    // iterate through the cubes
    Mvc_CoverForEachCube( pCover, pCube )
        Mvc_CubePrintBinary( pCover, pCube );

    if ( pCover->pLits )
    {
        for ( i = 0; i < pCover->nBits; i++ )
            printf( " %d", pCover->pLits[i] );
        printf( "\n" ); 
    }
    printf( "End of cover printout\n" ); 
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvc_CubePrintBinary( Mvc_Cover_t * pCover, Mvc_Cube_t * pCube )
{
    int iVar, Value;
    // iterate through the literals
//    printf( "Size = %2d   ", Mvc_CubeReadSize(pCube) );
    Mvc_CubeForEachVarValue( pCover, pCube, iVar, Value )
    {
        assert( Value != 0 );
        if ( Value == 3 )
            printf( "-" );
        else if ( Value == 1 )
            printf( "0" );
        else 
            printf( "1" );
    }
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvc_CoverPrintMv( Mvc_Data_t * pData, Mvc_Cover_t * pCover )
{
    Mvc_Cube_t * pCube;
    int i;
    // print general statistics
    printf( "The cover contains %d cubes (%d bits and %d words)\n", 
        pCover->lCubes.nItems, pCover->nBits, pCover->nWords );
    // iterate through the cubes
    Mvc_CoverForEachCube( pCover, pCube )
        Mvc_CubePrintMv( pData, pCover, pCube );

    if ( pCover->pLits )
    {
        for ( i = 0; i < pCover->nBits; i++ )
            printf( " %d", pCover->pLits[i] );
        printf( "\n" ); 
    }
    printf( "End of cover printout\n" ); 
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvc_CubePrintMv( Mvc_Data_t * pData, Mvc_Cover_t * pCover, Mvc_Cube_t * pCube )
{
    int iLit, iVar;
    // iterate through the literals
    printf( "Size = %2d   ", Mvc_CubeReadSize(pCube) );
    iVar = 0;
    for ( iLit = 0; iLit < pData->pVm->nValuesIn; iLit++ )
    {
        if ( iLit == pData->pVm->pValuesFirst[iVar+1] )
        {
            printf( " " );
            iVar++;
        }
        if ( Mvc_CubeBitValue( pCube, iLit ) )
            printf( "%c", '0' + iLit - pData->pVm->pValuesFirst[iVar] );
        else
            printf( "-" );
    }
    printf( "\n" );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


