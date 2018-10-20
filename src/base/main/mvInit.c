/**CFile****************************************************************

  FileName    [mvInit.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    []

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: mvInit.c,v 1.30 2005/05/18 04:42:15 alanmi Exp $]

***********************************************************************/

#include "mv.h"
#include "mvInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

extern void Ntki_Init( Mv_Frame_t * pMvsis );
extern void Ntki_End ( Mv_Frame_t * pMvsis );
extern void Simp_Init( Mv_Frame_t * pMvsis );
extern void Simp_End ( Mv_Frame_t * pMvsis );
extern void Pd_Init  ( Mv_Frame_t * pMvsis );
extern void Pd_End   ( Mv_Frame_t * pMvsis );
extern void Cb_Init  ( Mv_Frame_t * pMvsis );
extern void Enc_Init ( Mv_Frame_t * pMvsis );
extern void Enc_End  ( Mv_Frame_t * pMvsis );
extern void Mfs_Init ( Mv_Frame_t * pMvsis );
extern void Mfs_End  ( Mv_Frame_t * pMvsis );
extern void Mfsn_Init( Mv_Frame_t * pMvsis );
extern void Mfsn_End ( Mv_Frame_t * pMvsis );
extern void Mfsm_Init( Mv_Frame_t * pMvsis );
extern void Mfsm_End ( Mv_Frame_t * pMvsis );
extern void Au_Init ( Mv_Frame_t * pMvsis );
extern void Au_End  ( Mv_Frame_t * pMvsis );
extern void Aio_Init ( Mv_Frame_t * pMvsis );
extern void Aio_End  ( Mv_Frame_t * pMvsis );
extern void Auti_Init ( Mv_Frame_t * pMvsis );
extern void Auti_End  ( Mv_Frame_t * pMvsis );
extern void Dual_Init( Mv_Frame_t * pMvsis );
extern void Dual_End ( Mv_Frame_t * pMvsis );
extern void Exp_Init ( Mv_Frame_t * pMvsis );
extern void Exp_End  ( Mv_Frame_t * pMvsis );
extern void Mvn_Init ( Mv_Frame_t * pMvsis );
extern void Mvn_End  ( Mv_Frame_t * pMvsis );
//extern void Eijk_Init ( Mv_Frame_t * pMvsis );
//extern void Eijk_End  ( Mv_Frame_t * pMvsis );
extern void Lang_Init( Mv_Frame_t * pMvsis );
extern void Lang_End ( Mv_Frame_t * pMvsis );
extern void Mio_Init ( Mv_Frame_t * pMvsis );
extern void Mio_End  ( Mv_Frame_t * pMvsis );
extern void Super_Init( Mv_Frame_t * pMvsis );
extern void Super_End ( Mv_Frame_t * pMvsis );
extern void Map_Init( Mv_Frame_t * pMvsis );
extern void Map_End ( Mv_Frame_t * pMvsis );
extern void Fpga_Init( Mv_Frame_t * pMvsis );
extern void Fpga_End ( Mv_Frame_t * pMvsis );
extern void Ver_Init( Mv_Frame_t * pMvsis );
extern void Ver_End ( Mv_Frame_t * pMvsis );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Starts all the packages.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mv_FrameInit( Mv_Frame_t * pMvsis )
{
    Cmd_Init( pMvsis );
    Io_Init( pMvsis );
    Ntki_Init( pMvsis );
    Simp_Init( pMvsis );
    Pd_Init( pMvsis );
    Cb_Init( pMvsis );
    Enc_Init( pMvsis );
    Mfs_Init( pMvsis );
    Mfsn_Init( pMvsis );
    Mfsm_Init( pMvsis );
    Au_Init( pMvsis );
    Dual_Init( pMvsis );
    // Exp_Init( pMvsis );
    Mvn_Init( pMvsis );
//    Eijk_Init( pMvsis );
    Lang_Init( pMvsis );
    Mio_Init( pMvsis );
    Aio_Init( pMvsis );
    Auti_Init( pMvsis );
    Super_Init( pMvsis );
    Map_Init( pMvsis );
    Fpga_Init( pMvsis );
    Ver_Init( pMvsis );
}


/**Function*************************************************************

  Synopsis    [Stops all the packages.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mv_FrameEnd( Mv_Frame_t * pMvsis )
{
    Ver_End( pMvsis );
    Fpga_End( pMvsis );
    Map_End( pMvsis );
    Super_End( pMvsis );
    Auti_End( pMvsis );
    Aio_End( pMvsis );
    Mio_End( pMvsis );
    Lang_End( pMvsis );
    Mvn_End( pMvsis );
//    Eijk_End( pMvsis );
    // Exp_End( pMvsis );
    Au_End( pMvsis );
    Mfsn_End( pMvsis );
    Mfsm_End( pMvsis );
    Mfs_End( pMvsis );
    Enc_End( pMvsis );
    Pd_End( pMvsis );
    Simp_End( pMvsis );
    Ntki_End( pMvsis );
    Io_End( pMvsis );
    Cmd_End( pMvsis );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


