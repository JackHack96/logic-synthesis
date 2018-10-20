/**CFile****************************************************************

  FileName    [mv.h]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [External declarations of MVSIS.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: mv.h,v 1.30 2005/06/02 03:34:09 alanmi Exp $]

***********************************************************************/

#ifndef __MV_H__
#define __MV_H__

////////////////////////////////////////////////////////////////////////
///                         TYPEDEFS                                 ///
////////////////////////////////////////////////////////////////////////

#define BALM 1

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

// the MVSIS framework containing all MVSIS data
typedef struct MvFrameStruct      Mv_Frame_t;   

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

// this include should be the first one in the list
// it is used to catch memory leaks on Windows
#include "leaks.h"       
// special types like bool, uint8
#include "mvtypes.h"

// standard includes
#include <stdio.h>
#include <string.h>

// includes from GLU
#include "util.h"
#include "avl.h"
#include "array.h"
#include "st.h"

// decision diagram support from CUDD
#include "extra.h"     
#include "reo.h"

// MVSIS data structure packages
#include "vm.h"    // variable map
#include "vmx.h"   // extended variable map
#include "mvc.h"   // SOP
#include "ft.h"    // algebraic factoring
#include "fxu.h"   // fast extract
#include "lxu.h"   // layout aware fast extract
#include "cvr.h"   // MV SOP
#include "mva.h"   // array of BDDs
#include "mvr.h"   // MV relation
#include "fnc.h"   // top-level functionality (Vm/Vmx/Mvc/Cvr/Mvr)
#include "fraig.h" // the AND-INV graph package

// MVSIS core packages
#include "ntk.h"   // network/node
#include "ntki.h"   // network/node
#include "cmd.h"   // command package
#include "io.h"    // input/output

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                       GLOBAL VARIABLES                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFITIONS                            ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/*=== mvFrame.c ===========================================================*/
extern Ntk_Network_t * Mv_FrameReadNet( Mv_Frame_t * p );
extern FILE *          Mv_FrameReadOut( Mv_Frame_t * p );
extern FILE *          Mv_FrameReadErr( Mv_Frame_t * p );
extern bool            Mv_FrameReadMode( Mv_Frame_t * p );
extern bool            Mv_FrameSetMode( Mv_Frame_t * p, bool fNameMode );
extern Fnc_Manager_t * Mv_FrameReadMan( Mv_Frame_t * p );
extern Fnc_Manager_t * Mv_FrameSetMan( Mv_Frame_t * p, Fnc_Manager_t * pMan );
extern void            Mv_FrameRestart( Mv_Frame_t * p );

extern void            Mv_FrameSetCurrentNetwork( Mv_Frame_t * p, Ntk_Network_t * pNet );
extern void            Mv_FrameSwapCurrentAndBackup( Mv_Frame_t * p );
extern void            Mv_FrameReplaceCurrentNetwork( Mv_Frame_t * p, Ntk_Network_t * pNet );
extern void            Mv_FrameDeleteAllNetworks( Mv_Frame_t * p );
extern void            Mv_FrameFreeNetworkBindings( Mv_Frame_t * p );

// Singleton pattern for the global frame when mvsis run as interpreter

extern void			   Mv_FrameSetGlobalFrame( Mv_Frame_t * p );
extern Mv_Frame_t *	   Mv_FrameGetGlobalFrame();

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

#endif
