/**CFile****************************************************************

  FileName    [autWrite.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    []

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: aioWrite.c,v 1.1 2004/02/19 03:06:42 alanmi Exp $]

***********************************************************************/

#include "mv.h"
#include "aioInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Write the automaton into a KISS or AUT file with the given name.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aio_AutoWrite( Aut_Auto_t * pAut, char * FileName, bool fWriteAut, bool fWriteFsm )
{
    if ( fWriteAut )
    {
        if ( !Aut_AutoCheckIsBinary( pAut ) )
        {
            Aut_AutoBinaryEncode( pAut );
            printf( "The MV automaton was encoded for writing.\n" );
        }
        Aio_AutoWriteAut( pAut, FileName, fWriteFsm );
    }
    else
        Aio_AutoWriteBlifMv( pAut, FileName, fWriteFsm );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


