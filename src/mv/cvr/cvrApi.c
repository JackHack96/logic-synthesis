/**CFile****************************************************************

  FileName    [cvrApi.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [The APIs of the Cvr package.]

  Author      [MVSIS Group]

  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: cvrApi.c,v 1.11 2003/05/27 23:14:56 alanmi Exp $]
***********************************************************************/

#include "cvrInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void           Cvr_CoverIsetsFree( Cvr_Cover_t * p );
static Mvc_Cover_t ** Cvr_CoverIsetsCopy( Cvr_Cover_t * p );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
Cvr_Cover_t * Cvr_CoverCreate( Vm_VarMap_t * pVm, Mvc_Cover_t ** pIsets )
{
    Cvr_Cover_t * pCvr;
    pCvr = ALLOC( Cvr_Cover_t, 1 );
    memset( pCvr, 0, sizeof(Cvr_Cover_t) );
    pCvr->pVm      = pVm;
    pCvr->ppCovers = pIsets;
    return pCvr;
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
Cvr_Cover_t * Cvr_CoverDup( Cvr_Cover_t * p )
{
    Cvr_Cover_t * pCvr;
    if (p==NULL) return NULL;
    pCvr = ALLOC( Cvr_Cover_t, 1 );
    *pCvr = *p;
    pCvr->ppCovers = Cvr_CoverIsetsCopy( p );
    pCvr->pTree = NULL;
    return pCvr;
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cvr_CoverFree( Cvr_Cover_t * p )
{
    Cvr_CoverIsetsFree( p );
    Ft_TreeFree( p->pTree );
    FREE( p );
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvc_Cover_t ** Cvr_CoverReadIsets( Cvr_Cover_t * p )
{
    return p->ppCovers;
}

Mvc_Cover_t * Cvr_CoverReadIsetByIndex( Cvr_Cover_t * p, int iIndex )
{
    return p->ppCovers[iIndex];
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cvr_CoverReadDefault( Cvr_Cover_t * pCvr )
{
    int i,nValues;
    nValues = Vm_VarMapReadValuesOutput( pCvr->pVm );
    for ( i = 0; i < nValues; i++ )
        if ( pCvr->ppCovers[i] == NULL )  {
            return i;
        }
    return -1;
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
Vm_VarMap_t *  Cvr_CoverReadVm( Cvr_Cover_t * p )
{
    return p->pVm;
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cvr_CoverSetIsets( Cvr_Cover_t * p, Mvc_Cover_t ** pIsets )
{
    Cvr_CoverIsetsFree( p );
    p->ppCovers = pIsets;
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cvr_CoverSetVm( Cvr_Cover_t * p, Vm_VarMap_t * pVm )
{
    if ( p->pVm == pVm )
        return;
    p->pVm = pVm;
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int
Cvr_CoverReadCubeNum( Cvr_Cover_t * pCvr )
{
    int i,nValues, nCubes;
    nValues = Vm_VarMapReadValuesOutput( pCvr->pVm );
    nCubes = 0;
    for ( i = 0; i < nValues; i++ )
        if ( pCvr->ppCovers[i] ) 
            nCubes += Mvc_CoverReadCubeNum( pCvr->ppCovers[i] );
    return nCubes;
}


/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cvr_CoverReadLitSopNum( Cvr_Cover_t * pCvr )
{
    int i,nValues, nLits;
    nValues = Vm_VarMapReadValuesOutput( pCvr->pVm );
    nLits = 0;
    for ( i = 0; i < nValues; i++ )
        if ( pCvr->ppCovers[i] ) 
            nLits += Cvr_CoverCountLiterals( pCvr->pVm, pCvr->ppCovers[i] );
    return nLits;
}

/**Function*************************************************************

  Synopsis    [return the total number of values in all literals of all cubes]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int
Cvr_CoverReadLitSopValueNum( Cvr_Cover_t * pCvr )
{
    int i,nValues, nLitValues;
    nValues = Vm_VarMapReadValuesOutput( pCvr->pVm );
    nLitValues = 0;
    
    for ( i = 0; i < nValues; i++ )
        if ( pCvr->ppCovers[i] ) 
            nLitValues += Cvr_CoverCountLiteralValues( pCvr->pVm, pCvr->ppCovers[i] );

    return nLitValues;
}


/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cvr_CoverIsetsFree( Cvr_Cover_t * p )
{
    int nValues, i;
    if ( p->ppCovers ) {
        nValues = Vm_VarMapReadValuesOutput( p->pVm );
        for ( i = 0; i < nValues; i++ )
            if ( p->ppCovers[i] ) {
                Mvc_CoverFree( p->ppCovers[i] );
                p->ppCovers[i] = NULL;
            }
        FREE( p->ppCovers );
        p->ppCovers = NULL;
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvc_Cover_t ** Cvr_CoverIsetsCopy( Cvr_Cover_t * p )
{
    Mvc_Cover_t ** pCovers;
    int nValues, i;
    pCovers = NULL;
    if ( p->ppCovers ) {
        nValues = Vm_VarMapReadValuesOutput( p->pVm );
        pCovers = ALLOC( Mvc_Cover_t *, nValues );
        for ( i = 0; i < nValues; i++ ) {
            if ( p->ppCovers[i] )
                pCovers[i] = Mvc_CoverDup( p->ppCovers[i] );
            else
                pCovers[i] = NULL;
        }
    }
    return pCovers;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
