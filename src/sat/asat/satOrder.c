/**CFile****************************************************************

  FileName    [satOrder.c]

  PackageName [A C version of SAT solver MINISAT, originally developed 
  in C++ by Niklas Een and Niklas Sorensson, Chalmers University of 
  Technology, Sweden: http://www.cs.chalmers.se/~een/Satzoo.]

  Synopsis    [The manager of variable assignment.]

  Author      [Alan Mishchenko <alanmi@eecs.berkeley.edu>]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - January 1, 2004.]

  Revision    [$Id: satOrder.c,v 1.1 2005/07/08 01:01:28 alanmi Exp $]

***********************************************************************/

#include "satInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

struct Sat_Order_t_
{
    int *      pAssigns;     // var->val. Pointer to external assignment table.
    double *   pdActivity;   // var->act. Pointer to external activity table.
    int *      pVar2Pos;     // var->pos.
    int *      pPos2Var;     // pos->var.  
    int        iLastVar;     // the last used var
    int        nVars;        // the number of variables in the current run
    int        nVarsAlloc;   // the largest possible number of variables
};

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Allocates the ordering structure.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sat_Order_t * Sat_OrderAlloc( int * pAssigns, double * pdActivity, int nVarsAlloc )
{
    Sat_Order_t * p;
    int i;
    p = ALLOC( Sat_Order_t, 1 );
    memset( p, 0, sizeof(Sat_Order_t) );
    p->nVarsAlloc = nVarsAlloc;
    p->pAssigns   = pAssigns;
    p->pdActivity = pdActivity;
    p->pVar2Pos   = ALLOC( int, p->nVarsAlloc );
    p->pPos2Var   = ALLOC( int, p->nVarsAlloc );
    for ( i = 0; i < p->nVarsAlloc; i++ )
        p->pVar2Pos[i] = p->pPos2Var[i] = -1;
    return p;
}

/**Function*************************************************************

  Synopsis    [Extends the size of arrays.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sat_OrderRealloc( Sat_Order_t * p, int * pAssigns, double * pdActivity, int nVarsAlloc )
{
    int nVarsAllocOld, i;
    nVarsAllocOld = p->nVarsAlloc;
    p->nVarsAlloc = nVarsAlloc;
    p->pAssigns   = pAssigns;
    p->pdActivity = pdActivity;
    p->pVar2Pos   = REALLOC( int, p->pVar2Pos, p->nVarsAlloc );
    p->pPos2Var   = REALLOC( int, p->pPos2Var, p->nVarsAlloc );
    for ( i = nVarsAllocOld; i < p->nVarsAlloc; i++ )
        p->pVar2Pos[i] = p->pPos2Var[i] = -1;
}

/**Function*************************************************************

  Synopsis    [Cleans the ordering structure.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sat_OrderClean( Sat_Order_t * p, int nVars, int * pVars )
{
    int i;
    assert( nVars <= p->nVarsAlloc );
    p->iLastVar = 0;

    // consider the case when all variables should be ordered
    if ( pVars == NULL )
    {
        p->nVars = nVars;
        for ( i = 0; i < p->nVars; i++ )
            p->pVar2Pos[i] = p->pPos2Var[i] = i;
        return;
    }
    // clean the previously used variables
    for ( i = 0; i < p->nVars; i++ )
    {
        p->pVar2Pos[ p->pPos2Var[i] ] = -1;
        p->pPos2Var[ i ] = -1;
    }
    p->nVars = nVars;

/*
// make sure the total variable order is clean
for ( i = 0; i < p->nVarsAlloc; i++ )
{
    assert( p->pVar2Pos[i] == -1 );
    assert( p->pPos2Var[i] == -1 );
}
*/

    // put the variables in the order, in which they are given
    for ( i = 0; i < p->nVars; i++ )
    {
        assert( pVars[i] >= 0 );
        assert( pVars[i] < p->nVarsAlloc );
        p->pPos2Var[i] = pVars[i];
        p->pVar2Pos[pVars[i]] = i;

//        p->pdActivity[pVars[i]] = 0.0;
    }
}

/**Function*************************************************************

  Synopsis    [Frees the ordering structure.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sat_OrderFree( Sat_Order_t * p )
{
    free( p->pPos2Var );
    free( p->pVar2Pos );
    free( p );
}

/**Function*************************************************************

  Synopsis    [Selects the next variable to assign.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int  Sat_OrderSelect( Sat_Order_t * p )
{
    int i;
    for ( i = p->iLastVar; i < p->nVars; i++ )
        if ( p->pAssigns[p->pPos2Var[i]] == SAT_VAR_UNASSIGNED )
        {
            p->iLastVar = i;
            return p->pPos2Var[i]; 
        }
    return SAT_ORDER_UNKNOWN;
}

/**Function*************************************************************

  Synopsis    [Updates the order after a change in the activity of one variable.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sat_OrderUpdate( Sat_Order_t * p, int Var )
{
    int i;
//    int Counter = 0;

    // do not update the position of unbranchable variables
    if ( p->pVar2Pos[Var] < 0 )
        return;

    for ( i = p->pVar2Pos[Var]; i > 0 && p->pdActivity[p->pPos2Var[i-1]] < p->pdActivity[Var]; i--)
    {
        assert( p->pPos2Var[i] == Var );
        p->pVar2Pos[p->pPos2Var[i-1]]++; 
        p->pVar2Pos[p->pPos2Var[i]]--;
        p->pPos2Var[i-1] ^= p->pPos2Var[i]; 
        p->pPos2Var[i]   ^= p->pPos2Var[i-1]; 
        p->pPos2Var[i-1] ^= p->pPos2Var[i];
//        Counter++;
    }
//    printf( "%d ", Counter );
}

/**Function*************************************************************

  Synopsis    [Updates the order after a variable becomes unassigned.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sat_OrderUndo( Sat_Order_t * p, int Var )
{
    // skip the unbranchable variables
    if ( p->pVar2Pos[Var] < 0 )
        return;
    if ( p->iLastVar > p->pVar2Pos[Var] )
        p->iLastVar = p->pVar2Pos[Var];
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


