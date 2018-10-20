/**CFile****************************************************************

  FileName    [auDeterm.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Automata determinization procedure.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: auDetExp.c,v 1.1 2004/02/19 03:06:45 alanmi Exp $]

***********************************************************************/

#include "auInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct DetTableStruct Det_Table_t;
typedef struct DetEntryStruct Det_Entry_t;
struct DetTableStruct
{
    int              nEntries;
    int              nBins;
    Det_Entry_t **   pBins;

};
struct DetEntryStruct
{
    unsigned short * pSubs;
    int              nSubs;
    Det_Entry_t    * pNext;
    int              State;
};


static Det_Table_t * Det_TableStart( int nBins );
static void          Det_TableStop( Det_Table_t * p );
static int           Det_TableCompare( Det_Entry_t * pE, unsigned short * pSubs, int nSubs );
static unsigned      Det_TableHash( unsigned short * pSubs, int nSubs );
static void          Det_TableInsert( Det_Table_t * p, unsigned short * pSubs, int nSubs, int State );
static int           Det_TableLookup( Det_Table_t * p, unsigned short * pSubs, int nSubs );

static int Det_Compare( int ** ppS1, int ** ppS2 );

static int Au_AutoDeterminizeExpState( Au_Auto_t * pAut, Au_State_t * pStateD, 
    unsigned pTruths[], unsigned short * ppSubs[], int pnSubs[] );
static void Au_AutoCreateTruthTables( Au_Auto_t * pAut );
static DdNode * Au_AutoCreateMvcFromTruth( DdManager * dd, unsigned Truth, 
         int Start, int nBits, int iVar );
