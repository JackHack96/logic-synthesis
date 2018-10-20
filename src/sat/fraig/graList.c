/**CFile****************************************************************

  FileName    [graList.c]

  PackageName [FRAIG: Functionally reduced AND-INV graphs.]

  Synopsis    [Generic graph data struture.]

  Author      [Alan Mishchenko <alanmi@eecs.berkeley.edu>]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.1. Started - October 1, 2004]

  Revision    [$Id: graList.c,v 1.2 2005/07/08 01:01:34 alanmi Exp $]

***********************************************************************/

#include "graInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Adds the node to the list of all nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gra_NodeListAddLast( Gra_NodeList_t * pList, Gra_Node_t * pNode )
{
    if ( pList->pHead == NULL )
    {
        pList->pHead = pNode;
        pList->pTail = pNode;
        pNode->pPrev = NULL;
        pNode->pNext = NULL;
    }
    else
    {
        pNode->pNext = NULL;
        pList->pTail->pNext = pNode;
        pNode->pPrev = pList->pTail;
        pList->pTail = pNode;
    }
    pList->nItems++;
}

/**Function*************************************************************

  Synopsis    [Removes the node to the list of all nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gra_NodeListDelete( Gra_NodeList_t * pList, Gra_Node_t * pNode )
{
    if ( pList->pHead == pNode )
         pList->pHead = pNode->pNext;
    if ( pList->pTail == pNode )
         pList->pTail = pNode->pPrev;
    if ( pNode->pPrev )
         pNode->pPrev->pNext = pNode->pNext;
    if ( pNode->pNext )
         pNode->pNext->pPrev = pNode->pPrev;
    pList->nItems--;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


