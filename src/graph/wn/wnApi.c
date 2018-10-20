/**CFile****************************************************************

  FileName    [wnApi.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [The exported procedures of the windowing package.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: wnApi.c,v 1.3 2003/05/27 23:14:53 alanmi Exp $]

***********************************************************************/

#include "wnInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Network_t *   Wn_WindowReadNet      ( Wn_Window_t * p )  { return p->pNet; }
int               Wn_WindowReadLevelsTfi( Wn_Window_t * p )  { return p->nLevelsTfi; }
int               Wn_WindowReadLevelsTfo( Wn_Window_t * p )  { return p->nLevelsTfo; }
Ntk_Node_t **     Wn_WindowReadLeaves   ( Wn_Window_t * p )  { return p->ppLeaves; }
int               Wn_WindowReadLeafNum  ( Wn_Window_t * p )  { return p->nLeaves; }
Ntk_Node_t **     Wn_WindowReadRoots    ( Wn_Window_t * p )  { return p->ppRoots; }
int               Wn_WindowReadRootNum  ( Wn_Window_t * p )  { return p->nRoots; }
Ntk_Node_t **     Wn_WindowReadNodes    ( Wn_Window_t * p )  { return p->ppNodes; }
int               Wn_WindowReadNodeNum  ( Wn_Window_t * p )  { return p->nNodes; }
Wn_Window_t *     Wn_WindowReadWndCore  ( Wn_Window_t * p )  { return p->pWndCore; }
Vm_VarMap_t *     Wn_WindowReadVmL      ( Wn_Window_t * p )  { return p->pVmL; }
Vm_VarMap_t *     Wn_WindowReadVmR      ( Wn_Window_t * p )  { return p->pVmR; }
Vm_VarMap_t *     Wn_WindowReadVmS      ( Wn_Window_t * p )  { return p->pVmS; }

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Wn_WindowSetWndCore( Wn_Window_t * p, Wn_Window_t * pCore )
{
    assert( p->pWndCore == NULL );
    p->pWndCore = pCore;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


