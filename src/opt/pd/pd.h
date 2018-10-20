/**CFile****************************************************************

  FileName    [pd.h]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    []

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: pd.h,v 1.6 2003/05/27 23:48:20 yinghua Exp $]

***********************************************************************/

#ifndef _PD
#define _PD

/*---------------------------------------------------------------------------*/
/* Function prototypes                                                       */
/*---------------------------------------------------------------------------*/

EXTERN void Pd_Init();
EXTERN void Pd_End();

EXTERN int Pd_PairDecode( Ntk_Network_t *network, int thresh, int iCost, int resubOption, int timeLimit, int fVerbose );
EXTERN int Pd_PairValue( Ntk_Node_t *p1, Ntk_Node_t *p2, int iCost );
EXTERN Ntk_Node_t *Pd_PairNodes_Simp( Ntk_Node_t *a, Ntk_Node_t *b );
EXTERN Ntk_Node_t *Pd_PairNodes_Alg( Ntk_Node_t *a, Ntk_Node_t *b );

#endif /* _PD */
