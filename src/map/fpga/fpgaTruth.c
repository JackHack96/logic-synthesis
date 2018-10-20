/**CFile****************************************************************

  FileName    [fpgaTruth.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Technology mapping for variable-size-LUT FPGAs.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - August 18, 2004.]

  Revision    [$Id: fpgaTruth.c,v 1.5 2005/07/08 01:01:23 alanmi Exp $]

***********************************************************************/

#include "fpgaInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void  Fpga_TruthsCut_rec( Fpga_Cut_t * pCut, unsigned uTruth[] );
static void  Fpga_TruthsUnmark_rec( Fpga_Cut_t * pCut );
static void  Fpga_TruthsCutTopo_rec( Fpga_Man_t * pMan, Fpga_Node_t * pNode, unsigned uTruthRes[] );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Returns 1 if the constant cofactor property holds using truth table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fpga_CutHasConstCofactorFunctional( Fpga_Man_t * pMan, Fpga_Cut_t * pCut )
{
    unsigned uTruth[2];
    int i;

    if ( pCut->nLeaves != 5 )
        return 0;

    Fpga_TruthsCut( pMan, pCut, uTruth );
    for ( i = 0; i < pCut->nLeaves; i++ )
    {
        if ( ((uTruth[0] &  pMan->uTruths[i][0]) ==  pMan->uTruths[i][0]) || 
             ((uTruth[0] & ~pMan->uTruths[i][0]) == ~pMan->uTruths[i][0]) )
              return 1;
        if ( ((uTruth[0] &  pMan->uTruths[i][0]) ==  0) || 
             ((uTruth[0] & ~pMan->uTruths[i][0]) ==  0) )
              return 1;
    }
    return 0;
}

