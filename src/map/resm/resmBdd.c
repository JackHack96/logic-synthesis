/**CFile****************************************************************

  FileName    [resmBdd.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Generic resynthesis engine.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 8, 2003.]

  Revision    [$Id: resmBdd.c,v 1.1 2005/02/28 05:34:33 alanmi Exp $]

***********************************************************************/

#include "resmInt.h"
#include "cuddInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

/* this file contains a BDD based implementation of the resubstitution test */

// these procedures are exported

// call this procedure after the set of all candidates is computed
// before the time when individual set of candidates are tested
extern int Resm_NodeBddsPrepare( Resm_Node_t * pNode );
// call this procedure to evaluate an individual set of candidates
extern int Resm_NodeBddsCheckResub( Resm_Node_t * pNode, Resm_Node_t ** ppCands, int nCands );
// call this procedure after the evaluation of individual candidates is finished
extern int Resm_NodeBddsCleanup( Resm_Node_t * pNode );
// call this procedure at the very end to free the BDD manager and check that it is okay
extern void Resm_NodeBddsFree();


// this static variable is initialized on the first call 
// and not deallocated in the end, resulting in memory leaks,
// but the quick experiments it is okay
static DdManager * dd = NULL;

// the limits on the number of global and local variables
static int nVarsLimit = 100;
static int nVarsY     = 10;

// other static functions
static DdNode * Resm_NodeBddsBuild( DdManager * dd, Resm_Node_t * pNode );
static DdNode * Resm_NodeBddsBuildCube( Mvc_Cover_t * pMvc, Mvc_Cube_t * pCube, DdManager * dd, DdNode * pbFuncs[], int nFuncs );