static int Au_AutoDeterminizeAddState( unsigned short pSubs[], int nSubs, int State );


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Explicit automata determinization algorithm.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Au_Auto_t * Au_AutoDeterminizeExp( Au_Auto_t * pAut, int fAllAccepting, int fLongNames )
{
    ProgressBar * pProgress;
    Det_Table_t * pTable;
    Au_Auto_t * pAutD;
    Au_State_t * pS, * pState;
    Au_Trans_t * pTrans;
    char Buffer[100];
    char * pNamePrev;
    int s, i, nDigits;
    int NextSt;
    int nMints;

    unsigned * puTruthTemp;
    unsigned short ** ppSubsTemp;
    int * pnSubsTemp;
    int nStatesNew;

    DdManager * dd;
    Vm_VarMap_t * pVm;
    Vmx_VarMap_t * pVmx;
    Mvr_Relation_t * pMvrTemp;
    DdNode * bFunc;

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


    if ( pAut->nInputs > 5 )
    {
        printf( "Explicit determinization is currently limited to automata with %d inputs or less.\n", 5 );
        return NULL;
    }
    if ( pAut->nStates > (1<<16) )
    {
        printf( "Explicit determinization is currently limited to automata with %d states or less.\n", (1<<16) );
        return NULL;
    }

    dd = Fnc_ManagerReadDdLoc(pAut->pMan);
    pVm = Vm_VarMapCreateBinary( Fnc_ManagerReadManVm(pAut->pMan), pAut->nInputs, 0 );
    pVmx = Vmx_VarMapLookup( Fnc_ManagerReadManVmx(pAut->pMan), pVm, -1, NULL );
    pMvrTemp = Mvr_RelationCreate( Fnc_ManagerReadManMvr(pAut->pMan), pVmx, b1 );


    // create truth tables for each condition
    Au_AutoCreateTruthTables( pAut );


    // start the new automaton
    pAutD = Au_AutoClone( pAut );
    pAutD->nStates      = 0;
    pAutD->nStatesAlloc = 1000;
    pAutD->pStates      = ALLOC( Au_State_t *, pAutD->nStatesAlloc );

    // create the initial state
    pS = Au_AutoStateAlloc();
    pS->pSubs = ALLOC( unsigned short, 1 );
    pS->pSubs[0] = 0;
    pS->nSubs = 1;
    pAutD->pStates[ pAutD->nStates++ ] = pS;

    // start the table to collect the reachable states
    pTable = Det_TableStart( Cudd_Prime(100000) );
    Det_TableInsert( pTable, pS->pSubs, pS->nSubs, 0 );

    // allocate temporary storage for subsets
    nMints = (1 << pAut->nInputs);
    puTruthTemp   = ALLOC( unsigned,         nMints );
    pnSubsTemp    = ALLOC( int,              nMints );
    ppSubsTemp    = ALLOC( unsigned short *, nMints );
    ppSubsTemp[0] = ALLOC( unsigned short,   nMints * pAut->nStates );
    for ( i = 1; i < nMints; i++ )
        ppSubsTemp[i] = ppSubsTemp[i-1] + pAut->nStates;

    pProgress = Extra_ProgressBarStart( stdout, 1000 );

    // iteratively process the subsets
    for ( s = 0; s < pAutD->nStates; s++ )
    {
        nStatesNew = Au_AutoDeterminizeExpState( pAut, pAutD->pStates[s], 
            puTruthTemp, ppSubsTemp, pnSubsTemp );

        // check if these subsets exist
        for ( i = 0; i < nStatesNew; i++ )
        {
            if ( pnSubsTemp[i] == 0 )
                continue;

            // sort the states
//           qsort( ppSubsTemp[i], pnSubsTemp[i], sizeof(unsigned short), 
//                (int(*)(const void *, const void *))Det_Compare );

            if ( (NextSt = Det_TableLookup( pTable, ppSubsTemp[i], pnSubsTemp[i] )) == -1 )
            {
                // realloc storage for states if necessary
                if ( pAutD->nStatesAlloc <= pAutD->nStates )
                {
                    pAutD->pStates  = REALLOC( Au_State_t *, pAutD->pStates,  2 * pAutD->nStatesAlloc );
                    pAutD->nStatesAlloc *= 2;
                }

                pS = Au_AutoStateAlloc();
                pS->pSubs = ALLOC( unsigned short, pnSubsTemp[i] );
                memcpy( pS->pSubs, ppSubsTemp[i], sizeof(unsigned short) * pnSubsTemp[i] );
                pS->nSubs = pnSubsTemp[i];

                NextSt = pAutD->nStates;
                pAutD->pStates[ pAutD->nStates++ ] = pS;

                Det_TableInsert( pTable, pS->pSubs, pS->nSubs, NextSt );
            }

            // find the transition
            Au_StateForEachTransition( pAutD->pStates[s], pTrans )
            {
                if ( pTrans->StateNext == NextSt )
                {
                    pTrans->uCond |= puTruthTemp[i];
                    break;
                }
            }
            if ( pTrans == NULL )
            {
                // add the transition
                pTrans = Au_AutoTransAlloc();
                pTrans->uCond = puTruthTemp[i];
                pTrans->StateCur  = s;
                pTrans->StateNext = NextSt;
                // add the transition
                Au_AutoTransAdd( pAutD->pStates[s], pTrans );
            }
        }

        if ( s > 50 && (s % 20 == 0) )
            Extra_ProgressBarUpdate( pProgress, 1000 * s / pAutD->nStates, NULL );
    }
    Extra_ProgressBarStop( pProgress );

    Det_TableStop( pTable );
    FREE( puTruthTemp );
    FREE( pnSubsTemp );
    FREE( ppSubsTemp[0] );
    FREE( ppSubsTemp );

    // derive the conditions
    for ( s = 0; s < pAutD->nStates; s++ )
    {
        Au_StateForEachTransition( pAutD->pStates[s], pTrans )
        {        
            bFunc = Au_AutoCreateMvcFromTruth( dd, pTrans->uCond, 0, (1<<pAut->nInputs), pAut->nInputs - 1 );
            Cudd_Ref( bFunc );
            pTrans->uCond = 0;
            pTrans->pCond = Fnc_FunctionDeriveSopFromMdd( Fnc_ManagerReadManMvc(pAut->pMan), 
                pMvrTemp, bFunc, bFunc, pAut->nInputs );
            Cudd_RecursiveDeref( dd, bFunc );
        }
    }
    Mvr_RelationFree( pMvrTemp );



    // get the number of digits in the state number
	for ( nDigits = 0, i = pAutD->nStates - 1;  i;  i /= 10,  nDigits++ );

    // generate the state names and acceptable conditions
    for ( s = 0; s < pAutD->nStates; s++ )
    {
        // get the state
        pS = pAutD->pStates[s];
        if ( fLongNames )
        { // print state names as subsets of the original state names
            int fFirst = 1;
            // alloc the name
            pS->pName = ALLOC( char, 2 );
            pS->pName[0] = 0;
            for ( i = 0; i < pS->nSubs; i++ )
            {
                pState = pAut->pStates[ pS->pSubs[i] ];

                if ( fFirst )
                    fFirst = 0;
                else
                    strcat( pS->pName, "_" );
//              strcat( pS->pName, pState->pName );

                pNamePrev = pS->pName;
                pS->pName = ALLOC( char, strlen(pNamePrev) + strlen(pState->pName) + 3 );
                strcpy( pS->pName, pNamePrev );
                strcat( pS->pName, pState->pName );
                FREE( pNamePrev );
            }
        }
        else
        { // print simple state names
            sprintf( Buffer, "s%0*d", nDigits, s );
            pS->pName = util_strsav( Buffer );
        }
    }


    // set the accepting attributes of states
    for ( s = 0; s < pAutD->nStates; s++ )
    {
        // get the state
        pS = pAutD->pStates[s];
        // check whether this state is accepting
        if ( fAllAccepting )
        {
            pS->fAcc = 1;
            for ( i = 0; i < pS->nSubs; i++ )
            {
                pState = pAut->pStates[ pS->pSubs[i] ];
                if ( !pState->fAcc )
                {
                    pS->fAcc = 0;
                    break;
                }
            }
        }
        else
        {
            pS->fAcc = 0;
            for ( i = 0; i < pS->nSubs; i++ )
            {
                pState = pAut->pStates[ pS->pSubs[i] ];
                if ( pState->fAcc )
                {
                    pS->fAcc = 1;
                    break;
                }
            }
        }
    }

    printf( "Determinization:  (%d states, %d trans)  ->  (%d states, %d trans)\n", 
        pAut->nStates,  Au_AutoCountTransitions( pAut ),
        pAutD->nStates, Au_AutoCountTransitions( pAutD ) );
    return pAutD;
}


