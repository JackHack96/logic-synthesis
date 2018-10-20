/**CFile****************************************************************

  FileName    [vmx.h]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Declarations of the extended variable map (VMX) package.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: vmx.h,v 1.15 2004/01/06 21:02:58 alanmi Exp $]

***********************************************************************/

#ifndef __VMX_H__
#define __VMX_H__

/*
    The extended variable map (VMX) provides additional information
    compared to the original variable map (VM). This information 
    concerns the binary encoding of MV variables, and is used to
    derive and manipulate MVfunctions and relation in the MVP package.
*/

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFITIONS                            ///
////////////////////////////////////////////////////////////////////////

typedef struct VmxManagerStruct    Vmx_Manager_t;    // the VMX manager
typedef struct VmxVarMapStruct     Vmx_VarMap_t;     // the VMX data structure

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/*=== vmx.c ======================================================*/
/*=== vmxApi.c ===================================================*/
extern Vmx_Manager_t * Vmx_VarMapReadMan( Vmx_VarMap_t * p );
extern Vm_VarMap_t *   Vmx_VarMapReadVm( Vmx_VarMap_t * p );
extern int *           Vmx_VarMapReadBits( Vmx_VarMap_t * p );
extern int *           Vmx_VarMapReadBitsFirst( Vmx_VarMap_t * p );
extern int             Vmx_VarMapReadBitsNum( Vmx_VarMap_t * p );
extern int             Vmx_VarMapReadBitsInNum( Vmx_VarMap_t * p );
extern int             Vmx_VarMapReadBitsOutNum( Vmx_VarMap_t * p );
extern int *           Vmx_VarMapReadBitsOrder( Vmx_VarMap_t * p );
/*=== vmxMan.c ===================================================*/
extern Vmx_Manager_t * Vmx_ManagerAlloc();
extern void            Vmx_ManagerFree( Vmx_Manager_t * p );
extern Vmx_Manager_t * Vmx_ManagerRef( Vmx_Manager_t * p );
extern void            Vmx_ManagerDeref( Vmx_Manager_t * p );
/*=== vmxMap.c ===================================================*/
extern Vmx_VarMap_t *  Vmx_VarMapLookup( Vmx_Manager_t * pMan, Vm_VarMap_t * pVmxMv, int nBits, int * pBitsOrder );
extern Vmx_VarMap_t *  Vmx_VarMapRef( Vmx_VarMap_t * pVmx );
extern Vmx_VarMap_t *  Vmx_VarMapDeref( Vmx_VarMap_t * pVmx );
/*=== vmxCode.c ==================================================*/
extern DdNode **       Vmx_VarMapEncodeMap( DdManager * dd, Vmx_VarMap_t * pVmx );
extern DdNode **       Vmx_VarMapEncodeMapMinterm( DdManager * dd, Vmx_VarMap_t * pVmx );
extern DdNode **       Vmx_VarMapEncodeVarMinterm( DdManager * dd, Vmx_VarMap_t * pVmx, int iVar );
extern DdNode **       Vmx_VarMapEncodeMapUsingVars( DdManager * dd, DdNode * pbVars[], Vmx_VarMap_t * pVmx );
extern DdNode **       Vmx_VarMapEncodeVar( DdManager * dd, Vmx_VarMap_t * pVmx, int iVar );
extern DdNode **       Vmx_VarMapEncodeVarUsingVars( DdManager * dd, DdNode * pbVars[], Vmx_VarMap_t * pVmx, int iVar );
extern void            Vmx_VarMapEncodeDeref( DdManager * dd, Vmx_VarMap_t * pVmxBin, DdNode ** pbCodes );
extern void            Vmx_VarMapDecode( Vmx_VarMap_t * pVmxBin, int * pCubeBin, int * pCubeM );
extern int             Vmx_VarMapDecodeCubeVar( DdManager * dd, DdNode * bCube, Vmx_VarMap_t * pVmx, int iVar );
extern void            Vmx_VarMapDecodeCube( DdManager * dd, DdNode * bCube, Vmx_VarMap_t * pVmx, int * pVars, int * pCodes, int nVars );
extern DdNode *        Vmx_VarMapCharDomain( DdManager * dd, Vmx_VarMap_t * pVmx, int iVar );
/*=== vmxCube.c ==================================================*/
extern DdNode *        Vmx_VarMapCharCube( DdManager * dd, Vmx_VarMap_t * pVmxBin, int iVar );
extern DdNode *        Vmx_VarMapCharCubeArray( DdManager * dd, Vmx_VarMap_t * pVmxBin, int pVars[], int nVar );
extern DdNode *        Vmx_VarMapCharCubeRange( DdManager * dd, Vmx_VarMap_t * pVmx, int iVar, int nVars );
extern DdNode *        Vmx_VarMapCharCubeUsingVars( DdManager * dd, DdNode ** pbVars, Vmx_VarMap_t * pVmxBin, int iVar );
extern DdNode *        Vmx_VarMapCharCubeInput( DdManager * dd, Vmx_VarMap_t * pVmx );
extern DdNode *        Vmx_VarMapCharCubeInputUsingVars( DdManager * dd, DdNode ** pbVars, Vmx_VarMap_t * pVmx );
extern DdNode *        Vmx_VarMapCharCubeOutput( DdManager * dd, Vmx_VarMap_t * pVmx );
extern DdNode *        Vmx_VarMapCodeCube( DdManager * dd, Vmx_VarMap_t * pVmx, int pVars[], int pCodes[], int nVars );
/*=== vmxCreate.c ==================================================*/
extern Vmx_VarMap_t *  Vmx_VarMapCreateBinary( Vm_Manager_t * pManVm, Vmx_Manager_t * pManVmx, int nVarsIn, int nVarsOut );
extern Vmx_VarMap_t *  Vmx_VarMapCreateSimple( Vm_Manager_t * pManVm, Vmx_Manager_t * pManVmx, int nVarsIn, int nVarsOut, int * pValues );
extern Vmx_VarMap_t *  Vmx_VarMapCreateExpanded( Vmx_VarMap_t * pVmxN, Vm_VarMap_t * pVmNew, int * pTransNInv );
extern Vmx_VarMap_t *  Vmx_VarMapCreateReduced( Vmx_VarMap_t * pVmx, int * pSuppMv );
extern Vmx_VarMap_t *  Vmx_VarMapCreatePermuted( Vmx_VarMap_t * pVmx, int * pPermuteInv );
extern Vmx_VarMap_t *  Vmx_VarMapCreateOrdered( Vmx_VarMap_t * pVmx, int * pPermuteInv );
extern Vmx_VarMap_t *  Vmx_VarMapCreateReordered( Vmx_VarMap_t * pVmx, int * pOrder, int nBinVars );
extern Vmx_VarMap_t *  Vmx_VarMapCreatePower( Vmx_VarMap_t * pVmx, int Degree );
extern Vmx_VarMap_t *  Vmx_VarMapCreateAppended( Vmx_VarMap_t * pVmx, Vm_VarMap_t * pVmNew );
/*=== vmxRemap.c ==================================================*/
extern DdNode *        Vmx_VarMapRemapFunc( DdManager * dd, DdNode * bFunc, Vmx_VarMap_t * pVmxOld, Vmx_VarMap_t * pVmxNew, int * pVarTrans );
extern DdNode *        Vmx_VarMapRemapVar( DdManager * dd, DdNode * bFunc, Vmx_VarMap_t * pVmx, int iVar1, int iVar2 );
extern DdNode *        Vmx_VarMapRemapRange( DdManager * dd, DdNode * bFunc, Vmx_VarMap_t * pVmx, int iVar1, int nVars1, int iVar2, int nVars2 );
extern DdNode *        Vmx_VarMapRemapCouple( DdManager * dd, DdNode * bFunc, Vmx_VarMap_t * pVmx, int iVarA1, int iVarA2, int iVarB1, int iVarB2 );
extern void            Vmx_VarMapRemapVarSetup( DdManager * dd, Vmx_VarMap_t * pVmx, int iVar1, int iVar2 );
extern void            Vmx_VarMapRemapRangeSetup( DdManager * dd, Vmx_VarMap_t * pVmx, int iVar1, int nVars1, int iVar2, int nVars2 );
extern void            Vmx_VarMapRemapSetup( DdManager * dd, Vmx_VarMap_t * pVmx, int * pVarsCs, int * pVarsNs, int nVars );
/*=== vmxUtils.c ==================================================*/
extern void            Vmx_VarMapPrint( Vmx_VarMap_t * pVmx );
extern DdNode **       Vmx_VarMapBinVarsVar( DdManager * dd, Vmx_VarMap_t * pVmx, int iVar );
extern DdNode **       Vmx_VarMapBinVarsRange( DdManager * dd, Vmx_VarMap_t * pVmx, int iFirst, int nVars );
extern DdNode **       Vmx_VarMapBinVarsCouple( DdManager * dd, Vmx_VarMap_t * pVmx, int iVar1, int iVar2 );
extern int             Vmx_VarMapBitsNumRange( Vmx_VarMap_t * pVmx, int iFirst, int nVars );
extern int             Vmx_VarMapGetLargestBit( Vmx_VarMap_t * pVmx );

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

#endif
