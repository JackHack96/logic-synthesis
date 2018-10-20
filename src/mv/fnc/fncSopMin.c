/**CFile****************************************************************

  FileName    [fncSopMin.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Heuristic MV SOP minimization.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: fncSopMin.c,v 1.8 2003/05/27 23:15:06 alanmi Exp $]

***********************************************************************/

#include "fncInt.h"
#include "cvrInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static DdNode * Fnc_FunctionDomainEssen( Mvr_Relation_t * pMvr, DdNode * bValueCube, DdNode * bCharCube );
static DdNode * Fnc_FunctionDomainTotal( Mvr_Relation_t * pMvr, DdNode * bValueCube, DdNode * bCharCube );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Heuristic minimization of ND MV SOP using ND MV relation.]

  Description [Returns (possibly ND) MV-SOP derived from the MV relation
  using the simple strategy: first compute the essential cubes that can be 
  covered with one i-set only; next greedily cover the remaining domains
  on-set    of the i-set:   ON(i)    = Univ  v [ R(a,v) == Vi(v) ]
  on/dc-set of the i-set:   ON/DC(i) = Exist v [ R(a,v) &  Vi(v) ]
  when all the i-sets are ready, the largest one becomes the default.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Cvr_Cover_t * Fnc_FunctionMinimizeCvr( Mvc_Manager_t * pManMvc, Mvr_Relation_t * pMvr, bool fUseIsop )
{
    DdManager * dd = pMvr->pMan->pDdLoc;
    Vm_VarMap_t * pVm = pMvr->pVmx->pVm;
    Mvc_Cover_t ** pbIsets;
    DdNode * bOn, * bOnDc, * bCharCube;
    DdNode * bFunc, * bAreaRem, * bTemp;
    DdNode ** pbCodes, ** pbCodesOut;
    int v, nOutputValues;
    int DefValue;
    int nCubesMax, nCubes;
    int MaxCoverCubeCount = -1;
    int MaxCoverValue;
    int fOneTimeout = 0;  // 1 if ISOP times out for one of the essential covers

    // reorder the relation
    Mvr_RelationReorder( pMvr );

    // the relation cannot be zero
    assert( pMvr->bRel != b0 );
    // get the number of values
    nOutputValues = Vm_VarMapReadValuesOutNum(pVm);
    
    // get the MV-PLA covers
    pbIsets = ALLOC( Mvc_Cover_t *, nOutputValues );
    memset( pbIsets, 0, sizeof(Mvc_Cover_t *) * nOutputValues );

    // the relation can be one
    // meaning that the output of this node is a don't-care
    if ( pMvr->bRel == b1 )
    { // set the node to a constant ND, it will be eliminated during sweep
        // (if this node remains in the network, make sure it is never resubstituted!!!)
        for ( v = 0; v < nOutputValues; v++ )
        {
            pbIsets[v] = Mvc_CoverAlloc( pManMvc, pVm->nValuesIn );
            Mvc_CoverMakeTautology( pbIsets[v] );
        }
        return Cvr_CoverCreate( pVm, pbIsets );
    }

    // derive the encoding cubes
    pbCodes = Vmx_VarMapEncodeMap( dd, pMvr->pVmx );
    // set the pointer to the output encoding cubes
    pbCodesOut = pbCodes + pVm->nValuesIn;
    // get the char cube w.r.t. the output value
    bCharCube = Vmx_VarMapCharCube( dd, pMvr->pVmx, pVm->nVarsIn );  Cudd_Ref( bCharCube );


    // remove the inessential values
    for ( v = 0; v < nOutputValues; v++ )
    {
        bOn = Fnc_FunctionDomainEssen( pMvr, pbCodesOut[v], bCharCube );  Cudd_Ref( bOn );
        if ( bOn == b0 )
        {
            // this value can be removed
            pMvr->bRel = Cudd_bddAnd( dd, bTemp = pMvr->bRel, Cudd_Not(pbCodesOut[v]) );  
            Cudd_Ref( pMvr->bRel );
            Cudd_RecursiveDeref( dd, bTemp );
        }
        Cudd_RecursiveDeref( dd, bOn );
    }
    assert( Mvr_RelationIsWellDefined(pMvr) );


    // prepare the Espresso computation
    if ( !fUseIsop )
       Cvr_CoverEspressoSetup(pVm);

    // start the remaining area
    bAreaRem = b1;   Cudd_Ref( bAreaRem );

    // find the cover that covers the essential part of each i-set
    for ( v = 0; v < nOutputValues; v++ )
    {
        // extract the on-set and the on/dc-set of this value
        bOn   = Fnc_FunctionDomainEssen( pMvr, pbCodesOut[v], bCharCube );  Cudd_Ref( bOn );
        bOnDc = Fnc_FunctionDomainTotal( pMvr, pbCodesOut[v], bCharCube );  Cudd_Ref( bOnDc );

#ifndef NDEBUG
        // verify the containment
        if ( !Cudd_bddLeq( dd, bOn, bOnDc ) )
            fprintf( stdout, "Fnc_FunctionMinimizeCvr(): ON-set is not contained in ON/DC-set\n" );
#endif

        // minimize the cover
        if ( fUseIsop )
            pbIsets[v] = Fnc_FunctionDeriveSopFromMddLimited( pManMvc, pMvr, bOn, bOnDc, pVm->nVarsIn );
        else
            pbIsets[v] = Fnc_FunctionDeriveSopFromMddEspresso( pManMvc, pMvr, bOn, bOnDc, pVm->nVarsIn );

        // check if timeout/cubeout has occurred
        if ( pbIsets[v] == NULL )
        {
            if ( !fOneTimeout )
            {
                fOneTimeout = 1;
                // reset the timeout for the ISOP computation to continue
//              mvfsIsopTimeoutSet( enviNode->enviNet->timeoutNode );

                // set the current value as a default value
                MaxCoverCubeCount = 1000000;
                MaxCoverValue     = v;

                // subtract from the remaning area
                bAreaRem = Cudd_bddAnd( dd, bTemp = bAreaRem, Cudd_Not(bOnDc) );  Cudd_Ref( bAreaRem );
                Cudd_RecursiveDeref( dd, bOn );
                Cudd_RecursiveDeref( dd, bOnDc );
                Cudd_RecursiveDeref( dd, bTemp );
                continue;
            }
            else
            {
                // deref the temporary BDDs
                Cudd_RecursiveDeref( dd, bOn );
                Cudd_RecursiveDeref( dd, bOnDc );
                Cudd_RecursiveDeref( dd, bAreaRem );
                Vmx_VarMapEncodeDeref( dd, pMvr->pVmx, pbCodes );
                Cudd_RecursiveDeref( dd, bCharCube );
                // undo Espresso cube
                if ( !fUseIsop )
                    Cvr_CoverEspressoSetdown(pVm);
                // free the computed covers
                for ( v = 0; v < nOutputValues; v++ )
                    if ( pbIsets[v] )
                        Mvc_CoverFree( pbIsets[v] );
                FREE( pbIsets );
                // return NULL, meaning we cannot minimize the cover with these limits
                return NULL;
            }
        }

        // find the covered part of the area
        bFunc = Fnc_FunctionDeriveMddFromSop( dd, pVm, pbIsets[v], pbCodes );  Cudd_Ref( bFunc );

#ifndef NDEBUG
        // verification
        if ( !Cudd_bddLeq( dd, bOn, bFunc ) )
            fprintf( stdout, "Fnc_FunctionMinimizeCvr(): The on-set is not contained in the solution\n" );
        if ( !Cudd_bddLeq( dd, bFunc, bOnDc ) )
            fprintf( stdout, "Fnc_FunctionMinimizeCvr(): The solution is not contained in the on-set plus dc-set\n" );
#endif

        Cudd_RecursiveDeref( dd, bOn );
        Cudd_RecursiveDeref( dd, bOnDc );

        // subtract from the remaning area
        bAreaRem = Cudd_bddAnd( dd, bTemp = bAreaRem, Cudd_Not(bFunc) );  Cudd_Ref( bAreaRem );
        Cudd_RecursiveDeref( dd, bTemp );
        Cudd_RecursiveDeref( dd, bFunc );

        // get the value with the largest number of cubes
        nCubes = Mvc_CoverReadCubeNum(pbIsets[v]);
        if ( MaxCoverCubeCount < nCubes )
        {
             MaxCoverCubeCount = nCubes;
             MaxCoverValue     = v;
        }
    }

    // check the remaning area
    if ( bAreaRem != b0 )
    {   // the remaining area is not zero; there remains something to be covered
        DdNode * bCoveringValues, * bInputVarCube;

        // get the input variable cube
        bInputVarCube = Vmx_VarMapCharCubeInput( dd, pMvr->pVmx );  
        Cudd_Ref( bInputVarCube );

        // find out is it true that the remaining part can be covered by one i-set alone
        // SuchValues = Univ a ITE( bAreaRem, R(a,v), 1 ) = Univ a (R + A') = !Exist a (R' & A)
        // if this is true, then we can add this whole area to that i-set 
        // and even if it is very large, it can later became the default set
        bCoveringValues = Cudd_bddAndAbstract( dd, Cudd_Not(pMvr->bRel), bAreaRem, bInputVarCube );
        Cudd_Ref( bCoveringValues );

        bCoveringValues = Cudd_Not( bCoveringValues );
        Cudd_RecursiveDeref( dd, bInputVarCube );

        if ( bCoveringValues != b0 )
        { // we have found such values
            //printf( "Case B\n" );
            // use as default that value which has the largest cover
            // find the cube cover with the largest size among those belonging to the set
            nCubesMax = -1;
            DefValue = -1;
            for ( v = 0; v < nOutputValues; v++ )
//              if ( !bdd_leq( bCoveringValues, enviNode->pOutputValueCubes[v], 1, 0 ) )
                if ( Cudd_bddLeq( dd, pbCodesOut[v], bCoveringValues ) )
                { // this value belongs to the set
                    nCubes = Mvc_CoverReadCubeNum(pbIsets[v]);
                    if ( nCubesMax < nCubes )
                    {
                         nCubesMax = nCubes;
                         DefValue = v;
                    }
                }
            assert( DefValue != -1 );
            // deref 
            Cudd_RecursiveDeref( dd, bCoveringValues );
            goto finish;
        }
        else
        { // there is no such values
            DdNode * bRelCur, * bOn2;
            // deref 
            Cudd_RecursiveDeref( dd, bCoveringValues );
            // get the relation in the overlapping part
            bRelCur = Cudd_bddAnd( dd, pMvr->bRel, bAreaRem ); Cudd_Ref( bRelCur );
            // take values in the natural order (later we can improve this step)
            for ( v = 0; bRelCur != b0; v++ )
            {
                assert( v < nOutputValues );
                // extract the on-set and the on/dc-set of this value
                bOn   = Fnc_FunctionDomainEssen( pMvr, pbCodesOut[v], bCharCube );  Cudd_Ref( bOn );
                bOnDc = Fnc_FunctionDomainTotal( pMvr, pbCodesOut[v], bCharCube );  Cudd_Ref( bOnDc );
                // get the additional on-set
//              bOn2  = Fnc_FunctionDomainTotal    ( enviNode, bRelCur, v );
                bTemp = pMvr->bRel;
                pMvr->bRel = bRelCur;
                bOn2  = Fnc_FunctionDomainTotal( pMvr, pbCodesOut[v], bCharCube );  Cudd_Ref( bOn2 );
                pMvr->bRel = bTemp;

                // add this on-set to the first on-set
                bOn   = Cudd_bddOr( dd, bTemp = bOn, bOn2 );  Cudd_Ref( bOn );
                Cudd_RecursiveDeref( dd, bTemp );
                Cudd_RecursiveDeref( dd, bOn2 );
#ifndef NDEBUG
                // verify
                if ( !Cudd_bddLeq( dd, bOn, bOnDc ) )
                    fprintf( stdout, "Fnc_FunctionMinimizeCvr(): ON-set is not contained in ON/DC-set\n" );
#endif

                // remove the old cover
                Mvc_CoverFree( pbIsets[v] );
                pbIsets[v] = NULL;

                // minimize the cover
                if ( fUseIsop )
                    pbIsets[v] = Fnc_FunctionDeriveSopFromMddLimited( pManMvc, pMvr, bOn, bOnDc, pVm->nVarsIn );
                else
                    pbIsets[v] = Fnc_FunctionDeriveSopFromMddEspresso( pManMvc, pMvr, bOn, bOnDc, pVm->nVarsIn );


                if ( pbIsets[v] == NULL )
                {
                    // deref the temporary BDDs
                    Cudd_RecursiveDeref( dd, bOn );
                    Cudd_RecursiveDeref( dd, bOnDc );
                    Cudd_RecursiveDeref( dd, bAreaRem );
                    Cudd_RecursiveDeref( dd, bRelCur );
                    Vmx_VarMapEncodeDeref( dd, pMvr->pVmx, pbCodes );
                    Cudd_RecursiveDeref( dd, bCharCube );
                    // undo Espresso cube
                    if ( !fUseIsop )
                        Cvr_CoverEspressoSetdown(pVm);
                    // free the computed covers
                    for ( v = 0; v < nOutputValues; v++ )
                        if ( pbIsets[v] )
                            Mvc_CoverFree( pbIsets[v] );
                    FREE( pbIsets );
                    // return NULL, meaning we cannot minimize the cover with these limits
                    return NULL;
                }

                // find the covered part of the area
                bFunc = Fnc_FunctionDeriveMddFromSop( dd, pVm, pbIsets[v], pbCodes );  Cudd_Ref( bFunc );

#ifndef NDEBUG
                // verification
                if ( !Cudd_bddLeq( dd, bOn, bFunc ) )
                    fprintf( stdout, "Fnc_FunctionMinimizeCvr(): The on-set is not contained in the solution\n" );
                if ( !Cudd_bddLeq( dd, bFunc, bOnDc ) )
                    fprintf( stdout, "Fnc_FunctionMinimizeCvr(): The solution is not contained in the on-set plus dc-set\n" );
#endif

                Cudd_RecursiveDeref( dd, bOn );
                Cudd_RecursiveDeref( dd, bOnDc );

                // subtract this component from the remaining relation
                bRelCur = Cudd_bddAnd( dd, bTemp = bRelCur, Cudd_Not(bFunc) ); Cudd_Ref( bRelCur );
                Cudd_RecursiveDeref( dd, bFunc );
                Cudd_RecursiveDeref( dd, bTemp );
            }
            Cudd_RecursiveDeref( dd, bRelCur );
            //printf( "Case C\n" );
        }
    }
    else
    {
        //printf( "Case A\n" );
    }

    if ( fOneTimeout )
    {
        DefValue = MaxCoverValue;
    }
    else
    {
        // find the cube cover with the largest size
        nCubesMax = -1;
        DefValue = -1;
        for ( v = 0; v < nOutputValues; v++ )
        {
            nCubes = Mvc_CoverReadCubeNum(pbIsets[v]);
            if ( nCubesMax < nCubes )
            {
                 nCubesMax = nCubes;
                 DefValue = v;
            }
        }
        assert( nCubesMax != -1 );
    }

finish:
    // deref area
    Cudd_RecursiveDeref( dd, bAreaRem );
    // deref the temporary BDDs
    Vmx_VarMapEncodeDeref( dd, pMvr->pVmx, pbCodes );
    Cudd_RecursiveDeref( dd, bCharCube );
    // undo Espresso structures
    if ( !fUseIsop )
        Cvr_CoverEspressoSetdown(pVm);

    // remove the def-value cover
    if ( pbIsets[DefValue] )
    {
        Mvc_CoverFree( pbIsets[DefValue] );
        pbIsets[DefValue] = NULL;
    }
    // this value will be set as the default one now 
    return Cvr_CoverCreate( pVm, pbIsets );
}



/**Function*************************************************************

  Synopsis    [Computes the essential domain of one i-set.]

  Description [Computes the on-set of one i-set:
  ON(i) = Univ v [ R(a,v)==Vi(v) ] = !Exist v [ R(a,v)xor Vi(v) ] ]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Fnc_FunctionDomainEssen( Mvr_Relation_t * pMvr, DdNode * bValueCube, DdNode * bCharCube )
{
    DdNode * bRes;
    bRes = Cudd_bddXorExistAbstract( pMvr->pMan->pDdLoc, pMvr->bRel, bValueCube, bCharCube );
    bRes = Cudd_Not( bRes );
    return bRes;
}

/**Function*************************************************************

  Synopsis    [Computes the total domain of one i-set.]

  Description [Computes the on/dc-set of one i-set:
  ON/DC(i) = Exist v [ R(a,v) & Vi(v) ]  ]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Fnc_FunctionDomainTotal( Mvr_Relation_t * pMvr, DdNode * bValueCube, DdNode * bCharCube )
{
    return Cudd_bddAndAbstract( pMvr->pMan->pDdLoc, pMvr->bRel, bValueCube, bCharCube );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


