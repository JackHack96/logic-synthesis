/**CFile****************************************************************

  FileName    [shSign.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Pre-computing the canonical form of AND( AND(a,b), AND(c,d) ).]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: shSign.c,v 1.5 2004/04/12 03:42:31 alanmi Exp $]

***********************************************************************/

#include "shInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// elementary truth tables
#define St_Sign0    0xAAAA    // 1010 1010 1010 1010
#define St_Sign1    0xCCCC    // 1100 1100 1100 1100
#define St_Sign2    0xF0F0    // 1111 0000 1111 0000
#define St_Sign3    0xFF00    // 1111 1111 0000 0000

static unsigned Sh_SignGetTruth( short s, unsigned uVarSigns[] );
static short    Sh_SignSelectBest( short SignOld, short SignNew );
static int      Sh_SignCountLiterals( short Sign, int * pnInvs );


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Precomputes the canonical structures: AND(AND(a,b), AND(c,d)).]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
short * Sh_SignPrecompute()
{
    unsigned uVarSigns[4] = { St_Sign0, St_Sign1, St_Sign2, St_Sign3 };
    unsigned * pSign2True;   // mapping of a 14-bit signature into its truth table
    short    * pSign2Sign;   // mapping of a 14-bit signature into a canonical signature
    short    * pFunc2Sign;   // mapping of a 4-var-func truth table into its canonical signature
    unsigned uTruth;
    int nSigns = (1<<14);
    int nFuncs = (1<<16);
    int i;

    // allocate storage
    pSign2True = ALLOC( unsigned, nSigns );
    pSign2Sign = ALLOC( short, nSigns );
    pFunc2Sign = ALLOC( short, nFuncs );
//    memset( pFunc2Sign, 0, sizeof(short) * nFuncs );
    for ( i = 0; i < nFuncs; i++ )
        pFunc2Sign[i] = -3;
    // set the constant 0 and constant 1 functions
    pFunc2Sign[0]        = -1;
    pFunc2Sign[nFuncs-1] = -2;

    // compute signatures for each and save the canonical one
    for ( i = 0; i < nSigns; i++ )
    {
        unsigned uTruthB;
        // get the truth table for this signature
        uTruth = Sh_SignGetTruth( (short)i, uVarSigns );
        pSign2True[i] = uTruth;
        // assing the current signature
        if ( uTruth == 0 || uTruth == 0xFFFF )
            continue;
        // if the old signature was assigned
        // select the best one from the old sign and the new sign
        if ( pFunc2Sign[uTruth] >= 0 ) 
            pFunc2Sign[uTruth] = Sh_SignSelectBest( pFunc2Sign[uTruth], (short)i );
        else
            pFunc2Sign[uTruth] = i;

    // check if complementing the function would save
    uTruthB = ~uTruth & 0xFFFF;
    if (pFunc2Sign[uTruthB] >= 0) 
        pFunc2Sign[uTruthB] = Sh_SignSelectBest( pFunc2Sign[uTruthB], (short)(i | (1<<14)) );
    else
        pFunc2Sign[uTruthB] = (short)(i | (1<<14));
    }

    // collect the best signatures for each signature
    for ( i = 0; i < nSigns; i++ )
        pSign2Sign[i] = pFunc2Sign[pSign2True[i]];

    // print the precomputed table into file
//    Sh_SignTablePrint( pSign2Sign, pSign2True );

    FREE( pFunc2Sign );
    FREE( pSign2True );

    return pSign2Sign;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned Sh_SignGetTruth( short s, unsigned uVarSigns[] )
{
    unsigned uVar0, uVar1, uVar2, uVar3; 
    unsigned sAnd1, sAnd2;
    unsigned uAnd1, uAnd2;

    sAnd1 = St_SignReadAnd1( s );
    sAnd2 = St_SignReadAnd2( s );

    uVar0 = uVarSigns[ St_SignReadAndInput1(sAnd1) ];
    uVar1 = uVarSigns[ St_SignReadAndInput2(sAnd1) ];
    uVar2 = uVarSigns[ St_SignReadAndInput1(sAnd2) ];
    uVar3 = uVarSigns[ St_SignReadAndInput2(sAnd2) ];

    uVar0 = St_SignReadPolLL(s)? ~uVar0: uVar0;
    uVar1 = St_SignReadPolLR(s)? ~uVar1: uVar1;
    uVar2 = St_SignReadPolRL(s)? ~uVar2: uVar2;
    uVar3 = St_SignReadPolRR(s)? ~uVar3: uVar3;

    uAnd1 = uVar0 & uVar1;
    uAnd2 = uVar2 & uVar3;

    uAnd1 = St_SignReadPolL(s)? ~uAnd1: uAnd1;
    uAnd2 = St_SignReadPolR(s)? ~uAnd2: uAnd2;

    return (uAnd1 & uAnd2) & 0xFFFF;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sh_SignCountLiterals( short Sign, int * pnInvs )
{
    int iVar0, iVar1, iVar2, iVar3; 
    int fL, fR, fLL, fLR, fRL, fRR;
    unsigned sAnd1, sAnd2, s;
    int nLits, nInvs;

    if ( Sign == -1 )
        return 0;
    if ( Sign == -2 )
        return 1;

    assert( Sign >= 0 );
    s = (unsigned)Sign;

    sAnd1 = St_SignReadAnd1( s );
    sAnd2 = St_SignReadAnd2( s );

    iVar0 = St_SignReadAndInput1(sAnd1);
    iVar1 = St_SignReadAndInput2(sAnd1);
    iVar2 = St_SignReadAndInput1(sAnd2);
    iVar3 = St_SignReadAndInput2(sAnd2);
 
    fL  = St_SignReadPolL(s);
    fR  = St_SignReadPolR(s);
    fLL = St_SignReadPolLL(s);
    fLR = St_SignReadPolLR(s);
    fRL = St_SignReadPolRL(s);
    fRR = St_SignReadPolRR(s);

    nLits = 0;
    nInvs = 0;
    if ( iVar0 == iVar1 && fLL == fLR )
    {
        nLits++;
        nInvs += fLL;
    }
    else
    {
        nLits += 2;
        nInvs += fLL;
        nInvs += fLR;
    }
    if ( iVar2 == iVar3 && fRL == fRR )
    {
        nLits++;
        nInvs += fRL;
    }
    else
    {
        nLits += 2;
        nInvs += fRL;
        nInvs += fRR;
    }

    nInvs += fL;
    nInvs += fR;

    *pnInvs = nInvs;
    return nLits;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
short Sh_SignSelectBest( short SignOld, short SignNew )
{
    int nLitsOld, nInvsOld;
    int nLitsNew, nInvsNew;

    nLitsOld = Sh_SignCountLiterals( SignOld, &nInvsOld );
    nLitsNew = Sh_SignCountLiterals( SignNew, &nInvsNew );

    if ( nLitsOld < nLitsNew )
        return SignOld;
    if ( nLitsOld > nLitsNew )
        return SignNew;

    if ( nInvsOld < nInvsNew )
        return SignOld;
    if ( nInvsOld > nInvsNew )
        return SignNew;

    return SignOld;
}

/**Function*************************************************************

  Synopsis    [Construct the canonical combination of signature.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sh_Node_t * Sh_SignConstructNodes( Sh_Manager_t * p, short Sign, Sh_Node_t * pNodes[], int nNodes )
{
    Sh_Node_t * pN1, * pN2;
    Sh_Node_t * pNLL, * pNLR;
    Sh_Node_t * pNRL, * pNRR;
    Sh_Node_t * pRes;
    int iVar0, iVar1, iVar2, iVar3; 
    int fL, fR, fLL, fLR, fRL, fRR;
    unsigned sAnd1, sAnd2, s;

    assert( Sign >= 0 );
    s = (unsigned)Sign;

    sAnd1 = St_SignReadAnd1( s );
    sAnd2 = St_SignReadAnd2( s );

    iVar0 = St_SignReadAndInput1(sAnd1);
    iVar1 = St_SignReadAndInput2(sAnd1);
    iVar2 = St_SignReadAndInput1(sAnd2);
    iVar3 = St_SignReadAndInput2(sAnd2);
 
    fL  = St_SignReadPolL(s);
    fR  = St_SignReadPolR(s);
    fLL = St_SignReadPolLL(s);
    fLR = St_SignReadPolLR(s);
    fRL = St_SignReadPolRL(s);
    fRR = St_SignReadPolRR(s);

    pNLL = pNodes[iVar0];
    pNLR = pNodes[iVar1];
    pNRL = pNodes[iVar2];
    pNRR = pNodes[iVar3];

    pN1 = NULL;
    pN2 = NULL;
    if ( iVar0 == iVar1 )
    {
        if ( fLL == fLR )
//            sprintf( sAnd1, "(%c%s)", 'a' + iVar0, fLL? "\'": "" );
            pN1 = Sh_NotCond( pNLL, fLL );   shRef( pN1 );
//        else 
//            sprintf( sAnd1, "0" );
    }
    else
    {
//        sprintf( sAnd1, "(%c%s%c%s)", 'a' + iVar0, fLL? "\'": "", 
//                                      'a' + iVar1, fLR? "\'": "" );
            pNLL = Sh_NotCond( pNLL, fLL );
            pNLR = Sh_NotCond( pNLR, fLR );
            pN1 = Sh_TableLookup( p, pNLL, pNLR );   shRef( pN1 );
    }

    if ( iVar2 == iVar3 )
    {
        if ( fRL == fRR )
//            sprintf( sAnd2, "(%c%s)", 'a' + iVar2, fRL? "\'": "" );
            pN2 = Sh_NotCond( pNRL, fRL );   shRef( pN2 );
//        else 
//            sprintf( sAnd2, "0" );
    }
    else
    {
//        sprintf( sAnd2, "(%c%s%c%s)", 'a' + iVar2, fRL? "\'": "", 
//                                      'a' + iVar3, fRR? "\'": "" );
            pNRL = Sh_NotCond( pNRL, fRL );
            pNRR = Sh_NotCond( pNRR, fRR );
            pN2 = Sh_TableLookup( p, pNRL, pNRR );   shRef( pN2 );
    }

//    if ( strcmp( sAnd1, sAnd2 ) == 0 )
//    {
//        if ( fL == fR )
//            sprintf( sBuffer, "%s%s", sAnd1, fL? "\'": "" );
//        else
//            sprintf( sBuffer, "0" );
//    }
//    else
//        sprintf( sBuffer, "%s%s%s%s", sAnd1, fL? "\'": "", sAnd2, fR? "\'": "" );

    if ( pN1 )
        pN1 = Sh_NotCond( pN1, fL );
    if ( pN2 )
        pN2 = Sh_NotCond( pN2, fR );

    if ( pN1 && pN2 )
        pRes = Sh_TableLookup( p, pN1, pN2 );
    else if ( pN1 )
        pRes = pN1;
    else if ( pN2 )
        pRes = pN2;
    else
    {
        assert( 0 );
        pRes = NULL;
    }
    shRef( pRes );
    if ( pN1 )
        Sh_RecursiveDeref( p, pN1 );
    if ( pN2 )
        Sh_RecursiveDeref( p, pN2 );
    shDeref( pRes );

    if (s & (1<<14)) 
        return Sh_Not(pRes);
    return pRes;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Sh_SignPrint( short Sign )
{
    static char sBuffer[20];
    char sAnd1[10], sAnd2[10];
    int iVar0, iVar1, iVar2, iVar3; 
    int fL, fR, fLL, fLR, fRL, fRR;
    unsigned ssAnd1, ssAnd2;
    unsigned s;

    if ( Sign == -1 )
    {
        sBuffer[0] = '0';
        sBuffer[1] = 0;
        return sBuffer;
    }

    if ( Sign == -2 )
    {
        sBuffer[0] = '1';
        sBuffer[1] = 0;
        return sBuffer;
    }

    assert( Sign >= 0 );

    s = (unsigned)Sign;

    ssAnd1 = St_SignReadAnd1( s );
    ssAnd2 = St_SignReadAnd2( s );

    iVar0 = St_SignReadAndInput1(ssAnd1);
    iVar1 = St_SignReadAndInput2(ssAnd1);
    iVar2 = St_SignReadAndInput1(ssAnd2);
    iVar3 = St_SignReadAndInput2(ssAnd2);
 
    fL  = St_SignReadPolL(s);
    fR  = St_SignReadPolR(s);
    fLL = St_SignReadPolLL(s);
    fLR = St_SignReadPolLR(s);
    fRL = St_SignReadPolRL(s);
    fRR = St_SignReadPolRR(s);

    if ( iVar0 == iVar1 )
    {
        if ( fLL == fLR )
            sprintf( sAnd1, "(%c%s)", 'a' + iVar0, fLL? "\'": "" );
        else 
            sprintf( sAnd1, "0" );
    }
    else
        sprintf( sAnd1, "(%c%s%c%s)", 'a' + iVar0, fLL? "\'": "", 
                                      'a' + iVar1, fLR? "\'": "" );

    if ( iVar2 == iVar3 )
    {
        if ( fRL == fRR )
            sprintf( sAnd2, "(%c%s)", 'a' + iVar2, fRL? "\'": "" );
        else 
            sprintf( sAnd2, "0" );
    }
    else
        sprintf( sAnd2, "(%c%s%c%s)", 'a' + iVar2, fRL? "\'": "", 
                                      'a' + iVar3, fRR? "\'": "" );

    if ( strcmp( sAnd1, sAnd2 ) == 0 )
    {
        if ( fL == fR )
            sprintf( sBuffer, "%s%s", sAnd1, fL? "\'": "" );
        else
            sprintf( sBuffer, "0" );
    }
    else
        sprintf( sBuffer, "%s%s%s%s", sAnd1, fL? "\'": "", sAnd2, fR? "\'": "" );
    if (s & (1<<14)) 
        sprintf( sBuffer, "\'" );
    return sBuffer;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sh_SignTablePrint( short * pSign2Sign, unsigned * pSign2True )
{
    int nSigns = (1 << 14);
    unsigned Unsigned;
    FILE * pFile;
    short i;

    pFile = fopen( "table.txt", "w" );
    for ( i = 0; i < nSigns; i++ )
    {
        // the current signature
        fprintf( pFile, "%5d  =  ", i );
        // the binary representation
        Unsigned = (unsigned)i;
        Extra_PrintBinary( pFile, &Unsigned, 14 );
        // the corresponding circuit
        fprintf( pFile, "  %16s", Sh_SignPrint(i) );

        // the truth table
        fprintf( pFile, " --> " );
        Extra_PrintBinary( pFile, &(pSign2True[i]), 16 );
        fprintf( pFile, " --> " );

        // get the current signature
        fprintf( pFile, "%5d  =  ", pSign2Sign[i] );
        // the binary representation
        if ( pSign2Sign[i] < 0 )
            fprintf( pFile, "%14d", pSign2Sign[i] );
        else
        {
            Unsigned = (unsigned)pSign2Sign[i];
            Extra_PrintBinary( pFile, &Unsigned, 14 );
        }
        // the corresponding circuit
        fprintf( pFile, "  %16s", Sh_SignPrint(pSign2Sign[i]) );
        fprintf( pFile, "\n" );
    }
    fclose( pFile );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


