/**CFile****************************************************************

  FileName    [ioReadFile.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Loads data from the file into the internal IO data structures.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ioFileOpen.c,v 1.2 2004/10/28 17:53:02 alanmi Exp $]

***********************************************************************/

#include "mv.h"
#include "mvInt.h"
#include "io.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define BUFSIZE (4 * 1024)

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Provide an fopen replacement with path lookup]

  Description [Provide an fopen replacement where the path stored
               in pathvar MVSIS variable is used to look up the path
               for name. Returns NULL if file cannot be opened.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
FILE * Io_FileOpen(const char * FileName, const char * PathVar, const char *Mode)
{
    char * t = 0, * c = 0, * i;
    Mv_Frame_t * pMvsis = Mv_FrameGetGlobalFrame();

    assert ( pMvsis );

    if ( PathVar == 0 )
    {
        return fopen( FileName, Mode );
    }
    else
    {
        if ( avl_lookup( pMvsis->tFlags, (char *)PathVar, &c ) )
        {
            char ActualFileName[BUFSIZE];
            FILE * fp = 0;
            t = util_strsav( c );
            for (i = strtok( t, ":" ); i != 0; i = strtok( 0, ":") )
            {
#ifdef WIN32
                _snprintf ( ActualFileName, BUFSIZE, "%s/%s", i, FileName );
#else
                snprintf ( ActualFileName, BUFSIZE, "%s/%s", i, FileName );
#endif
                if ( ( fp = fopen ( ActualFileName, Mode ) ) )
                {
                    fprintf ( pMvsis->Out, "Using file %s\n", ActualFileName );
                    free( t );
                    return fp;
                }
            }
            free( t );
            return 0;
        }
        else
        {
            return fopen( FileName, Mode );
        }
    }
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


