/**CFile****************************************************************

  FileName    [extraUtilQueue.c]

  PackageName [extra]

  Synopsis    [Queue data structure.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 1, 2003.]

  Revision    [$Id: extraUtilQueue.c,v 1.1 2004/04/08 05:51:00 alanmi Exp $]

***********************************************************************/

#include "extra.h"

/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Stucture declarations                                                     */
/*---------------------------------------------------------------------------*/

struct Extra_Queue_t_
{
    char **     pData;
    int         nSize;
    int         iWrite;
    int         iRead;
    int         nResizes;
    int         nShifts;
};

/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/


/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

/**AutomaticEnd***************************************************************/


/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function*************************************************************

  Synopsis    [Start the FIFO queue.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Extra_Queue_t * Extra_QueueStart( int nSize )
{
    Extra_Queue_t * p;
    p = ALLOC( Extra_Queue_t, 1 );
    p->nSize    = nSize;
    p->iWrite   = 0;
    p->iRead    = 0;
    p->nResizes = 0;
    p->nShifts  = 0;
    p->pData  = ALLOC( char *, nSize );
    return p;
}

/**Function*************************************************************

  Synopsis    [Stops the FIFO queue.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_QueueStop( Extra_Queue_t * p )
{
    FREE( p->pData );
    FREE( p );
}

/**Function*************************************************************

  Synopsis    [Reads the next entry from the queue.]

  Description [If there are no entries in the queue, return 0. Otherwise,
  returns 1.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Extra_QueueRead( Extra_Queue_t * p, char ** ppEntry )
{
    if ( p->iRead == p->iWrite )
        return 0;
    *ppEntry = p->pData[p->iRead++];
    return 1;
}

/**Function*************************************************************

  Synopsis    [Writes an entry into the queue.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_QueueWrite( Extra_Queue_t * p, char * pEntry )
{
    if ( p->iWrite >= p->nSize )
    {
        // there is not enough room
        if ( p->iRead > p->nSize/2 && p->nSize > 1000 )
        {
            // shift entries in the queue
            memmove( p->pData, p->pData + p->iRead, sizeof(char*) * (p->iWrite - p->iRead) );
            p->iWrite = p->iWrite - p->iRead;
            p->iRead  = 0;
            p->nShifts++;
        }
        else
        {
            // double the size of the queue
            char ** pDataNew;
            pDataNew = ALLOC( char *, p->nSize * 2 );
            memcpy( pDataNew, p->pData + p->iRead, sizeof(char*) * (p->iWrite - p->iRead) );
            p->iWrite = p->iWrite - p->iRead;
            p->iRead  = 0;
            FREE( p->pData );
            p->pData = pDataNew;
            p->nSize *= 2;
            p->nResizes++;
        }
    }
    p->pData[p->iWrite++] = pEntry;
}



////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


