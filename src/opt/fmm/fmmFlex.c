/**CFile****************************************************************

  FileName    [fmmFlex.c]

  PackageName [Multi-valued flexibility manager.]

  Synopsis    [Complete flexibility computation.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 3.0. Started - September 27, 2003.]

  Revision    [$Id: fmmFlex.c,v 1.2 2004/05/12 04:30:13 alanmi Exp $]

***********************************************************************/

#include "fmmInt.h"
 
////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int * Fmm_FlexCreatePermMap( Fmm_Manager_t * pMan );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Get the flexibility in the global space.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Fmm_FlexComputeGlobal( Fmm_Manager_t * pMan )
{
    DdManager * dd = pMan->ddGlo;
    Ntk_Node_t * pNode;
    DdNode ** pbFuncs, ** pbFuncsZ;
    DdNode * bRel, * bCond, * bTemp;
    int v, clk;

    Extra_OperationTimeoutSet( FMM_OPER_TIMEOUT );
clk = clock();
    // create the containment condition
    bRel = b1;   Cudd_Ref( bRel );
    // go through the outputs
    Ntk_NetworkForEachCo( pMan->pNet, pNode )
    {
        // get the modified functions
        pbFuncsZ = Ntk_NodeReadFuncGlobZ( pNode );
        if ( pbFuncsZ[0] == NULL )
            continue;
        // get the global functions of the spec
//        pbFuncs = Ntk_NodeReadFuncGlob( pNode );
        pbFuncs = Ntk_NodeReadFuncGlobS( pNode );
        assert( pbFuncs[0] != NULL );
        for ( v = 0; v < pNode->nValues; v++ )
        {
            // get the condition  (bFuncZ => bFunc)
//            bCond = Cudd_bddOr( dd, Cudd_Not(bFuncZ), bFunc ); Cudd_Ref( bCond );
            bCond = Extra_bddAnd( dd, pbFuncsZ[v], Cudd_Not(pbFuncs[v]) );  
            if ( bCond == NULL )
            {
//                printf( "Oper timeout\n" );
                Cudd_RecursiveDeref( dd, bRel );
                Extra_OperationTimeoutReset();
                return NULL;
            }
            bCond = Cudd_Not( bCond );
            Cudd_Ref( bCond );
//PRB( dd, pbFuncsZ[v] );
//PRB( dd, pbFuncs[v] );

            // multiply this condition by the general condition
            bRel = Extra_bddAnd( dd, bTemp = bRel, bCond );     
            if ( bRel == NULL )
            {
//                printf( "Oper timeout\n" );
                Cudd_RecursiveDeref( dd, bTemp );
                Cudd_RecursiveDeref( dd, bCond );
                Extra_OperationTimeoutReset();
                return NULL;
            }
            Cudd_Ref( bRel );
            Cudd_RecursiveDeref( dd, bTemp );
            Cudd_RecursiveDeref( dd, bCond );
        }
    }
//PRT( "Oper", clock() - clk );
    Extra_OperationTimeoutReset();
    Cudd_Deref( bRel );
    return bRel;
}


