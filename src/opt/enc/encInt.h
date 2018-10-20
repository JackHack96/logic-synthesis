/**CFile****************************************************************

  FileName    [encInt.h]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    []

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: encInt.h,v 1.2 2003/05/27 23:15:30 alanmi Exp $]

***********************************************************************/

#ifndef _ENCINT
#define _ENCINT

#include "mv.h"
#include "enc.h"

/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Structure declarations                                                    */
/*---------------------------------------------------------------------------*/

/**Struct**********************************************************************
  Synopsis    [Information for the toplevel routine.]
  Description []
******************************************************************************/
struct EncInfoStruct {
    bool verbose;
    bool fFromOutputs;        /* encoding direction */
    Enc_Meth_t method;        /* encoding method */
    Enc_Simp_t simp;          /* simplification method */
};


/**Struct**********************************************************************
  Synopsis    []
  Description []
******************************************************************************/
struct EncCodeStruct {
  int numValues;     // num output values of the original MV node
  int codeLength;
  int *codeArray;   // indexed by [value * codeLength + bit]
  int numCodes;     // 2^codeLength
  int *assignArray; // has size numCodes, assignArray[i] associates code i with its assigned value
  int defaultValue;
};


/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Internal function prototypes                                              */
/*---------------------------------------------------------------------------*/


/*======== encNtk.c ========*/
EXTERN void Enc_NetworkEncode( Ntk_Network_t * pNet, int nNodes, Enc_Info_t * pInfo );


/*======== encCode.c ========*/
EXTERN Enc_Code_t * Enc_NodeComputeCode( Ntk_Network_t * pNet, Ntk_Node_t * pNode, Enc_Info_t * pInfo );


/*======== encUtil.c ========*/
EXTERN bool Enc_NodeCanBeEncoded( Ntk_Node_t * pNode ); 
EXTERN void Enc_CodeFree( Enc_Code_t *pCode ); 
EXTERN int  Enc_CodeLength( int numValues );
EXTERN int  Enc_IthPowerOf2( int i );


/**AutomaticEnd***************************************************************/

#endif /* _ENCINT */
