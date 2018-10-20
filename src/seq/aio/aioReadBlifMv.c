/**CFile****************************************************************

  FileName    [aioReadBlifMv.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [The top level procedure to read BLIF/BLIF-MVS/BLIF-MV files.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: aioReadBlifMv.c,v 1.1 2004/02/19 03:06:41 alanmi Exp $]

***********************************************************************/

#include "aioInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static Aio_Read_t * Aio_ReadStructAllocate();
static void        Aio_ReadStructDeallocate( Aio_Read_t * p );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Reads the automaton from BLIF-MV file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aut_Auto_t * Aio_AutoReadBlifMv( Mv_Frame_t * pMvsis, char * FileName, Aut_Man_t * pManDd )
{
    Aut_Auto_t * pAut;
    Aio_Read_t * p;

    // allocate the reading structure
    p = Aio_ReadStructAllocate();
    p->FileName = FileName;
    p->Output = Mv_FrameReadErr(pMvsis);
    p->pMvsis = pMvsis;
    p->pManDd = pManDd;

    // read the file contents, remove comments
    if ( Aio_ReadFile( p ) )
        goto failure;

    // split the file into lines and dot-statements
    if ( Aio_ReadFileMarkLinesAndDots( p ) )
        goto failure;

    // read the primary network
    if ( Aio_ReadLines( p ) )
        goto failure;
    pAut = Aio_ReadNetworkStructure( p );
    if ( pAut == NULL )
        goto failure;

    // read the EXDC network
    assert( p->LineExdc == 0 );
    Aio_ReadStructDeallocate( p );
    return pAut;

failure:
    Aio_ReadStructDeallocate( p );
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Creates the reading structure.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aio_Read_t * Aio_ReadStructAllocate()
{
    Aio_Read_t * p;
    p = ALLOC( Aio_Read_t, 1 );
    memset( p, 0, sizeof(Aio_Read_t) );
    p->sError = ALLOC( char, 300 );
    p->aOutputs = array_alloc( Ntk_Node_t *, 50 );
    return p;
}

/**Function*************************************************************

  Synopsis    [Creates the reading structure.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aio_ReadStructAllocateAdditional( Aio_Read_t * p )
{
    FREE( p->pArray );
    FREE( p->pTokens );
    if ( p->pCube )
    {
        // return the cube to the previous size
        p->pCube->iLast   = p->pCover->nWords - 1 + (p->pCover->nWords == 0);
        p->pCube->nUnused = p->pCover->nUnused;
        Mvc_CubeFree( p->pCover, p->pCube );
        Mvc_CoverFree( p->pCover );
    }
    p->pArray  = ALLOC( int, p->nFaninsMax * p->nValuesMax );
    p->pCover  = Mvc_CoverAlloc( Fnc_ManagerReadManMvc(Mv_FrameReadMan(p->pMvsis)), p->nFaninsMax * p->nValuesMax );
    p->pCube   = Mvc_CubeAlloc( p->pCover );
    p->pTokens = ALLOC( char *, 2 * p->nValuesMax );
}

/**Function*************************************************************

  Synopsis    [Destroys the reading structure.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aio_ReadStructDeallocate( Aio_Read_t * p )
{
    if ( p->pFileContents )     
        free( p->pFileContents );
    if ( p->pLines )            
        free( p->pLines );
    if ( p->pDots )     
        free( p->pDots );
    if ( p->pDotInputs );
        free( p->pDotInputs );
    if ( p->pDotOutputs );
        free( p->pDotOutputs );
    if ( p->pDotMvs )
        free( p->pDotMvs );
    if ( p->pAioLatches )
        free( p->pAioLatches );
    if ( p->pAioNodes )
        free( p->pAioNodes );
    if ( p->pArray )
        free( p->pArray );
    if ( p->pCube )
    {
        // return the cube to the previous size
        p->pCube->iLast   = p->pCover->nWords - 1 + (p->pCover->nWords == 0);
        p->pCube->nUnused = p->pCover->nUnused;
        // free the cube and the cover
        Mvc_CubeFree( p->pCover, p->pCube );
        Mvc_CoverFree( p->pCover );
    }
    if ( p->pTokens )
        free( p->pTokens );
    if ( p->sError )
        free( p->sError );
    if ( p->aOutputs )
    {
        array_free( p->aOutputs );
        p->aOutputs = NULL;
    }
    Aio_ReadStructCleanTables( p );
    free( p );
}

/**Function*************************************************************

  Synopsis    [Clearn the hash tables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aio_ReadStructCleanTables( Aio_Read_t * p )
{
    int i;
    if ( p->aOutputs )
    {
        array_free( p->aOutputs );
        p->aOutputs = array_alloc( Ntk_Node_t *, 50 );
    }
    if ( p->tLatch2ResLine )
    {
        st_free_table( p->tLatch2ResLine );
        p->tLatch2ResLine = NULL;
    }
    if ( p->tName2Values )
    {
        st_free_table( p->tName2Values );
        p->tName2Values = NULL;
    }
    if ( p->tName2Var )
    { // remove the Var entries from the table
        st_generator * gen;
        Aio_Var_t * pEntry;
        char * pKey;
        st_foreach_item( p->tName2Var, gen, (char **)&pKey, (char **)&pEntry )
        {
            for ( i = 0; i < pEntry->nValues; i++ )
                free( pEntry->pValueNames[i] );
            free( pEntry->pValueNames );
            free( pEntry );
        }
        st_free_table( p->tName2Var );
        p->tName2Var = NULL;
    }
}

/**Function*************************************************************

  Synopsis    [Prints the error message including the file name and line number.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aio_ReadPrintErrorMessage( Aio_Read_t * p )
{
    if ( p->LineCur == 0 ) // the line number is not given
        fprintf( p->Output, "%s: %s\n", p->FileName, p->sError );
    else // print the error message with the line number
        fprintf( p->Output, "%s (%d): %s\n", p->FileName, p->LineCur+1, p->sError );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


