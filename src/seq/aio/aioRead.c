/**CFile****************************************************************

  FileName    [aioRead.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [The root file for reading automata.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: aioRead.c,v 1.2 2005/06/02 03:34:19 alanmi Exp $]

***********************************************************************/

#include "mv.h"
#include "aioInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static bool Aio_AutoFileIsAut( char * FileName );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aut_Auto_t * Aio_AutoRead( Mv_Frame_t * pMvsis, char * FileName, Aut_Man_t * pManDd )
{
    if ( Aio_AutoFileIsAut( FileName ) )
    {
        printf( "The automaton appears to be in an input format similar to KISS.\n" );
        printf( "BALM does not support this input format. Please use BLIF-MV.\n" );
        return NULL;

        return Aio_AutoReadAut( pMvsis, FileName, pManDd );
    }
    else
    {
//        printf( "Cannot read blif-mv\n" );
        return Aio_AutoReadBlifMv( pMvsis, FileName, pManDd );
//        return NULL;
    }
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Aio_AutoFileIsAut( char * FileName )
{
    char * pBuffer;
    int RetValue;
    pBuffer = Io_ReadFileFileContents( FileName, NULL );
    if ( strstr( pBuffer, ".i " ) )
        RetValue = 1;
    else
        RetValue = 0;
    FREE( pBuffer );
    return RetValue;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


