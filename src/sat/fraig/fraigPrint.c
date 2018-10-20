/**CFile****************************************************************

  FileName    [fraigPrint.c]

  PackageName [FRAIG: Functionally reduced AND-INV graphs.]

  Synopsis    [Various printing procedures.]

  Author      [Alan Mishchenko <alanmi@eecs.berkeley.edu>]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.1. Started - October 1, 2004]

  Revision    [$Id: fraigPrint.c,v 1.5 2005/07/08 01:01:32 alanmi Exp $]

***********************************************************************/

#include "fraigInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void Extra_PrintBinary( FILE * pFile, unsigned Sign[], int nBits );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Prints the AI-graph nodes in the list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Prin( Fraig_Man_t * pMan, Sat_IntVec_t * vNodes )
{
    Fraig_Node_t * pNode;
    int i, nSize, * pArray;

    nSize  = Sat_IntVecReadSize( vNodes );
    pArray = Sat_IntVecReadArray( vNodes );
    for ( i = 0; i < nSize; i++ )
    {
        pNode = pMan->vAnds->pArray[ pArray[i] ];
        if ( Fraig_NodeIsAnd(pNode) )
        {
            printf( "Node %2d = %2d%s * %2d%s.\n", pNode->Num, 
                Fraig_Regular(pNode->p1)->Num, Fraig_IsComplement(pNode->p1)? "\'" : " ",  
                Fraig_Regular(pNode->p2)->Num, Fraig_IsComplement(pNode->p2)? "\'" : " " );
        }
        else
            printf( "Node %2d is a PI.\n", pNode->Num );
    }
}

/**Function*************************************************************

  Synopsis    [Prints the supports of two nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_PrintNodesSupps( Fraig_Man_t * p, Fraig_Node_t * p1, Fraig_Node_t * p2 )
{
    extern void Extra_PrintBinary( FILE * pFile, unsigned Sign[], int nBits );
    printf( "Node %d:\n", p1->Num );
    Extra_PrintBinary( stdout, p1->pSuppStr, p->nInputs );  printf( "\n" );
    printf( "Node %d:\n", p2->Num );
    Extra_PrintBinary( stdout, p2->pSuppStr, p->nInputs );  printf( "\n" );
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    [Prints the supports of two nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_PrintNodesSupps_old( Fraig_Man_t * p, Fraig_Node_t * p1, Fraig_Node_t * p2 )
{
    int i;
    printf( "%d & %d ", Fraig_NodeCountSuppVars(p,p1,1), Fraig_NodeCountSuppVars(p,p2,1) );
    for ( i = 0; i < p->nSuppWords; i++ )
        p1->pSuppStr[i] ^= p2->pSuppStr[i];
    printf( " = %d   ", Fraig_NodeCountSuppVars(p,p1,1) );
    for ( i = 0; i < p->nSuppWords; i++ )
        p1->pSuppStr[i] ^= p2->pSuppStr[i];
}

/**Function*************************************************************

  Synopsis    [Prints the bit string.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_PrintBinary( FILE * pFile, unsigned Sign[], int nBits )
{
    int Remainder, nWords;
    int w, i;

    Remainder = (nBits%(sizeof(unsigned)*8));
    nWords    = (nBits/(sizeof(unsigned)*8)) + (Remainder>0);

    for ( w = nWords-1; w >= 0; w-- )
        for ( i = ((w == nWords-1 && Remainder)? Remainder-1: 31); i >= 0; i-- )
            fprintf( pFile, "%c", '0' + (int)((Sign[w] & (1<<i)) > 0) );

//  fprintf( pFile, "\n" );
}

/**Function*************************************************************

  Synopsis    [Prints the levels of choice nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_PrintChoiceNodeLevels( Fraig_Man_t * p )
{
    Fraig_Node_t * pNode, * pEquiv;
    int i, Counter;
    // perform the traversal
    Counter = 0;
    for ( i = 0; i < p->vAnds->nSize; i++ )
    {
        pNode = p->vAnds->pArray[i];
        if ( pNode->pNextE == NULL || pNode->pRepr )
            continue;
        // the node has equivalent ones, and has not repr (he is the repr)
        printf( "%4d : Node %5d  : ", ++Counter, pNode->Num );
        for ( pEquiv = pNode; pEquiv; pEquiv = pEquiv->pNextE )
            printf( "%d ", pEquiv->Level );
        printf( "\n" );
    }
}

/**Function*************************************************************

  Synopsis    [Prints the levels of choice nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_PrintLevelStats( Fraig_Man_t * p )
{
    int nLevelMax, nOutputs, i;
    nOutputs = 0;
    nLevelMax = -1;
    for ( i = 0; i < p->nOutputs; i++ )
        if ( nLevelMax == Fraig_Regular(p->pOutputs[i])->Level )
            nOutputs++;
        else if ( nLevelMax < Fraig_Regular(p->pOutputs[i])->Level )
        {
            nLevelMax = Fraig_Regular(p->pOutputs[i])->Level;
            nOutputs = 1;
        }
    printf( "Max = %3d (%2d).  Levels =", nLevelMax, nOutputs );
    for ( i = 0; i < p->nOutputs; i++ )
    {
        if ( i == 15 )
            break;
        printf( " %3d", Fraig_Regular(p->pOutputs[i])->Level );
    }
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    [Prints the choicing stats.]

  Description [Count the number of choice nodes and the total number of choices.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_PrintChoicingStats( Fraig_Man_t * p )
{
    Fraig_Node_t * pNode, * pEquiv;
    int i, nChoiceNodes, nChoices;
    // do not print the stats if the choicing is not enabled
    if ( p->fChoicing == 0 )
        return;
    // count the number of choices and choice nodes
    nChoiceNodes = nChoices = 0;
    for ( i = 0; i < p->vAnds->nSize; i++ )
    {
        pNode = p->vAnds->pArray[i];
        if ( pNode->pNextE == NULL || pNode->pRepr )
            continue;
        nChoiceNodes++;
        for ( pEquiv = pNode->pNextE; pEquiv; pEquiv = pEquiv->pNextE )
            nChoices++;
    }
    printf( "Number of choice nodes = %d.  Total number of choices = %d.\n", nChoiceNodes, nChoices );
}

/**Function*************************************************************

  Synopsis    [Prints the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_PrintNode( Fraig_Man_t * p, Fraig_Node_t * pNode )
{
    Fraig_NodeVec_t * vNodes;
    Fraig_Node_t * pTemp;
    int fCompl1, fCompl2, i;

    vNodes = Fraig_DfsOne( p, pNode, 0 );
    for ( i = 0; i < vNodes->nSize; i++ )
    {
        pTemp = vNodes->pArray[i];
        if ( Fraig_NodeIsVar(pTemp) )
        {
            printf( "%3d : PI\n", pTemp->Num );
            continue;
        }

        fCompl1 = Fraig_IsComplement(pTemp->p1);
        fCompl2 = Fraig_IsComplement(pTemp->p2);
        printf( "%3d : %c%3d %c%3d\n", pTemp->Num,
            (fCompl1? '-':'+'), Fraig_Regular(pTemp->p1)->Num,
            (fCompl2? '-':'+'), Fraig_Regular(pTemp->p2)->Num );
    }
    Fraig_NodeVecFree( vNodes );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


