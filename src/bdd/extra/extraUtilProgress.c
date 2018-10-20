/**CFile****************************************************************

  FileName    [extraUtilProgress.c]

  PackageName [extra]

  Synopsis    [Progress bar.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 2.0. Started - September 1, 2003.]

  Revision    [$Id: extraUtilProgress.c,v 1.6 2004/02/18 01:16:25 satrajit Exp $]

***********************************************************************/

#include "stdio.h"
#include "extra.h"

#include "mv.h"			/* For Mv_FrameGetGlobalFrame() */
#include "mvInt.h"		

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

struct ProgressBarStruct
{
    FILE *           pFile;
    int              nItemsTotal;
    int              posTotal;
    int              posCur;
};

static void Extra_ProgressBarShow( ProgressBar * p, char * pString );
static void Extra_ProgressBarClean( ProgressBar * p );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Starts the progress bar.]

  Description [The first parameter is the output stream (pFile), where
  the progress is printed. The current printing position should be the
  first one on the given line. The second parameters is the total
  number of items that correspond to 100% position of the progress bar.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
ProgressBar * Extra_ProgressBarStart( FILE * pFile, int nItemsTotal )
{
    ProgressBar * p;
	Mv_Frame_t *  mvFrame;

    p = ALLOC( ProgressBar, 1 );
    memset( p, 0, sizeof(ProgressBar) );

    p->nItemsTotal = nItemsTotal;
    p->posTotal    = 78;
    p->posCur      = 1;

	mvFrame = Mv_FrameGetGlobalFrame();

	if ((mvFrame != 0) && (mvFrame->fBatchMode != 1))
	{
    	p->pFile       = pFile;
	}
	else
	{
		 p->pFile      = 0;
	}

    Extra_ProgressBarShow( p, NULL );
    return p;
}

/**Function*************************************************************

  Synopsis    [Updates the progress bar.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_ProgressBarUpdate( ProgressBar * p, int nItemsCur, char * pString )
{
    if ( nItemsCur > p->nItemsTotal )
        nItemsCur = p->nItemsTotal;
    p->posCur = nItemsCur * p->posTotal / p->nItemsTotal;
    if ( p->posCur == 0 )
        p->posCur = 1;
    Extra_ProgressBarShow( p, pString );
}


/**Function*************************************************************

  Synopsis    [Stops the progress bar.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_ProgressBarStop( ProgressBar * p )
{
    Extra_ProgressBarClean( p );
    FREE( p );
}

/**Function*************************************************************

  Synopsis    [Prints the progress bar of the given size.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_ProgressBarShow( ProgressBar * p, char * pString )
{
    int i;

	if (p->pFile != 0)
	{
		if ( pString )
			fprintf( p->pFile, "%s ", pString );
		for ( i = (pString? strlen(pString) + 1 : 0); i < p->posCur; i++ )
			fprintf( p->pFile, "-" );
		if ( i == p->posCur )
			fprintf( p->pFile, ">" );
		for ( i++  ; i <= p->posTotal; i++ )
			fprintf( p->pFile, " " );
		fprintf( p->pFile, "\r" );
		fflush( p->pFile );
	}
}

/**Function*************************************************************

  Synopsis    [Cleans the progress bar before quitting.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_ProgressBarClean( ProgressBar * p )
{
    int i;
	if (p->pFile != 0)
	{
		for ( i = 0; i <= p->posTotal; i++ )
			fprintf( p->pFile, " " );
		fprintf( p->pFile, "\r" );
		fflush( p->pFile );
	}
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


