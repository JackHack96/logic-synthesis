/**CFile****************************************************************

  FileName    [simpFlex.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Compute complete flexibity for a node.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: simpFlex.c,v 1.5 2003/05/27 23:16:13 alanmi Exp $]

***********************************************************************/

#include "simpInt.h"

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

  Synopsis    [Compute complete flexibility]
  Description [(ref: Alan Mishchenko, Robert Brayton, ICCAD'02)]
  
  SideEffects []
  SeeAlso     []
******************************************************************************/
DdNode *
Simp_ComputeFlex(
    Simp_Info_t *pInfo,
    Ntk_Node_t  *pNode)
{
    /* we do not support complete flexibility in this version of fullsimp */
    return NULL;
}


/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/
