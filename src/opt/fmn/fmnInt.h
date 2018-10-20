/**CFile****************************************************************

  FileName    [dcmn.h]

  PackageName [New don't care manager.]

  Synopsis    [External declarations of the don't-care manager.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - July 13, 2003.]

  Revision    [$Id: fmnInt.h,v 1.2 2003/11/18 18:55:12 alanmi Exp $]

***********************************************************************/

#ifndef __FMN_INT_H__
#define __FMN_INT_H__

#include "mv.h"
#include "fmn.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFITIONS                            ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

struct Dcmn_Man_t_ 
{
//	DdManager *  ddLoc;      // the local BDD manager
    DdManager *  ddGlo;      // the global BDD manager
	// data related to the network
	Ntk_Network_t * pNet;    // the network, on which this manager works
	int          FaninMax;   // the maximum fanin count over all nodes of the primary and EXDC networks
	DdNode **    pbArray;    // temporary storage for BDDs
	DdNode **    pbArray2;   // temporary storage for BDDs
    int          nArrayAlloc;// the number of allocated entries
    // data related to the global BDDs
    st_table *   pLeaves;    // the mapping of PIs into elementary BDD var numbers
//    int          nExdcs;
//    DdNode **    pbExdcs;    // the storage for external don't-cares
    DdNode **    pbVars0;    // the BDD variables
    DdNode **    pbVars1;    // the BDD variables
    Vm_VarMap_t * pVm;       // the variable map for CIs, intermediates, and COs
    Vmx_VarMap_t * pVmx;     // the encoded variable map for CIs, intermediates, and COs
    DdNode **    pbCodes;    // the codes
    int          nCodes;
    // the variable maps
    Vmx_VarMap_t * pVmxG;    // the global space of the node
    Vmx_VarMap_t * pVmxGS;   // the global set space of the node
    Vmx_VarMap_t * pVmxL;    // the local space of the node

    // special nodes
    int          nSpecials;  // the number of special nodes 
    int          nSpecialsAlloc;  // the number of allocated special nodes 
    Ntk_Node_t** ppSpecials; // the array of special nodes


	// statistical data
	int          timeTemp;   // temporary clock
	int          timeStart;  // starting time
	int          timeOrder;  // time to compute variable order
	int          timeReorder;// time to compute variable order
	int          timeGlo;    // time to compute global BDDs for the first time
	int          timeGloZ;   // time to update global BDDs
	int          timeImage;  // time to compute image
	int          timeDc;     // time to compute the don't-care
	int          timeUser;   // time spent by the user
	int          timeOther;  // other time
	int          timeTotal;  // total time

    // various parameters
    Dcmn_Par_t * pPars;
}; 


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/*=== dcmnImage.c =======================================================*/
/*=== dcmnImageMv.c =======================================================*/
extern DdNode * Dcmn_ImageMvCompute( DdManager * dd, DdNode * bCare, DdNode ** pbFuncs, int nFuncs, 
    Vmx_VarMap_t * pVmxGlo, Vmx_VarMap_t * pVmxLoc, DdNode ** pbVars, int nBddSize );
extern void dcmnImageTimeoutSet( int Timeout );
extern void dcmnImageTimeoutReset();
/*=== fmImageMv.c =============================================================*/

#endif
////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
