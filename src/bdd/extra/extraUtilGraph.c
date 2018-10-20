/**CFile****************************************************************

  FileName    [extraUtilGraph.c]

  PackageName [extra]

  Synopsis    [Various reusable software utilities.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 1, 2003.]

  Revision    [$Id: extraUtilGraph.c,v 1.1 2004/01/06 21:02:54 alanmi Exp $]

***********************************************************************/

#include "extra.h"

/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Stucture declarations                                                     */
/*---------------------------------------------------------------------------*/

typedef unsigned long             Extra_BitWord_t;
typedef struct Extra_BitVert_t_   Extra_BitVert_t;

struct Extra_BitVert_t_
{
    Extra_BitVert_t * pNext;        // the pointer to the next node in the list
    unsigned          iLast   :  9; // the index of the last word
    unsigned          nUnused :  6; // the number of unused bits in the last word
    unsigned          fFlag1  :  1; // custom mark
    unsigned          fFlag2  :  1; // custom mark
    unsigned          nOnes   : 15; // the number of 1's in the bit data
    Extra_BitWord_t   pData[1];     // the first Extra_BitWord_t filled with bit data

};

struct Extra_BitGraph_t_
{
    int                nVerts;       // the number of vertices
    int                nEdges;       // the number of edges
    int                nWords;
    int                nUnused;
    int                nBytes;       // the number of bytes per vertex
    Extra_BitVert_t ** ppVerts;
    Extra_BitVert_t *  pRoot;        // the pointer to the linked list of vertices
    Extra_BitVert_t *  pColors;      // the linked list of colored vertices
    int                nColors;      // the number of colors
    int *              pOnes;        // the number of 1's
    Extra_MmFixed_t *  pMem;         // the memory manager
};

/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/

static int bit_count[256] = {
  0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4,1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
  1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
  1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
  2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
  1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
  2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
  2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
  3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8
};

/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/


// these parameters can be computed but setting them manually makes it faster
#define BITS_PER_WORD         32                      // sizeof(Extra_BitWord_t) * 8 
#define BITS_PER_WORD_MINUS   31                      // the same minus 1
#define BITS_PER_WORD_LOG     5                       // log2(sizeof(Extra_BitWord_t) * 8)
#define BITS_FULL             ((Extra_BitWord_t)0xffffffff)  // the mask of the type "11111111"

// getting one data bit
#define Extra_BitGraphWhichWord(Bit)       ((Bit) >> BITS_PER_WORD_LOG)
#define Extra_BitGraphWhichBit(Bit)        ((Bit) &  BITS_PER_WORD_MINUS)
// accessing individual bits
#define Extra_BitGraphBitValue(Vert, Bit) (((Vert)->pData[Extra_BitGraphWhichWord(Bit)] &   (((Extra_BitWord_t)1)<<(Extra_BitGraphWhichBit(Bit)))) > 0)
#define Extra_BitGraphBitInsert(Vert, Bit) ((Vert)->pData[Extra_BitGraphWhichWord(Bit)] |=  (((Extra_BitWord_t)1)<<(Extra_BitGraphWhichBit(Bit))))
#define Extra_BitGraphBitRemove(Vert, Bit) ((Vert)->pData[Extra_BitGraphWhichWord(Bit)] &= ~(((Extra_BitWord_t)1)<<(Extra_BitGraphWhichBit(Bit))))
// complementing the cube
#define Extra_BitGraphBitNot( Vert )\
{\
    int _i_;\
    (Vert)->pData[(Vert)->iLast] ^= (BITS_FULL >> (Vert)->nUnused);\
    for( _i_ = (Vert)->iLast - 1; _i_ >= 0; _i_-- )\
        (Vert)->pData[_i_] ^=  BITS_FULL;\
}
// filling the cube with ones
#define Extra_BitGraphBitFill( Vert )\
{\
    int _i_;\
    (Vert)->pData[(Vert)->iLast] = (BITS_FULL >> (Vert)->nUnused);\
    for( _i_ = (Vert)->iLast - 1; _i_ >= 0; _i_-- )\
        (Vert)->pData[_i_] =  BITS_FULL;\
}
// checks if the vertext is empty
#define Extra_BitGraphEmpty( Res, Vert )\
{\
    int _i_; Res = 1;\
    for (_i_ = (Vert)->iLast; _i_ >= 0; _i_--)\
        if ( (Vert)->pData[_i_] )\
           { Res = 0; break; }\
}
// copying the bits
#define Extra_BitGraphBitCopy( Vert1, Vert2 )\
{\
    int _i_;\
    for (_i_ = (Vert1)->iLast; _i_ >= 0; _i_--)\
        ((Vert1)->pData[_i_]) = ((Vert2)->pData[_i_]);\
}
// intersection of bits
#define Extra_BitGraphBitAnd( VertR, Vert1, Vert2 )\
{\
    int _i_;\
    for (_i_ = (Vert1)->iLast; _i_ >= 0; _i_--)\
        (((VertR)->pData[_i_]) = ((Vert1)->pData[_i_] & (Vert2)->pData[_i_]));\
} 
// union of bits
#define Extra_BitGraphBitOr( VertR, Vert1, Vert2 )\
{\
    int _i_;\
    for (_i_ = (Vert1)->iLast; _i_ >= 0; _i_--)\
        (((VertR)->pData[_i_]) = ((Vert1)->pData[_i_] | (Vert2)->pData[_i_]));\
} 
// sharp of bits
#define Extra_BitGraphBitSharp( VertR, Vert1, Vert2 )\
{\
    int _i_;\
    for (_i_ = (Vert1)->iLast; _i_ >= 0; _i_--)\
        (((VertR)->pData[_i_]) = ((Vert1)->pData[_i_] & ~((Vert2)->pData[_i_])));\
} 

