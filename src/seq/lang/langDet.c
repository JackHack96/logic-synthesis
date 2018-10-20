/**CFile****************************************************************

  FileName    [langDet.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Implicit language solving flow.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: langDet.c,v 1.5 2004/02/19 03:06:54 alanmi Exp $]

***********************************************************************/

#include "langInt.h"
#include "auInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void Lang_AutoDeterminizeLongNames( Lang_Auto_t * pLang, Au_Auto_t * pAutD );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Determinizes the automaton.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Lang_Auto_t * Lang_AutoDeterminize( Lang_Auto_t * pLang, int fAllAccepting, int fLongNames )
{
    ProgressBar * pProgress;
    Lang_Auto_t * pLangNew;
    Au_Auto_t * pAutD;
    st_table * tSubset2Num;
    Au_State_t * pS, * pState;
    Au_Trans_t * pTrans;
    int * pPermute, * pBitsOrder;
    DdManager * dd, * ddNew;
    DdNode * bTrans, * bMint, * bSubset, * bSubsetCs, * bCond, * bTemp;
    DdNode * bCubeCs, * bCubeNs, * bCubeIO;
    int * pEntry, s, i, v, nBitsIO;
    int clk, clkTotal, clkPart;

    if ( pLang->nStates == 0 )
    {
        printf( "Trying to determinize the automaton with no states.\n" );
        return Lang_AutoDup( pLang );
    }

    if ( pLang->nStates == 1 )
        return Lang_AutoDup( pLang );

    if ( Lang_AutoCheckNd( stdout, pLang, pLang->nInputs, 0 ) == 0 )
    {
        printf( "The automaton is deterministic; determinization is not performed.\n" );
        return Lang_AutoDup( pLang );
    }

    // start a new manager to be used for the determinized automaton
    // so far this manager will contain only IO variables for the remapped conditions
    nBitsIO = Vmx_VarMapBitsNumRange( pLang->pVmx, 0, pLang->nInputs );
    ddNew = Cudd_Init( nBitsIO, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );

    // get the permutation map, which remaps the IOs of pLang
    // in their natural order into the first variables of ddNew
    pPermute = ALLOC( int, pLang->dd->size );
    for ( v = 0; v < pLang->dd->size; v++ )
        pPermute[v] = -1;
    pBitsOrder = Vmx_VarMapReadBitsOrder( pLang->pVmx );
    for ( v = 0; v < nBitsIO; v++ )
        pPermute[ pBitsOrder[v] ] = v;


    // prepare the manager to remap CS into NS vars and vice versa
    dd = pLang->dd;
    Vmx_VarMapRemapRangeSetup( dd, pLang->pVmx, 
        pLang->nInputs, pLang->nLatches, pLang->nInputs + pLang->nLatches, pLang->nLatches );

    // set up the quantification cubes
    bCubeIO = Lang_AutoCharCubeIO( dd, pLang );     Cudd_Ref( bCubeIO );
    bCubeCs = Lang_AutoCharCubeCs( dd, pLang );     Cudd_Ref( bCubeCs );
    bCubeNs = Lang_AutoCharCubeNs( dd, pLang );     Cudd_Ref( bCubeNs );

    // start the new automaton
    pAutD = Au_AutoAlloc();
    pAutD->pName        = util_strsav( "det" );
    pAutD->pMan         = pLang->pMan;
    pAutD->nInputs      = nBitsIO;
    pAutD->nStates      = 0;
    pAutD->nStatesAlloc = 1000;
    pAutD->pStates      = ALLOC( Au_State_t *, pAutD->nStatesAlloc );

    // create the initial state
    pState = Au_AutoStateAlloc();
    pState->bSubset = pLang->bInit;           Cudd_Ref( pState->bSubset ); 
    pAutD->pStates[ pAutD->nStates++ ] = pState;

    // start the table to collect the reachable states
    tSubset2Num = st_init_table(st_ptrcmp,st_ptrhash);
    st_insert( tSubset2Num, (char *)pState->bSubset, (char *)0 );

    clkTotal = clock();
    clkPart  = 0;

    // iteratively process the subsets
    pProgress = Extra_ProgressBarStart( stdout, 1000 );
    for ( s = 0; s < pAutD->nStates; s++ )
    {
        // get the state
        pS = pAutD->pStates[s];
        // get the transitions for this subset in term of NS vars
        bTrans = Cudd_bddAndAbstract( dd, pLang->bRel, pS->bSubset, bCubeCs ); Cudd_Ref( bTrans );
        // extract the subsets reachable by certain inputs
clk = clock();
        while ( bTrans != b0 )
        {
            // get a minterm in terms of the input vars
            bMint = Extra_bddGetOneMinterm( dd, bTrans, bCubeIO );       Cudd_Ref( bMint );
            // get the state subset
            bSubset = Cudd_bddAndAbstract( dd, bTrans, bMint, bCubeIO ); Cudd_Ref( bSubset ); // NS vars
            Cudd_RecursiveDeref( dd, bMint );
            bSubsetCs = Cudd_bddVarMap( dd, bSubset );                   Cudd_Ref( bSubsetCs ); // CS vars
            // check if this subset was already visited
            if ( !st_find_or_add( tSubset2Num, (char *)bSubsetCs, (char ***)&pEntry ) )
            { // does not exist - add it to storage

                // realloc storage for states if necessary
                if ( pAutD->nStatesAlloc <= pAutD->nStates )
                {
                    pAutD->pStates  = REALLOC( Au_State_t *, pAutD->pStates,  2 * pAutD->nStatesAlloc );
                    pAutD->nStatesAlloc *= 2;
                }

                pState = Au_AutoStateAlloc();
                pState->bSubset = bSubsetCs;   Cudd_Ref( bSubsetCs );
                pAutD->pStates[pAutD->nStates] = pState;
                *pEntry = pAutD->nStates;
                pAutD->nStates++;
            }
//            bCond = Cudd_bddAndAbstract( dd, bTrans, bSubset, pTR->bCubeNs ); Cudd_Ref( bCond );
            // existential quantification cannot be used here

            // get the condition
            bCond = Cudd_bddXorExistAbstract( dd, bTrans, bSubset, bCubeNs ); Cudd_Ref( bCond );
            bCond = Cudd_Not( bCond );
            Cudd_RecursiveDeref( dd, bSubset );
            Cudd_RecursiveDeref( dd, bSubsetCs );

            // add the transition
            pTrans = Au_AutoTransAlloc();
//            pTrans->pCond = Fnc_FunctionDeriveSopFromMddSpecial( Fnc_ManagerReadManMvc(pLang->pMan), 
//                dd, bCond, bCond, pTR->pVmx, pLang->nInputs );
            pTrans->bCond = bCond;   Cudd_Ref( bCond );
            pTrans->StateCur  = s;
            pTrans->StateNext = *pEntry;

            // remap the condition into the new manager
            pTrans->bCond = Extra_TransferPermute( dd, ddNew, bTemp = pTrans->bCond, pPermute ); Cudd_Ref( pTrans->bCond );
            Cudd_RecursiveDeref( dd, bTemp );

            // add the transition
            Au_AutoTransAdd( pS, pTrans );

            // reduce the transitions
            bTrans = Cudd_bddAnd( dd, bTemp = bTrans, Cudd_Not( bCond ) ); Cudd_Ref( bTrans );
            Cudd_RecursiveDeref( dd, bTemp );
            Cudd_RecursiveDeref( dd, bCond );
        }
        Cudd_RecursiveDeref( dd, bTrans );
clkPart += clock() - clk;

        if ( s > 30 && (s % 20 == 0) )
        {
            char Buffer[100];
            sprintf( Buffer, "%d states", s );
            Extra_ProgressBarUpdate( pProgress, 1000 * s / pAutD->nStates, Buffer );
        }
    }
    Extra_ProgressBarStop( pProgress );
    st_free_table( tSubset2Num );
    FREE( pPermute );

    Cudd_RecursiveDeref( dd, bCubeCs );
    Cudd_RecursiveDeref( dd, bCubeNs );
    Cudd_RecursiveDeref( dd, bCubeIO );

//PRT( "Total", clock() - clkTotal );
//PRT( "Part",  clkPart );

    if ( fLongNames )
    {
        if ( pLang->tCode2Name == NULL )
        {
            printf( "Warning: Cannot create long product names of the determinized\n" );
            printf( "automaton because the input automaton has no long names.\n" );
        }
        else
            Lang_AutoDeterminizeLongNames( pLang, pAutD );
    }

    // set the accepting attributes of states
    for ( s = 0; s < pAutD->nStates; s++ )
    {
        // get the state
        pS = pAutD->pStates[s];
        // check whether this state is accepting
        if ( fAllAccepting ) // requires all states to be accepting
            pS->fAcc = Cudd_bddLeq( dd, pS->bSubset, pLang->bAccepting );
        else // requires at least one state to be accepting
            pS->fAcc = !Cudd_bddLeq( dd, pS->bSubset, Cudd_Not(pLang->bAccepting) );
        // deref the subset
        Cudd_RecursiveDeref( dd, pS->bSubset );
        pS->bSubset = NULL;
    }

    // convert the determized automaton into the implicit automaton
    // extra bits for CS/NS vars will be added to ddNew
    pLangNew = Lang_AutomatonConvert( pAutD, ddNew );
    Au_AutoFree( pAutD );

    // assign the input names
    pLangNew->nInputs = pLang->nInputs;
    pLangNew->ppNamesInput = ALLOC( char *, pLang->nInputs );
    for ( i = 0; i < pLang->nInputs; i++ )
        pLangNew->ppNamesInput[i] = util_strsav( pLang->ppNamesInput[i] );

    // update the variable map to reflect the fact that it is an MV input automaton
    if ( pLang->nInputs != nBitsIO )
    {
        int * pValues, * pValuesNew;
        // create a straight-forward variable map
        pValuesNew = ALLOC( int, pLang->nInputs + 2 );
        pValues = Vm_VarMapReadValuesArray( Vmx_VarMapReadVm(pLang->pVmx) );
        memcpy( pValuesNew, pValues, sizeof(int) * pLang->nInputs );
        pValuesNew[pLang->nInputs+0] = pLangNew->nStates;
        pValuesNew[pLang->nInputs+1] = pLangNew->nStates;
        pLangNew->pVmx = Vmx_VarMapCreateSimple( Fnc_ManagerReadManVm(pLang->pMan), 
            Fnc_ManagerReadManVmx(pLang->pMan), pLang->nInputs, 2, pValuesNew );
        FREE( pValuesNew );
    }

    printf( "Determinization:  (%d states, %d trans)  ->  (%d states, %d trans)\n", 
        pLang->nStates,  Lang_AutoCountTransitions( pLang ),
        pLangNew->nStates, Lang_AutoCountTransitions( pLangNew ) );

    return pLangNew;
}

