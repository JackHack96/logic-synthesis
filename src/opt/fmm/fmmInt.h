/**CFile****************************************************************

  FileName    [fmmInt.h]

  PackageName [Multi-valued flexibility manager.]

  Synopsis    [Internal declarations of the MV flexibility manager.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 3.0. Started - September 27, 2003.]

  Revision    [$Id: fmmInt.h,v 1.2 2004/05/12 04:30:14 alanmi Exp $]

***********************************************************************/

#ifndef __FMM_INT_H__
#define __FMM_INT_H__

#include "mv.h"
#include "fmm.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFITIONS                            ///
////////////////////////////////////////////////////////////////////////

#define FMM_IMAGE_TIMEOUT   300  // timeout for the image computation
#define FMM_OPER_TIMEOUT    300  // timeout for computing global flexiblity
#define FMM_UPDATE_TIMEOUT  300  // timeout for updating global BDDs

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

struct Fmm_Manager_t_ 
{
    // BDD variable managers
//	DdManager *     ddLoc;      // the local BDD manager
    DdManager *     ddGlo;      // the global BDD manager
    // elementary variables of the global manager
    DdNode **       pbVars0;    // the global space + extended variables
    DdNode **       pbVars1;    // the local space
	// data related to the network
	Ntk_Network_t * pNet;       // the network processed by the manager
	int             nBitsGlo;   // the number of bits in the global space
	int             nBitsLoc;   // the maximum number of bits in the local space over all nodes of the primary and EXDC networks
    // data related to the global BDDs
    int             nExdcs;
    DdNode **       pbExdcs;    // the storage for external don't-cares
    // the variable maps
    Vm_VarMap_t *   pVm;        // the variable map for CIs, intermediates, and COs
    Vmx_VarMap_t *  pVmx;       // the encoded variable map for CIs, intermediates, and COs
    Vmx_VarMap_t *  pVmxG;      // the global space of the node
    Vmx_VarMap_t *  pVmxGS;     // the set global space of the node
    Vmx_VarMap_t *  pVmxL;      // the local space of the node

    // temporary storage
    DdNode **       pbArray;    // temporary storage for BDDs
    int             nArrayAlloc;// the number of allocated entries

	// statistical data
	int             timeTemp;   // temporary clock
	int             timeStart;  // starting time
	int             timeGlo;    // time to compute global BDDs for the first time
	int             timeGloZ;   // time to update global BDDs
	int             timeFlex;   // time to compute global flexibility
	int             timeImage;  // time to compute image
	int             timeSopMin; // time to compute the don't-care
	int             timeResub;  // time spent by the user
	int             timeOther;  // other time
	int             timeTotal;  // total time

    // various parameters
    Fmm_Params_t *  pPars;
}; 


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////
 
/*=== fmmUtils.c =========================================================*/
extern int              Fmm_UtilsFaninMddSupportMax( Ntk_Network_t * pNet );
extern Vmx_VarMap_t *   Fmm_UtilsGetNetworkVmx( Ntk_Network_t * pNet );
extern int              Fmm_UtilsNumSharedBddNodes( Fmm_Manager_t * pMan );
extern Vmx_VarMap_t *   Fmm_UtilsCreateGlobalVmx( Fmm_Manager_t * p, Ntk_Node_t * pNode );
extern Vmx_VarMap_t *   Fmm_UtilsCreateGlobalSetVmx( Fmm_Manager_t * p, Ntk_Node_t * pNode );
extern Vmx_VarMap_t *   Fmm_UtilsCreateLocalVmx( Fmm_Manager_t * p, Ntk_Node_t * pNode, Ntk_Node_t ** ppFanins, int nFanins );
extern DdNode *         Fmm_UtilsTransferFromSetOutputs( Fmm_Manager_t * pMan, DdNode * bFlexGlo );
extern void             Fmm_UtilsPrepareSpec( Ntk_Network_t * pNet );
extern DdNode *         Fmm_UtilsRelationDomainCompute( DdManager * dd, DdNode * bRel, Vmx_VarMap_t * pVmx );
extern bool             Fmm_UtilsRelationDomainCheck( DdManager * dd, DdNode * bRel, Vmx_VarMap_t * pVmx );
extern DdNode *         Fmm_UtilsRelationGlobalCompute( DdManager * dd, Ntk_Node_t * pPivot, Vmx_VarMap_t * pVmxG );

#endif
////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
