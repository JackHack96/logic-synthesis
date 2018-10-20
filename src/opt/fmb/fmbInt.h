/**CFile****************************************************************

  FileName    [fmbInt.h]

  PackageName [Binary flexibility manager.]

  Synopsis    [Internal declarations of the MV flexibility manager.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 3.0. Started - September 27, 2003.]

  Revision    [$Id: fmbInt.h,v 1.3 2004/05/12 04:30:12 alanmi Exp $]

***********************************************************************/

#ifndef __FMB_INT_H__
#define __FMB_INT_H__

#include "mv.h"
#include "sh.h"
#include "wn.h"
#include "fmb.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFITIONS                            ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

struct Fmb_Manager_t_ 
{
    // BDD variable managers
    DdManager *     ddGlo;      // the global BDD manager
    // elementary variables of the global manager
    DdNode **       pbVars0;    // the global space + extended variables
    DdNode **       pbVars1;    // the local space
	// data related to the network
	Ntk_Network_t * pNet;       // the network processed by the manager
    Wn_Window_t *   pWnd;       // the current window
	int             nBitsGlo;   // the number of bits in the global space
	int             nBitsExt;   // the maximum number of bits in the local space over all nodes of the primary and EXDC networks
	int             nBitsLoc;   // the maximum number of bits in the local space over all nodes of the primary and EXDC networks
    // the variable maps
    Vm_VarMap_t *   pVmG;       // the global space of the node
    Vm_VarMap_t *   pVmL;       // the local space of the node
    Vm_VarMap_t *   pVmS;       // the local space of the node
    Vmx_VarMap_t *  pVmxG;      // the global space of the node
    Vmx_VarMap_t *  pVmxL;      // the local space of the node
    Vmx_VarMap_t *  pVmxS;      // the local space of the node

    // temporary storage
    DdNode **       pbArray;    // temporary storage for BDDs
    int             nArrayAlloc;// the number of allocated entries

	// statistical data
	int             timeStart;  // starting time
    int             timeWnd;    // windowing
	int             timeGlo;    // time to compute global BDDs for the first time
	int             timeGloZ;   // time to update global BDDs
	int             timeFlex;   // time to compute global flexibility
	int             timeImage;  // time to compute image
	int             timeSopMin; // time to compute the don't-care
	int             timeResub;  // time spent by the user
	int             timeOther;  // other time
	int             timeTotal;  // total time
	int             timeTemp;   // temporary clock
	int             timeCdc;    // total time

    // various parameters
    Fmb_Params_t *  pPars;
}; 


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////
 
/*=== fmbUtils.c =========================================================*/
extern int              Fmb_UtilsNumSharedBddNodes( Fmb_Manager_t * pMan );
extern DdNode *         Fmb_UtilsRelationDomainCompute( DdManager * dd, DdNode * bRel, Vmx_VarMap_t * pVmx );
extern bool             Fmb_UtilsRelationDomainCheck( DdManager * dd, DdNode * bRel, Vmx_VarMap_t * pVmx );
extern DdNode *         Fmb_UtilsRelationGlobalCompute( Fmb_Manager_t * p, Ntk_Node_t * pPivots[], int nPivots, Vmx_VarMap_t * pVmxG );
extern Ntk_Node_t *     Fmb_UtilsComputeGlobal( Fmb_Manager_t * p );
extern int *            Fmb_UtilsCreatePermMapRel( Fmb_Manager_t * pMan );
extern bool             Fmb_UtilsHasZFanin( Ntk_Node_t * pNode );

#endif
////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
