/**CFile****************************************************************

  FileNameIn  [equiv_class.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [VanEijk-based sequential optimization]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: equiv_class.c,v 1.2 2005/04/05 19:42:44 casem Exp $]

***********************************************************************/

#include "eijk.h"
#include "util.h"

EquivClsSet* InitEquivClassSet() {
  EquivClsSet* pRet = (EquivClsSet*) malloc( sizeof(EquivClsSet) );
  pRet->pPrev = 0;
  pRet->pNext = 0;
  pRet->pEquivCls = 0;
  return pRet;
}

void FreeEquivClassSet(EquivClsSet* pFree) {
  EquivClsSet* pEquivClsSetIter;  
  EquivCls* pEquivClsIter;

  // iterate through every node in every equivalence class, freeing everything
  assert( pFree->pPrev == 0 );
  pEquivClsSetIter = pFree;
  while(pEquivClsSetIter != 0) {
    assert( pEquivClsSetIter->pEquivCls->pPrev == 0 );
    pEquivClsIter = pEquivClsSetIter->pEquivCls;
    while (pEquivClsIter != 0) {
      if (pEquivClsIter->pNext != 0) {
        pEquivClsIter = pEquivClsIter->pNext;
        free(pEquivClsIter->pPrev);
      } else {
        free(pEquivClsIter);
        pEquivClsIter = 0;
      }
    } // end of loop over all nodes in an equivalence class
    if (pEquivClsSetIter->pNext != 0) {
      pEquivClsSetIter = pEquivClsSetIter->pNext;
      free(pEquivClsSetIter->pPrev);
    } else {
      free(pEquivClsSetIter);
      pEquivClsSetIter = 0;
    }
  } // end of loop over all equivalence classes
}

EquivCls* NewEquivClass(EquivClsSet* pSet) {
  EquivClsSet* pNewClass;

  if (pSet->pEquivCls != 0) {
    pNewClass = (EquivClsSet*) malloc( sizeof(EquivClsSet) );
    pNewClass->pNext = pSet->pNext;
    pNewClass->pPrev = pSet;
    if (pNewClass->pNext != 0)
      pNewClass->pNext->pPrev = pNewClass;
    pSet->pNext = pNewClass;
    pSet = pNewClass;
  }

  pSet->pEquivCls = (EquivCls*) malloc( sizeof(EquivCls) );
  pSet->pEquivCls->pNext = 0;
  pSet->pEquivCls->pPrev = 0;
  pSet->pEquivCls->pNode = 0;
  return pSet->pEquivCls;
}

void MoveNodeToNewClass(EquivCls* pFrom,
                        EquivCls* pTo) {
  AddToClass(pFrom->pNode, pTo);

  if (pFrom->pPrev != 0)
    pFrom->pPrev->pNext = pFrom->pNext;
  if (pFrom->pNext != 0)
    pFrom->pNext->pPrev = pFrom->pPrev;
  
  if (pFrom->pPrev || pFrom->pNext) {
    free(pFrom);
  } else {
    pFrom->pNode = 0;
  }
}

void AddToClass(Fraig_Node_t* pNode, 
                EquivCls* pTo) {

  EquivCls* pNewEntry;

  if (pTo->pNode == 0) {
    pTo->pNode = pNode;
    return;
  }

  pNewEntry = (EquivCls*) malloc( sizeof(EquivCls) );
  pNewEntry->pNode = pNode;
  pNewEntry->pPrev = pTo;
  pNewEntry->pNext = pTo->pNext;

  if (pNewEntry->pNext)
    pNewEntry->pNext->pPrev = pNewEntry;
  pTo->pNext = pNewEntry;

}

// this will return 1 if there the two classes don't have the same data
int DiffEquivClasses(EquivCls* pFirst,
                     EquivCls* pSecond) {
  if ((pFirst == 0) || (pSecond == 0)) {
    if ((pFirst == 0) && (pSecond == 0))
      return 0;
    else
      return 1;
  } else if (pFirst->pNode == pSecond->pNode) {
    return DiffEquivClasses(pFirst->pNext, pSecond->pNext);
  } else {
    return 1;
  }
}
  
// this will return 1 if there is a class in the first not equal to a class in
// the second
int DiffEquivClassSets(EquivClsSet* pFirst,
                       EquivClsSet* pSecond) {
  int nMatchSum;
  EquivClsSet* pComp;

  if ((pFirst == 0) || (pSecond == 0)) {
    return 1; // they're different
  } else {
    pComp = pSecond;
    nMatchSum = 0;
    while (pComp != 0) {
      if (DiffEquivClasses(pFirst->pEquivCls, pComp->pEquivCls) == 0) {
        nMatchSum++;
        break;
      }
      pComp = pComp->pNext;
    }
    if (nMatchSum == 0)
      return 1; // different
    else
      if (pFirst->pNext != 0)
        return DiffEquivClassSets(pFirst->pNext, pSecond);
      else
        return 0; // same
  }
    
}
