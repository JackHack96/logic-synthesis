/**CFile****************************************************************

  FileName    [ioRead.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [The top level procedure to read BLIF/BLIF-MVS/BLIF-MV files.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ioRead.c,v 1.23 2004/06/18 00:20:39 satrajit Exp $]

***********************************************************************/

#include "ioInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static Io_Read_t * Io_ReadStructAllocate();
static void        Io_ReadStructDeallocate( Io_Read_t * p );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Reads the network from BLIF/BLIF-MVS/BLIF-MV file.]

  Description [This is a universal reader. It reads the network 
  specified in any of the three file formats. The file name and 
  extension may be any. The reader determines the type of the input
  file by analizing the file contents (see Io_ReadLinesDetectFileType).]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Network_t * Io_ReadNetwork( Mv_Frame_t * pMvsis, char * FileName )
{
    Ntk_Network_t * pNet, * pNetExdc;
    Io_Read_t * p;
    int FileType;

    // check the case when the file is PLA or PLA-MV
    FileType = Io_ReadFileType( FileName );
    if ( FileType == IO_FILE_PLA )
        return Io_ReadPla( pMvsis, FileName );
//    if ( FileType == IO_FILE_PLA_MV )
//        return Io_ReadPlaMv( pMvsis, FileName );

    // allocate the reading structure
    p = Io_ReadStructAllocate();
    p->FileName = FileName;
    p->Output = Mv_FrameReadErr(pMvsis);
    p->pMvsis = pMvsis;

    // read the file contents, remove comments
    if ( Io_ReadFile( p ) )
        goto failure;

    // split the file into lines and dot-statements
    if ( Io_ReadFileMarkLinesAndDots( p ) )
        goto failure;

    // read the primary network
    if ( Io_ReadLines( p ) )
        goto failure;
    pNet = Io_ReadNetworkStructure( p );
    p->pNet = pNet;
    if ( pNet == NULL )
        goto failure;

    // read the EXDC network
    if ( p->LineExdc )
    {
        if ( Io_ReadLines( p ) )
            goto failure;
        pNetExdc = Io_ReadNetworkStructure( p );
        if ( pNetExdc == NULL )
        {
            Ntk_NetworkDelete( p->pNet );
            goto failure;
        }
        Ntk_NetworkSetNetExdc( pNet, pNetExdc );
    }
    // store the variable value names (very bag hack and a memory leak!!!)
    pNet->pData = (char *)p->tName2Var;
    p->tName2Var = NULL;

    Io_ReadStructDeallocate( p );
    return pNet;

failure:
    Io_ReadStructDeallocate( p );
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Creates the reading structure.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Io_Read_t * Io_ReadStructAllocate()
{
    Io_Read_t * p;
    p = ALLOC( Io_Read_t, 1 );
    memset( p, 0, sizeof(Io_Read_t) );
    p->sError = ALLOC( char, 300 );
    p->aOutputs = array_alloc( Ntk_Node_t *, 50 );
    p->pLineDefaultInputArrival = NULL;
    return p;
}

/**Function*************************************************************

  Synopsis    [Creates the reading structure.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_ReadStructAllocateAdditional( Io_Read_t * p )
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
void Io_ReadStructDeallocate( Io_Read_t * p )
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
    if ( p->pIoLatches )
        free( p->pIoLatches );
    if ( p->pIoNodes )
        free( p->pIoNodes );
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
    Io_ReadStructCleanTables( p );
    free( p );
}

/**Function*************************************************************

  Synopsis    [Clearn the hash tables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_ReadStructCleanTables( Io_Read_t * p )
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
        Io_Var_t * pEntry;
        char * pKey;
        st_foreach_item( p->tName2Var, gen, (char **)&pKey, (char **)&pEntry )
        {
            free( pKey );
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
void Io_ReadPrintErrorMessage( Io_Read_t * p )
{
    if ( p->LineCur == 0 ) // the line number is not given
        fprintf( p->Output, "%s: %s\n", p->FileName, p->sError );
    else // print the error message with the line number
        fprintf( p->Output, "%s (%d): %s\n", p->FileName, p->LineCur+1, p->sError );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


