/**CFile****************************************************************

  FileName    [wnInt.h]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Internal declarations of windowing package.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: wnInt.h,v 1.5 2004/04/12 03:42:31 alanmi Exp $]

***********************************************************************/

#ifndef __WN_INT_H__
#define __WN_INT_H__
 
////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "mv.h"
#include "ntkInt.h"
#include "shInt.h"
#include "wn.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

struct WnWindowStruct
{
    // general information
    Ntk_Network_t *    pNet;         // the parent network
    Ntk_Node_t *       pNode;        // the node for which this network is contructed
    int                nLevelsTfi;   // the number of TFI levels to include
    int                nLevelsTfo;   // the number of TFO levels to include
    // window nodes
    Ntk_Node_t **      ppLeaves;     // the leaf nodes (can be CIs)
    int                nLeaves;      // the number of these nodes
    Ntk_Node_t **      ppRoots;      // the root nodes (cannot be CIs/COs)
    int                nRoots;       // the number of these nodes
    Ntk_Node_t **      ppNodes;      // the internal nodes of the window (cannot be CIs/COs)
    int                nNodes;       // the number of these nodes
    Ntk_Node_t **      ppSpecials;   // the special nodes 
    int                nSpecials;    // the number of these nodes
    // the window core (may be NULL)
    Wn_Window_t *      pWndCore;     // the nodes around which this window is built
    // variable maps
    Vm_VarMap_t *      pVmL;         // MV var map for the leaves
    Vm_VarMap_t *      pVmR;         // MV var map for the roots
    Vm_VarMap_t *      pVmS;         // MV var map for the special nodes

    // the vectors
    Ntk_NodeVec_t *    pVecLeaves;   // the vector of leaves
    Ntk_NodeVec_t *    pVecRoots;    // the vector of roots
    Ntk_NodeVec_t *    pVecNodes;    // the vector of internal nodes 
};

////////////////////////////////////////////////////////////////////////
///                       GLOBAL VARIABLES                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFITIONS                            ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/*=== wnUtils.c ===========================================================*/

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

#endif
