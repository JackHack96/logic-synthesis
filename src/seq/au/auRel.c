/**CFile****************************************************************

  FileName    [auRel.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Various procedures to manipulate transition relations.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: auRel.c,v 1.1 2004/02/19 03:06:48 alanmi Exp $]

***********************************************************************/

#include "auInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Au_Rel_t * Au_AutoRelCreate( DdManager * dd, Au_Auto_t * pAut, bool fReorder )
{
    ProgressBar * pProgress;
    Au_Rel_t * pTR;
    Au_Trans_t * pTrans;
    Vm_VarMap_t * pVmTemp;
    DdNode ** pbCodesTemp;
    DdNode * bProd, * bTemp;
    int * pValuesFirst;
    int i, nVarsBin;

    // get the temporary relation
    pVmTemp  = Vm_VarMapCreateBinary( Fnc_ManagerReadManVm(pAut->pMan), pAut->nInputs, 1 );
    
    // start the structure
    pTR = ALLOC( Au_Rel_t, 1 );
    memset( pTR, 0, sizeof(Au_Rel_t) );
    pTR->pAut = pAut;

    // create the variable map
    pTR->pVmx = Au_UtilsCreateVmx( pAut->pMan, pAut->nInputs, pAut->nStates, 1 );
    pTR->pVm  = Vmx_VarMapReadVm(pTR->pVmx);
    pValuesFirst = Vm_VarMapReadValuesFirstArray(pTR->pVm);

    // get the manager
    pTR->dd = dd;

    // extend manager if necessary
    nVarsBin = Vmx_VarMapReadBitsNum( pTR->pVmx );
    if ( dd->size < nVarsBin )
    {
        for ( i = dd->size; i < nVarsBin; i++ )
            Cudd_bddNewVar( dd );
    }

    // get the cubes
    pTR->bCubeCs = Vmx_VarMapCharCube( dd, pTR->pVmx, pAut->nInputs     );    Cudd_Ref( pTR->bCubeCs );
    pTR->bCubeNs = Vmx_VarMapCharCube( dd, pTR->pVmx, pAut->nInputs + 1 );    Cudd_Ref( pTR->bCubeNs );

    pTR->bCubeIn = Vmx_VarMapCharCubeInput( dd, pTR->pVmx );                        Cudd_Ref( pTR->bCubeIn );
    pTR->bCubeIn = Cudd_bddExistAbstract( dd, bTemp = pTR->bCubeIn, pTR->bCubeCs ); Cudd_Ref( pTR->bCubeIn );
    Cudd_RecursiveDeref( dd, bTemp );

    // get the codes
    pbCodesTemp = Vmx_VarMapEncodeMap( dd, pTR->pVmx );
    pTR->nStates = pAut->nStates;
    pTR->pbStatesCs = ALLOC( DdNode *, pTR->nStates );
    pTR->pbStatesNs = ALLOC( DdNode *, pTR->nStates );
    for ( i = 0; i < pTR->nStates; i++ )
    {
        pTR->pbStatesCs[i] = pbCodesTemp[pValuesFirst[pAut->nInputs + 0] + i];  Cudd_Ref( pTR->pbStatesCs[i] );
        pTR->pbStatesNs[i] = pbCodesTemp[pValuesFirst[pAut->nInputs + 1] + i];  Cudd_Ref( pTR->pbStatesNs[i] );
    }

    if ( fReorder )
        Cudd_AutodynEnable( dd, CUDD_REORDER_SYMM_SIFT );

    pProgress = Extra_ProgressBarStart( stdout, pAut->nStates );

    // derive the transition relation
    pTR->bRel = b0;  Cudd_Ref( pTR->bRel );
    for ( i = 0; i < pAut->nStates; i++ )
    {
        Au_StateForEachTransition( pAut->pStates[i], pTrans )
        {
            bProd = Fnc_FunctionDeriveMddFromSop( dd, pVmTemp, pTrans->pCond, pbCodesTemp );  Cudd_Ref( bProd );
            bProd = Cudd_bddAnd( dd, bTemp = bProd, pTR->pbStatesCs[pTrans->StateCur] );      Cudd_Ref( bProd );
            Cudd_RecursiveDeref( dd, bTemp );
            bProd = Cudd_bddAnd( dd, bTemp = bProd, pTR->pbStatesNs[pTrans->StateNext] );     Cudd_Ref( bProd );
            Cudd_RecursiveDeref( dd, bTemp );
            // add this product
            pTR->bRel = Cudd_bddOr( dd, bTemp = pTR->bRel, bProd );                           Cudd_Ref( pTR->bRel );
            Cudd_RecursiveDeref( dd, bTemp );
            Cudd_RecursiveDeref( dd, bProd );
        }

        if ( i && (i % 30 == 0) )
        {
            char Buffer[100];
            sprintf( Buffer, "%d states", i );
            Extra_ProgressBarUpdate( pProgress, i, Buffer );
        }
    }
    Extra_ProgressBarStop( pProgress );

    if ( fReorder )
        Cudd_ReduceHeap( dd, CUDD_REORDER_SYMM_SIFT, 100 );
    Cudd_AutodynDisable( dd );

//    printf( "Transition relation has %d BDD nodes.\n", Cudd_DagSize(pTR->bRel) );

    Vmx_VarMapEncodeDeref( dd, pTR->pVmx, pbCodesTemp );
    assert( pAut->pTR == NULL );
    pAut->pTR = pTR;
    return pTR;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Au_AutoRelFree( Au_Rel_t * pTR )
{
    int i;
    for ( i = 0; i < pTR->nStates; i++ )
    {
        Cudd_RecursiveDeref( pTR->dd, pTR->pbStatesCs[i] );
        Cudd_RecursiveDeref( pTR->dd, pTR->pbStatesNs[i] );
    }
    Cudd_RecursiveDeref( pTR->dd, pTR->bRel );
    Cudd_RecursiveDeref( pTR->dd, pTR->bCubeCs );
    Cudd_RecursiveDeref( pTR->dd, pTR->bCubeNs );
    Cudd_RecursiveDeref( pTR->dd, pTR->bCubeIn );
    FREE( pTR->pbStatesCs );
    FREE( pTR->pbStatesNs );
    FREE( pTR );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Au_Auto_t * Au_AutoRelCreateAuto( Au_Rel_t * pTR )
{
    Au_Auto_t * pAut;
    Au_Trans_t * pTrans;
    Mvr_Relation_t * pMvrTemp;
    DdNode * bCofCs, * bCofNs;
    DdManager * dd = pTR->dd;
    int s, n;

    // get the temporary relation
    pMvrTemp = Mvr_RelationCreate( Fnc_ManagerReadManMvr(pTR->pAut->pMan), pTR->pVmx, b1 );

    // start the automaton
    pAut = Au_AutoClone( pTR->pAut );
    pAut->nStatesAlloc = pTR->pAut->nStates;
    pAut->nStates = pTR->pAut->nStates;
    pAut->pStates = ALLOC( Au_State_t *, pAut->nStatesAlloc );

    // create state and transitions
    for ( s = 0; s < pAut->nStates; s++ )
    {
        pAut->pStates[s] = Au_AutoStateAlloc();
        pAut->pStates[s]->pName = util_strsav( pTR->pAut->pStates[s]->pName );
        bCofCs = Cudd_bddAndAbstract( dd, pTR->bRel, pTR->pbStatesCs[s], pTR->bCubeCs ); Cudd_Ref( bCofCs );
        for ( n = 0; n < pAut->nStates; n++ )
        {
            bCofNs = Cudd_bddAndAbstract( dd, bCofCs, pTR->pbStatesNs[n], pTR->bCubeNs ); Cudd_Ref( bCofNs );
            if ( bCofNs == b0 )
            {
                Cudd_RecursiveDeref( dd, bCofNs );
                continue;
            }

            // create the transition
            pTrans = Au_AutoTransAlloc();
            pTrans->pCond = Fnc_FunctionDeriveSopFromMdd( Fnc_ManagerReadManMvc(pTR->pAut->pMan), 
                        pMvrTemp, bCofNs, bCofNs, pAut->nInputs );
            pTrans->StateCur  = s;
            pTrans->StateNext = n;
            // add the transition
            Au_AutoTransAdd( pAut->pStates[s], pTrans );
            Cudd_RecursiveDeref( dd, bCofNs );
        }
        Cudd_RecursiveDeref( dd, bCofCs );
    }
    Mvr_RelationFree( pMvrTemp );
    return pAut;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


