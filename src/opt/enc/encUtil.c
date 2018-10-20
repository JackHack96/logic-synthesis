/**CFile****************************************************************

  FileName    [encUtil.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    []

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: encUtil.c,v 1.2 2003/05/27 23:15:31 alanmi Exp $]

***********************************************************************/


#include "encInt.h"


/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/


/**AutomaticEnd***************************************************************/


/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/


/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
void
Enc_CodeFree(
    Enc_Code_t *pCode) 
{
    assert(pCode != NULL);
    assert(pCode->codeArray!=NULL);
    assert(pCode->assignArray!=NULL);
    FREE(pCode->codeArray);
    FREE(pCode->assignArray);
    FREE(pCode);
}


/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
bool
Enc_NodeCanBeEncoded(
    Ntk_Node_t *pNode) 
{
    if(Ntk_NodeReadValueNum(pNode) <= 2)
        return FALSE;
    if(!Ntk_NodeIsInternal(pNode))
        return FALSE;
    
    return TRUE;
}


/**Function********************************************************************

  Synopsis    []
  
  Description []

  SideEffects []
   
  SeeAlso     []

******************************************************************************/
int 
Enc_CodeLength(
    int numValues)
{
  int i, length;

  length = 1;
  for(i = 2; numValues > i; i <<= 1) { 
    length++;
  }

  return length;
}


/**Function********************************************************************

  Synopsis    []
  
  Description []

  SideEffects []
   
  SeeAlso     []

******************************************************************************/
int
Enc_IthPowerOf2(
    int i)
{
    return (1<<i);
}


/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/




