/**CFile****************************************************************

  FileName    [shRewrite.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Rewriting of the AND-INV graph.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: shRewrite.c,v 1.1 2004/04/12 03:42:31 alanmi Exp $]

***********************************************************************/

#include "shInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

extern void Sh_CutSetsCompute( Sh_Manager_t * pMan, Sh_Node_t * ppNodes[], int nNodes );

static Sh_Node_t * Sh_GraphRewriteCone( Sh_Manager_t * pMan, Sh_Node_t * ppNodes[], int nNodes, char * pFormula );
static Sh_Node_t * Sh_GraphRewriteCone_rec( Sh_Manager_t * pMan, Sh_Node_t * ppNodes[], char * pBegin, char * pEnd );
static char * Sh_GraphFindEnd( char * pBegin );

static unsigned Sh_ReadUnsigned( char ** ppCur );
static char * Sh_ReadFormula( char ** ppCur );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Performs rewriting of the AND-INV graph.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sh_Node_t * Sh_GraphRewrite( Sh_Manager_t * pMan, Sh_Node_t * pNode )
{
    Sh_Node_t ** ppNodes;
    Sh_Node_t * pRes = NULL;
    int nNodes;
    // collects the nodes in the DFS order
    nNodes = Sh_NodeCollectDfs( pMan, pNode, &ppNodes );
    // compute the cuts sets of each node
    Sh_CutSetsCompute( pMan, ppNodes, nNodes );
    
    // determine the largest cutsets for each node
    // re-write the graph in topological order
    // make sure it is indeed small than the original graph
    return pRes;
}







/**Function*************************************************************

  Synopsis    [Build the AND-INV graph representing the formula.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sh_Node_t * Sh_GraphRewriteCone( Sh_Manager_t * pMan, Sh_Node_t * ppNodes[], int nNodes, char * pFormula )
{
//    char * pFormula = "(<<<Ac><Ca>>(BD)>(<<Ca>d><<Da>(Cb)>))";
//    pRes = Sh_GraphRewriteCone( pMan, pMan->pVars, 4, pFormula );
    char * pBegin, * pEnd;
    pBegin = pFormula;
    pEnd   = pFormula + strlen(pFormula);
    return Sh_GraphRewriteCone_rec( pMan, ppNodes, pBegin, pEnd );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sh_Node_t * Sh_GraphRewriteCone_rec( Sh_Manager_t * pMan, Sh_Node_t * ppNodes[], char * pBegin, char * pEnd )
{
    Sh_Node_t * pNode1, * pNode2;
    char * pBegin1, * pBegin2, * pEnd1, * pEnd2;

    if ( pEnd - pBegin == 1 )
    {
        if ( *pBegin < 'a' ) // upper-case
            return Sh_Not( ppNodes[*pBegin-'A'] );
        else
            return ppNodes[*pBegin-'a'];
    }

    // get the left branch
    pBegin1 = pBegin;
    if ( *pBegin1 == '<' )
    {
        pEnd1 = Sh_GraphFindEnd( pBegin1 );
        pNode1 = Sh_GraphRewriteCone_rec( pMan, ppNodes, pBegin1 + 1, pEnd1 );
        pNode1 = Sh_Not( pNode1 );
    }
    else if ( *pBegin1 == '(' )
    {
        pEnd1 = Sh_GraphFindEnd( pBegin1 );
        pNode1 = Sh_GraphRewriteCone_rec( pMan, ppNodes, pBegin1 + 1, pEnd1 );
    }
    else
    {
        if ( *pBegin1 < 'a' ) // upper-case
            pNode1 = Sh_Not( ppNodes[*pBegin1-'A'] );
        else
            pNode1 = ppNodes[*pBegin1-'a'];
        pEnd1 = pBegin1;
    }
    if ( pEnd1 + 1 == pEnd )
        return pNode1;

    pBegin2 = pEnd1 + 1;
    if ( *pBegin2 == '<' )
    {
        pEnd2 = Sh_GraphFindEnd( pBegin2 );
        pNode2 = Sh_GraphRewriteCone_rec( pMan, ppNodes, pBegin2 + 1, pEnd2 );
        pNode2 = Sh_Not( pNode2 );
    }
    else if ( *pBegin2 == '(' )
    {
        pEnd2 = Sh_GraphFindEnd( pBegin2 );
        pNode2 = Sh_GraphRewriteCone_rec( pMan, ppNodes, pBegin2 + 1, pEnd2 );
    }
    else
    {
        if ( *pBegin2 < 'a' ) // upper-case
            pNode2 = Sh_Not( ppNodes[*pBegin2-'A'] );
        else
            pNode2 = ppNodes[*pBegin2-'a'];
        pEnd2 = pBegin2;
    }
    assert( pEnd2 + 1 == pEnd );
    return Sh_NodeAnd( pMan, pNode1, pNode2 );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Sh_GraphFindEnd( char * pBegin )
{
    char * pTemp;
    int Counter = 0;

    assert( *pBegin == '<' || *pBegin == '(' );
    for ( pTemp = pBegin; pTemp; pTemp++ )
    {
        if ( *pTemp == '<' || *pTemp == '(' )
            Counter++;
        else if ( *pTemp == '>' || *pTemp == ')' )
            Counter--;
        if ( Counter == 0 )
            return pTemp;
    }
    return NULL;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sh_ManagerReadTable( Sh_Manager_t * p, char * pFileName )
{
    FILE * pFile;
    char * pCur;
    unsigned uTruth;
    char * pFormula;
    pFile = fopen( pFileName, "rb" );
    if ( pFile == NULL )
    {
        printf( "Sh_ManagerAlloc(): Cannot open the precomputed file \"%s\".\n", pFileName );
        return;
    }
    p->pBuffer    = Extra_FileRead( pFile );
    fclose( pFile );
    p->tSubgraphs = st_init_table(st_ptrcmp,st_ptrhash);
    pCur          = p->pBuffer;
    while ( *pCur )
    {
        uTruth = Sh_ReadUnsigned( &pCur );
        pFormula = Sh_ReadFormula( &pCur );
        if ( st_insert( p->tSubgraphs, (char *)uTruth, pFormula ) )
        {
            assert( 0 );
        }
//        if ( uTruth > 32760 )
//        printf( "Adding %5d   %s\n", uTruth, pFormula );
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned Sh_ReadUnsigned( char ** ppCur )
{
    unsigned Number = 0;
    int i;
    while ( **ppCur != '0' && **ppCur != '1' )
        (*ppCur)++;
    assert ( **ppCur == '0' || **ppCur == '1' );
    for ( i = 0; i < 16; i++ )
    {
        if ( **ppCur == '1' )
            Number |= (1 << i);
        (*ppCur)++;
    }
    return Number;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Sh_ReadFormula( char ** ppCur )
{
    char * pResult;
    while ( **ppCur == ' ' )
        (*ppCur)++;
    pResult = *ppCur;
    while ( **ppCur != ' ' && **ppCur != '\r' && **ppCur != '\n' )
        (*ppCur)++;
    if ( **ppCur )
    {
        **ppCur = 0;
        (*ppCur)++;
    }
    while ( **ppCur && **ppCur != '0' && **ppCur != '1' )
        (*ppCur)++;
    return pResult;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


