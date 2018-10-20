/**CFile****************************************************************

  FileName    [fmbImage.c]

  PackageName [Binary flexibility manager.]

  Synopsis    [Image computation based on output splitting.]

  Author      [MVSIS Group]
  
  Affiliation [ECE Department. Portland State University, Portland, Oregon.]

  Date        [Ver. 3.0. Started - September 27, 2003.]

  Revision    [$Id: fmbImage.c,v 1.1 2003/11/18 18:55:09 alanmi Exp $]

***********************************************************************/

#include "extra.h"
#include "array.h"
#include <time.h>

////////////////////////////////////////////////////////////////////////
///                       DATA STRUCTURES                            ///
////////////////////////////////////////////////////////////////////////

//#define PRB(dd,f)    printf("%s = ", #f); Extra_bddPrint(dd,f); printf("\n")

// this structure stores the information about one transition function
typedef struct partinfo_ 
{
    DdNode * bPart;
    DdNode * bSupp;
    DdNode * bVar;
} partinfo;


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// these are needed to implement timeout
static int s_Timeout   = 0;
static int s_TimeLimit = 0; 

// static functions
static DdNode * Fmb_Image_rec( DdManager * dd, array_t * pParts, int nBddSize );
static partinfo * Fmb_ImagePartInfoAllocate();
static void Fmb_ImagePartInfoDelocate( DdManager * dd, partinfo * p );
static void Fmb_ImagePartInfoArrayFree( DdManager * dd, array_t * pParts );
static DdNode * Fmb_ImageSmallSize( DdManager * dd, array_t * pParts, int nBddSize );
static array_t * Fmb_ImageDisjointSupport( DdManager * dd, array_t * pParts );


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////


/**Function*************************************************************

  Synopsis    [Computes the image of a set of function.]

  Description [Computes the image of the care set (bCareSet) with the set 
  of functions (pbFuncs). The variables used in the image are given (pbVars). 
  The number of functions in the set is also given (nFuncs). Stops iterative 
  splitting when the shared BDD size is less than nBddSize. ]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Fmb_ImageCompute( DdManager * dd, DdNode ** pbFuncs, DdNode ** pbVars, DdNode * bCareSet, int nFuncs, int nBddSize )
{
    array_t * pParts;
    DdNode * bImage, * bImageProper;
    DdNode * bTemp, * bConstr;
    partinfo * pNew;
    int i;

    s_TimeLimit = (int)(s_Timeout /* in miliseconds*/ * (float)(CLOCKS_PER_SEC) / 1000 ) + clock();

    // create the array of partitions
    pParts = array_alloc( partinfo *, 10 );

    // constrain the functions
    bImage = b1;  Cudd_Ref( bImage );
    for ( i = 0; i < nFuncs; i++ )
    {
        bConstr = Cudd_bddConstrain( dd, pbFuncs[i], bCareSet ); Cudd_Ref( bConstr );
        if ( Cudd_IsConstant(bConstr) )
        {
            bImage = Cudd_bddAnd( dd, bTemp = bImage, Cudd_NotCond(pbVars[i], (bConstr == b0)) );  Cudd_Ref( bImage );
            Cudd_RecursiveDeref( dd, bTemp );
            Cudd_RecursiveDeref( dd, bConstr );
        }
        else
        {
            pNew = Fmb_ImagePartInfoAllocate();
            pNew->bPart = bConstr; // takes ref
            pNew->bSupp = Cudd_Support( dd, bConstr );   Cudd_Ref( pNew->bSupp );
            pNew->bVar  = pbVars[i];
            array_insert_last( partinfo *, pParts, pNew );
//PRB(dd,pNew->bSupp);
        }
    }

    // call the recursive image computation
    bImageProper = Fmb_Image_rec( dd, pParts, nBddSize );  
    if ( bImageProper == NULL )
    {
        Cudd_RecursiveDeref( dd, bImage );
        Fmb_ImagePartInfoArrayFree( dd, pParts );
        return NULL;
    }
    Cudd_Ref( bImageProper );
    Fmb_ImagePartInfoArrayFree( dd, pParts );

    // add to the image
    bImage = Cudd_bddAnd( dd, bTemp = bImage, bImageProper );  Cudd_Ref( bImage );
    Cudd_RecursiveDeref( dd, bTemp );
    Cudd_RecursiveDeref( dd, bImageProper );

    Cudd_Deref( bImage );
    return bImage;
}



