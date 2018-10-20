/**CFile****************************************************************

  FileName    [enc.h]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    []

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: enc.h,v 1.2 2003/05/27 23:15:29 alanmi Exp $]

***********************************************************************/

#ifndef _ENC
#define _ENC


/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/
typedef struct EncInfoStruct Enc_Info_t;
typedef struct EncNodeStruct Enc_Node_t;
typedef struct EncCodeStruct Enc_Code_t;
typedef enum   EncMethodType Enc_Meth_t;
typedef enum   EncSimpType   Enc_Simp_t;


/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Structure declarations                                                    */
/*---------------------------------------------------------------------------*/

enum EncMethodType{
    ENC_INOUT, /* 0 */
    ENC_IN,    /* 1 */
    ENC_OUT    /* 2 */
};


enum EncSimpType{
    SIMP_ESPRESSO, /* 0 */
    SIMP_NOCOMP,   /* 1 */
    SIMP_SNOCOMP,  /* 2 */
    SIMP_DCSIMP    /* 3 */
};


/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Function prototypes                                                       */
/*---------------------------------------------------------------------------*/

/*======== encCmd.c ========*/
EXTERN void            Enc_Init( Mv_Frame_t *pMvsis );
EXTERN void            Enc_End ( Mv_Frame_t *pMvsis );


/**AutomaticEnd***************************************************************/

#endif /* _ENC */





