/**CFile****************************************************************

  FileName    [parseStack.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Stacks used by the formula parser.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - August 18, 2003.]

  Revision    [$Id: parseStack.c,v 1.1 2004/01/06 21:02:55 alanmi Exp $]

***********************************************************************/

#include "parseInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

struct ParseStackFnStruct
{
    DdNode **     pData;        // the array of elements
    int           Top;          // the index
    int           Size;         // the stack size
};

struct ParseStackOpStruct
{
    int *         pData;        // the array of elements
    int           Top;          // the index
    int           Size;         // the stack size
};

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Starts the stack.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Parse_StackFn_t * Parse_StackFnStart( int nDepth )
{
    Parse_StackFn_t * p;
    p = ALLOC( Parse_StackFn_t, 1 );
    memset( p, 0, sizeof(Parse_StackFn_t) );
    p->pData = ALLOC( DdNode *, nDepth );
    p->Size = nDepth;
    return p;
}

/**Function*************************************************************

  Synopsis    [Checks whether the stack is empty.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Parse_StackFnIsEmpty( Parse_StackFn_t * p )
{
    return (bool)(p->Top == 0);
}

/**Function*************************************************************

  Synopsis    [Pushes an entry into the stack.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Parse_StackFnPush( Parse_StackFn_t * p, DdNode * bFunc )
{
    if ( p->Top >= p->Size )
    {
        printf( "Parse_StackFnPush(): Stack size is too small!\n" );
        return;
    }
    p->pData[ p->Top++ ] = bFunc;
}

/**Function*************************************************************

  Synopsis    [Pops an entry out of the stack.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Parse_StackFnPop( Parse_StackFn_t * p )
{
    if ( p->Top == 0 )
    {
        printf( "Parse_StackFnPush(): Trying to extract data from the empty stack!\n" );
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
void Parse_StackFnFree( Parse_StackFn_t * p )
{
    FREE( p->pData );
    FREE( p );
}




/**Function*************************************************************

  Synopsis    [Starts the stack.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Parse_StackOp_t * Parse_StackOpStart( int nDepth )
{
    Parse_StackOp_t * p;
    p = ALLOC( Parse_StackOp_t, 1 );
    memset( p, 0, sizeof(Parse_StackOp_t) );
    p->pData = ALLOC( int, nDepth );
    p->Size = nDepth;
    return p;
}

/**Function*************************************************************

  Synopsis    [Checks whether the stack is empty.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Parse_StackOpIsEmpty( Parse_StackOp_t * p )
{
    return (bool)(p->Top == 0);
}

/**Function*************************************************************

  Synopsis    [Pushes an entry into the stack.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Parse_StackOpPush( Parse_StackOp_t * p, int Oper )
{
    if ( p->Top >= p->Size )
    {
        printf( "Parse_StackOpPush(): Stack size is too small!\n" );
        return;
    }
    p->pData[ p->Top++ ] = Oper;
}

/**Function*************************************************************

  Synopsis    [Pops an entry out of the stack.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Parse_StackOpPop( Parse_StackOp_t * p )
{
    if ( p->Top == 0 )
    {
        printf( "Parse_StackOpPush(): Trying to extract data from the empty stack!\n" );
        return -1;
    }
    return p->pData[ --p->Top ];
}

/**Function*************************************************************

  Synopsis    [Deletes the stack.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Parse_StackOpFree( Parse_StackOp_t * p )
{
    FREE( p->pData );
    FREE( p );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


