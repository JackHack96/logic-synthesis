/**CFile****************************************************************

  FileName    [encCode.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    []

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: encCode.c,v 1.3 2003/05/27 23:15:30 alanmi Exp $]

***********************************************************************/


#include "encInt.h"
#include "ntkInt.h"

/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/
static Enc_Code_t * EncEncode( Ntk_Node_t * pNode, Enc_Meth_t encMeth );
static void EncMarkCodesUsed( Enc_Code_t * pCode );
static int EncNewMinterm( int * pfMarker, int markerSize, int offset, int num2s, int base );


/**AutomaticEnd***************************************************************/


/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
Enc_Code_t *
Enc_NodeComputeCode( 
    Ntk_Network_t * pNet, 
    Ntk_Node_t * pNode, 
    Enc_Info_t * pInfo)
{
    Enc_Code_t *pCode;
    Enc_Meth_t encMeth;

    if (pInfo->method == ENC_INOUT){
        int nSize, nValue, nDc;

        nValue = Ntk_NodeReadValueNum(pNode);
	nSize  = Enc_CodeLength(nValue);
	nSize  = Enc_IthPowerOf2(nSize);
	nDc = nSize - nValue;

        if(Ntk_NodeReadFanoutNum(pNode) >= nDc)
	  encMeth = ENC_IN;
	else
	  encMeth = ENC_OUT;

    } else {
        encMeth = pInfo->method;
    }

    pCode = EncEncode(pNode, encMeth);
    
    return pCode;
}


/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    []
  
  Description []

  SideEffects []
   
  SeeAlso     []

******************************************************************************/
static Enc_Code_t *
EncEncode( 
    Ntk_Node_t *pNode,
    Enc_Meth_t encMeth)
{
    int i, vec, value, maxValue;
    bool saturated, dontcare;
    Enc_Code_t *pCode;

    pCode = ALLOC(Enc_Code_t, 1);
    pCode->numValues    = Ntk_NodeReadValueNum(pNode);
    pCode->defaultValue = (Ntk_NodeReadFuncCvr(pNode)==NULL)?-1:Ntk_NodeReadDefault(pNode);
    pCode->codeLength   = Enc_CodeLength(pCode->numValues);
    pCode->codeArray    = ALLOC(int, pCode->numValues * pCode->codeLength);
    memset(pCode->codeArray, 0, 
           pCode->numValues * pCode->codeLength * sizeof(int));
    pCode->numCodes     = Enc_IthPowerOf2(pCode->codeLength);
    pCode->assignArray  = ALLOC(int, pCode->numCodes);

    assert( pCode->numValues > 2 );

    maxValue = pCode->numValues - 1;
    value = 0;
    for(vec = (pCode->defaultValue == -1)?0:1; vec < pCode->numValues; vec++) {
        if(value == pCode->defaultValue){
	    vec--;
	} else {
            saturated = TRUE;
            for(i = pCode->codeLength-1; i >= 0; i--){
                dontcare = FALSE;
                if(encMeth == ENC_OUT && saturated == TRUE && ((vec>>i)%2 == (maxValue>>i)%2)){
  	            dontcare = ((vec>>i)%2)?FALSE:TRUE;
                }
                else {
	            saturated = FALSE;
	            dontcare  = FALSE;
                }
                pCode->codeArray[value * pCode->codeLength + i] = 
	        (value==pCode->defaultValue)?0:((dontcare)?2:(vec>>i)%2);
            }
        }
	value++;
    }

    EncMarkCodesUsed(pCode);
        
    return pCode;
}


/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    []

  Description [This function fills in the details of assignArray, which says
  the ith vector spaned by the binary bits is assigned to value j if
  assignArray[i]=j.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
void
EncMarkCodesUsed(
    Enc_Code_t * pCode)
{
    int i, v, mintermBase, num2s, nMinterms;
    bool * pfMarker;  // indicate which bits are "2"
    
    pfMarker = ALLOC(int, pCode->codeLength);
    
    for (i = 0; i < pCode->numCodes; i++)
        pCode->assignArray[i] = -1;
	
    for (v = 0; v < pCode->numValues; v++) {
	mintermBase = 0;
	num2s = 0;
	
	//memset(pfMarker, 0, pCode->numValues * sizeof(int));
        for (i = 0; i < pCode->codeLength; i++)
	  pfMarker[i] = 0;

	for (i = 0; i < pCode->codeLength; i++) {
	    switch (pCode->codeArray[v*pCode->codeLength + i]){
	        case 0:
		    break;
		case 1:
                    mintermBase |= (1<<i);   
		    break;
		case 2:
		    num2s++;
		    pfMarker[i] = TRUE;
		    break;
		default:
		    fprintf(stdout, "ENC error.\n");
		    exit(1);
	    }
        }
	nMinterms = Enc_IthPowerOf2(num2s);
	for (i = 0; i < nMinterms; i++){
	    int mintermNew = EncNewMinterm(pfMarker, pCode->codeLength, 
	                                   i, num2s, mintermBase);
	    pCode->assignArray[mintermNew] = v;
	}
    }
    FREE(pfMarker);
}    


/**Function********************************************************************

  Synopsis    []
  
  Description []

  SideEffects []
   
  SeeAlso     []

******************************************************************************/
static int
EncNewMinterm(
    int *pfMarker,
    int markerSize,
    int offset,
    int num2s,
    int base)
{
    int i, shift, minterm;
    
    minterm = base;
    for (i = 0; i < num2s; i++) {
        if ( (offset>>i)%2 ) {
	    int counter = 0;
            for (shift = 0; shift < markerSize; shift++) {
	        if (pfMarker[shift]) {
		    if (counter == i)
		        break;
		    counter++;
		}
	    }
	    minterm |= (1<<shift);
	}  
    }
    return minterm;
}


