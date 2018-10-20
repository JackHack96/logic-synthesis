/**CFile****************************************************************

  FileName    [pdPairDecode.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: pdPairDecode.c,v 1.11 2003/05/27 23:33:41 wjiang Exp $]

***********************************************************************/

#include "pdInt.h"

static int _MinimizeValues( Ntk_Node_t * pDec );

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Pd_PairDecode( Ntk_Network_t * pNet, int threshValue, int iCost, int resubOption, int timelimit, int fVerbose )
{
    Ntk_Node_t * pNode1, * pNode2, * a, * b, * pFanout;
    Ntk_Node_t * new_ab;
    Ntk_Pin_t * pin;
    int i, j, bestValue, value, changed;
    array_t *nodeVec;

    // we only handle two cost functions for now
    if ( iCost > 1 ) iCost = 1;
    if ( fVerbose )
        printf(" search starts\n");

    bestValue = -INFINITY;
    changed = 0;
    nodeVec = array_alloc( Ntk_Node_t *, 0 );

    // collect all nodes in network to nodeVec
    Ntk_NetworkForEachCi( pNet, pNode1 ) 
    {
        array_insert_last( Ntk_Node_t *, nodeVec, pNode1 );
    }
    Ntk_NetworkForEachNode( pNet, pNode1 ) 
    {
        array_insert_last( Ntk_Node_t *, nodeVec, pNode1 );
    }

    // search good pairs
    for ( i = 0; i < array_n(nodeVec); i++ ) 
    {
        pNode1 = array_fetch( Ntk_Node_t *, nodeVec, i );
        for ( j = i+1; j < array_n(nodeVec); j++ )
        {
            // timeout comes in
            if ( clock() > timelimit )
                break;
            pNode2 = array_fetch( Ntk_Node_t *, nodeVec, j );
            value = Pd_PairValue( pNode1, pNode2, iCost );
            // printf( "node1 %s node2 %s value %d\n", Ntk_NodeReadName( pNode1 ), Ntk_NodeReadName( pNode2 ), value );
    	    if( value > bestValue )
            {
    	        a = pNode1;
	            b = pNode2;
		        bestValue = value;
	        }   
	    }
        if ( clock() > timelimit )
            break;
    }
    array_free( nodeVec );

    // debug
    if ( fVerbose )
        printf(" search ends\n" );

    if ( bestValue >= threshValue ) 
    {	
	    // this function do the real pair work
    	new_ab = Pd_PairNodes_Simp( a, b );
        if ( new_ab ) 
        {
            changed = 1;
        	// to avoid infinite loop
            nodeVec = array_alloc( Ntk_Node_t *, 0 );
	        Ntk_NodeForEachFanout( new_ab, pin, pFanout ) 
            {
                array_insert_last( Ntk_Node_t *, nodeVec, pFanout );
            }
            for ( i=0; i<array_n( nodeVec ); i++ )
            {
                pFanout = array_fetch( Ntk_Node_t *, nodeVec, i );
	            Ntk_NodeMakeMinimumBase( pFanout );
            }
	        if ( Ntk_NodeReadFanoutNum(new_ab) == 0 )
	            changed = 0;
            array_free( nodeVec );
        }
        // verbose
        if ( changed && fVerbose )
            Ntk_NodePrintCvr( stdout, new_ab, 0, 0 );
    }    
    return changed;
}