/**Function*************************************************************

  Synopsis    [Get the flexibility in the local space.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvr_Relation_t * Fmm_FlexComputeLocal( Fmm_Manager_t * pMan, DdNode * bFlexGlo, Ntk_Node_t ** ppFanins, int nFanins )
{
    Mvr_Manager_t * pMvrMan;
    Mvr_Relation_t * pMvrLoc;
    DdManager * ddGlo;      // bFlexGlo is expressed using this manager
    DdManager * ddLoc;      // the resulting relation uses this manager
    DdNode * bTemp, * bImage;
    int * pPermute;
    int nValues, i;
    DdNode ** pbFuncs;
    int clk;
clk = clock();
    pMvrMan = Ntk_NetworkReadManMvr( pMan->pNet );

    // get the global and local BDD managers
    ddGlo = pMan->ddGlo;
    ddLoc = Mvr_ManagerReadDdLoc(pMvrMan);

    nValues = 0;
    for ( i = 0; i < nFanins; i++ )
    {
        pbFuncs = Ntk_NodeReadFuncGlob( ppFanins[i] );
        Ntk_NodeGlobalCopyFuncs( pMan->pbArray + nValues, pbFuncs, ppFanins[i]->nValues );
        nValues += ppFanins[i]->nValues;
    }
    Fmm_ImageTimeoutSet( FMM_IMAGE_TIMEOUT );
    bImage = Fmm_ImageMvCompute( ddGlo, Cudd_Not(bFlexGlo), pMan->pbArray, nValues, 
                pMan->pVmxG, pMan->pVmxL, pMan->pbVars1, 100 ); 
    if ( bImage == NULL )
    {
//        printf( "Image timeout\n" );
        return NULL;
    }
    else
    {
        bImage = Cudd_Not( bImage ); // ImageMv does not complement image!!!
        Cudd_Ref( bImage );
        // transfer from the FM's own global manager into the MVR's local manager
        pPermute = Fmm_FlexCreatePermMap( pMan );
        bImage = Extra_TransferPermute( ddGlo, ddLoc, bTemp = bImage, pPermute );
        Cudd_Ref( bImage );
        Cudd_RecursiveDeref( ddGlo, bTemp );
        FREE( pPermute );
    }
//PRT( "Image", clock() - clk );
    // create the relation
    pMvrLoc = Mvr_RelationCreate( pMvrMan, pMan->pVmxL, bImage ); 
    Cudd_Deref( bImage );
    // reorder the relation
//    Mvr_RelationReorder( pMvrLoc );
    return pMvrLoc;
}

/**Function*************************************************************

  Synopsis    [Composes the special node functions.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
/*
// this function does not permute the output var
int * Fmm_FlexCreatePermMap( Fmm_Manager_t * pMan )
{
    DdManager * dd = pMan->ddGlo;
    int * pPermute;
    int i, iVar, nBits;

    // allocate the permutation map
    pPermute = ALLOC( int, dd->size );
    for ( i = 0; i < dd->size; i++ )
        pPermute[i] = -1;

    // get the parameters
    iVar = 0;
    nBits = Vmx_VarMapReadBitsNum( pMan->pVmxL );
    for ( i = 0; i < nBits; i++ )
        pPermute[pMan->pbVars1[i]->index] = iVar++;
    return pPermute;
}
*/


int * Fmm_FlexCreatePermMap( Fmm_Manager_t * pMan )
{
    DdManager * dd = pMan->ddGlo;
    int * pPermute, * pBitOrder, * pBitsFirst;
    int i, iVar, nBits;
    int nVarsIn;

    // allocate the permutation map
    pPermute = ALLOC( int, dd->size );
    for ( i = 0; i < dd->size; i++ )
        pPermute[i] = -1;

    // get the parameters
    pBitsFirst = Vmx_VarMapReadBitsFirst( pMan->pVmxL );
    nVarsIn    = Vm_VarMapReadVarsInNum( Vmx_VarMapReadVm(pMan->pVmxL) );

    // add the input variables
    iVar = 0;
    for ( i = 0; i < pBitsFirst[nVarsIn]; i++ )
//        pPermute[pMan->nVars0 + i] = iVar++;
//        pPermute[dd->size/2 + i] = iVar++;
        pPermute[pMan->pbVars1[i]->index] = iVar++;

    // get the parameters
    pBitOrder  = Vmx_VarMapReadBitsOrder( pMan->pVmxG );
    pBitsFirst = Vmx_VarMapReadBitsFirst( pMan->pVmxG );
    nVarsIn    = Vm_VarMapReadVarsInNum( Vmx_VarMapReadVm(pMan->pVmxG) );
    nBits      = Vmx_VarMapReadBitsNum( pMan->pVmxG );

    // add the output variables
    for ( i = pBitsFirst[nVarsIn]; i < nBits; i++ )
        pPermute[pBitOrder[i]] = iVar++;
    return pPermute;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