/**Function*************************************************************

  Synopsis    [Computes the image of a set of function.]

  Description [Checks the terminal case (n=0,1), Checks the special case (n=2). 
  Checks disjoint supports. Checks the total BDD node limit (nBddSize). 
  If none of these trivial cases apply, splits recursively.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Fmb_Image_rec( DdManager * dd, array_t * pParts, int nBddSize )
{
    DdNode * bImage, * bImage0, * bImage1, * bConstr, * bTemp;
    DdNode * bImageProper0, * bImageProper1;
    array_t * pParts0, * pParts1;
    array_t * pDisjs, * pArray;
    partinfo * pCur, * pStart, * pNew;
    int i, k;

    // if there is no more than 1 non-constant function, the image is ready
    if ( array_n(pParts) <= 1 )
        return dd->one;

    // checking the special case of 2 non-constant functions
    if ( array_n(pParts) == 2 )
    { 
        partinfo * p1, * p2;
        p1 = array_fetch( partinfo *, pParts, 0 );
        p2 = array_fetch( partinfo *, pParts, 1 );
        if ( p1->bPart == p2->bPart )
            return Cudd_bddXnor( dd, p1->bVar, p2->bVar );
        else if ( p1->bPart == Cudd_Not(p2->bPart) )
            return Cudd_bddXor( dd, p1->bVar, p2->bVar );
    }

    // checking for disjoint supports
    pDisjs = Fmb_ImageDisjointSupport( dd, pParts );
    if ( pDisjs != NULL ) // there are disjoint support groups
    {
        DdNode * bImageNew, * bTemp;
        bImage = b1; Cudd_Ref( bImage );
        for ( i = 0; i < array_n(pDisjs); i++ )
        {
            pArray = array_fetch( array_t *, pDisjs, i );
            bImageNew = Fmb_Image_rec( dd, pArray, nBddSize );   
            if ( bImageNew == NULL )
            {
                Cudd_RecursiveDeref( dd, bImage );
                for ( k = i; k < array_n(pDisjs); k++ )
                {
                    pArray = array_fetch( array_t *, pDisjs, k );
                    array_free(pArray);
                }
                array_free( pDisjs );
                return NULL;
            }
            Cudd_Ref( bImageNew );

            bImage = Cudd_bddAnd( dd, bTemp = bImage, bImageNew );  Cudd_Ref( bImage );
            Cudd_RecursiveDeref( dd, bTemp );
            Cudd_RecursiveDeref( dd, bImageNew );
            array_free(pArray);
        }
        array_free( pDisjs );
        Cudd_Deref( bImage );
        return bImage;
    }

    // checking the BDD size
    if ( bImage = Fmb_ImageSmallSize( dd, pParts, nBddSize ) )
        return bImage;

    if ( s_Timeout && clock() > s_TimeLimit )
        return NULL;

    // the regular case - split around the first function in the array
    pParts0 = array_alloc( partinfo *, 10 );
    pParts1 = array_alloc( partinfo *, 10 );
    bImage0 = b1;  Cudd_Ref( bImage0 );
    bImage1 = b1;  Cudd_Ref( bImage1 );
    pStart  = array_fetch( partinfo *, pParts, 0 );
    for ( i = 1; i < array_n(pParts); i++ )
    {
        // get the current partition
        pCur = array_fetch( partinfo *, pParts, i );

        // constrain using the negative phase
        bConstr = Cudd_bddConstrain( dd, pCur->bPart, Cudd_Not(pStart->bPart) ); Cudd_Ref( bConstr );
        if ( Cudd_IsConstant(bConstr) )
        {
            bImage0 = Cudd_bddAnd( dd, bTemp = bImage0, Cudd_NotCond(pCur->bVar, (bConstr == b0)) );  Cudd_Ref( bImage0 );
            Cudd_RecursiveDeref( dd, bTemp );
            Cudd_RecursiveDeref( dd, bConstr );
        }
        else
        {
            pNew = Fmb_ImagePartInfoAllocate();
            pNew->bPart = bConstr; // takes ref
            pNew->bSupp = Cudd_Support( dd, bConstr );   Cudd_Ref( pNew->bSupp );
            pNew->bVar  = pCur->bVar;
            array_insert_last( partinfo *, pParts0, pNew );
        }

        // constrain using the positive phase
        bConstr = Cudd_bddConstrain( dd, pCur->bPart, pStart->bPart ); Cudd_Ref( bConstr );
        if ( Cudd_IsConstant(bConstr) )
        {
            bImage1 = Cudd_bddAnd( dd, bTemp = bImage1, Cudd_NotCond(pCur->bVar, (bConstr == b0)) );  Cudd_Ref( bImage1 );
            Cudd_RecursiveDeref( dd, bTemp );
            Cudd_RecursiveDeref( dd, bConstr );
        }
        else
        {
            pNew = Fmb_ImagePartInfoAllocate();
            pNew->bPart = bConstr; // takes ref
            pNew->bSupp = Cudd_Support( dd, bConstr );   Cudd_Ref( pNew->bSupp );
            pNew->bVar  = pCur->bVar;
            array_insert_last( partinfo *, pParts1, pNew );
        }
    }

    // call the recursive image computation
    bImageProper0 = Fmb_Image_rec( dd, pParts0, nBddSize );  
    if ( bImageProper0 == NULL )
    {
        Cudd_RecursiveDeref( dd, bImage0 );
        Cudd_RecursiveDeref( dd, bImage1 );
        Fmb_ImagePartInfoArrayFree( dd, pParts0 );
        Fmb_ImagePartInfoArrayFree( dd, pParts1 );
        return NULL;
    }
    Cudd_Ref( bImageProper0 );

    // call the recursive image computation
    bImageProper1 = Fmb_Image_rec( dd, pParts1, nBddSize );  
    if ( bImageProper1 == NULL )
    {
        Cudd_RecursiveDeref( dd, bImageProper0 );
        Cudd_RecursiveDeref( dd, bImage0 );
        Cudd_RecursiveDeref( dd, bImage1 );
        Fmb_ImagePartInfoArrayFree( dd, pParts0 );
        Fmb_ImagePartInfoArrayFree( dd, pParts1 );
        return NULL;
    }
    Cudd_Ref( bImageProper1 );

    Fmb_ImagePartInfoArrayFree( dd, pParts0 );
    Fmb_ImagePartInfoArrayFree( dd, pParts1 );

    // add to the image
    bImage0 = Cudd_bddAnd( dd, bTemp = bImage0, bImageProper0 );  Cudd_Ref( bImage0 );
    Cudd_RecursiveDeref( dd, bTemp );
    Cudd_RecursiveDeref( dd, bImageProper0 );
    bImage1 = Cudd_bddAnd( dd, bTemp = bImage1, bImageProper1 );  Cudd_Ref( bImage1 );
    Cudd_RecursiveDeref( dd, bTemp );
    Cudd_RecursiveDeref( dd, bImageProper1 );

    // compose the resulting image
    bImage = Cudd_bddIte( dd, pStart->bVar, bImage1, bImage0 ); Cudd_Ref( bImage );
    Cudd_RecursiveDeref( dd, bImage0 );
    Cudd_RecursiveDeref( dd, bImage1 );

    Cudd_Deref( bImage );
    return bImage;
}

/**Function*************************************************************

  Synopsis    [Splits the image using disjoint support of the functions.]

  Description [Returns NULL, if there is no way to split. Otherwise, returns
  the array of arrays holding the sets of disjoint partitions.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
array_t * Fmb_ImageDisjointSupport( DdManager * dd, array_t * pParts )
{
    array_t * pArray = NULL, * pPartNew;
    char ** pMatrix, * pSet, * pAll;
    int i, k, c, nComps, nCompsTotal;
    partinfo * p1, * p2;
    int fPrintPartitions = 0;
    
    assert( array_n(pParts) > 1 );

    // allocate the interaction matrix
    pMatrix = ALLOC( char *, array_n(pParts) );
    pMatrix[0] = ALLOC( char, array_n(pParts) * array_n(pParts) );
    for ( i = 1; i < array_n(pParts); i++ )
        pMatrix[i] = pMatrix[i-1] + sizeof(char) * array_n(pParts);

    if ( fPrintPartitions )
    {
        for ( i = 0; i < array_n(pParts); i++ )
        {
            p1 = array_fetch( partinfo *, pParts, i );
//          PRB( dd, p1->bSupp );
        }
    }

    // fill in the matrix
    for ( i = 0; i < array_n(pParts); i++ )
    {
        pMatrix[i][i] = 0;
        for ( k = i + 1; k < array_n(pParts); k++ )
        {
            p1 = array_fetch( partinfo *, pParts, i );
            p2 = array_fetch( partinfo *, pParts, k );
            pMatrix[i][k] = Extra_bddSuppOverlapping( dd, p1->bSupp, p2->bSupp );
            pMatrix[k][i] = pMatrix[i][k];
        }
        if ( fPrintPartitions )
        {
            for ( k = 0; k < array_n(pParts); k++ )
                printf( "%d", pMatrix[i][k] );
            printf( "\n" );
        }
    }

    // allocate the place for the set
    pSet = ALLOC( char, array_n(pParts) );
    pAll = ALLOC( char, array_n(pParts) );
    memset( pAll, 0, sizeof(char) * array_n(pParts) );

    // isolate strongly connected components
    nCompsTotal = 0;
    while ( nCompsTotal < array_n(pParts) )
    {
        // find the next comp
        nComps = 0;
        for ( i = 0; i < array_n(pParts); i++ )
            if ( pAll[i] == 0 )
            {
                // add this comp
                pAll[i]        = 1;
                pSet[nComps++] = i;
                nCompsTotal++;
                break;
            }
        // add all the related comps
        for ( c = 0; c < nComps; c++ )
            for ( k = 0; k < array_n(pParts); k++ )
                if ( pAll[k] == 0 && pMatrix[pSet[c]][k] )
                {
                    pAll[k]        = 1;
                    pSet[nComps++] = k;
                    nCompsTotal++;
                }
        // add this set of components to the result
        if ( nComps < array_n(pParts) )
        {
            if ( pArray == NULL )
                pArray = array_alloc( array_t *, 5 );
            // create the new set of partitions
            pPartNew = array_alloc( partinfo *, 10 );
            for ( i = 0; i < nComps; i++ )
            {
                p1 = array_fetch( partinfo *, pParts, pSet[i] );
                array_insert_last( partinfo *, pPartNew, p1 );
            }
            // insert the new set
            array_insert_last( array_t *, pArray, pPartNew );
        }
    }
    assert( nCompsTotal == array_n(pParts) );
    free( pMatrix[0] );
    free( pMatrix );
    free( pSet );
    free( pAll );

    if ( fPrintPartitions && pArray )
    {
        partinfo * p;
        array_t * pComp;
        int i, k;

        printf( "The total number of components = %d.\n", array_n(pParts) );
        for ( i = 0; i < array_n(pArray); i++ )
        {
            pComp = array_fetch( array_t *, pArray, i );
            printf( "The number of components = %d.\n", array_n(pComp) );
            for ( k = 0; k < array_n(pComp); k++ )
            {
                p = array_fetch( partinfo *, pComp, k );
//              PRB( dd, p->bSupp );
            }
        }
        i = 0;
    }

    return pArray;
}


/**Function*************************************************************

  Synopsis    [Solves small problems using monolithic transition relation.]

  Description [Returns NULL if the problem is larger than the given size. 
  Otherwise, returns the solution.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Fmb_ImageSmallSize( DdManager * dd, array_t * pParts, int nBddSize )
{
    DdNode * bTransRel, * bSupps;
    DdNode * bComp, * bTemp, * bImage;
    partinfo * pCur;
    int i, nNodes;

    assert( array_n(pParts) > 1 );

    nNodes = 0;
    for ( i = 0; i < array_n(pParts); i++ )
    {
        pCur    = array_fetch( partinfo *, pParts, i );
        nNodes += Cudd_DagSize( pCur->bPart );
        if ( nNodes >= nBddSize )
            return NULL;
    }

    // build the transition relation mapping X space into P space using bPFuncs
    bTransRel = b1;  Cudd_Ref( b1 );
    bSupps    = b1;  Cudd_Ref( b1 );
    for ( i = 0; i < array_n(pParts); i++ )
    {
        if ( s_Timeout && clock() > s_TimeLimit )
        {
            Cudd_RecursiveDeref( dd, bTransRel );
            Cudd_RecursiveDeref( dd, bSupps );
            return NULL;
        }

        // get the partition
        pCur = array_fetch( partinfo *, pParts, i );
        // get the component
        bComp = Cudd_bddXnor( dd, pCur->bPart, pCur->bVar );    Cudd_Ref( bComp );
        // multiply this component with the transitive relation
        bTransRel = Cudd_bddAnd( dd, bTemp = bTransRel, bComp );  Cudd_Ref( bTransRel );
        Cudd_RecursiveDeref( dd, bTemp );
        Cudd_RecursiveDeref( dd, bComp );
        // multiply this component with the transitive relation
        bSupps = Cudd_bddAnd( dd, bTemp = bSupps, pCur->bSupp );  Cudd_Ref( bSupps );
        Cudd_RecursiveDeref( dd, bTemp );
    }

    bImage = Cudd_bddExistAbstract( dd, bTransRel, bSupps );    Cudd_Ref( bImage );
    Cudd_RecursiveDeref( dd, bTransRel );
    Cudd_RecursiveDeref( dd, bSupps );

    Cudd_Deref( bImage );
    return bImage;
}


    
/**Function*************************************************************

  Synopsis    [Allocates the data structure to store one partition.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
partinfo * Fmb_ImagePartInfoAllocate()
{
    return ALLOC( partinfo, 1 );
}

/**Function*************************************************************

  Synopsis    [Delocates the data structure.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fmb_ImagePartInfoDelocate( DdManager * dd, partinfo * p )
{
    if ( p->bPart )   Cudd_RecursiveDeref( dd, p->bPart );
    if ( p->bSupp )   Cudd_RecursiveDeref( dd, p->bSupp );
    free( p );
}

/**Function*************************************************************

  Synopsis    [Frees the array of partitions.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fmb_ImagePartInfoArrayFree( DdManager * dd, array_t * pParts )
{
    int i;
    partinfo * pCur;
    // free the partitions
    for ( i = 0; i < array_n(pParts); i++ )
    {
        pCur = array_fetch( partinfo *, pParts, i );
        Fmb_ImagePartInfoDelocate( dd, pCur );
    }
    array_free( pParts );
}


/**Function********************************************************************

  Synopsis    [Sets the timeout.]

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
void Fmb_ImageTimeoutSet( int timeout )
{
    s_Timeout = timeout;
}


/**Function********************************************************************

  Synopsis    [Resets the timeout.]

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
void Fmb_ImageTimeoutReset()
{
    s_Timeout = 0;
}