// external procedure to compute the image using output splitting 
// (this procedure comes from the file "src\opt\fmb\fmbImage.c")
extern DdNode * Fmb_ImageCompute( DdManager * dd, DdNode ** pbFuncs, DdNode ** pbVars, 
           DdNode * bCareSet, int nFuncs, int nBddSize );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [This procedure prepares BDDs for the nodes in the window.]

  Description [The window is given by p->vWinTotal (all nodes) and
  p->vWinNodes (internal nodes). The rest of the nodes are PIs of the window.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Resm_NodeBddsPrepare( Resm_Node_t * pNode )
{
    Resm_NodeVec_t * vWinTotal = pNode->p->vWinTotal;
    Resm_NodeVec_t * vWinNodes = pNode->p->vWinNodes;
    int i, k;

    // check whether the capacity of the manager allows to evaluate this node
    if ( pNode->p->vWinTotal->nSize - pNode->p->vWinNodes->nSize > nVarsLimit )
    {
        printf( "The number of PIs (%d) is more than the limit (%d).\n", 
            pNode->p->vWinTotal->nSize - pNode->p->vWinNodes->nSize, nVarsLimit );
        return 0;
    }

    // start the manager
    if ( dd == NULL )
        dd = Cudd_Init( nVarsLimit + nVarsY, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
    // the manager is never reorered (this would make computation very slow)

    // clean the temporary BDD slot
    for ( i = 0; i < vWinTotal->nSize; i++ )
        vWinTotal->pArray[i]->pData[0] = NULL;

    // mark the internal nodes 
    for ( i = 0; i < vWinNodes->nSize; i++ )
        vWinNodes->pArray[i]->fMark0 = 1;
    // set the elementary BDDs
    for ( i = k = 0; i < vWinTotal->nSize; i++ )
        if ( vWinTotal->pArray[i]->fMark0 == 0 )
        {
            vWinTotal->pArray[i]->pData[0] = (char *)dd->vars[k++];  
            Cudd_Ref( (DdNode *)vWinTotal->pArray[i]->pData[0] );
        }
    // unmark the internal nodes
    for ( i = 0; i < vWinNodes->nSize; i++ )
        vWinNodes->pArray[i]->fMark0 = 0;

    // construct the BDDs in the topological order
    // (fortunately, the nodes in the array vWinTotal are in the topological order)
    for ( i = 0; i < vWinTotal->nSize; i++ )
    {
        if ( vWinTotal->pArray[i]->pData[0] )
            continue;
        vWinTotal->pArray[i]->pData[0] = (char *)Resm_NodeBddsBuild( dd, vWinTotal->pArray[i] );  
        Cudd_Ref( (DdNode *)vWinTotal->pArray[i]->pData[0] );
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Computes the BDD of the node, if the BDDs of the fanins are known.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Resm_NodeBddsBuild( DdManager * dd, Resm_Node_t * pNode )
{
    Resm_Node_t * pFanin;
    DdNode * pbFuncs[10];
    DdNode * bRes, * bCube, * bTemp;
    Mvc_Cover_t * pMvc;
    Mvc_Cube_t * pCube;
    int i;

    // collect the fanin BDDs (remember that fanins can be complemented)
    for ( i = 0; i < (int)pNode->nLeaves; i++ )
    {
        pFanin     = Resm_Regular(pNode->ppLeaves[i]);
        pbFuncs[i] = Cudd_NotCond( (DdNode *)pFanin->pData[0], Resm_IsComplement(pNode->ppLeaves[i]) );
    }
    // get the cover
    pMvc = Mio_GateReadMvc(pNode->pGate);
    // compute BDDs for the output of the node
    bRes = b0;  Cudd_Ref( bRes );
    Mvc_CoverForEachCube( pMvc, pCube )
    {
        bCube = Resm_NodeBddsBuildCube( pMvc, pCube, dd, pbFuncs, pNode->nLeaves );  Cudd_Ref( bCube );
        bRes  = Cudd_bddOr( dd, bTemp = bRes, bCube );   Cudd_Ref( bRes );
        Cudd_RecursiveDeref( dd, bCube );
        Cudd_RecursiveDeref( dd, bTemp );
    }
    Cudd_Deref( bRes );
    return bRes;
}

/**Function*************************************************************

  Synopsis    [Simulate one word.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Resm_NodeBddsBuildCube( Mvc_Cover_t * pMvc, Mvc_Cube_t * pCube, DdManager * dd, DdNode * pbFuncs[], int nFuncs )
{
    DdNode * bRes, * bVar, * bTemp;
    int iVar, Value;

    bRes = b1;  Cudd_Ref( bRes );
    Mvc_CubeForEachVarValue( pMvc, pCube, iVar, Value )
    {
        if ( Value == 1 )
            bVar = Cudd_Not( pbFuncs[iVar] );
        else if ( Value == 2 )
            bVar = pbFuncs[iVar];
        else
            continue;
        bRes  = Cudd_bddAnd( dd, bTemp = bRes, bVar );   Cudd_Ref( bRes );
        Cudd_RecursiveDeref( dd, bTemp );
    }
    Cudd_Deref( bRes );
    return bRes;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Resm_NodeBddsCleanup( Resm_Node_t * pNode )
{
    Resm_NodeVec_t * vWinTotal = pNode->p->vWinTotal;
    int i;
    for ( i = 0; i < vWinTotal->nSize; i++ )
    {
        Cudd_RecursiveDeref( dd, (DdNode *)vWinTotal->pArray[i]->pData[0] );
        vWinTotal->pArray[i]->pData[0] = NULL;
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if resubstitution is possible.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Resm_NodeBddsCheckResub( Resm_Node_t * pNode, Resm_Node_t ** ppCands, int nCands )
{
    DdNode * bImg0, * bImg1;
    DdNode * pbFuncs[10];
    DdNode * bCareSet;
    int i, RetValue;

    // collect the nodes
    assert( nCands < 10 );
    for ( i = 0; i < nCands; i++ )
        pbFuncs[i] = (DdNode *)ppCands[i]->pData[0];

    // compute the first image
    bCareSet = (DdNode *)pNode->pData[0];
    bImg0 = Fmb_ImageCompute( dd, pbFuncs, dd->vars + nVarsLimit, Cudd_Not(bCareSet), nCands, 100 ); Cudd_Ref( bImg0 );
    bImg1 = Fmb_ImageCompute( dd, pbFuncs, dd->vars + nVarsLimit, bCareSet, nCands, 100 ); Cudd_Ref( bImg1 );
    RetValue = Cudd_bddLeq( dd, bImg0, Cudd_Not(bImg1) );
    Cudd_RecursiveDeref( dd, bImg0 );
    Cudd_RecursiveDeref( dd, bImg1 );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Cleans up the BDD manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Resm_NodeBddsFree()
{
    int RetValue;
    // check for remaining references in the package
    RetValue = Cudd_CheckZeroRef( dd );
    if ( RetValue > 0 )
        printf( "\nThe number of referenced nodes = %d\n\n", RetValue );
//  Cudd_PrintInfo( dd, stdout );
    Cudd_Quit( dd );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