/**Function*************************************************************

  Synopsis    [Derive the long names of the automaton.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lang_AutoDeterminizeLongNames( Lang_Auto_t * pLang, Au_Auto_t * pAutD )
{
    Au_State_t * pS;
    DdManager * dd = pLang->dd;
    DdNode * bFunc, * bTemp, * bMint, * bCubeCs;
    char * pNamePrev, * pName;
    int fFirst, s;

    // get the char cube of the CS vars
    bCubeCs  = Vmx_VarMapCharCubeRange( dd, pLang->pVmx, pLang->nInputs, pLang->nLatches );  Cudd_Ref( bCubeCs );
    // generate the state names and acceptable conditions
    for ( s = 0; s < pAutD->nStates; s++ )
    {
        // get the state
        pS = pAutD->pStates[s];

        // alloc the name
        pS->pName = ALLOC( char, 2 );
        pS->pName[0] = 0;

        // prepare to print the subset names
        fFirst = 1;
        // go though the components of this subset
        bFunc = pS->bSubset;   Cudd_Ref( bFunc );
        while ( bFunc != b0 )
        {
            // extract one minterm from the encoded state
            bMint  = Extra_bddGetOneMinterm( dd, bFunc, bCubeCs );        Cudd_Ref( bMint );
            // get the current state code
            if ( !st_lookup( pLang->tCode2Name, (char *)bMint, &pName ) )
            {
                // remove all state names produced so far
                Lang_AutoFreeStateNames( dd, pLang->tCode2Name );
                pLang->tCode2Name = NULL;
                Cudd_RecursiveDeref( dd, bMint );
                printf( "Cannot assign long names to the determinized automaton because of bug #74.\n" );
                goto QUITS;
            }

            // add this name to the subset name
            if ( fFirst )
                fFirst = 0;
            else
                strcat( pS->pName, "_" );
            pNamePrev = pS->pName;
            pS->pName = ALLOC( char, strlen(pNamePrev) + strlen(pName) + 3 );
            strcpy( pS->pName, pNamePrev );
            strcat( pS->pName, pName );
            FREE( pNamePrev );

            // subtract the minterm from the reachable states
            bFunc = Cudd_bddAnd( dd, bTemp = bFunc, Cudd_Not(bMint) );  Cudd_Ref( bFunc );
            Cudd_RecursiveDeref( dd, bTemp );
            Cudd_RecursiveDeref( dd, bMint );
        }
        Cudd_RecursiveDeref( dd, bFunc );
    }
QUITS:
    Cudd_RecursiveDeref( dd, bCubeCs );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


