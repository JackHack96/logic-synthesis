/**CFile****************************************************************

  FileName    [ftTrans.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Transformation of FFs in the MV case.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ftList.c,v 1.2 2003/05/27 23:14:47 alanmi Exp $]

***********************************************************************/

#include "ft.h"

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
void Ft_ListAddLeaf( Ft_List_t * pList, Ft_Node_t * pLink ) 
{
	if ( pList->pHead == NULL )
	{
		pList->pHead = pLink;
		pList->pTail = pLink;
		pLink->pTwo  = NULL;
		pLink->pOne  = NULL;
	}
	else
	{
		pLink->pOne  = NULL;
		pList->pTail->pOne = pLink;
		pLink->pTwo  = pList->pTail;
		pList->pTail = pLink;
	}
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ft_ListDelLeaf( Ft_List_t * pList, Ft_Node_t * pLink )
{
	if ( pList->pHead == pLink )
		 pList->pHead = pLink->pOne;
	if ( pList->pTail == pLink )
		 pList->pTail = pLink->pTwo;
	if ( pLink->pTwo )
		 pLink->pTwo->pOne = pLink->pOne;
	if ( pLink->pOne )
		 pLink->pOne->pTwo = pLink->pTwo;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


