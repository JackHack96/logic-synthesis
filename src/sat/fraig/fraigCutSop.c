/**CFile****************************************************************

  FileName    [fraigChoices.c]

  PackageName [FRAIG: Functionally reduced AND-INV graphs.]

  Synopsis    [Adds choices using refactoring of logic cones.]

  Author      [Alan Mishchenko <alanmi@eecs.berkeley.edu>]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.1. Started - October 1, 2004]

  Revision    [$Id: fraigCutSop.c,v 1.2 2005/07/08 01:01:31 alanmi Exp $]

***********************************************************************/

#include "fraigInt.h"
#include "extra.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void     Fraig_AddChoicesNode( Fraig_Man_t * p, Fraig_Node_t * pNode, DdManager * dd, int nCutSize );
static DdNode * Fraig_ConstructBddFromAig( DdManager * dd, Fraig_NodeVec_t * vInside, Fraig_NodeVec_t * vFanins );
static DdNode * Fraig_ConstructBddFromSop( DdManager * dd, unsigned uCubes[], int nCubes );
static int      Fraig_ConstructSopFromZdd( DdManager * dd, DdNode * zCover, unsigned uCubes[] );
static void     Fraig_ConstructSopFromZdd_rec( DdManager * dd, DdNode * zCover, unsigned uCubes[], int * pnCubes, unsigned uTemp );
static int      Fraig_CountZdd( DdManager * dd, DdNode * zCover );
static void     Fraig_CountZdd_rec( DdManager * dd, DdNode * zCover, int * pnCubes );

