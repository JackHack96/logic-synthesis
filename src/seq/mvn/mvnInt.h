/**CFile****************************************************************

  FileName    [mvnInt.h]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Various procedures working with FSMs on the network level.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: mvnInt.h,v 1.8 2004/04/08 04:48:26 alanmi Exp $]

***********************************************************************/

#ifndef __MVN_INT_H__
#define __MVN_INT_H__

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "mv.h"
#include "mvn.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////
///                       GLOBAL VARIABLES                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFITIONS                            ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DECLARATIONS                        ///
////////////////////////////////////////////////////////////////////////

/*=== mvnProd.c =============================================================*/
extern Ntk_Network_t * Mvn_NetAutomataProduct( Ntk_Network_t * pNet1, Ntk_Network_t * pNet2, char * ppNames1[], char * ppNames2[], int nPairs );
/*=== mvnStgExt.c =============================================================*/
extern int             Mvn_NetworkStgExtract( Ntk_Network_t * pNet, char * pFileName, int nStatesLimit, int fUseFsm, int fWriteAut, int fVerbose );
/*=== mvnStgExt2.c =============================================================*/
extern int             Mvn_NetworkStgExtract2( Ntk_Network_t * pNet, char * pFileName, int nStatesLimit, int fUseFsm, int fVerbose );
/*=== mvnSeqDc.c =============================================================*/
extern int             Mvn_NetworkExtractSequentialDc( Ntk_Network_t * pNet, int fVerbose );
/*=== mvnSeqDcOld.c =============================================================*/
extern int             Mvn_NetworkExtractSeqDcs( Ntk_Network_t * pNet );
extern DdNode *        Ntk_NetworkComputeUnreachable( Ntk_Network_t * pNet, DdNode * bTR );
extern void            Mvn_NetworkSetupVarMap( DdManager * dd, Vmx_VarMap_t * pVmx, int * pVarsCs, int * pVarsNs, int nVars );
extern void            Mvn_NetworkVarsAndResetValues( Ntk_Network_t * pNet, Ntk_Node_t * pNodeGlo, int * pVarsCs, int * pVarsNs, int * pCodes );
extern DdNode *        Mvn_ExtractGetCube( DdManager * dd, DdNode * pbCodes[], int * pValuesFirst, int pVars[], int pCodes[], int nVars );
extern void            Mvn_NetworkCreateVarCubes( Ntk_Network_t * pNet, Ntk_Node_t * pNodeGlo, DdNode ** pbCubeIn, DdNode ** pbCubeCs, DdNode ** pbCubeOut, DdNode ** pbCubeNs );
/*=== mvnUtils.c =============================================================*/
extern int             Mvn_ReadNamePairs( FILE * pOutput, Ntk_Network_t * pNet1, Ntk_Network_t * pNet2, char * argv[], int argc, char * ppNames1[], char * ppNames2[] );
/*=== mvnUtils.c =============================================================*/
extern int             Mvn_NetworkIOEncode( Ntk_Network_t * pNet, bool fEncodeLatches );
/*=== mvnSolve.c =============================================================*/
extern int Mvn_NetworkSolve( Ntk_Network_t * pNetF, Ntk_Network_t * pNetS,  
    char * pStrU, char * pStrV, char * pFileName, int nStateLimit, int fProgressive, int fUseLongNames, int fMoore, int fVerbose, int fWriteAut, int fAlgorithm );
/*=== mvnSolve2.c =============================================================*/
extern int Mvn_NetworkSolve2( Ntk_Network_t * pNetF, Ntk_Network_t * pNetS,  
    char * pStrU, char * pStrV, char * pFileName, int nStateLimit, int fProgressive, int fUseLongNames, int fMoore, int fVerbose );
/*=== mvnSplit.c =============================================================*/
extern int             Mvn_NetworkSplit( Ntk_Network_t * pNet, char * pNamesLatch, char * pNamesOut, bool fVerbose );
/*=== mvnSplit2.c =============================================================*/
extern int             Mvn_NetworkSplit2( Ntk_Network_t * pNet, char * pNamesLatch, char * pNamesOut, bool fVerbose );

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
#endif
