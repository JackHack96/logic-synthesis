/**CFile****************************************************************

  FileName    [langMinim.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Implicit language solving flow.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: langMinim.c,v 1.4 2004/02/19 03:06:54 alanmi Exp $]

***********************************************************************/

#include "langInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static DdNode * Lang_ReencodeMinOne( Lang_Auto_t * pAut, Vmx_VarMap_t * pVmxNew, DdNode * bObject, DdManager * dd, DdNode * bReenc );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Implicit state minimization.]

  Description [Returns the same automaton if the state minimization 
  did not reduce the number of states or if we have run out of time.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Lang_Auto_t * Lang_AutoMinimize( Lang_Auto_t * pAut )
{
    DdManager * dd;
    DdNode * bRel1, * bRel1t, * bRel2, * bRel, * bProj1, * bProj2, * bTemp;
    DdNode * bAcc, * bAcc1, * bAcc2, * bImage, * bImageCs, * bImageNs;
    DdNode * bCubeNs, * bCubeCs, * bCubeCs2, * bCubeIO;
    DdNode ** pbVarsCs, ** pbVarsNs;
    Vmx_VarMap_t * pVmxNew;
    int iVarCS1, iVarCS2, iVarNS1, iVarNS2;
    int nIters, nBits, v;
    int * pPermute;

    if ( pAut->nStates == 0 )
    {
        printf( "Trying to determinize automaton with no states.\n" );
        return Lang_AutoDup( pAut );
    }

    // create the new manager with the same ordering of variable
    dd = Cudd_Init( pAut->dd->size, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
    Cudd_ShuffleHeap( dd, pAut->dd->invperm );

    // interleave the new CS/NS variables with the old ones
    nBits = Vmx_VarMapReadBits(pAut->pVmx)[pAut->nInputs];
    pbVarsCs = Vmx_VarMapBinVarsRange( dd, pAut->pVmx, pAut->nInputs + 0, 1 );
    pbVarsNs = Vmx_VarMapBinVarsRange( dd, pAut->pVmx, pAut->nInputs + 1, 1 );
    for ( v = 0; v < nBits; v++ )
    {
        Cudd_bddNewVarAtLevel( dd, pAut->dd->perm[pbVarsCs[v]->index] + 1 );
        Cudd_bddNewVarAtLevel( dd, pAut->dd->perm[pbVarsNs[v]->index] + 1 );
    }
    FREE( pbVarsCs );
    FREE( pbVarsNs );

    // remember this permutation to be able to reconstruct it later
    pPermute = ALLOC( int, dd->size );
    for ( v = 0; v < dd->size; v++ )
        pPermute[v] = dd->invperm[v];

    // create the new extended variable map
    pVmxNew = Lang_VarMapDupLatch( pAut );
    iVarCS1 = pAut->nInputs + 0;  // PROBLEMS!!!
    iVarCS2 = pAut->nInputs + 1;
    iVarNS1 = pAut->nInputs + 2;
    iVarNS2 = pAut->nInputs + 3;

    // set up the internal variable replacement map (CS1,CS2)->(NS1,NS2)
    Vmx_VarMapRemapRangeSetup( dd, pVmxNew, iVarCS1, 2, iVarNS1, 2 ); 

    // get the cube of all next state vars
    bCubeNs = Vmx_VarMapCharCubeRange( dd, pVmxNew, iVarNS1, 2 );       Cudd_Ref( bCubeNs );

    // set up the variable replacement map (CS1,NS1)->(CS2,NS2)
    // transfer the initial partition Image(NS1,NS2)
    bAcc  = Cudd_bddTransfer( pAut->dd, dd, pAut->bAccepting );            Cudd_Ref( bAcc );
    bAcc1 = Vmx_VarMapRemapVar( dd, bAcc, pVmxNew, iVarCS1, iVarNS1 );        Cudd_Ref( bAcc1 );
    bAcc2 = Vmx_VarMapRemapVar( dd, bAcc, pVmxNew, iVarCS1, iVarNS2 );        Cudd_Ref( bAcc2 );
    Cudd_RecursiveDeref( dd, bAcc );
    bImage = Cudd_bddXnor( dd, bAcc1, bAcc2 );                             Cudd_Ref( bImage );
    Cudd_RecursiveDeref( dd, bAcc1 );
    Cudd_RecursiveDeref( dd, bAcc2 );

    // transfer the relations Rel1(CS1,NS1,io) and Rel2(CS2,NS2,io)
    bRel1 = Cudd_bddTransfer( pAut->dd, dd, pAut->bRel );                  Cudd_Ref( bRel1 );
    bRel1t= Vmx_VarMapRemapVar( dd, bRel1, pVmxNew, iVarCS1, iVarCS2 );       Cudd_Ref( bRel1t );
    bRel2 = Vmx_VarMapRemapVar( dd, bRel1t, pVmxNew, iVarNS1, iVarNS2 );      Cudd_Ref( bRel2 );
    bCubeIO = Vmx_VarMapCharCubeRange( dd, pVmxNew, 0, pAut->nInputs ); Cudd_Ref( bCubeIO );
    Cudd_RecursiveDeref( dd, bRel1t );

    // transfer the relation Rel(CS1,CS2,NS1,NS2)
    Cudd_AutodynEnable( dd, CUDD_REORDER_SYMM_SIFT );
    bRel  = Cudd_bddAndAbstract( dd, bRel1, bRel2, bCubeIO );              Cudd_Ref( bRel );
    Cudd_RecursiveDeref( dd, bRel1 );
    Cudd_RecursiveDeref( dd, bRel2 );
    Cudd_RecursiveDeref( dd, bCubeIO );
    Cudd_ReduceHeap( dd, CUDD_REORDER_SYMM_SIFT, 100 );
    Cudd_AutodynDisable( dd );

    // perform reachability computation
    nIters = 0;
    while ( 1 )
    {
        nIters++;
        // compute the partition after refinement
        // Image(CS1,CS2) = !Exist NS1,NS2 [Rel(CS1,CS2,NS1,NS2) & !Image(NS1,NS2)]
        bImageCs = Cudd_bddAndAbstract( dd, bRel, Cudd_Not(bImage), bCubeNs );    Cudd_Ref( bImageCs );
        bImageCs = Cudd_Not(bImageCs);
        // transfer to the NS variables
        bImageNs = Cudd_bddVarMap( dd, bImageCs );                                Cudd_Ref( bImageNs );
        // compare with the previous image
        if ( bImageNs == bImage ) // there is no refinement
        {
            Cudd_RecursiveDeref( dd, bImageNs );
            Cudd_RecursiveDeref( dd, bImage );
            break;
        }
        Cudd_RecursiveDeref( dd, bImageCs );
        // replace bImage by bImageNs
        Cudd_RecursiveDeref( dd, bImage ); 
        bImage = bImageNs;   // takes ref
    }
    fprintf( stdout, "State minimization completed in %d iterations.\n", nIters );
    // check for the case when the refinement is finished after the first iteration
    if ( nIters == 1 )
    {
        Cudd_Quit( dd );
        FREE( pPermute );
        return pAut;
    }
    // deref everything
    Cudd_RecursiveDeref( dd, bRel );
    Cudd_RecursiveDeref( dd, bCubeNs );
    // Cudd_RecursiveDeref( dd, bImageCs );
    // only bImageCs is refed at this point

    // get the compatible projection of the final partition
    bCubeCs2 = Vmx_VarMapCharCube( dd, pVmxNew, iVarCS2 );             Cudd_Ref( bCubeCs2 );
    bProj1  = Cudd_CProjection( dd, bImageCs, bCubeCs2 );                 Cudd_Ref( bProj1 );
    Cudd_RecursiveDeref( dd, bImageCs );
    // only bProj1 is refed at this point

    // reconstruct the original permutation
    Cudd_ShuffleHeap( dd, pPermute );

    // remove the reachable states if present
    if ( pAut->bStates )
        Cudd_RecursiveDeref( pAut->dd, pAut->bStates );
    pAut->bStates  = NULL;

    // reencode the initial state
    pAut->bInit = Lang_ReencodeMinOne( pAut, pVmxNew, bTemp = pAut->bInit, dd, bProj1 );  Cudd_Ref( pAut->bInit );
    Cudd_RecursiveDeref( pAut->dd, bTemp );

    // reencode the reachable states
    pAut->bStates = Lang_ReencodeMinOne( pAut, pVmxNew, bTemp = pAut->bStates, dd, bProj1 );  Cudd_Ref( pAut->bStates );
    Cudd_RecursiveDeref( pAut->dd, bTemp );
    pAut->nStates = (int)Cudd_CountMinterm( dd, pAut->bStates, pAut->nStateBits );

    // reencode the accepting states
    pAut->bAccepting = Lang_ReencodeMinOne( pAut, pVmxNew, bTemp = pAut->bAccepting, dd, bProj1 );  Cudd_Ref( pAut->bAccepting );
    Cudd_RecursiveDeref( pAut->dd, bTemp );
    pAut->nAccepting = (int)Cudd_CountMinterm( dd, pAut->bAccepting, pAut->nStateBits );


    // transfer the relation again
    bRel = Cudd_bddTransfer( pAut->dd, dd, pAut->bRel );                          Cudd_Ref( bRel );
    // transfer the projection to the NS variables
    bProj2 = Cudd_bddVarMap( dd, bProj1 );                                        Cudd_Ref( bProj2 );

    // enable the reordering
    Cudd_AutodynEnable( dd, CUDD_REORDER_SYMM_SIFT );
    // reduce the CS vars 
    bCubeCs = Vmx_VarMapCharCube( dd, pVmxNew, iVarCS1 );                      Cudd_Ref( bCubeCs );
    bRel = Cudd_bddAndAbstract( dd, bTemp = bRel, bProj1, bCubeCs );              Cudd_Ref( bRel );
    Cudd_RecursiveDeref( dd, bTemp );
    Cudd_RecursiveDeref( dd, bProj1 );
    Cudd_RecursiveDeref( dd, bCubeCs );
    Cudd_RecursiveDeref( dd, bCubeCs2 );
    // reduce the NS vars
    bCubeNs = Vmx_VarMapCharCube( dd, pVmxNew, iVarNS1 );                      Cudd_Ref( bCubeNs );
    bRel = Cudd_bddAndAbstract( dd, bTemp = bRel, bProj2, bCubeNs );              Cudd_Ref( bRel );
    Cudd_RecursiveDeref( dd, bTemp );
    Cudd_RecursiveDeref( dd, bProj2 );
    Cudd_RecursiveDeref( dd, bCubeNs );
    // remap the relation back into CS/NS vars
//    bRel = Cudd_bddSwapVariables( dd, bTemp = bRel, pbVars, pbVarsNew, nBits );   Cudd_Ref( bRel );
//    Cudd_RecursiveDeref( dd, bTemp );
    bRel1t= Vmx_VarMapRemapVar( dd, bRel, pVmxNew, iVarCS1, iVarCS2 );               Cudd_Ref( bRel1t );
    Cudd_RecursiveDeref( dd, bRel );
    bRel = Vmx_VarMapRemapVar( dd, bRel1t, pVmxNew, iVarNS1, iVarNS2 );              Cudd_Ref( bRel );
    Cudd_RecursiveDeref( dd, bRel1t );

    // transfer it back to the original manager
    Cudd_RecursiveDeref( pAut->dd, pAut->bRel );
    pAut->bRel = Cudd_bddTransfer( dd, pAut->dd, bRel );                          Cudd_Ref( pAut->bRel );
    Cudd_RecursiveDeref( dd, bRel );

    // reencode the resulting relation
//    if ( !Lang_Reencode( pAut, NULL ) )
//        printf( "Reencoding is not necessary\n" );

    // get rid of the old manager
    Extra_StopManager( dd );
    FREE( pPermute );

    // reorder the original manager
//    Cudd_ReduceHeap( dd, CUDD_REORDER_SYMM_SIFT, 100 );
    return pAut;
}

/**Function*************************************************************

  Synopsis    [Reencodes one object.]

  Description [Reencodes CS vars only. The object is given and returned 
  in the original manager.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Lang_ReencodeMinOne( Lang_Auto_t * pAut, Vmx_VarMap_t * pVmxNew, DdNode * bObject, DdManager * dd, DdNode * bReenc )
{
    DdNode * bTemp, * bRes;
    DdNode * bCubeCs1;
    // tranfer into the new manager
    bRes = Cudd_bddTransfer( pAut->dd, dd, bObject );                      Cudd_Ref( bRes );
    // reencode the object (CS1 -> CS2)
    bCubeCs1 = Vmx_VarMapCharCubeRange( dd, pVmxNew, pAut->nInputs, pAut->nLatches ); Cudd_Ref( bCubeCs1 );
    bRes = Cudd_bddAndAbstract( dd, bTemp = bRes, bReenc, bCubeCs1 );      Cudd_Ref( bRes );
    Cudd_RecursiveDeref( dd, bTemp );
    Cudd_RecursiveDeref( dd, bCubeCs1 );
    // return to the previous variables (CS2 -> CS1)
    bRes = Vmx_VarMapRemapRange( dd, bTemp = bRes, pVmxNew, 
        pAut->nInputs, pAut->nLatches, pAut->nInputs + pAut->nLatches, pAut->nLatches ); Cudd_Ref( bRes );
    Cudd_RecursiveDeref( dd, bTemp );
    // transfer back into the old manager
    bRes = Cudd_bddTransfer( dd, pAut->dd, bTemp = bRes );                 Cudd_Ref( bRes );
    Cudd_RecursiveDeref( dd, bTemp );
    Cudd_Deref( bRes );
    return bRes;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