DdManager * dd = NULL;

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [SOP minimization experiment for one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_CutSopCountCubes( Fraig_Man_t * p, Fraig_NodeVec_t * vFanins, Fraig_NodeVec_t * vInside )
{
    Fraig_Node_t * pNode;
    DdNode * bFunc, * bFunc2, * zCover;
    unsigned * puCubes;
    int nCubes, nCubesAlloc;

    assert( vFanins->nSize < 50 );

    if ( dd == NULL )
    {
        // start the BDD manager
        dd = Cudd_Init( 50, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
        Cudd_zddVarsFromBddVars( dd, 2 );
    }

    // get the topmost node
    pNode = vInside->pArray[vInside->nSize-1];

    // compute the BDD for the function in the cut
    bFunc  = Fraig_ConstructBddFromAig( dd, vInside, vFanins );   Cudd_Ref( bFunc );
    zCover = Extra_zddIsopCover( dd, bFunc, bFunc );              Cudd_Ref( zCover );

    // convert the cover into the SOP
    nCubesAlloc = Fraig_CountZdd( dd, zCover );


    Cudd_RecursiveDeref( dd, bFunc );
    Cudd_RecursiveDerefZdd( dd, zCover );
    return nCubesAlloc;


    puCubes = ALLOC( unsigned, nCubesAlloc );
    nCubes = Fraig_ConstructSopFromZdd( dd, zCover, puCubes );
    assert( nCubes == nCubesAlloc );
    Cudd_RecursiveDerefZdd( dd, zCover );

    // verify the result
    bFunc2 = Fraig_ConstructBddFromSop( dd, puCubes, nCubes );   Cudd_Ref( bFunc2 );
    if ( bFunc2 != bFunc )
        printf( "Verification failed.\n" );
    free( puCubes );

    
    Cudd_RecursiveDeref( dd, bFunc );
    Cudd_RecursiveDeref( dd, bFunc2 );
}

/**Function*************************************************************

  Synopsis    [Derive the BDD for the function in the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Fraig_ConstructBddFromAig( DdManager * dd, Fraig_NodeVec_t * vInside, Fraig_NodeVec_t * vFanins )
{
    Fraig_Node_t * pNode, * pNode1, * pNode2;
    DdNode * bFunc;
    int i, fComp1, fComp2;

    // set the leaf info
    for ( i = 0; i < vFanins->nSize; i++ )
    {
        pNode = vFanins->pArray[i];
        assert( pNode->fMark0 == 0 );
        pNode->fMark0 = 1;
        pNode->pData0 = (Fraig_Node_t *)dd->vars[i];  
    }

    // simulate
    Fraig_NodeVecSortByLevel( vInside );
    for ( i = 0; i < vInside->nSize; i++ )
    {
        pNode  = vInside->pArray[i];
        assert( pNode->fMark0 == 0 );
        pNode1 = Fraig_Regular( pNode->p1 );
        pNode2 = Fraig_Regular( pNode->p2 );
        assert( pNode1->fMark0 == 1 );
        assert( pNode2->fMark0 == 1 );
        fComp1 = Fraig_IsComplement( pNode->p1 );
        fComp2 = Fraig_IsComplement( pNode->p2 );
        // compute the BDD of this node
        bFunc = Cudd_bddAnd( dd, Cudd_NotCond( (DdNode *)pNode1->pData0, fComp1 ),
            Cudd_NotCond( (DdNode *)pNode2->pData0, fComp2 ) );   Cudd_Ref( bFunc );
        pNode->pData0 = (Fraig_Node_t *)bFunc;
        pNode->fMark0 = 1;
    }
    // get the result
    Cudd_Ref( bFunc );

    // deref the intermediate results
    for ( i = 0; i < vFanins->nSize; i++ )
        vFanins->pArray[i]->fMark0 = 0;
    for ( i = 0; i < vInside->nSize; i++ )
    {
        vInside->pArray[i]->fMark0 = 0;
        Cudd_RecursiveDeref( dd, (DdNode *)vInside->pArray[i]->pData0 );
    }
    Cudd_Deref( bFunc );
    return bFunc;
}

/**Function*************************************************************

  Synopsis    [Derive the BDD for the function in the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Fraig_ConstructBddFromSop( DdManager * dd, unsigned uCubes[], int nCubes )
{
    DdNode * bCube, * bTemp, * bVar, * bRes;
    int Lit, i, c;

    bRes = b0;   Cudd_Ref( bRes );
    for ( c = 0; c < nCubes; c++ )
    {
        bCube = b1;   Cudd_Ref( bCube );
        for ( i = 0; i < dd->size; i++ )
        {
            Lit = (uCubes[c] >> (2*i)) & 3;
            assert( Lit > 0 );
            if ( Lit == 1 )
                bVar = Cudd_Not( dd->vars[i] );
            else if ( Lit == 2 )
                bVar = dd->vars[i];
            else
                continue;
            bCube  = Cudd_bddAnd( dd, bTemp = bCube, bVar );   Cudd_Ref( bCube );
            Cudd_RecursiveDeref( dd, bTemp );
        }
        bRes = Cudd_bddOr( dd, bTemp = bRes, bCube );   Cudd_Ref( bRes );
        Cudd_RecursiveDeref( dd, bCube );
        Cudd_RecursiveDeref( dd, bTemp );
    }
    Cudd_Deref( bRes );
    return bRes;
}

/**Function*************************************************************

  Synopsis    [Derive the BDD for the function in the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_ConstructSopFromZdd( DdManager * dd, DdNode * zCover, unsigned uCubes[] )
{
    unsigned uTemp = FRAIG_MASK(2*dd->size);
    int nCubes = 0;
    Fraig_ConstructSopFromZdd_rec( dd, zCover, uCubes, &nCubes, uTemp );
    return nCubes;
}

/**Function*************************************************************

  Synopsis    [Derive the BDD for the function in the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_ConstructSopFromZdd_rec( DdManager * dd, DdNode * zCover, unsigned uCubes[], int * pnCubes, unsigned uTemp )
{
    DdNode * zC0, * zC1, * zC2;
    int Index;

    if ( zCover == dd->zero )
        return;
    if ( zCover == dd->one )
    {
        uCubes[(*pnCubes)++] = uTemp;
        assert( *pnCubes < 1000 );
        return;
    }
    Index = 2*(zCover->index/2);
    extraDecomposeCover( dd, zCover, &zC0, &zC1, &zC2 );
    Fraig_ConstructSopFromZdd_rec( dd, zC0, uCubes, pnCubes, uTemp ^ (1 << (Index+1)) );
    Fraig_ConstructSopFromZdd_rec( dd, zC1, uCubes, pnCubes, uTemp ^ (1 << (Index+0)) );
    Fraig_ConstructSopFromZdd_rec( dd, zC2, uCubes, pnCubes, uTemp );
}

/**Function*************************************************************

  Synopsis    [Count the number of paths in the ZDD.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_CountZdd( DdManager * dd, DdNode * zCover )
{
    int nCubes = 0;
    Fraig_CountZdd_rec( dd, zCover, &nCubes );
    return nCubes;
}

/**Function*************************************************************

  Synopsis    [Count the number of paths in the ZDD.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_CountZdd_rec( DdManager * dd, DdNode * zCover, int * pnCubes )
{
    DdNode * zC0, * zC1, * zC2;
    if ( zCover == dd->zero )
        return;
    if ( zCover == dd->one )
    {
        (*pnCubes)++;
        return;
    }
    extraDecomposeCover( dd, zCover, &zC0, &zC1, &zC2 );
    Fraig_CountZdd_rec( dd, zC0, pnCubes );
    Fraig_CountZdd_rec( dd, zC1, pnCubes );
    Fraig_CountZdd_rec( dd, zC2, pnCubes );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


