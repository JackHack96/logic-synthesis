/**CFile****************************************************************

  FileName    [fmData.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Procedures to access user's data stored in Sh_Node_t.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: fmData.c,v 1.3 2003/05/27 23:15:32 alanmi Exp $]

***********************************************************************/

#include "fmInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Retrieves the global BDD of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Fm_DataGetNodeGlo( Sh_Node_t * pNode )
{
    Sh_Node_t * pNodeR;
    bool fCompl;
    pNodeR = Sh_Regular(pNode);
    fCompl = Sh_IsComplement(pNode);
    if ( pNodeR->pData == 0 )
        return NULL;
    return Cudd_NotCond( (DdNode *)pNodeR->pData,  fCompl );
}

/**Function*************************************************************

  Synopsis    [Retrieves the second global BDD of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Fm_DataGetNodeGloZ( Sh_Node_t * pNode )
{
    Sh_Node_t * pNodeR;
    bool fCompl;
    pNodeR = Sh_Regular(pNode);
    fCompl = Sh_IsComplement(pNode);
    if ( pNodeR->pData2 == 0 )
        return NULL;
    return Cudd_NotCond( (DdNode *)pNodeR->pData2,  fCompl );
}

/**Function*************************************************************

  Synopsis    [Retrieves the global BDD of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fm_DataSetNodeGlo( Sh_Node_t * pNode, DdNode * bGlo )
{
    Sh_Node_t * pNodeR;
    bool fCompl;
    pNodeR = Sh_Regular(pNode);
    fCompl = Sh_IsComplement(pNode);
    assert( pNodeR->pData == 0 );
    pNodeR->pData = (unsigned)Cudd_NotCond( bGlo, fCompl );
}

/**Function*************************************************************

  Synopsis    [Retrieves the global BDD of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fm_DataSetNodeGloZ( Sh_Node_t * pNode, DdNode * bGlo )
{
    Sh_Node_t * pNodeR;
    bool fCompl;
    pNodeR = Sh_Regular(pNode);
    fCompl = Sh_IsComplement(pNode);
    assert( pNodeR->pData2 == 0 );
    pNodeR->pData2 = (unsigned)Cudd_NotCond( bGlo, fCompl );
}

/**Function*************************************************************

  Synopsis    [Retrieves the global BDD of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fm_DataSwapNodeGloAndGloZ( Sh_Node_t * pNode )
{
    unsigned Temp;
    Sh_Node_t * pNodeR;
    bool fCompl;

    pNodeR = Sh_Regular(pNode);
    fCompl = Sh_IsComplement(pNode);
    assert( pNodeR->pData  );
    assert( pNodeR->pData2 );

    Temp           = pNodeR->pData;
    pNodeR->pData  = pNodeR->pData2;
    pNodeR->pData2 = Temp;
}

/**Function*************************************************************

  Synopsis    [Derefs all BDDs at the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fm_DataDerefNodeGlo( DdManager * dd, Sh_Node_t * pNode )
{
    Sh_Node_t * pNodeR;
    pNodeR = Sh_Regular(pNode);
    if ( pNodeR->pData )
    {
        Cudd_RecursiveDeref( dd, (DdNode *)pNodeR->pData );
        pNodeR->pData = 0;
    }
    if ( pNodeR->pData2 )
    {
        Cudd_RecursiveDeref( dd, (DdNode *)pNodeR->pData2 );
        pNodeR->pData2 = 0;
    }
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


