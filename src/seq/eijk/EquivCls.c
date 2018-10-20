/**CFile****************************************************************

  FileNameIn  [EquivCls.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [VanEijk-based sequential optimization]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: EquivCls.c,v 1.2 2005/04/05 19:42:43 casem Exp $]

***********************************************************************/

#include "eijk.h"
#include "fraig.h"
#include "fraigInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Function: InitEquivClsSet
// Purpose: This will make a new equivalence class set.
////////////////////////////////////////////////////////////////////////////////
EquivClsSet* InitEquivClsSet() {
    EquivClsSet* pRet = (EquivClsSet*) malloc( sizeof(EquivClsSet) );
    pRet->pNext = 0;
    pRet->pEquivCls = 0;
    return pRet;  
}

////////////////////////////////////////////////////////////////////////////////
// Function: MakeNewEquivCls
// Purpose: This will make a new equivalence class and add it to the equivalence
//          class set.
////////////////////////////////////////////////////////////////////////////////
EquivCls* MakeNewEquivCls( EquivClsSet* pClsSet ) {

  EquivClsSet* pNewEntry;
  EquivCls* pRet;

  pRet = (EquivCls*) malloc( sizeof(EquivCls) );

  if (pClsSet->pEquivCls) {
    pNewEntry = (EquivClsSet*) malloc( sizeof(EquivClsSet) );
    pNewEntry->pEquivCls = pRet;
    pNewEntry->pNext = pClsSet->pNext;
    pClsSet->pNext = pNewEntry;
  } else {
    pClsSet->pEquivCls = pRet;
  }

  return pRet;

}
