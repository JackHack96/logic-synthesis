/**CFile****************************************************************

  FileName    [ntkList.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Specialized double-linked lists for nodes and pins.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ntkList.c,v 1.22 2004/05/12 04:30:08 alanmi Exp $]

***********************************************************************/

#include "ntkInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

/* 
    A minor inconveniece is that we have to duplicate the code,
    which manages the linked list for nodes, latches, and pins.
*/

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkNodeListAddFirst( Ntk_Node_t * pNode )
{
    Ntk_NodeList_t * pList = &pNode->pNet->lNodes;
    if ( pList->pHead == NULL )
    {
        pList->pHead = pNode;
        pList->pTail = pNode;
        pNode->pPrev = NULL;
        pNode->pNext = NULL;
    }
    else
    {
        pNode->pPrev = NULL;
        pList->pHead->pPrev = pNode;
        pNode->pNext = pList->pHead;
        pList->pHead = pNode;
    }
    pList->nItems++;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkNodeListAddLast( Ntk_Node_t * pNode )
{
    Ntk_NodeList_t * pList = &pNode->pNet->lNodes;
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

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkNodeListAddLast2( Ntk_Node_t * pNode )
{
    Ntk_NodeList_t * pList = &pNode->pNet->lNodes2;
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

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkNodeListDelete( Ntk_Node_t * pNode )
{
    Ntk_NodeList_t * pList = &pNode->pNet->lNodes;
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







/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkNodeCiListAddFirst( Ntk_Node_t * pNode )
{
    Ntk_NodeList_t * pList = &pNode->pNet->lCis;
    if ( pList->pHead == NULL )
    {
        pList->pHead = pNode;
        pList->pTail = pNode;
        pNode->pPrev = NULL;
        pNode->pNext = NULL;
    }
    else
    {
        pNode->pPrev = NULL;
        pList->pHead->pPrev = pNode;
        pNode->pNext = pList->pHead;
        pList->pHead = pNode;
    }
    pList->nItems++;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkNodeCiListAddLast( Ntk_Node_t * pNode )
{
    Ntk_NodeList_t * pList = &pNode->pNet->lCis;
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

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkNodeCiListDelete( Ntk_Node_t * pNode )
{
    Ntk_NodeList_t * pList = &pNode->pNet->lCis;
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




/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkNodeCoListAddFirst( Ntk_Node_t * pNode )
{
    Ntk_NodeList_t * pList = &pNode->pNet->lCos;
    if ( pList->pHead == NULL )
    {
        pList->pHead = pNode;
        pList->pTail = pNode;
        pNode->pPrev = NULL;
        pNode->pNext = NULL;
    }
    else
    {
        pNode->pPrev = NULL;
        pList->pHead->pPrev = pNode;
        pNode->pNext = pList->pHead;
        pList->pHead = pNode;
    }
    pList->nItems++;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkNodeCoListAddLast( Ntk_Node_t * pNode )
{
    Ntk_NodeList_t * pList = &pNode->pNet->lCos;
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

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkNodeCoListDelete( Ntk_Node_t * pNode )
{
    Ntk_NodeList_t * pList = &pNode->pNet->lCos;
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





/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkLatchListAddFirst( Ntk_Latch_t * pLatch )
{
    Ntk_LatchList_t * pList = &pLatch->pNet->lLatches;
    if ( pList->pHead == NULL )
    {
        pList->pHead  = pLatch;
        pList->pTail  = pLatch;
        pLatch->pPrev = NULL;
        pLatch->pNext = NULL;
    }
    else
    {
        pLatch->pPrev = NULL;
        pList->pHead->pPrev = pLatch;
        pLatch->pNext = pList->pHead;
        pList->pHead  = pLatch;
    }
    pList->nItems++;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkLatchListAddLast( Ntk_Latch_t * pLatch )
{
    Ntk_LatchList_t * pList = &pLatch->pNet->lLatches;
    if ( pList->pHead == NULL )
    {
        pList->pHead  = pLatch;
        pList->pTail  = pLatch;
        pLatch->pPrev = NULL;
        pLatch->pNext = NULL;
    }
    else
    {
        pLatch->pNext = NULL;
        pList->pTail->pNext = pLatch;
        pLatch->pPrev = pList->pTail;
        pList->pTail  = pLatch;
    }
    pList->nItems++;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkLatchListDelete( Ntk_Latch_t * pLatch )
{
    Ntk_LatchList_t * pList = &pLatch->pNet->lLatches;
    if ( pList->pHead == pLatch )
         pList->pHead = pLatch->pNext;
    if ( pList->pTail == pLatch )
         pList->pTail = pLatch->pPrev;
    if ( pLatch->pPrev )
         pLatch->pPrev->pNext = pLatch->pNext;
    if ( pLatch->pNext )
         pLatch->pNext->pPrev = pLatch->pPrev;
    pList->nItems--;
}







/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodeFaninListAddFirst( Ntk_Node_t * pNode, Ntk_Pin_t * pPin )
{
    Ntk_PinList_t * pList = &pNode->lFanins;
    if ( pList->pHead == NULL )
    {
        pList->pHead = pPin;
        pList->pTail = pPin;
        pPin->pPrev  = NULL;
        pPin->pNext  = NULL;
    }
    else
    {
        pPin->pPrev  = NULL;
        pList->pHead->pPrev = pPin;
        pPin->pNext  = pList->pHead;
        pList->pHead = pPin;
    }
    pList->nItems++;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodeFaninListAddLast( Ntk_Node_t * pNode, Ntk_Pin_t * pPin )
{
    Ntk_PinList_t * pList = &pNode->lFanins;
    if ( pList->pHead == NULL )
    {
        pList->pHead = pPin;
        pList->pTail = pPin;
        pPin->pPrev = NULL;
        pPin->pNext = NULL;
    }
    else
    {
        pPin->pNext = NULL;
        pList->pTail->pNext = pPin;
        pPin->pPrev = pList->pTail;
        pList->pTail = pPin;
    }
    pList->nItems++;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodeFaninListDelete( Ntk_Node_t * pNode, Ntk_Pin_t * pPin )
{
    Ntk_PinList_t * pList = &pNode->lFanins;
    if ( pList->pHead == pPin )
         pList->pHead = pPin->pNext;
    if ( pList->pTail == pPin )
         pList->pTail = pPin->pPrev;
    if ( pPin->pPrev )
         pPin->pPrev->pNext = pPin->pNext;
    if ( pPin->pNext )
         pPin->pNext->pPrev = pPin->pPrev;
    pList->nItems--;
}





/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodeFanoutListAddFirst( Ntk_Node_t * pNode, Ntk_Pin_t * pPin )
{
    Ntk_PinList_t * pList = &pNode->lFanouts;
    if ( pList->pHead == NULL )
    {
        pList->pHead = pPin;
        pList->pTail = pPin;
        pPin->pPrev  = NULL;
        pPin->pNext  = NULL;
    }
    else
    {
        pPin->pPrev  = NULL;
        pList->pHead->pPrev = pPin;
        pPin->pNext  = pList->pHead;
        pList->pHead = pPin;
    }
    pList->nItems++;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodeFanoutListAddLast( Ntk_Node_t * pNode, Ntk_Pin_t * pPin )
{
    Ntk_PinList_t * pList = &pNode->lFanouts;
    if ( pList->pHead == NULL )
    {
        pList->pHead = pPin;
        pList->pTail = pPin;
        pPin->pPrev = NULL;
        pPin->pNext = NULL;
    }
    else
    {
        pPin->pNext = NULL;
        pList->pTail->pNext = pPin;
        pPin->pPrev = pList->pTail;
        pList->pTail = pPin;
    }
    pList->nItems++;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodeFanoutListDelete( Ntk_Node_t * pNode, Ntk_Pin_t * pPin )
{
    Ntk_PinList_t * pList = &pNode->lFanouts;
    if ( pList->pHead == pPin )
         pList->pHead = pPin->pNext;
    if ( pList->pTail == pPin )
         pList->pTail = pPin->pPrev;
    if ( pPin->pPrev )
         pPin->pPrev->pNext = pPin->pNext;
    if ( pPin->pNext )
         pPin->pNext->pPrev = pPin->pPrev;
    pList->nItems--;
}




////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