/**Function********************************************************************

  Synopsis       [calculate the saving of pairing a and b]

  Description    [The saving includes the changes in the fanouts of a and b. We 
  consider two options: create a new node with a and b as fanins, merge a and b.
  The one that leads to larger saving is taken.

  Variable 'merge' is set to 1 if we will merge a and b, 0 otherwise.]

  SideEffects    []

******************************************************************************/
int Pd_PairValue( Ntk_Node_t *p1, Ntk_Node_t *p2, int iCost ) 
{
    int i, j, v, index1, index2, value, nVal, nWords;
    int sameoutput, paired;
    Ntk_Node_t * fanout;
    pdst_table * cube_table;
    Cvr_Cover_t * pCvr;
    Ntk_Pin_t * pin1;
    Vm_VarMap_t *pVm;
    Mvc_Cover_t **tempppMvc;
    Mvc_Cube_t * pCube, * mask, * temp, * dummy;
    int nVarsIn, nValuesIn;
    int * pValues, iValues;
    bool fDontCare;
    int Counter, start1, start2;

    // check if p1 and p2 have same output. If not, no need to pair them
    sameoutput = 0;
    Ntk_NodeForEachFanout( p1, pin1, fanout ) 
    {
        index2 = Ntk_NodeReadFaninIndex( fanout, p2 );
        if ( index2 != -1 ) 
        {
	        sameoutput = 1;
	        break;
	    }
    }
    if( sameoutput == 0 ) {
        return -INFINITY;
    }

    // check if p1 and p2 are already paired
    // NOTE: if we don't check here, if thresh is given low enough, there will be permenent loop
    // if p1 and p2 has single fanout, which only has them as fanins, we do not pair them
    paired = 0;
    /*
    if ( (Ntk_NodeReadFanoutNum(p1) == 1) && (Ntk_NodeReadFanoutNum(p2) == 1) ) 
    {  
    */
        Ntk_NodeForEachFanout( p1, pin1, fanout )  
        {
	        if ( Ntk_NodeReadFaninNum( fanout ) == 2 && Ntk_NodeReadFaninIndex( fanout, p2 ) != -1 )
	            paired = 1;
	    }
    // }
    if( paired == 1 ) 
    {
        return -INFINITY;
    }     

    // The number of values no more than 32!!
    i = Ntk_NodeReadValueNum( p1 );
    j = Ntk_NodeReadValueNum( p2 );
    if ( i*j > 32 ) 
    {
        return -INFINITY;
    }
        
    /* value is the possible saving by pairing two nodes in the fanouts*/
    value = 0;
    Ntk_NodeForEachFanout( p1, pin1, fanout ) 
    {
        // added by Yunjian for data-path 
        if ( Ntk_NodeBelongsToLatch( fanout ) ) continue;
	    index2 = Ntk_NodeReadFaninIndex( fanout, p2 );
    	if( index2 == -1 )  continue;
	    index1 = Ntk_NodeReadFaninIndex( fanout, p1 );
        if ( index1 > index2 )
        {
            i = index1;
            index1 = index2;
            index2 = i;
        }
        pCvr = Ntk_NodeGetFuncCvr( fanout );
        pVm  = Ntk_NodeGetFuncVm( fanout );

 	    // set the values of p1 and p2 to be the same in all cubes 
        nVal = Vm_VarMapReadValuesOutNum( pVm );
        tempppMvc = Cvr_CoverReadIsets( pCvr );
        for ( i=0; i<nVal; i++ )
        {
            if ( tempppMvc[i] != NULL )
                break;
        }
        mask = Mvc_CubeAlloc( tempppMvc[i] );
        Mvc_CubeNBitClean( mask );
        nWords = Mvc_CoverReadWordNum( tempppMvc[i] );
        // get the var map parameters
        nVarsIn   = Vm_VarMapReadVarsInNum( pVm );
        nValuesIn = Vm_VarMapReadValuesInNum( pVm );
        pValues   = Vm_VarMapReadValuesArray( pVm );
        iValues = 0;
        for ( i=0; i<index1; i++ )
        {
            iValues += pValues[i];
        }
        start1 = iValues;
        for ( i=0; i<pValues[index1]; i++ )
        {
            Mvc_CubeBitInsert( mask, iValues + i );
        }
        for ( i=index1; i<index2; i++ )
        {
            iValues += pValues[i];
        }
        start2 = iValues;
        for ( i=0; i<pValues[index2]; i++ )
        {
            Mvc_CubeBitInsert( mask, iValues + i );
        }

        for ( i=0; i<nVal; i++ ) 
        {
            if ( tempppMvc[i]==NULL )
                continue;
            cube_table = pdst_init_table( pdst_cubecmp, pdst_cubehash );
            temp = Mvc_CubeAlloc( tempppMvc[i] );
            Mvc_CoverForEachCube( tempppMvc[i], pCube )
            {
                Mvc_CubeNBitOr( temp, pCube, mask );
                if ( !pdst_lookup( cube_table, temp, &dummy ) )
                {
                    if ( iCost == NTK_COST_LIT )
                    {
                        for ( j = 0; j < pValues[index1]; j++ )
                            if ( !Mvc_CubeBitValue( pCube, start1 + j ) )
                            {
                                for ( v = 0; v < pValues[index2]; v++ )
                                {
                                    if ( !Mvc_CubeBitValue( pCube, start2 + v ) )
                                    {
                                        value++;
                                        break;
                                    }
                                }
                                break;
                            }
                    }
                    pdst_insert( cube_table, temp, NIL(Mvc_Cube_t) );
                    temp = Mvc_CubeAlloc( tempppMvc[i] );
                }
                else 
                {
                    if ( iCost == NTK_COST_LIT )
                    {
                        // count the number of literals in pCube
                        Counter = 0;
                        iValues = 0;
                        for ( j=0; j<nVarsIn; j++ )
                        {
                            fDontCare = 1;
                            for ( v = 0; v < pValues[j]; v++ )
                                if ( !Mvc_CubeBitValue( pCube, iValues + v ) )
                                {
                                    fDontCare = 0;
                                    break;
                                }
                            if ( !fDontCare )
                                Counter++;
                            iValues += pValues[j];
                        }
                        value += Counter;
                    }
                    else
                        value++;
                }
            }
            pdst_free_table( cube_table );
            Mvc_CubeFree( tempppMvc[i], temp );
        }

        // memory free
        for ( i=0; i<nVal; i++ )
        {
            if ( tempppMvc[i] != NULL )
               break;
        }
        Mvc_CubeFree( tempppMvc[i], mask );
    }
    // the cost for the decoder, assume no i-set merge
    i = Ntk_NodeReadValueNum( p1 );
    j  = Ntk_NodeReadValueNum( p2 );
    if ( iCost == NTK_COST_LIT )
        value = value - 2*(i*j-1);
    else
        value = value - i*j + 1;
    return value;
} 
/**Function********************************************************************

  Synopsis       [create a new node with a and b as inputs or merge a and b.]

  Description    [The fanout nodes of a and b are updated.]

  SideEffects    []

******************************************************************************/
Ntk_Node_t * Pd_PairNodes_Simp( Ntk_Node_t * a, Ntk_Node_t * b )
{
    Ntk_Node_t * pDec, * pNodeSdc, * fanout;
    Ntk_Pin_t * pin;
    array_t * lFanin, * lResub;
    array_t * abFanout;
    Ntk_Network_t * pNet;
    int i;
    int timelimit;

    // timeout, we gave this procedure 200 seconds most
    timelimit = (int) ( 200*(float)(CLOCKS_PER_SEC) ) + clock();
    // create a decoder which is not added in network and has no output
    pNet = Ntk_NodeReadNetwork( a );
    pDec = Ntk_NodeCreateDecoder( pNet, a, b );
    Ntk_NodeOrderFanins( pDec );
    Ntk_NetworkAddNode( pNet, pDec, 1 );
      
    // find outputs for new node
    abFanout = array_alloc( Ntk_Node_t *, 0 );
    Ntk_NodeForEachFanout( a, pin, fanout )
    {
        if ( fanout == pDec || Ntk_NodeReadFaninIndex( fanout, b ) == -1 )
            continue;
        array_insert_last( Ntk_Node_t *, abFanout, fanout );
    }
    for ( i=0; i<array_n( abFanout ); i++ )
    {
        if ( clock() > timelimit )
            break;
        fanout = array_fetch( Ntk_Node_t *, abFanout, i );
        // make the sdc node
        lFanin = array_alloc( Ntk_Node_t *, Ntk_NodeReadFaninNum( fanout ) );
        Ntk_NodeReadFanins( fanout, (Ntk_Node_t **)(lFanin->space) );
        lFanin->num = Ntk_NodeReadFaninNum( fanout );
        array_insert_last( Ntk_Node_t *, lFanin, pDec );
        lResub = array_alloc( Ntk_Node_t *, 0 );
        array_insert_last( Ntk_Node_t *, lResub, pDec );

        pNodeSdc = Simp_ComputeSdcResub( fanout, (struct SimpArrayStruct *)lFanin, (struct SimpArrayStruct *)lResub );
        // for debug
        // printf("start to simplify node %s\n", Ntk_NodeGetNamePrintable( fanout ) );
        // simplify with sdc
        Simp_NodeSimplifyDc( fanout, pNodeSdc, SIMP_SNOCOMP, SIMP_CUBE, 1, 0, 0, 0 );
        Ntk_NodeDelete( pNodeSdc );
        array_free( lResub );
        array_free( lFanin );

        // for debugging
        // Ntk_NodePrintCvr( stdout, fanout, 0, 0 );
    }

    array_free( abFanout );
    // Ntk_NetworkSweepIsets( pDec, 0 );
    assert ( Ntk_NetworkCheck( Ntk_NodeReadNetwork(a) ) == 1 );
    if ( Ntk_NodeReadFanoutNum( pDec ) ) {
        _MinimizeValues( pDec );
    }
    else
    {
        // node is useless delete it here.
        Ntk_NetworkDeleteNode( pNet, pDec, 1, 1 );
        pDec = NULL;
    }
    return pDec;
}