/**Function*************************************************************

  Synopsis    [Derives the truth table for one cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_TruthsCut( Fpga_Man_t * pMan, Fpga_Cut_t * pCut, unsigned * uTruth )
{
    int i;
    // generally speaking, 1-input cut can be matched into a wire!
    if ( pCut->nLeaves == 1 )
        return;
    // check if it a resynthesized cut
    if ( pCut->pOne == NULL )
    {
        uTruth[0] = pCut->uTruthTemp[0];
        uTruth[1] = pCut->uTruthTemp[1];
        return;
    }
    // set the leaf truth tables
    for ( i = 0; i < pCut->nLeaves; i++ )
    {
        pCut->ppLeaves[i]->pCuts->uTruthTemp[0] = pMan->uTruths[i][0];
        pCut->ppLeaves[i]->pCuts->uTruthTemp[1] = pMan->uTruths[i][1];
    }
    // recursively compute the truth table
    pCut->nVolume = 0;
    Fpga_TruthsCut_rec( pCut, uTruth );
    // recursively unmark the visited cuts
    Fpga_TruthsUnmark_rec( pCut );
//printf( " %d", pCut->nVolume );
}

/**Function*************************************************************

  Synopsis    [Recursively derives the truth table for the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_TruthsCut_rec( Fpga_Cut_t * pCut, unsigned uTruthRes[] )
{
    unsigned uTruth1[2], uTruth2[2];
    // if this is the elementary cut, its truth table is already available
    if ( pCut->nLeaves == 1 )
    {
        uTruthRes[0] = pCut->uTruthTemp[0];
        uTruthRes[1] = pCut->uTruthTemp[1];
        return;
    }
    // if this node was already visited, return its computed truth table
    if ( pCut->fMark )
    {
        uTruthRes[0] = pCut->uTruthTemp[0];
        uTruthRes[1] = pCut->uTruthTemp[1];
        return;
    }
    pCut->fMark = 1;
    pCut->nVolume++;

    assert( !Fpga_IsComplement(pCut) );
    Fpga_TruthsCut_rec( Fpga_CutRegular(pCut->pOne), uTruth1 );
    if ( Fpga_CutIsComplement(pCut->pOne) )
    {
        uTruth1[0] = ~uTruth1[0];
        uTruth1[1] = ~uTruth1[1];
    }
    Fpga_TruthsCut_rec( Fpga_CutRegular(pCut->pTwo), uTruth2 );
    if ( Fpga_CutIsComplement(pCut->pTwo) )
    {
        uTruth2[0] = ~uTruth2[0];
        uTruth2[1] = ~uTruth2[1];
    }
    if ( !pCut->Phase )
    {
        uTruthRes[0] = pCut->uTruthTemp[0] = uTruth1[0] & uTruth2[0];
        uTruthRes[1] = pCut->uTruthTemp[1] = uTruth1[1] & uTruth2[1];
    }
    else
    {
        uTruthRes[0] = pCut->uTruthTemp[0] = ~(uTruth1[0] & uTruth2[0]);
        uTruthRes[1] = pCut->uTruthTemp[1] = ~(uTruth1[1] & uTruth2[1]);
    }
}

/**Function*************************************************************

  Synopsis    [Recursively derives the truth table for the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_TruthsUnmark_rec( Fpga_Cut_t * pCut )
{
    if ( pCut->nLeaves == 1 )
        return;
    // if this node was already visited, return its computed truth table
    if ( pCut->fMark == 0 )
        return;
    pCut->fMark = 0;
    Fpga_TruthsUnmark_rec( Fpga_CutRegular(pCut->pOne) );
    Fpga_TruthsUnmark_rec( Fpga_CutRegular(pCut->pTwo) );
}


/**Function*************************************************************

  Synopsis    [Computes the truth table of the topological cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_TruthsCutTopo( Fpga_Man_t * pMan, Fpga_Node_t * pNode, Fpga_Cut_t * pCut, unsigned * uTruth )
{
    int i;
    assert( pCut->nLeaves > 1 );
    // set the leaf truth tables
    for ( i = 0; i < pCut->nLeaves; i++ )
    {
        pCut->ppLeaves[i]->Num2   = i;
        pCut->ppLeaves[i]->fMark0 = 1;
    }
    // recursively compute the truth table
    Fpga_TruthsCutTopo_rec( pMan, pNode, uTruth );
    // unset the leaf truth tables
    for ( i = 0; i < pCut->nLeaves; i++ )
        pCut->ppLeaves[i]->fMark0 = 0;
}

/**Function*************************************************************

  Synopsis    [Recursively derives the truth table for the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_TruthsCutTopo_rec( Fpga_Man_t * pMan, Fpga_Node_t * pNode, unsigned uTruthRes[] )
{
    unsigned uTruth1[2], uTruth2[2];
    assert( !Fpga_IsComplement(pNode) );
    // if this node was already visited, return its computed truth table
    if ( pNode->fMark0 )
    {
        uTruthRes[0] = pMan->uTruths[pNode->Num2][0];
        uTruthRes[1] = pMan->uTruths[pNode->Num2][1];
        return;
    }

    Fpga_TruthsCutTopo_rec( pMan, Fpga_Regular(pNode->p1), uTruth1 );
    if ( Fpga_IsComplement(pNode->p1) )
    {
        uTruth1[0] = ~uTruth1[0];
        uTruth1[1] = ~uTruth1[1];
    }
    Fpga_TruthsCutTopo_rec( pMan, Fpga_Regular(pNode->p2), uTruth2 );
    if ( Fpga_IsComplement(pNode->p2) )
    {
        uTruth2[0] = ~uTruth2[0];
        uTruth2[1] = ~uTruth2[1];
    }
    uTruthRes[0] = uTruth1[0] & uTruth2[0];
    uTruthRes[1] = uTruth1[1] & uTruth2[1];
}

/**Function*************************************************************

  Synopsis    [Returns the truth table of the don't-care set.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fpga_TruthsCutDontCare( Fpga_Man_t * pMan, Fpga_Cut_t * pCut, unsigned * uTruthDc )
{
    if ( pCut->pOne || (pCut->uTruthZero[0] == 0 && pCut->uTruthZero[1] == 0) )
        return 0;
    assert( (pCut->uTruthTemp[0] & pCut->uTruthZero[0]) == 0 );
    assert( (pCut->uTruthTemp[1] & pCut->uTruthZero[1]) == 0 );
    uTruthDc[0] = ((~0) & (~pCut->uTruthTemp[0]) & (~pCut->uTruthZero[0]));
    uTruthDc[1] = ((~0) & (~pCut->uTruthTemp[1]) & (~pCut->uTruthZero[1]));
    if ( uTruthDc[0] == 0 && uTruthDc[1] == 0 )
        return 0;
    return 1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


