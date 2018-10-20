/**CFile****************************************************************

  FileName    [simp.h]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Two-level node minimization using don't cares.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: simp.h,v 1.14 2003/05/27 23:16:10 alanmi Exp $]

***********************************************************************/

#ifndef _SIMP_H
#define _SIMP_H


/*---------------------------------------------------------------------------*/
/* Nested includes                                                           */
/*---------------------------------------------------------------------------*/
#include "simpArray.h"

/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/
typedef struct SimpInfoStruct Simp_Info_t;
typedef struct SimpNodeStruct Simp_Node_t;
typedef enum   SimpMethodType Simp_Method_t;
typedef enum   SimpAcceptType Simp_AcceptType_t;


/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Structure declarations                                                    */
/*---------------------------------------------------------------------------*/

enum SimpMethodType{
    SIMP_ESPRESSO,   /* 0 */
    SIMP_NOCOMP,     /* 1 */
    SIMP_SNOCOMP,    /* 2 */
    SIMP_DCSIMP,     /* 3 */
    SIMP_EXACT,      /* 4 */
    SIMP_EXACT_LITS, /* 5 */
    SIMP_EBD_ISOP,   /* 6 */
    SIMP_SIMPLE      /* 7 */
};

enum SimpAcceptType{
    SIMP_CUBE,       /* 0 */
    SIMP_SOP_LIT,    /* 1 */
    SIMP_FCT_LIT,    /* 2 */
    SIMP_ALWAYS      /* 3 */
};


/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Function prototypes                                                       */
/*---------------------------------------------------------------------------*/

/*=== simpCmd.c ====================================================*/
EXTERN void            Simp_Init();
EXTERN void            Simp_End ();
EXTERN Simp_Info_t *   Simp_InfoInit( Mv_Frame_t *pMvsis, Ntk_Network_t *net);


EXTERN bool            Simp_FullsimpInit(Ntk_Network_t *net, Simp_Info_t *i);
EXTERN bool            Simp_FullsimpEnd (Ntk_Network_t *net, Simp_Info_t *i);

EXTERN void            Simp_NetworkSimplify    (Ntk_Network_t *n, Simp_Info_t *i);
EXTERN bool            Simp_NetworkFullSimplify(Ntk_Network_t *n, Simp_Info_t *i);

EXTERN bool            Simp_NodeSimplify  (Ntk_Node_t *f,Simp_Method_t m,
                                           Simp_AcceptType_t a, bool fSparse,
                                           bool fConserve, bool fPhase, bool fRel);
EXTERN bool            Simp_NodeSimplifyDc(Ntk_Node_t *f,Ntk_Node_t *d, Simp_Method_t m,
                                           Simp_AcceptType_t a, bool fSparse,
                                           bool fConserve, bool fPhase, bool fRel);
EXTERN bool            Simp_AcceptResult(Cvr_Cover_t * pCo,Cvr_Cover_t * newcvr,
                                         Simp_AcceptType_t accept, bool verbose);

EXTERN DdNode*         Simp_ComputeCodc (Simp_Info_t *i, Ntk_Node_t *n);
EXTERN DdNode*         Simp_ComputeFlex (Simp_Info_t *i, Ntk_Node_t *n);
EXTERN Ntk_Node_t *    Simp_ComputeImage(Simp_Info_t *i, Ntk_Node_t *n, DdNode *dc);
EXTERN Ntk_Node_t *    Simp_ComputeSdcLocal    (Ntk_Node_t *node);
EXTERN Ntk_Node_t *    Simp_ComputeSdcNewBase  (Ntk_Node_t *node);
EXTERN Ntk_Node_t *    Simp_ComputeSdcResub    (Ntk_Node_t *pNode, sarray_t *lFanin, 
                                                sarray_t *lResub);
EXTERN bool            SimpNodeIsHardCase( Ntk_Node_t *pNode ) ;



/**AutomaticEnd***************************************************************/

#endif /* _SIMP_H */
