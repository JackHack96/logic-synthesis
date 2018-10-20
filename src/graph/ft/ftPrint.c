/**CFile****************************************************************

  FileName    [ftPrint.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Procedures to print the factored tree.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ftPrint.c,v 1.3 2003/05/27 23:14:47 alanmi Exp $]

***********************************************************************/

#include "ft.h"
#include <string.h>

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void   Ft_TreePrint_rec( FILE * pFile, Ft_Node_t * pNode, char * pNamesIn[], int * pPos, int LitSizeMax );
static char * Ft_TreePrintGetLeafName( Ft_Node_t * pNode, char * pNamesIn[] );
static void   Ft_TreePrintUpdatePos( FILE * pFile, int * pPos, int LitSizeMax );
static int    Ft_TreePrintOutputName( FILE * pFile, char * pNameOut, int nValuesOut, int iSet );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ft_TreePrint( FILE * pFile, Ft_Tree_t * pTree, char * pNamesIn[], char * pNameOut )
{
    char Buffer[5];
    Vm_VarMap_t * pVm;
    int nVarsIn, * pValues;
    int Pos, i, LitSizeMax, LitSizeCur;
    int fMadeupNames;
    assert( pTree->nLeaves == Vm_VarMapReadValuesInNum(pTree->pVm) );
    // print the tree statistics
//    fprintf( pFile, "Factored tree stats: %d leaves, %d roots, %d nodes\n",
//        pTree->nLeaves, pTree->nRoots, pTree->nNodes );

    // get the parameters
    pVm = pTree->pVm;
    nVarsIn = Vm_VarMapReadVarsInNum(pVm);
    pValues = Vm_VarMapReadValuesArray(pVm);

    // create the names if not given by the user
    fMadeupNames = 0;
    if ( pNamesIn == NULL )
    {
        fMadeupNames = 1;
        Buffer[1] = 0;
        pNamesIn = ALLOC( char *, nVarsIn );
        for ( i = 0; i < nVarsIn; i++ )
        {
            Buffer[0] = 'a' + i;
            pNamesIn[i] = util_strsav( Buffer );
        }
    }

    // get the size of the longest literal
    LitSizeMax = 0;
    for ( i = 0; i < nVarsIn; i++ )
    {
        LitSizeCur = strlen(pNamesIn[i]) + 2 * pValues[i] + 1;
        if ( pValues[i] > 10 )
            LitSizeCur += pValues[i] - 10;
        if ( LitSizeMax < LitSizeCur )
            LitSizeMax = LitSizeCur;
    }
    if ( LitSizeMax > 60 )
        LitSizeMax = 50;

    // print the tree recursively
    for ( i = 0; i < pTree->nRoots; i++ )
        if ( pTree->pRoots[i] )
        {
            Pos = Ft_TreePrintOutputName( pFile, pNameOut, Vm_VarMapReadValuesOutNum(pTree->pVm), i );
            Ft_TreePrint_rec( pFile, pTree->pRoots[i], pNamesIn, &Pos, LitSizeMax );
            fprintf( pFile, "\n" );
        }

    if ( fMadeupNames )
    {
        for ( i = 0; i < nVarsIn; i++ )
            free( pNamesIn[i] );
        free( pNamesIn );
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ft_TreePrint_rec( FILE * pFile, Ft_Node_t * pNode, char * pNamesIn[], int * pPos, int LitSizeMax )
{
    char * pName;
    if ( pNode->Type == FT_NODE_LEAF )
    {
        pName = Ft_TreePrintGetLeafName( pNode, pNamesIn );
        fprintf( pFile, "%s", pName );
        (*pPos) += strlen( pName );
        return;
    }
    if ( pNode->Type == FT_NODE_AND )
    {
        if ( pNode->pOne->Type != FT_NODE_OR )
            Ft_TreePrint_rec( pFile, pNode->pOne, pNamesIn, pPos, LitSizeMax );
        else
        {
            fprintf( pFile, "(" );
            (*pPos)++;
            Ft_TreePrint_rec( pFile, pNode->pOne, pNamesIn, pPos, LitSizeMax );
            fprintf( pFile, ")" );
            (*pPos)++;
        }
        fprintf( pFile, " " );
        (*pPos)++;

        Ft_TreePrintUpdatePos( pFile, pPos, LitSizeMax );

        if ( pNode->pTwo->Type != FT_NODE_OR )
            Ft_TreePrint_rec( pFile, pNode->pTwo, pNamesIn, pPos, LitSizeMax );
        else
        {
            fprintf( pFile, "(" );
            (*pPos)++;
            Ft_TreePrint_rec( pFile, pNode->pTwo, pNamesIn, pPos, LitSizeMax );
            fprintf( pFile, ")" );
            (*pPos)++;
        }
        return;
    }
    if ( pNode->Type == FT_NODE_OR )
    {
        Ft_TreePrint_rec( pFile, pNode->pOne, pNamesIn, pPos, LitSizeMax );
        fprintf( pFile, " + " );
        (*pPos) += 3;

        Ft_TreePrintUpdatePos( pFile, pPos, LitSizeMax );

        Ft_TreePrint_rec( pFile, pNode->pTwo, pNamesIn, pPos, LitSizeMax );
        return;
    }
    if ( pNode->Type == FT_NODE_0 )
    {
        fprintf( pFile, "Constant 0" );
        return;
    }
    if ( pNode->Type == FT_NODE_1 )
    {
        fprintf( pFile, "Constant 1" );
        return;
    }
    assert( 0 );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Ft_TreePrintGetLeafName( Ft_Node_t * pNode, char * pNamesIn[] )
{
    static char Buffer[100];
    char Value[10];
    int v, fFirst;
    if ( pNode->nValues == 2 )
    {
        if ( pNode->uData == 1 )
        {
            if ( pNamesIn )
                sprintf( Buffer, "%s\'", pNamesIn[pNode->VarNum] );
            else
                sprintf( Buffer, "%c\'", 'a' + pNode->VarNum );
        }
        else if ( pNode->uData == 2 )
        {
            if ( pNamesIn )
                sprintf( Buffer, "%s", pNamesIn[pNode->VarNum] );
            else
                sprintf( Buffer, "%c", 'a' + pNode->VarNum );
        }
        else
        {
            assert( 0 );
        }
    }
    else
    {
        if ( pNamesIn )
            sprintf( Buffer, "%s{", pNamesIn[pNode->VarNum] );
        else
            sprintf( Buffer, "%c{", 'a' + pNode->VarNum );
            
        fFirst = 1;
        for ( v = 0; v < pNode->nValues; v++ )
            if ( pNode->uData & (1<<v) )
            {
                if ( fFirst )
                    fFirst = 0;
                else
                    strcat( Buffer, "," );
                sprintf( Value, "%d", v );
                strcat( Buffer, Value );
            }
        strcat( Buffer, "}" );
    }
    return Buffer;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ft_TreePrintUpdatePos( FILE * pFile, int * pPos, int LitSizeMax )
{
    int i;
    if ( *pPos + LitSizeMax < 77 )
        return;
    fprintf( pFile, "\n" );
    for ( i = 0; i < 10; i++ )
        fprintf( pFile, " " );
    *pPos = 10;
}

/**Function*************************************************************

  Synopsis    [Starts the printout for a factored form or cover.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ft_TreePrintOutputName( FILE * pFile, char * pNameOut, int nValuesOut, int iSet )
{
    char Buffer[500];
    char Value[10];

    sprintf( Buffer, "%s", pNameOut? pNameOut: "?" );
    if ( nValuesOut > 2 )
    {
        sprintf( Value, "{%d} ", iSet );
        strcat( Buffer, Value );
    }
    else
    {
        sprintf( Value, "%s", (iSet==0)? "\'": " " );
        strcat( Buffer, Value );
    }
    fprintf( pFile, "%8s: ", Buffer );
    return 10;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