/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

static Extra_BitVert_t *  Extra_BitGraphVertAlloc( Extra_BitGraph_t * pGraph );
static Extra_BitVert_t *  Extra_BitGraphColorStep( Extra_BitGraph_t * pGraph, Extra_BitVert_t * pMask );
static int                Extra_BitGraphColorCountOnes( Extra_BitGraph_t * pGraph, Extra_BitVert_t * pMask );
static int                Extra_BitGraphColorCountOnesInter( Extra_BitVert_t * pVert1, Extra_BitVert_t * pVert2 );
static void               Extra_BitGraphCleanFlag1( Extra_BitGraph_t * pGraph );

/**AutomaticEnd***************************************************************/

/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function*************************************************************

  Synopsis    [Starts the graph represented as bit matrix.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Extra_BitGraph_t * Extra_BitGraphAlloc( int nVerts )
{
    Extra_BitVert_t * pVert, ** ppVert;
    Extra_BitGraph_t *  pGraph;
    int nBitsInUnsigned, i;

    // start the graph
    pGraph = ALLOC( Extra_BitGraph_t, 1 );
    memset( pGraph, 0, sizeof(Extra_BitGraph_t) );

    // set the parameters
    nBitsInUnsigned  = 8 * sizeof(Extra_BitWord_t);
    pGraph->nVerts  = nVerts;
    pGraph->nEdges  = 0;
    pGraph->nWords  = nVerts / nBitsInUnsigned + (int)(nVerts % nBitsInUnsigned > 0);
    pGraph->nUnused = pGraph->nWords * nBitsInUnsigned - pGraph->nVerts;
    pGraph->nBytes  = sizeof(Extra_BitVert_t) + sizeof(Extra_BitWord_t) * (pGraph->nWords - 1);

    // allocate memory
    pGraph->pMem    = Extra_MmFixedStart( pGraph->nBytes, 1000, 1000 );
    pGraph->ppVerts = ALLOC( Extra_BitVert_t *, pGraph->nVerts );
    pGraph->pOnes   = ALLOC( int, pGraph->nVerts );

    // creat the empty graph
    ppVert = &pGraph->pRoot;
    for ( i = 0; i < nVerts; i++ )
    {
        pVert = Extra_BitGraphVertAlloc( pGraph );
        // connect to the list
        *ppVert = pVert;
        ppVert = &pVert->pNext;
        // add to the array
        pGraph->ppVerts[i] = pVert;
    }
    *ppVert = NULL;

    return pGraph;
}

/**Function*************************************************************

  Synopsis    [Starts the graph representation as a bit matrix.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Extra_BitVert_t * Extra_BitGraphVertAlloc( Extra_BitGraph_t * pGraph )
{
    Extra_BitVert_t * pVert;
    pVert = (Extra_BitVert_t *)Extra_MmFixedEntryFetch( pGraph->pMem );
    memset( pVert, 0, pGraph->nBytes );
    pVert->iLast   = pGraph->nWords - 1;
    pVert->nUnused = pGraph->nUnused;
    return pVert;
}

/**Function*************************************************************

  Synopsis    [Add edge to the graph.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_BitGraphEdgeAdd( Extra_BitGraph_t * pGraph, int i, int j )
{
    assert( i < pGraph->nVerts );
    assert( j < pGraph->nVerts );
    assert( i != j );
    assert( Extra_BitGraphBitValue( pGraph->ppVerts[i], j ) == 0 );
    assert( Extra_BitGraphBitValue( pGraph->ppVerts[j], i ) == 0 );
    Extra_BitGraphBitInsert( pGraph->ppVerts[i], j );
    Extra_BitGraphBitInsert( pGraph->ppVerts[j], i );
    pGraph->ppVerts[i]->nOnes++;
    pGraph->ppVerts[j]->nOnes++;
    pGraph->nEdges += 1;
}

/**Function*************************************************************

  Synopsis    [Delete the graph representation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_BitGraphFree( Extra_BitGraph_t * pGraph )
{
    Extra_MmFixedStop( pGraph->pMem, 0 );
    FREE( pGraph->ppVerts );
    FREE( pGraph->pOnes );
    FREE( pGraph );
}


/**Function*************************************************************

  Synopsis    [Inverts the current graph.]

  Description [Useful to derive the incompatibility graph from the
  compatibility graph, and vice versa.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_BitGraphInvert( Extra_BitGraph_t * pGraph )
{
    Extra_BitVert_t * pVert;
    int i;
    for ( i = 0; i < pGraph->nVerts; i++ )
    {
        pVert = pGraph->ppVerts[i];
        Extra_BitGraphBitNot( pVert );
        pVert->nOnes = pGraph->nVerts - pVert->nOnes;
    }
    pGraph->nEdges = pGraph->nVerts * (pGraph->nVerts - 1) / 2 - pGraph->nEdges;
}

/**Function*************************************************************

  Synopsis    [Generate a random graph.]

  Description [The Density is the ratio of edges to the total number of 
  edges.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Extra_BitGraph_t * Extra_BitGraphRandom( int nVerts, double Density )
{
    Extra_BitGraph_t * pGraph;
    int nEdges;
    int Vert1, Vert2, i;

    // start the graph
    pGraph = Extra_BitGraphAlloc( nVerts );
    // get the number of random edges
    nEdges = (int)(Density * nVerts * (nVerts - 1) / 2); 

    // generate the random edges
    srand( time(NULL) );
    for ( i = 0; i < nEdges; i++ )
    {
        Vert1 = rand() % nVerts;
        Vert2 = rand() % nVerts;
        // skip if the same vertex
        if ( Vert1 == Vert2 )
        {
            i--;
            continue;
        }
        // skip if this edge is already in the graph
        if ( Extra_BitGraphBitValue( pGraph->ppVerts[Vert1], Vert2 ) )
        {
            i--;
            continue;
        }
        Extra_BitGraphEdgeAdd( pGraph, Vert1, Vert2 );
    }
    return pGraph;
}

/**Function*************************************************************

  Synopsis    [Prints the graph.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_BitGraphPrint( Extra_BitGraph_t * pGraph )
{
    int i, k;
    printf( "The graph has %d vertices and %d edges. Density = %.2f %%.\n", 
        pGraph->nVerts, pGraph->nEdges, 
        200.0 * pGraph->nEdges / pGraph->nVerts / (pGraph->nVerts - 1) );
    printf( "      : " );
    for ( i = 0; i < pGraph->nVerts; i++ )
        printf( "%d", i % 10 );
    printf( "\n" );
    for ( i = 0; i < pGraph->nVerts; i++ )
    {
        printf( " %4d : ", i );
        for ( k = 0; k < pGraph->nVerts; k++ )
            printf( "%d", Extra_BitGraphBitValue( pGraph->ppVerts[i], k ) );
        printf( "\n" );
    }
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    [Verifies that the coloring is correct.]

  Description [Returns 1 if the coloring is correct.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Extra_BitGraphColorVerify( Extra_BitGraph_t * pGraph, int * pColors )
{
    int c1, c2;
    int RetValue = 1;
    int Counter = 0;
    // the coloring is correct if every two nodes in the 
    // incompatibility graph are colored with different colors
    for ( c1 = 0; c1 < pGraph->nVerts; c1++ )
        for ( c2 = c1 + 1; c2 < pGraph->nVerts; c2++ )
            if ( Extra_BitGraphBitValue( pGraph->ppVerts[c1], c2 ) )
                if ( pColors[c1] == pColors[c2] )
                {
                    printf( "Incompatible vertices %2d and %2d have the same color.\n", c1, c2 );
                    RetValue = 0;
                    if ( Counter++ == 10 )
                        return 0;
                }
//    if ( RetValue )
//        printf( "Graph Coloring: Verification is okay.\n" );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Solves the graph coloring problem using clique covering.]

  Description [Returns the number of colors.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Extra_BitGraphColor( Extra_BitGraph_t * pGraph, int * pColors, int fVerbose )
{
    ProgressBar * pProgress;
    Extra_BitVert_t * pClique;
    Extra_BitVert_t * pMask;
    char Buffer[100];
    int iColor, v, Res;
    int nColors, nColorsNext;
    int clk;

    clk = clock();

    if ( fVerbose )
        Extra_BitGraphPrint( pGraph );

    // create the compatibility graph
    Extra_BitGraphInvert( pGraph );
    Extra_BitGraphCleanFlag1( pGraph );

//    if ( fVerbose )
//        Extra_BitGraphPrint( pGraph );

    // start with the mask contained all vertices
    pMask = Extra_BitGraphVertAlloc( pGraph );
    Extra_BitGraphBitFill( pMask );
    // extract one clique at a time
    nColors = 0;
    nColorsNext = 0;
    pProgress = Extra_ProgressBarStart( stdout, pGraph->nVerts );
    while ( 1 )
    {
        // check if the vertex is empty
        Extra_BitGraphEmpty( Res, pMask );        
        if ( Res == 1 )
            break;
        // get the next clique
        pClique = Extra_BitGraphColorStep( pGraph, pMask );
        // subtract these vertices from the mask
        Extra_BitGraphBitSharp( pMask, pMask, pClique );
        // add the clique to the list
        pClique->pNext = pGraph->pColors;
        pGraph->pColors = pClique;        

        // update the number of vertices processed
        nColors += Extra_BitGraphColorCountOnesInter( pClique, pClique );

        if ( nColors >= nColorsNext )
        {
            sprintf( Buffer, "%.2f %%", 100.0 * nColors / pGraph->nVerts );
            Extra_ProgressBarUpdate( pProgress, nColors, Buffer );
            fflush( stdout );
            nColorsNext = nColors + 50;
        }
    }
    Extra_ProgressBarStop( pProgress );
    Extra_MmFixedEntryRecycle( pGraph->pMem, (char *)pMask );

    // create the incompatibility graph
    Extra_BitGraphCleanFlag1( pGraph );
    Extra_BitGraphInvert( pGraph );
 
    // translate the colors into the array
    for ( v = 0; v < pGraph->nVerts; v++ )
        pColors[v] = -1;
    iColor = 0;
    for ( pClique = pGraph->pColors; pClique; pClique = pClique->pNext )
    {
        for ( v = 0; v < pGraph->nVerts; v++ )
            if ( Extra_BitGraphBitValue( pClique, v ) )
            {
                assert( pColors[v] == -1 );
                pColors[v] = iColor;
            }
        iColor++;
    }
    for ( v = 0; v < pGraph->nVerts; v++ )
    {
        assert( pColors[v] != -1 );
    }

    if ( fVerbose )
    {
        PRT( "Coloring time", clock() - clk );
        printf( "The total number of colors = %d.\n", iColor );
        for ( v = 0; v < pGraph->nVerts; v++ )
            printf( "Vert = %2d. Color = %c.\n", v, 'A' + (char)pColors[v] );
    }
    Extra_BitGraphColorVerify( pGraph, pColors );
    return iColor;
}

/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Definition of static Functions                                            */
/*---------------------------------------------------------------------------*/

/**Function*************************************************************

  Synopsis    [Finds the best clique of the graph.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Extra_BitVert_t * Extra_BitGraphColorStep( Extra_BitGraph_t * pGraph, Extra_BitVert_t * pMask )
{
    Extra_BitVert_t * pClique, * pVert;
    int Best;

    // start with the candidate set of nodes equal to the mask
    pClique = Extra_BitGraphVertAlloc( pGraph );
    Extra_BitGraphBitCopy( pClique, pMask );

    // remove vertices from the clique
    while ( (Best = Extra_BitGraphColorCountOnes( pGraph, pClique )) != -1 )
    {
        // get the new vertex
        pVert = pGraph->ppVerts[Best];
        // intersect it with the current clique
        Extra_BitGraphBitAnd( pClique, pClique, pVert );
        // mark this vertex as used
        pVert->fFlag1 = 1;
    }
    return pClique;
}

/**Function*************************************************************

  Synopsis    [First the row that has most ones overlapping with Mask.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Extra_BitGraphColorCountOnes( Extra_BitGraph_t * pGraph, Extra_BitVert_t * pMask )
{
    Extra_BitVert_t * pVert;
    int nOnesBest, nOnesCur, Best, i;
    Best = -1;
    nOnesBest = -1;
    for ( i = 0; i < pGraph->nVerts; i++ )
    {
        pVert = pGraph->ppVerts[i];
        if ( pVert->fFlag1 ) // already taken
            continue;
        if ( Extra_BitGraphBitValue( pMask, i ) == 0 ) // not in the current clique
            continue;
        nOnesCur = Extra_BitGraphColorCountOnesInter( pVert, pMask );
        if ( nOnesBest < nOnesCur )
        {
             nOnesBest = nOnesCur;
             Best = i;
        }
    }
    return Best;
}

/**Function*************************************************************

  Synopsis    [Returns the number of 1's in the intersection of the two cubes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Extra_BitGraphColorCountOnesInter( Extra_BitVert_t * pVert1, Extra_BitVert_t * pVert2 )
{
    Extra_BitWord_t Word;
    unsigned char * pByte;
    int nOnes, i;
    // clean the counter of ones
    nOnes = 0;
    for ( i = pVert1->iLast; i >= 0; i-- )
    {
        Word   = pVert1->pData[i] & pVert2->pData[i];
        pByte  = (unsigned char *)&Word;
        nOnes += bit_count[pByte[0]];
        nOnes += bit_count[pByte[1]];
        nOnes += bit_count[pByte[2]];
        nOnes += bit_count[pByte[3]];
    }
    return nOnes;
}

/**Function*************************************************************

  Synopsis    [Clean pVert->fFlag1 in all the vertices of the graph.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_BitGraphCleanFlag1( Extra_BitGraph_t * pGraph )
{
    int i;
    for ( i = 0; i < pGraph->nVerts; i++ )
        pGraph->ppVerts[i]->fFlag1 = 0;
}


/**Function*************************************************************

  Synopsis    [Little test program.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_BitGraphColorTest( int nVerts )
{
    Extra_BitGraph_t * pGraph;
    int * pColors;
    int nColors;

    pColors = ALLOC( int, nVerts );
    pGraph  = Extra_BitGraphRandom( nVerts, 0.2 );
    nColors = Extra_BitGraphColor( pGraph, pColors, 1 );
    Extra_BitGraphFree( pGraph );
    FREE( pColors );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


