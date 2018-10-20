/**CFile****************************************************************

  FileName    [lxuList.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Operations on lists.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: lxuList.c,v 1.2 2004/01/30 00:18:49 satrajit Exp $]

***********************************************************************/

#include "lxuInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

// matrix -> var

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lxu_ListMatrixAddVariable( Lxu_Matrix * p, Lxu_Var * pLink ) 
{
	Lxu_ListVar * pList = &p->lVars;
	if ( pList->pHead == NULL )
	{
		pList->pHead = pLink;
		pList->pTail = pLink;
		pLink->pPrev = NULL;
		pLink->pNext = NULL;
	}
	else
	{
		pLink->pNext = NULL;
		pList->pTail->pNext = pLink;
		pLink->pPrev = pList->pTail;
		pList->pTail = pLink;
	}
	pList->nItems++;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lxu_ListMatrixDelVariable( Lxu_Matrix * p, Lxu_Var * pLink )
{
	Lxu_ListVar * pList = &p->lVars;
	if ( pList->pHead == pLink )
		 pList->pHead = pLink->pNext;
	if ( pList->pTail == pLink )
		 pList->pTail = pLink->pPrev;
	if ( pLink->pPrev )
		 pLink->pPrev->pNext = pLink->pNext;
	if ( pLink->pNext )
		 pLink->pNext->pPrev = pLink->pPrev;
	pList->nItems--;
}


// matrix -> cube

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lxu_ListMatrixAddCube( Lxu_Matrix * p, Lxu_Cube * pLink )
{
	Lxu_ListCube * pList = &p->lCubes;
	if ( pList->pHead == NULL )
	{
		pList->pHead = pLink;
		pList->pTail = pLink;
		pLink->pPrev = NULL;
		pLink->pNext = NULL;
	}
	else
	{
		pLink->pNext = NULL;
		pList->pTail->pNext = pLink;
		pLink->pPrev = pList->pTail;
		pList->pTail = pLink;
	}
	pList->nItems++;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lxu_ListMatrixDelCube( Lxu_Matrix * p, Lxu_Cube * pLink )
{
	Lxu_ListCube * pList = &p->lCubes;
	if ( pList->pHead == pLink )
		 pList->pHead = pLink->pNext;
	if ( pList->pTail == pLink )
		 pList->pTail = pLink->pPrev;
	if ( pLink->pPrev )
		 pLink->pPrev->pNext = pLink->pNext;
	if ( pLink->pNext )
		 pLink->pNext->pPrev = pLink->pPrev;
	pList->nItems--;
}


// matrix -> single

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lxu_ListMatrixAddSingle( Lxu_Matrix * p, Lxu_Single * pLink )
{
	Lxu_ListSingle * pList = &p->lSingles;
	if ( pList->pHead == NULL )
	{
		pList->pHead = pLink;
		pList->pTail = pLink;
		pLink->pPrev = NULL;
		pLink->pNext = NULL;
	}
	else
	{
		pLink->pNext = NULL;
		pList->pTail->pNext = pLink;
		pLink->pPrev = pList->pTail;
		pList->pTail = pLink;
	}
	pList->nItems++;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lxu_ListMatrixDelSingle( Lxu_Matrix * p, Lxu_Single * pLink )
{
	Lxu_ListSingle * pList = &p->lSingles;
	if ( pList->pHead == pLink )
		 pList->pHead = pLink->pNext;
	if ( pList->pTail == pLink )
		 pList->pTail = pLink->pPrev;
	if ( pLink->pPrev )
		 pLink->pPrev->pNext = pLink->pNext;
	if ( pLink->pNext )
		 pLink->pNext->pPrev = pLink->pPrev;
	pList->nItems--;
}


// table -> divisor

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lxu_ListTableAddDivisor( Lxu_Matrix * p, Lxu_Double * pLink ) 
{
	Lxu_ListDouble * pList = &(p->pTable[pLink->Key]);
	if ( pList->pHead == NULL )
	{
		pList->pHead = pLink;
		pList->pTail = pLink;
		pLink->pPrev = NULL;
		pLink->pNext = NULL;
	}
	else
	{
		pLink->pNext = NULL;
		pList->pTail->pNext = pLink;
		pLink->pPrev = pList->pTail;
		pList->pTail = pLink;
	}
	pList->nItems++;
    p->nDivs++;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lxu_ListTableDelDivisor( Lxu_Matrix * p, Lxu_Double * pLink ) 
{
	Lxu_ListDouble * pList = &(p->pTable[pLink->Key]);
	if ( pList->pHead == pLink )
		 pList->pHead = pLink->pNext;
	if ( pList->pTail == pLink )
		 pList->pTail = pLink->pPrev;
	if ( pLink->pPrev )
		 pLink->pPrev->pNext = pLink->pNext;
	if ( pLink->pNext )
		 pLink->pNext->pPrev = pLink->pPrev;
	pList->nItems--;
    p->nDivs--;
}


// cube -> literal 

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lxu_ListCubeAddLiteral( Lxu_Cube * pCube, Lxu_Lit * pLink )
{
	Lxu_ListLit * pList = &(pCube->lLits);
	if ( pList->pHead == NULL )
	{
		pList->pHead = pLink;
		pList->pTail = pLink;
		pLink->pHPrev = NULL;
		pLink->pHNext = NULL;
	}
	else
	{
		pLink->pHNext = NULL;
		pList->pTail->pHNext = pLink;
		pLink->pHPrev = pList->pTail;
		pList->pTail = pLink;
	}
	pList->nItems++;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lxu_ListCubeDelLiteral( Lxu_Cube * pCube, Lxu_Lit * pLink )
{
	Lxu_ListLit * pList = &(pCube->lLits);
	if ( pList->pHead == pLink )
		 pList->pHead = pLink->pHNext;
	if ( pList->pTail == pLink )
		 pList->pTail = pLink->pHPrev;
	if ( pLink->pHPrev )
		 pLink->pHPrev->pHNext = pLink->pHNext;
	if ( pLink->pHNext )
		 pLink->pHNext->pHPrev = pLink->pHPrev;
	pList->nItems--;
}


// var -> literal

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lxu_ListVarAddLiteral( Lxu_Var * pVar, Lxu_Lit * pLink )
{
	Lxu_ListLit * pList = &(pVar->lLits);
	if ( pList->pHead == NULL )
	{
		pList->pHead = pLink;
		pList->pTail = pLink;
		pLink->pVPrev = NULL;
		pLink->pVNext = NULL;
	}
	else
	{
		pLink->pVNext = NULL;
		pList->pTail->pVNext = pLink;
		pLink->pVPrev = pList->pTail;
		pList->pTail = pLink;
	}
	pList->nItems++;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lxu_ListVarDelLiteral( Lxu_Var * pVar, Lxu_Lit * pLink )
{
	Lxu_ListLit * pList = &(pVar->lLits);
	if ( pList->pHead == pLink )
		 pList->pHead = pLink->pVNext;
	if ( pList->pTail == pLink )
		 pList->pTail = pLink->pVPrev;
	if ( pLink->pVPrev )
		 pLink->pVPrev->pVNext = pLink->pVNext;
	if ( pLink->pVNext )
		 pLink->pVNext->pVPrev = pLink->pVPrev;
	pList->nItems--;
}



// divisor -> pair

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lxu_ListDoubleAddPairLast( Lxu_Double * pDiv, Lxu_Pair * pLink )
{
	Lxu_ListPair * pList = &pDiv->lPairs;
	if ( pList->pHead == NULL )
	{
		pList->pHead = pLink;
		pList->pTail = pLink;
		pLink->pDPrev = NULL;
		pLink->pDNext = NULL;
	}
	else
	{
		pLink->pDNext = NULL;
		pList->pTail->pDNext = pLink;
		pLink->pDPrev = pList->pTail;
		pList->pTail = pLink;
	}
	pList->nItems++;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lxu_ListDoubleAddPairFirst( Lxu_Double * pDiv, Lxu_Pair * pLink )
{
	Lxu_ListPair * pList = &pDiv->lPairs;
	if ( pList->pHead == NULL )
	{
		pList->pHead = pLink;
		pList->pTail = pLink;
		pLink->pDPrev = NULL;
		pLink->pDNext = NULL;
	}
	else
	{
		pLink->pDPrev = NULL;
		pList->pHead->pDPrev = pLink;
		pLink->pDNext = pList->pHead;
		pList->pHead = pLink;
	}
	pList->nItems++;
}

/**Function*************************************************************

  Synopsis    [Adds the entry in the middle of the list after the spot.]

  Description [Assumes that spot points to the link, after which the given
  link should be added. Spot cannot be NULL or the tail of the list.
  Therefore, the head and the tail of the list are not changed.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lxu_ListDoubleAddPairMiddle( Lxu_Double * pDiv, Lxu_Pair * pSpot, Lxu_Pair * pLink )
{
	Lxu_ListPair * pList = &pDiv->lPairs;
	assert( pSpot );
	assert( pSpot != pList->pTail );
	pLink->pDPrev = pSpot;
	pLink->pDNext = pSpot->pDNext;
	pLink->pDPrev->pDNext = pLink;
	pLink->pDNext->pDPrev = pLink;
	pList->nItems++;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lxu_ListDoubleDelPair( Lxu_Double * pDiv, Lxu_Pair * pLink )
{
	Lxu_ListPair * pList = &pDiv->lPairs;
	if ( pList->pHead == pLink )
		 pList->pHead = pLink->pDNext;
	if ( pList->pTail == pLink )
		 pList->pTail = pLink->pDPrev;
	if ( pLink->pDPrev )
		 pLink->pDPrev->pDNext = pLink->pDNext;
	if ( pLink->pDNext )
		 pLink->pDNext->pDPrev = pLink->pDPrev;
	pList->nItems--;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lxu_ListDoubleAddPairPlace( Lxu_Double * pDiv, Lxu_Pair * pPair, Lxu_Pair * pPairSpot )
{
	printf( "Lxu_ListDoubleAddPairPlace() is called!\n" );
}



////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


