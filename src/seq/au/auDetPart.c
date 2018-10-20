/**CFile****************************************************************

  FileName    [auDeterm.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Automata determinization procedure.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: auDetPart.c,v 1.1 2004/02/19 03:06:45 alanmi Exp $]

***********************************************************************/

#include "auInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct AuDetermStruct     Au_Determ_t;     // the transition relation 

struct AuDetermStruct
{
    Au_Auto_t *     pAut;            // the parent automaton
    // functionality
    DdManager *      dd;              // the manager
    // variable maps
    Vm_VarMap_t *    pVm;             // the variable map
    Vmx_VarMap_t *   pVmx;            // the extended variable map

    // quantification cubes
    DdNode *         bCubeCs;         // the cube of current state variables
    DdNode *         bCubeIn;         // the cube of input variables
    // state codes
    int              nStates;         // the number of states
    DdNode **        pbStatesCs;      // the codes of the current states
    DdNode **        pbParts;         // partitions of the transition relation
};


static Au_Determ_t * Au_AutoPrecomputePartitions( Au_Auto_t * pAut );
static void Au_AutoDetermFree( Au_Determ_t * pDet );

static DdNode * Au_TestOr( DdManager * dd, DdNode * A, DdNode * B );



////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Another determinization procedure.]

  Description [This procedure differs from the above in that it does not
  build the transition relation. It simply considers the transition relations
  for each state.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Au_Auto_t * Au_AutoDeterminizePart( Au_Auto_t * pAut, int fAllAccepting, int fLongNames )
{
    ProgressBar * pProgress;
    Au_Auto_t * pAutD;
    Au_Determ_t * pDet;
    Mvr_Relation_t * pMvrTemp;
    st_table * tSubset2Num;
    Au_State_t * pS, * pState;
    Au_Trans_t * pTrans;
    DdManager * dd;
    DdNode * bTrans, * bMint, * bSubset, * bCond, * bTemp;
    DdNode * bAccepting;
    int * pEntry, s, i, iState;
    int nDigits;
    char Buffer[100];
    char * pNamePrev;
    int Counter;

    if ( pAut->nStates == 0 )
    {
        printf( "Trying to determinize the automaton with no states.\n" );
        return pAut;
    }

    if ( pAut->nStates == 1 )
        return pAut;

    if ( Au_AutoCheckNd( stdout, pAut, pAut->nInputs, 0 ) == 0 )
    {
        printf( "The automaton is deterministic; determinization is not performed.\n" );
        return pAut;
    }

    // get the partitioned transition relation
    pDet = Au_AutoPrecomputePartitions(pAut);
    dd = pDet->dd;

    // get the temporary MVR
    pMvrTemp = Mvr_RelationCreate( Fnc_ManagerReadManMvr(pAut->pMan), pDet->pVmx, b1 );

    // start the new automaton
    pAutD = Au_AutoClone( pAut );
    pAutD->nStates      = 0;
    pAutD->nStatesAlloc = 1000;
    pAutD->pStates      = ALLOC( Au_State_t *, pAutD->nStatesAlloc );

    // create the initial state
    pState = Au_AutoStateAlloc();
    pState->bSubset = pDet->pbStatesCs[0];   Cudd_Ref( pDet->pbStatesCs[0] );  
    pAutD->pStates[ pAutD->nStates++ ] = pState;

    // start the table to collect the reachable states
    tSubset2Num = st_init_table(st_ptrcmp,st_ptrhash);
    st_insert( tSubset2Num, (char *)pDet->pbStatesCs[0], (char *)0 );

    pProgress = Extra_ProgressBarStart( stdout, 1000 );

    // iteratively process the subsets
    for ( s = 0; s < pAutD->nStates; s++ )
    {
        // get the subset corresponding to this state of the ND automaton
        bSubset = pAutD->pStates[s]->bSubset;    Cudd_Ref( bSubset );
//PRB( dd, bSubset );
        Counter = 0;
        // get the transitions for this subset
        bTrans = b0;  Cudd_Ref( bTrans );
        while ( bSubset != b0 )
        {
            // get the state cube, which may contain many states
            bMint = Extra_bddGetOneCube( dd, bSubset );  Cudd_Ref( bMint );
            // get the number of one state contained in this cube
            iState = Vmx_VarMapDecodeCubeVar( dd, bMint, pDet->pVmx, pAut->nInputs );
            assert( !Cudd_bddLeq( dd, bMint, Cudd_Not(pDet->pbStatesCs[iState]) ) );
            Cudd_RecursiveDeref( dd, bMint );
            // add the partition of this state to the transitions
            bTrans = Cudd_bddOr( dd, bTemp = bTrans, pDet->pbParts[iState] );  Cudd_Ref( bTrans );
//            bTrans = Au_TestOr( dd, bTemp = bTrans, pDet->pbParts[iState] );  Cudd_Ref( bTrans );
            Cudd_RecursiveDeref( dd, bTemp );
            // reduce the set of subsets
            bSubset = Cudd_bddAnd( dd, bTemp = bSubset, Cudd_Not(pDet->pbStatesCs[iState]) );  Cudd_Ref( bSubset );
            Cudd_RecursiveDeref( dd, bTemp );
            Counter++;
        }
        Cudd_RecursiveDeref( dd, bSubset );
//PRB( dd, bTrans );
//        printf( " %d (%d)", Counter, Cudd_DagSize(bTrans) );

        // extract the subsets reachable by certain inputs
        while ( bTrans != b0 )
        {
            // get the minterm in terms of the input vars
            bMint = Extra_bddGetOneCube( dd, bTrans );                         Cudd_Ref( bMint );
            bMint = Cudd_bddExistAbstract( dd, bTemp = bMint, pDet->bCubeCs );  Cudd_Ref( bMint );
            Cudd_RecursiveDeref( dd, bTemp );
            // get the state subset
            bSubset = Cudd_bddAndAbstract( dd, bTrans, bMint, pDet->bCubeIn );  Cudd_Ref( bSubset ); // NS vars
            Cudd_RecursiveDeref( dd, bMint );
            // check if this subset was already visited
            if ( !st_find_or_add( tSubset2Num, (char *)bSubset, (char ***)&pEntry ) )
            { // does not exist - add it to storage

                // realloc storage for states if necessary
                if ( pAutD->nStatesAlloc <= pAutD->nStates )
                {
                    pAutD->pStates  = REALLOC( Au_State_t *, pAutD->pStates,  2 * pAutD->nStatesAlloc );
                    pAutD->nStatesAlloc *= 2;
                }

                pState = Au_AutoStateAlloc();
                pState->bSubset = bSubset;   Cudd_Ref( bSubset );
                pAutD->pStates[pAutD->nStates] = pState;
                *pEntry = pAutD->nStates;
                pAutD->nStates++;
            }
            // get the condition
            bCond = Cudd_bddXorExistAbstract( dd, bTrans, bSubset, pDet->bCubeCs ); Cudd_Ref( bCond );
            bCond = Cudd_Not( bCond );
            Cudd_RecursiveDeref( dd, bSubset );

            // add the transition
            pTrans = Au_AutoTransAlloc();
            pTrans->pCond = Fnc_FunctionDeriveSopFromMdd( Fnc_ManagerReadManMvc(pAut->pMan), 
                pMvrTemp, bCond, bCond, pAut->nInputs );
            pTrans->StateCur  = s;
            pTrans->StateNext = *pEntry;
            // add the transition
            Au_AutoTransAdd( pAutD->pStates[s], pTrans );

            // reduce the transitions
            bTrans = Cudd_bddAnd( dd, bTemp = bTrans, Cudd_Not( bCond ) ); Cudd_Ref( bTrans );
            Cudd_RecursiveDeref( dd, bTemp );
            Cudd_RecursiveDeref( dd, bCond );
        }
        Cudd_RecursiveDeref( dd, bTrans );

        if ( s > 50 && (s % 20 == 0) )
        {
            char Buffer[100];
            sprintf( Buffer, "%d states", s );
            Extra_ProgressBarUpdate( pProgress, 1000 * s / pAutD->nStates, Buffer );
        }
    }
    Extra_ProgressBarStop( pProgress );
 
    // get the number of digits in the state number
	for ( nDigits = 0, i = pAutD->nStates - 1;  i;  i /= 10,  nDigits++ );

    // generate the state names and acceptable conditions
    for ( s = 0; s < pAutD->nStates; s++ )
    {
        // get the state
        pS = pAutD->pStates[s];
        // set the subset
        bSubset = pS->bSubset;
        if ( fLongNames )
        { // print state names as subsets of the original state names
            int fFirst = 1;
            // alloc the name
            pS->pName = ALLOC( char, 2 );
            pS->pName[0] = 0;
            for ( i = 0; i < pAut->nStates; i++ )
            {
                if ( Cudd_bddLeq( dd, pDet->pbStatesCs[i], bSubset ) )
                {
                    if ( fFirst )
                        fFirst = 0;
                    else
                        strcat( pS->pName, "_" );

                    pNamePrev = pS->pName;
                    pS->pName = ALLOC( char, strlen(pNamePrev) + strlen(pAut->pStates[i]->pName) + 3 );
                    strcpy( pS->pName, pNamePrev );
                    strcat( pS->pName, pAut->pStates[i]->pName );
                    FREE( pNamePrev );
                }
            }
        }
        else
        { // print simple state names
            sprintf( Buffer, "s%0*d", nDigits, s );
            pS->pName = util_strsav( Buffer );
        }
    }

    // set the accepting attributes of states
    bAccepting = Au_AutoStateSumAccepting( pAut, dd, pDet->pbStatesCs ); Cudd_Ref( bAccepting );
    for ( s = 0; s < pAutD->nStates; s++ )
    {
        // get the state
        pS = pAutD->pStates[s];
        // check whether this state is accepting
//        pS->fAcc = !Cudd_bddLeq( dd, pS->bSubset, Cudd_Not(bAccepting) );
        // check whether this state is accepting
        if ( fAllAccepting ) // requires all states to be accepting
            pS->fAcc = Cudd_bddLeq( dd, pS->bSubset, bAccepting );
        else // requires at least one state to be accepting
            pS->fAcc = !Cudd_bddLeq( dd, pS->bSubset, Cudd_Not(bAccepting) );
        // deref the subset
        Cudd_RecursiveDeref( dd, pS->bSubset );
    }
    Cudd_RecursiveDeref( dd, bAccepting );

    // free the table
    st_free_table( tSubset2Num );
    Mvr_RelationFree( pMvrTemp );
    Au_AutoDetermFree( pDet );

    printf( "Determinization:  (%d states, %d trans)  ->  (%d states, %d trans)\n", 
        pAut->nStates,  Au_AutoCountTransitions( pAut ),
        pAutD->nStates, Au_AutoCountTransitions( pAutD ) );
    return pAutD;

}


/**Function*************************************************************

  Synopsis    [Precomputes the partitions of the TR.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Au_Determ_t * Au_AutoPrecomputePartitions( Au_Auto_t * pAut )
{
    Au_Determ_t * pDet;
    Au_Trans_t * pTrans;
    Vm_VarMap_t * pVmTemp;
    DdNode ** pbCodesTemp;
    DdNode * bProd, * bTemp, * bPart;
    DdManager * dd;
    int * pValuesFirst;
    int i;

    // get the temporary relation
    pVmTemp  = Vm_VarMapCreateBinary( Fnc_ManagerReadManVm(pAut->pMan), pAut->nInputs, 1 );
    
    // start the structure
    pDet = ALLOC( Au_Determ_t, 1 );
    memset( pDet, 0, sizeof(Au_Determ_t) );
    pDet->pAut = pAut;

    // create the variable map
    pDet->pVmx   = Au_UtilsCreateVmx( pAut->pMan, pAut->nInputs, pAut->nStates, 0 );
    pDet->pVm    = Vmx_VarMapReadVm(pDet->pVmx);
    pValuesFirst = Vm_VarMapReadValuesFirstArray(pDet->pVm);

    // get the manager
    pDet->dd = dd = Mvr_ManagerReadDdLoc( Fnc_ManagerReadManMvr(pAut->pMan) );

    // get the cubes
    pDet->bCubeCs = Vmx_VarMapCharCubeOutput( dd, pDet->pVmx );  Cudd_Ref( pDet->bCubeCs );
    pDet->bCubeIn = Vmx_VarMapCharCubeInput( dd, pDet->pVmx );   Cudd_Ref( pDet->bCubeIn );


    // get the codes
    pbCodesTemp      = Vmx_VarMapEncodeMap( dd, pDet->pVmx );
    pDet->nStates    = pAut->nStates;
    pDet->pbStatesCs = ALLOC( DdNode *, pDet->nStates );
    pDet->pbParts    = ALLOC( DdNode *, pDet->nStates );
    for ( i = 0; i < pDet->nStates; i++ )
    {
        pDet->pbStatesCs[i] = pbCodesTemp[pValuesFirst[pAut->nInputs] + i];  
        Cudd_Ref( pDet->pbStatesCs[i] );
    }

    // derive the transition relation
    for ( i = 0; i < pAut->nStates; i++ )
    {
        bPart = b0;  Cudd_Ref( bPart );
        Au_StateForEachTransition( pAut->pStates[i], pTrans )
        {
            bProd = Fnc_FunctionDeriveMddFromSop( dd, pVmTemp, pTrans->pCond, pbCodesTemp );  Cudd_Ref( bProd );
//PRB( dd, bProd );
            bProd = Cudd_bddAnd( dd, bTemp = bProd, pDet->pbStatesCs[pTrans->StateNext] );    Cudd_Ref( bProd );
            Cudd_RecursiveDeref( dd, bTemp );
//PRB( dd, bProd );
            // add this product
            bPart = Cudd_bddOr( dd, bTemp = bPart, bProd );             Cudd_Ref( bPart );
            Cudd_RecursiveDeref( dd, bTemp );
            Cudd_RecursiveDeref( dd, bProd );
        }
        pDet->pbParts[i] = bPart;  // takes ref
    }

    Vmx_VarMapEncodeDeref( dd, pDet->pVmx, pbCodesTemp );
    return pDet;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Au_AutoDetermFree( Au_Determ_t * pDet )
{
    int i;
    for ( i = 0; i < pDet->nStates; i++ )
    {
        Cudd_RecursiveDeref( pDet->dd, pDet->pbStatesCs[i] );
        Cudd_RecursiveDeref( pDet->dd, pDet->pbParts[i] );
    }
    Cudd_RecursiveDeref( pDet->dd, pDet->bCubeCs );
    Cudd_RecursiveDeref( pDet->dd, pDet->bCubeIn );
    FREE( pDet->pbStatesCs );
    FREE( pDet->pbParts );
    FREE( pDet );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Au_TestOr( DdManager * dd, DdNode * A, DdNode * B )
{
    return Cudd_bddOr( dd, A, B );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