/* I add all the static functions because the current version of them 
        are based on mdd and give me stupid results 
*/
static int _MinimizeValues( Ntk_Node_t * pDec )
{
    Ntk_Node_t * fanout;
    Ntk_Pin_t * pin;
    int i, j, k, *mergeValue, iValues, nVal, oVal, index, *pValues, *pVarValues, nomerge;
    int position, positionNew, start, nVarsIn, iVal;
    Vm_VarMap_t *pVm, *pVmNew;
    Vm_Manager_t *pManVm;
    Mvc_Cover_t **mvcIsets, **newIsets;
    Mvc_Cover_t * temp;
    Mvc_Cube_t * pCube, * pCubeNew;
    Cvr_Cover_t * pCvr, * pCvrNew;

    
    // find out any i-set can be merged since they are used together all the time
    // mergeValue contain info that the value belongs to which value group
    // initially we assume all values can be merged
    nVal = Ntk_NodeReadValueNum( pDec );
    mergeValue = ALLOC( int, nVal );
    for ( i=0; i<nVal; i++ )
    {
        mergeValue[i] = 0;
    }
    // iValues contain how many groups we have
    iValues = 1;
    nomerge = 0;

    assert ( Ntk_NetworkCheck( Ntk_NodeReadNetwork(pDec) ) == 1 );

    Ntk_NodeForEachFanout( pDec, pin, fanout )
    {
        pCvr = Ntk_NodeGetFuncCvr( fanout );
        pVm  = Ntk_NodeGetFuncVm( fanout );
        pValues   = Vm_VarMapReadValuesArray( pVm );
        mvcIsets = Cvr_CoverReadIsets( pCvr );
        oVal = Ntk_NodeReadValueNum( fanout );
        
        start = 0;
        index = Ntk_NodeReadFaninIndex( fanout, pDec );
        for ( i=0; i<index; i++ )
        {
            start += pValues[i];
        }
        
        for ( i=0; i<oVal; i++ )
        {
            if ( mvcIsets[i] == NULL ) continue;
            Mvc_CoverForEachCube( mvcIsets[i], pCube )
            {
                for ( j=pValues[index]-1; j>0; j-- )
                {
                    if ( Mvc_CubeBitValue( pCube, start+j ) != Mvc_CubeBitValue( pCube, start+mergeValue[j] ) )
                    {
                        for ( k=mergeValue[j]; k<j; k++ )
                        {
                            if ( mergeValue[k] == mergeValue[j] 
                                && Mvc_CubeBitValue( pCube, start+j ) == Mvc_CubeBitValue( pCube, start+k ) ) 
                            {
                                mergeValue[j] = k;
                                break;
                            }
                        }
                        // did not find any value can merge with it
                        if ( mergeValue[j] != k )
                        {    
                            mergeValue[j] = j;
                            iValues++;
                            if ( iValues == nVal )
                            {
                                // no group possible
                                nomerge = 1;
                                break;
                            }
                        }
                    }
                }
                if ( nomerge ) break;
            }
            if ( nomerge ) break;
        }
        if ( nomerge ) break;
    }

    // mergeValue[j] contains the least value that j can be merged to
    if ( !nomerge )
    {
        // justify mergeValue. this seems inefficient, but take it now
        // old mvsis uses sf_compress
        j = 1;
        for ( i=1; i<nVal; i++ )
        {
            if ( mergeValue[i] > 0 )
            {
                // mergeValue[i] must be equal to i
                for ( k=i+1; k<nVal; k++ )
                {
                    if ( mergeValue[k] == i )
                        mergeValue[k] = -j;
                }
                mergeValue[i] = -j;
                j++;
            }
        }

        // update the output nodes
        Ntk_NodeForEachFanout( pDec, pin, fanout )
        {
            pCvr = Ntk_NodeGetFuncCvr( fanout );
            pVm  = Ntk_NodeGetFuncVm( fanout );
            mvcIsets = Cvr_CoverReadIsets( pCvr );
            pValues   = Vm_VarMapReadValuesArray( pVm );
            nVarsIn   = Vm_VarMapReadVarsInNum( pVm );
            oVal = Ntk_NodeReadValueNum( fanout );
            index = Ntk_NodeReadFaninIndex( fanout, pDec );

            pManVm = Ntk_NetworkReadManVm( Ntk_NodeReadNetwork( fanout ) );
            pVarValues = ALLOC( int, nVarsIn+1 );
            for ( i=0; i<nVarsIn+1; i++ )
            {
                pVarValues[i] = pValues[i];
            }
            pVarValues[index] = iValues;
            pVmNew = Vm_VarMapLookup( pManVm, nVarsIn, 1, pVarValues );
            newIsets = ALLOC( Mvc_Cover_t *, oVal );
            
            for ( iVal=0; iVal<oVal; iVal++ ) {
                if ( mvcIsets[iVal] ) {
                    newIsets[iVal] = Mvc_CoverAlloc( mvcIsets[iVal]->pMem, Vm_VarMapReadValuesInNum( pVmNew ) );
                    // handle each cube
                    Mvc_CoverForEachCube( mvcIsets[iVal], pCube )
                    {
                        pCubeNew = Mvc_CubeAlloc( newIsets[iVal] );
                        Mvc_CubeBitClean( pCubeNew );
                        position = 0;
                        for ( i=0; i<index; i++ )
                        {
                            for ( j=0; j<pVarValues[i]; j++ )
                            {
                                if ( Mvc_CubeBitValue( pCube, position+j ) )
                                    Mvc_CubeBitInsert( pCubeNew, position+j );
                            }
                            position += pVarValues[i];
                        }
                        for ( j=0; j<pValues[index]; j++ )
                        {
                            if ( Mvc_CubeBitValue( pCube, position+j ) )
                                Mvc_CubeBitInsert( pCubeNew, -mergeValue[j] + position );
                        }
                        positionNew = position + pVarValues[index];
                        position += pValues[index];
                        for ( i=index+1; i<nVarsIn; i++ )
                        {
                            for ( j=0; j<pVarValues[i]; j++ )
                            {
                                if ( Mvc_CubeBitValue( pCube, position+j ) )
                                    Mvc_CubeBitInsert( pCubeNew, positionNew+j );
                            }
                            positionNew += pVarValues[i];
                            position += pVarValues[i];
                        }
                        Mvc_CoverAddCubeTail( newIsets[iVal], pCubeNew );
                    }
                }
                else {
                    newIsets[iVal] = NULL;
                }
            }
            // set the new cover to fanout
            pCvrNew = Cvr_CoverCreate( pVmNew, newIsets );
            Ntk_NodeSetFuncVm( fanout, pVmNew );
            Ntk_NodeSetFuncCvr( fanout, pCvrNew );
            FREE( pVarValues );
        }
        // update pDec
        pCvr = Ntk_NodeReadFuncCvr( pDec );
        mvcIsets = Cvr_CoverReadIsets( pCvr );
        pVm = Ntk_NodeReadFuncVm( pDec );
        pValues   = Vm_VarMapReadValuesArray( pVm );
        nVarsIn   = Vm_VarMapReadVarsInNum( pVm );

        pVarValues = ALLOC( int, nVarsIn+1 );
        for ( i=0; i<nVarsIn; i++ )
        {
            pVarValues[i] = pValues[i];
        }
        pVarValues[nVarsIn] = iValues;
        pVmNew = Vm_VarMapLookup( pManVm, nVarsIn, 1, pVarValues );
        FREE( pVarValues );
        newIsets = ALLOC( Mvc_Cover_t *, iValues );
    
        for ( i=0; i<iValues; i++ )
        {
            newIsets[i] = NULL;
        }
        for ( i=0; i<nVal; i++ )
        {
            // no API for this!!
            if ( mvcIsets[i] == NULL ) continue;
            if ( newIsets[-mergeValue[i]] == NULL )
            {
                if ( Cvr_CoverReadDefault( pCvr ) != -mergeValue[i] )
                    newIsets[-mergeValue[i]] = Mvc_CoverDup( mvcIsets[i] );
            }
            else 
            {
                temp = Mvc_CoverBooleanOr( newIsets[-mergeValue[i]], mvcIsets[i] );
                Mvc_CoverFree( newIsets[-mergeValue[i]] );
                newIsets[-mergeValue[i]] = temp;
            }
        }
        pCvrNew = Cvr_CoverCreate( pVmNew, newIsets );
        Cvr_CoverResetDefault( pCvrNew );
        Ntk_NodeSetFuncVm( pDec, pVmNew );
        Ntk_NodeSetFuncCvr( pDec, pCvrNew );
        Ntk_NodeSetValueNum( pDec, iValues );
    }

    FREE( mergeValue );
    return 0;
}
