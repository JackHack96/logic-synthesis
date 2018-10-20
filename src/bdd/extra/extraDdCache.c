/**CFile****************************************************************

  FileName    [extraDdCache.c]

  PackageName [extra]

  Synopsis    [Various reusable software utilities.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 1, 2003.]

  Revision    [$Id: extraDdCache.c,v 1.1 2004/04/08 05:51:00 alanmi Exp $]

***********************************************************************/

#include "extra.h"

/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Stucture declarations                                                     */
/*---------------------------------------------------------------------------*/

typedef struct Extra_OverlapEntry_t_ Extra_OverlapEntry_t;

struct Extra_OverlapCache_t_
{
    Extra_OverlapEntry_t * pCache;
    int                    nSize;
    DdManager *            dd;
    int                    nSuccess;
    int                    nFailure;
};

struct Extra_OverlapEntry_t_
{
    DdNode * bFunc1;
    DdNode * bFunc2;
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

  Synopsis    [Starts the bit matrix.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Extra_OverlapCache_t * Extra_OverlapCacheStart( DdManager * dd, int nSize )
{
    Extra_OverlapCache_t * p;
    int i;
    p = ALLOC( Extra_OverlapCache_t, 1 );
    memset( p, 0, sizeof(Extra_OverlapCache_t) );
    p->dd        = dd;
    p->nSize     = Cudd_Prime(nSize);
    p->pCache    = ALLOC( Extra_OverlapEntry_t, p->nSize );
    for ( i = 0; i < p->nSize; i++ )
        p->pCache[i].bFunc1 = NULL;
    return p;
}

/**Function*************************************************************

  Synopsis    [Stops the bit matrix.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_OverlapCacheClean( Extra_OverlapCache_t * p )
{
    int i;
    for ( i = 0; i < p->nSize; i++ )
        p->pCache[i].bFunc1 = NULL;
}

/**Function*************************************************************

  Synopsis    [Stops the overlap cache.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_OverlapCacheStop( Extra_OverlapCache_t * p )
{
    FREE( p->pCache );
    FREE( p );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Extra_OverlapCacheReadSuccess( Extra_OverlapCache_t * p )
{
    return p->nSuccess;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Extra_OverlapCacheReadFailure( Extra_OverlapCache_t * p )
{
    return p->nFailure;
}

/**Function*************************************************************

  Synopsis    [Checks the overlapping of two BDDs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Extra_bddOverlap( Extra_OverlapCache_t * p, DdNode * bFunc1, DdNode * bFunc2 )
{
    DdNode * bFunc1R = Cudd_Regular(bFunc1);
    DdNode * bFunc2R = Cudd_Regular(bFunc2);
    Extra_OverlapEntry_t * pEntry;
    int Value;

    if ( cuddIsConstant(bFunc1R) )
    {
        if ( cuddIsConstant(bFunc2R) )
        {
            if ( bFunc1 != bFunc1R || bFunc2 != bFunc2R )
                return 0;
            return 1;
        }
        if ( bFunc1 != bFunc1R )
            return 0;
        return 1;
    }
    if ( cuddIsConstant(bFunc2R) )
    {
        if ( bFunc2 != bFunc2R )
            return 0;
        return 1;
    }

    pEntry = p->pCache + hashKey2( bFunc1, bFunc2, p->nSize );
    if ( pEntry->bFunc1 == bFunc1 && Hash_Regular(pEntry->bFunc2) == bFunc2 )
    {
        p->nSuccess++;
        return Hash_IsComplement(pEntry->bFunc2);
    }
    p->nFailure++;

    {
        DdNode * bCof10, * bCof11, * bCof20, * bCof21;
        int Level1   = p->dd->perm[bFunc1R->index];
        int Level2   = p->dd->perm[bFunc2R->index];
        int Level    = (Level1 < Level2)? Level1: Level2;
        // cofactor the functions
        if ( Level1 == Level )
        {
            if ( bFunc1R != bFunc1 ) // bFunc1 is complemented 
            {
                bCof10 = Cudd_Not( cuddE(bFunc1R) );
                bCof11 = Cudd_Not( cuddT(bFunc1R) );
            }
            else
            {
                bCof10 = cuddE(bFunc1R);
                bCof11 = cuddT(bFunc1R);
            }
        }
        else
            bCof10 = bCof11 = bFunc1;
        if ( Level2 == Level )
        {
            if ( bFunc2R != bFunc2 ) // bFunc2 is complemented 
            {
                bCof20 = Cudd_Not( cuddE(bFunc2R) );
                bCof21 = Cudd_Not( cuddT(bFunc2R) );
            }
            else
            {
                bCof20 = cuddE(bFunc2R);
                bCof21 = cuddT(bFunc2R);
            }
        }
        else
            bCof20 = bCof21 = bFunc2;

        Value = Extra_bddOverlap( p, bCof10, bCof20 );
        if ( Value == 0 ) 
            Value = Extra_bddOverlap( p, bCof11, bCof21 );
    }
    // put the result into cache
    pEntry->bFunc1 = bFunc1;
    pEntry->bFunc2 = Hash_NotCond( bFunc2, Value );
    return Value;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


