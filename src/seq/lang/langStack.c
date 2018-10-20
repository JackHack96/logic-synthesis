/**CFile****************************************************************

  FileName    [langStack.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Stacks used by the formula parser.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - August 18, 2003.]

  Revision    [$Id: langStack.c,v 1.3 2004/02/19 03:06:54 alanmi Exp $]

***********************************************************************/

#include "langInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Starts the stack.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Lang_Stack_t * Lang_StackStart( int nDepth )
{
    Lang_Stack_t * p;
    p = ALLOC( Lang_Stack_t, 1 );
    memset( p, 0, sizeof(Lang_Stack_t) );
    p->pData = ALLOC( Lang_Auto_t *, nDepth );
    p->Size = nDepth;
    return p;
}

/**Function*************************************************************

  Synopsis    [Checks whether the stack is empty.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Lang_StackIsEmpty( Lang_Stack_t * p )
{
    return (bool)(p->Top == 0);
}

/**Function*************************************************************

  Synopsis    [Pushes an entry into the stack.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lang_StackPush( Lang_Stack_t * p, Lang_Auto_t * pAut )
{
    if ( p->Top >= p->Size )
    {
        printf( "Lang_StackPush(): Stack size is too small!\n" );
        return;
    }
    p->pData[ p->Top++ ] = pAut;
}

/**Function*************************************************************

  Synopsis    [Pops an entry out of the stack.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Lang_Auto_t * Lang_StackPop( Lang_Stack_t * p )
{
    if ( p->Top == 0 )
    {
        printf( "Lang_StackPop(): Trying to extract data from the empty stack!\n" );
        return NULL;
    }
    return p->pData[ --p->Top ];
}

/**Function*************************************************************

  Synopsis    [Deletes the stack.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lang_StackFree( Lang_Stack_t * p )
{
    FREE( p->pData );
    FREE( p );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


