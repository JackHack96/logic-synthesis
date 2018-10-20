/**CFile****************************************************************

  FileName    [auEncode.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Transforms an non-encoded FSM into an encoded one.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: auEncode.c,v 1.1 2004/02/19 03:06:45 alanmi Exp $]

***********************************************************************/

#include "auInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int Au_AutoEncodeGetValue( Mvc_Cube_t * pCube, int nValues, int ValueFirst );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Performs the encoding of the FSM.]

  Description [Reads BLIF-MV file with one ND node represented a non-encoded 
  FSM and transforms into an encoded FSM, written in standard KISS format.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Au_AutoEncodeSymbolicFsm( Mv_Frame_t * pMvsis, char * pFileNameIn, char * pFileNameOut )
{
    Ntk_Network_t * pNet;
    Ntk_Node_t * pNode, * pFanin, * pNodeCs, * pNodeNs;
    Ntk_Node_t ** ppNodes;
    int * pValues, * pBits, * pValuesFirst;
    int * pVarMap, i, b;
    int nVars, iValue, nValues, nRest;
    FILE * pFile, * pOutput;
    Vm_VarMap_t * pVm;
    int nInputs, nBitsIn;
    int nOutputs, nBitsOut;
    Cvr_Cover_t * pCvr;
    Mvc_Cover_t * pMvc;
    Mvc_Cube_t * pCube;
    int nDigits;

    // read the BLIF-MV file
    pOutput = Mv_FrameReadOut(pMvsis);
    pNet = Io_ReadNetwork( pMvsis, pFileNameIn );
    if ( pNet == NULL )
        return 1;
    // make sure that the network is okay
    if ( Ntk_NetworkReadNodeIntNum(pNet) != 1 )
    {
        fprintf( pOutput, "An FMS in BLIF-MV file should have one internal node.\n" );
        fprintf( pOutput, "The given network has %d internal nodes.\n", Ntk_NetworkReadNodeTotalNum(pNet) );
        return 1; 
    }
    if ( (pNodeCs = Ntk_NetworkFindNodeByName(pNet, "CS")) == NULL )
    {
        fprintf( pOutput, "An FMS in BLIF-MV file should have PI node \"CS\".\n" );
        fprintf( pOutput, "The given network does not have such node.\n" );
        return 1; 
    }
    if ( (pNodeNs = Ntk_NetworkFindNodeByName(pNet, "NS")) == NULL )
    {
        fprintf( pOutput, "An FMS in BLIF-MV file should have PI node \"NS\".\n" );
        fprintf( pOutput, "The given network does not have such node.\n" );
        return 1; 
    }
    if ( Ntk_NodeReadValueNum(pNodeCs) != Ntk_NodeReadValueNum(pNodeNs) )
    {
        fprintf( pOutput, "The number of CS values is different from the number of NS values.\n" );
        return 1; 
    }

    // get the internal node
    pNode = Ntk_NodeReadFaninNode( Ntk_NetworkReadCoNode(pNet, 0), 0 );

    // create the variable map
    nVars = Ntk_NetworkReadCiNum(pNet);
    ppNodes = ALLOC( Ntk_Node_t *, nVars );
    pValues = ALLOC( int, nVars );
    pBits   = ALLOC( int, nVars );
    pVarMap = ALLOC( int, nVars );
    nInputs = -1;
    i = 0;
    Ntk_NetworkForEachCi( pNet, pFanin )
    {
        ppNodes[i] = pFanin;
        pValues[i] = Ntk_NodeReadValueNum( pFanin );
        pBits[i]   = Extra_Base2Log(pValues[i]);
        pVarMap[i] = Ntk_NodeReadFaninIndex( pNode, pFanin );
        assert( pVarMap[i] >= 0 );
        if ( strcmp(pFanin->pName, "CS") == 0 )
            nInputs = i;
        i++;
    }

    // get the number of outputs
    nOutputs = nVars - nInputs - 2;
    assert( nInputs > 0 && nOutputs > 0 );

    // count the number of input bits
    nBitsIn = 0;
    for ( i = 0; i < nInputs; i++ )
        nBitsIn += pBits[i];

    // count the number of output bits;
    nBitsOut = 0;
    for ( i = nInputs + 2; i < nVars; i++ )
        nBitsOut += pBits[i];

    pVm  = Ntk_NodeReadFuncVm(pNode);
    pCvr = Ntk_NodeReadFuncCvr(pNode);
    pMvc = Cvr_CoverReadIsets(pCvr)[1];

    // write the FSM
    pFile = fopen( pFileNameOut, "w" );
    fprintf( pFile, ".i %d\n", nBitsIn );
    fprintf( pFile, ".o %d\n", nBitsOut );
    fprintf( pFile, ".s %d\n", Ntk_NodeReadValueNum(pNodeCs) );
    fprintf( pFile, ".p %d\n", Mvc_CoverReadCubeNum(pMvc) );
    fprintf( pFile, ".ilb" );
    for ( i = 0; i < nInputs; i++ )
    {
        fprintf( pFile, " " );
        for ( b = 0; b < pBits[i]; b++ )
            fprintf( pFile, " %s%d", ppNodes[i]->pName, b );
    }
    fprintf( pFile, "\n" );
    fprintf( pFile, ".ob" );
    for ( i = nInputs + 2; i < nVars; i++ )
    {
        fprintf( pFile, " " );
        for ( b = 0; b < pBits[i]; b++ )
            fprintf( pFile, " %s%d", ppNodes[i]->pName, b );
    }
    fprintf( pFile, "\n" );

    // get the number of symbols in the state number
	for ( nDigits = 0,  i = Ntk_NodeReadValueNum(pNodeCs) - 1;  i;  i /= 10,  nDigits++ );

    // write the cubes
    pValuesFirst = Vm_VarMapReadValuesFirstArray(pVm);
    Mvc_CoverForEachCube( pMvc, pCube )
    {
        // write the encoding input variables
        for ( i = 0; i < nInputs; i++ )
        {
            nValues = pValues[i];
            nRest   = (1 << pBits[i]) - nValues;
            iValue  = Au_AutoEncodeGetValue( pCube, nValues, pValuesFirst[pVarMap[i]] );
            if ( iValue == -2 )
            {
                fprintf( pOutput, "In BLIF-MV files describing FSMs with MV inputs and outputs,\n" );
                fprintf( pOutput, "the input literal can have one value (e.g. 3) or all values (e.g. -).\n" );
                fprintf( pOutput, "Partial care literals (e.g. {1,3}) are not allowed.\n" );
                fclose(pFile);
                return 1;
            }
            if ( iValue == -1 )
            {
                for ( b = 0; b < pBits[i]; b++ )
                    fprintf( pFile, "-" );
                continue;
            }
            if ( iValue < nRest )
            {
                iValue *= 2;
                fprintf( pFile, "-" );
                for ( b = 1; b < pBits[i]; b++ )
                    fprintf( pFile, "%d", ((iValue & (1 << b))>0) );
            }
            else
            {
                iValue += nRest;
                for ( b = 0; b < pBits[i]; b++ )
                    fprintf( pFile, "%d", ((iValue & (1 << b))>0) );
            }
        }

        // write the state variables
        iValue  = Au_AutoEncodeGetValue( pCube, pValues[nInputs], pValuesFirst[pVarMap[nInputs]] );
        if ( iValue < 0 )
        {
            fprintf( pOutput, "In BLIF-MV files describing FSMs with MV inputs and outputs,\n" );
            fprintf( pOutput, "the state literal can have one value (e.g. 3). Don't-care or \n" );
            fprintf( pOutput, "partial care literals (e.g. {1,3}) are not allowed.\n" );
            fclose(pFile);
            return 1;
        }
        fprintf( pFile, " %*d", nDigits, iValue );

        // write the state variables
        iValue  = Au_AutoEncodeGetValue( pCube, pValues[nInputs+1], pValuesFirst[pVarMap[nInputs+1]] );
        if ( iValue < 0 )
        {
            fprintf( pOutput, "In BLIF-MV files describing FSMs with MV inputs and outputs,\n" );
            fprintf( pOutput, "the state literal can have one value (e.g. 3). Don't-care or \n" );
            fprintf( pOutput, "partial care literals (e.g. {1,3}) are not allowed.\n" );
            fclose(pFile);
            return 1;
        }
        fprintf( pFile, " %*d ", nDigits, iValue );

        // write the encoding output variables
        for ( i = nInputs + 2; i < nVars; i++ )
        {
            nValues = pValues[i];
            nRest   = (1 << pBits[i]) - nValues;
            iValue  = Au_AutoEncodeGetValue( pCube, nValues, pValuesFirst[pVarMap[i]] );
            if ( iValue == -2 )
            {
                fprintf( pOutput, "In BLIF-MV files describing FSMs with MV inputs and outputs,\n" );
                fprintf( pOutput, "the output literal can have one value (e.g. 3) or all values (e.g. -).\n" );
                fprintf( pOutput, "Partial care literals (e.g. {1,3}) are not allowed.\n" );
                fclose(pFile);
                return 1;
            }
            if ( iValue == -1 )
            {
                for ( b = 0; b < pBits[i]; b++ )
                    fprintf( pFile, "-" );
                continue;
            }
            if ( iValue < nRest )
            {
                iValue *= 2;
                fprintf( pFile, "-" );
                for ( b = 1; b < pBits[i]; b++ )
                    fprintf( pFile, "%d", ((iValue & (1 << b))>0) );
            }
            else
            {
                iValue += nRest;
                for ( b = 0; b < pBits[i]; b++ )
                    fprintf( pFile, "%d", ((iValue & (1 << b))>0) );
            }
        }
        fprintf( pFile, "\n" );
    }
    fprintf( pFile, ".e\n" );
    fclose(pFile);

    FREE( ppNodes );
    FREE( pValues );
    FREE( pBits   );
    FREE( pVarMap );
    return 0;
}

/**Function*************************************************************

  Synopsis    [Find the value to be used.]

  Description [Returns the only value that it used. Returns -1 if this
  is a don't-care literal. Returns -2 when this is a partial care or 
  the literal has no values (error).]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Au_AutoEncodeGetValue( Mvc_Cube_t * pCube, int nValues, int ValueFirst )
{
    int Counter, iValue, i;
    Counter = 0;
    for ( i = 0; i < nValues; i++ )
        if ( Mvc_CubeBitValue( pCube, ValueFirst + i ) )
        {
            Counter++;
            iValue = i;
        }
    if ( Counter == 1 )
        return iValue;
    if ( Counter == nValues )
        return -1;
    return -2;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