/**Function*************************************************************

  Synopsis    [Determines, which subsets are reachable from the given subset.]

  Description [Given are the original automaton (pAut), the subset of its
  states (pStateD), and the storage for the resulting truth tables (pTruths),
  the set of new subsets (ppSubs), and the number of the new subsets (pnSubs).]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Au_AutoDeterminizeExpState( Au_Auto_t * pAut, Au_State_t * pStateD, 
    unsigned pTruths[], unsigned short * ppSubs[], int pnSubs[] )
{
    Au_Trans_t * pTrans;
    unsigned uTruth, uTruth0, uTruth1;
    int nSubsNext, nSubsNextOld, i, k;

    assert( pStateD->nSubs > 0 );

    // the first domain is equal to all boolean space
    uTruth = (((unsigned)(~0)) >> (32 - (1<<pAut->nInputs)));

    pTruths[0] = (((unsigned)(~0)) >> (32 - (1<<pAut->nInputs)));
    ppSubs[0][0] = 0;
    pnSubs[0] = 0;
    nSubsNext = 1;

    // split this domain using the states in the subset
    for ( i = 0; i < pStateD->nSubs; i++ )
    {
        Au_StateForEachTransition( pAut->pStates[pStateD->pSubs[i]], pTrans )
        {
            uTruth = pTrans->uCond;
            nSubsNextOld = nSubsNext;
            for ( k = 0; k < nSubsNextOld; k++ )
            {
                uTruth0 = pTruths[k] & ~uTruth;
                uTruth1 = pTruths[k] &  uTruth;
                // consider three cases:
                // (1) the current partition (pTruth[k]) is covered by the complement
                // (2) the current partition (pTruth[k]) is covered by the condition
                // (3) they overlap but there is no complete containment
                if ( uTruth0 == pTruths[k] )
                { // (1)
                }
                else if ( uTruth1 == pTruths[k] ) 
                { // (2)
//                    ppSubs[k][ pnSubs[k]++ ] = pTrans->StateNext;
                    if ( Au_AutoDeterminizeAddState( ppSubs[k], pnSubs[k], pTrans->StateNext ) )
                        pnSubs[k]++;

                }
                else
                { // (3)
                    // modify the current paritition to be uTruth0
                    pTruths[k] = uTruth0;

                    // create the new partition with uTruth1
                    pTruths[nSubsNext] = uTruth1;
                    pnSubs[nSubsNext] = pnSubs[k];
                    memcpy( ppSubs[nSubsNext], ppSubs[k], sizeof(unsigned short) * pnSubs[k] );
//                    ppSubs[nSubsNext][ pnSubs[k]++ ] = pTrans->StateNext;
                    if ( Au_AutoDeterminizeAddState( ppSubs[nSubsNext], pnSubs[nSubsNext], pTrans->StateNext ) )
                        pnSubs[nSubsNext]++;
                    nSubsNext++;
                }
            }
        }
    }
    return nSubsNext;
}

/**Function*************************************************************

  Synopsis    []

  Description [Returns 1 if it was the new state.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Au_AutoDeterminizeAddState( unsigned short * pSubs, int nSubs, int State )
{
    int i, k;
    if ( nSubs == 0 )
    {
        pSubs[0] = State;
        return 1;
    }
    for ( i = 0; i < nSubs; i++ )
        if ( pSubs[i] == State )
            return 0;
        else if ( pSubs[i] > State )
            break;
    // shift if necessary
    for ( k = nSubs; k > i; k-- )
        pSubs[k] = pSubs[k-1];
    pSubs[i] = State;
    return 1;
}



/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Au_AutoCreateTruthTables( Au_Auto_t * pAut )
{
    unsigned * pVarsTruth;
    unsigned * pCubeTruth;
    unsigned uCube, uMask, uMaskAll;
    int nInputs, nMints, nCubes;
    int i, m, c, s;
    Au_Trans_t * pTrans;
    Mvc_Cube_t * pCube;

    nInputs = pAut->nInputs;
    nMints  = (1 << nInputs);
    nCubes  = (1 << (2 * nInputs));

    // create the map of vars into their truth tables
    pVarsTruth = ALLOC( unsigned, nInputs );
    memset( pVarsTruth, 0, sizeof(unsigned) * nInputs );
    for ( m = 0; m < nMints; m++ )
        for ( i = 0; i < nInputs; i++ )
            if ( m & (1 << i) )
                pVarsTruth[i] |= (1 << m);

//Extra_PrintBinary( stdout, pVarsTruth, nMints );
//Extra_PrintBinary( stdout, pVarsTruth+1, nMints );
//Extra_PrintBinary( stdout, pVarsTruth+2, nMints );
//fprintf( stdout, "\n" );

    // create the map of cubes into their truth tables
    uMaskAll = ((unsigned)(~0)) >> nMints;
    pCubeTruth = ALLOC( unsigned, nCubes );
    for ( c = 0; c < nCubes; c++ )
    {
        uCube = ((unsigned)(~0));
        for ( i = 0; i < nInputs; i++ )
        {
            uMask = (((unsigned)c) >> (2*i)) & 3;
            if ( uMask == 0 )
                break;
            if ( uMask == 1 )
                uCube &= ~pVarsTruth[i];
            else if ( uMask == 2 )
                uCube &=  pVarsTruth[i];
        }
        if ( i == nInputs )
        {
            pCubeTruth[c] = uCube & uMaskAll;
//Extra_PrintBinary( stdout, &pCubeTruth[c], nMints );
//fprintf( stdout, "\n" );
        }
        else
            pCubeTruth[c] = 0;
    }

    for ( s = 0; s < pAut->nStates; s++ )
        Au_StateForEachTransition( pAut->pStates[s], pTrans )
        {
            pTrans->uCond = 0;
            Mvc_CoverForEachCube( pTrans->pCond, pCube )
            {
                pTrans->uCond |= pCubeTruth[pCube->pData[0]];
            }
//Extra_PrintBinary( stdout, &pTrans->uCond, nMints );
//fprintf( stdout, "\n" );
    }

    FREE( pVarsTruth );
    FREE( pCubeTruth );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Au_AutoCreateMvcFromTruth( DdManager * dd, unsigned Truth, 
         int Start, int nBits, int iVar )
{
    DdNode * bF0, * bF1, * bF;

    if ( nBits == 1 )
    {
        if ( Truth & (1 << Start) )
            return b1;
        return b0;
    }

    bF0 = Au_AutoCreateMvcFromTruth( dd, Truth, Start, nBits/2, iVar - 1 );
    Cudd_Ref( bF0 );

    bF1 = Au_AutoCreateMvcFromTruth( dd, Truth, Start + nBits/2, nBits/2, iVar - 1 );
    Cudd_Ref( bF1 );

    bF = Cudd_bddIte( dd, dd->vars[iVar], bF1, bF0 );  Cudd_Ref( bF );
    Cudd_RecursiveDeref( dd, bF0 );
    Cudd_RecursiveDeref( dd, bF1 );

    Cudd_Deref( bF );
    return bF;
}






/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Det_Table_t * Det_TableStart( int nBins )
{
    Det_Table_t * p;
    p = ALLOC( Det_Table_t  , 1 );
    memset( p, 0, sizeof(Det_Table_t) );
    p->pBins = ALLOC( Det_Entry_t *, nBins );
    memset( p->pBins, 0, sizeof(Det_Entry_t *) * nBins );
    p->nBins = nBins;
    return p;    
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Det_TableStop( Det_Table_t * p )
{
    FREE( p->pBins );
    FREE( p );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Det_TableCompare( Det_Entry_t * pE, unsigned short * pSubs, int nSubs )
{
    int i;
    if ( pE->nSubs != nSubs )
        return 0;
    for ( i = 0; i < nSubs; i++ )
        if ( pE->pSubs[i] != pSubs[i] )
            return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned Det_TableHash( unsigned short * pSubs, int nSubs )
{
    unsigned Hash;
    int i;
    Hash = 0;
    for ( i = 0; i < nSubs; i++ )
    {
        if ( i & 1 )
            Hash *= pSubs[i];
        else
            Hash ^= pSubs[i];
    }
    return Hash;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Det_TableInsert( Det_Table_t * p, unsigned short * pSubs, int nSubs, int State )
{
    Det_Entry_t * pE;
    int iBin;
    pE = ALLOC( Det_Entry_t, 1 );
    pE->nSubs = nSubs;
    pE->pSubs = pSubs;
    pE->pNext = NULL;
    pE->State = State;

    iBin = Det_TableHash( pE->pSubs, pE->nSubs ) % p->nBins;
    pE->pNext      = p->pBins[iBin];
    p->pBins[iBin] = pE;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Det_TableLookup( Det_Table_t * p, unsigned short * pSubs, int nSubs )
{
    Det_Entry_t * pE;
    int iBin;
    iBin = Det_TableHash( pSubs, nSubs ) % p->nBins;
    for ( pE = p->pBins[iBin]; pE; pE = pE->pNext )
        if ( Det_TableCompare( pE, pSubs, nSubs ) )
            return pE->State;
    return -1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Det_Compare( int ** ppS1, int ** ppS2 )
{
    if ( *ppS1 < *ppS2 )
        return -1;
    if ( *ppS1 > *ppS2 )
        return 1;
    assert( 0 );
    return 0;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


