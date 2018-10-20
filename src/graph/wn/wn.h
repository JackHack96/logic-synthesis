/**CFile****************************************************************

  FileName    [wn.h]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [External declarations of windowing package.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: wn.h,v 1.6 2004/05/12 04:30:11 alanmi Exp $]

***********************************************************************/

#ifndef __WN_H__
#define __WN_H__

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

typedef struct WnWindowStruct      Wn_Window_t;   

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

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

/*=== wnApi.c =========================================================*/
extern Ntk_Network_t * Wn_WindowReadNet      ( Wn_Window_t * p );
extern int             Wn_WindowReadLevelsTfi( Wn_Window_t * p );
extern int             Wn_WindowReadLevelsTfo( Wn_Window_t * p );
extern Ntk_Node_t **   Wn_WindowReadLeaves   ( Wn_Window_t * p );
extern int             Wn_WindowReadLeafNum  ( Wn_Window_t * p );
extern Ntk_Node_t **   Wn_WindowReadRoots    ( Wn_Window_t * p );
extern int             Wn_WindowReadRootNum  ( Wn_Window_t * p );
extern Ntk_Node_t **   Wn_WindowReadNodes    ( Wn_Window_t * p );
extern int             Wn_WindowReadNodeNum  ( Wn_Window_t * p );
extern Wn_Window_t *   Wn_WindowReadWndCore  ( Wn_Window_t * p );
extern Vm_VarMap_t *   Wn_WindowReadVmL      ( Wn_Window_t * p );
extern Vm_VarMap_t *   Wn_WindowReadVmR      ( Wn_Window_t * p );
extern Vm_VarMap_t *   Wn_WindowReadVmS      ( Wn_Window_t * p );
extern void            Wn_WindowSetWndCore  ( Wn_Window_t * p, Wn_Window_t * pCore );
/*=== wnCreate.c =========================================================*/
extern Wn_Window_t *   Wn_WindowCreateFromNode( Ntk_Node_t * pNode );
extern Wn_Window_t *   Wn_WindowCreateFromNodeArray( Ntk_Network_t * pNet );
extern Wn_Window_t *   Wn_WindowCreateFromNetwork( Ntk_Network_t * pNet );
extern Wn_Window_t *   Wn_WindowCreateFromNetworkNames( Ntk_Network_t * pNet, char ** pNamesIn, char ** pNamesOut );
extern void            Wn_WindowDelete( Wn_Window_t * p );
/*=== wnDfs.c =========================================================*/
extern int             Wn_WindowDfs( Wn_Window_t * pWnd );
extern int             Wn_WindowInterleave( Wn_Window_t * pWnd );
extern int             Wn_WindowGetNumLevels( Wn_Window_t * pWnd );
extern int             Wn_WindowLevelizeNodes( Wn_Window_t * pWnd, int fFromRoots );
extern void            Wn_WindowCollectInternal( Wn_Window_t * pWnd );
/*=== wnStrash.c =========================================================*/
extern Sh_Network_t *  Wn_WindowStrash( Sh_Manager_t * pMan, Wn_Window_t * pWnd, bool fUseUnique );
extern Sh_Network_t *  Wn_WindowStrashCore( Sh_Manager_t * pMan, Wn_Window_t * pWnd, Wn_Window_t * pWndC );
extern void            Wn_WindowStrashClean( Wn_Window_t * pWnd );

/*=== wnStrashBin.c =========================================================*/
extern Sh_Network_t *  Wn_WindowStrashBinaryMiter( Sh_Manager_t * pMan, Wn_Window_t * pWnd );

/*=== wnDerive.c =========================================================*/
extern Wn_Window_t *   Wn_WindowDerive( Wn_Window_t * pWndCore, int nLevelsTfi, int nLevelsTfo );
extern Wn_Window_t *   Wn_WindowDeriveForNodeOld( Ntk_Node_t * pNode, int nLevelsTfi, int nLevelsTfo );
extern Wn_Window_t *   Wn_WindowDeriveForNode( Ntk_Node_t * pNode, int nLevelsTfi, int nLevelsTfo );
extern Wn_Window_t *   Wn_WindowComputeForNode( Ntk_Node_t * pNode, int nNodesLimit );
extern void            Wn_WindowShow( Wn_Window_t * pWnd );

/*=== wnUtils.c =========================================================*/
extern int             Wn_WindowLargestFaninCount( Wn_Window_t * pWnd );
extern bool            Wn_WindowIsND( Wn_Window_t * pWnd );
extern void            Wn_WindowCreateVarMaps( Wn_Window_t * pWnd );


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

#endif
