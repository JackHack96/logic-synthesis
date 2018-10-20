/**CFile****************************************************************

  FileName    [mvrPrint.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Functionality of the package to manipulate MV relations.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - Aug 20, 2000  Last update - Aug 20, 2000
               Ver. 2.0. Started - Oct 09, 2001  Last update - Oct 09, 2001
               Ver. 2.1. Started - Nov 28, 2001  Last update - Nov 28, 2001]
               Ver. 3.0. Started - Mar 11, 2003  Last update - Mar 11, 2003]

  Revision    [$Id: mvrPrint.c,v 1.10 2003/05/27 23:15:19 alanmi Exp $]

***********************************************************************/

#include "mvrInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#if 0

// single line
#define SINGLE_VERTICAL     (char)179
#define SINGLE_HORIZONTAL   (char)196
#define SINGLE_TOP_LEFT     (char)218
#define SINGLE_TOP_RIGHT    (char)191
#define SINGLE_BOT_LEFT     (char)192
#define SINGLE_BOT_RIGHT    (char)217

// double line
#define DOUBLE_VERTICAL     (char)186
#define DOUBLE_HORIZONTAL   (char)205
#define DOUBLE_TOP_LEFT     (char)201
#define DOUBLE_TOP_RIGHT    (char)187
#define DOUBLE_BOT_LEFT     (char)200
#define DOUBLE_BOT_RIGHT    (char)188

// line intersections
#define SINGLES_CROSS       (char)197
#define DOUBLES_CROSS       (char)206
#define S_HOR_CROSS_D_VER   (char)215
#define S_VER_CROSS_D_HOR   (char)216

// single line joining
#define S_JOINS_S_VER_LEFT  (char)180
#define S_JOINS_S_VER_RIGHT (char)195
#define S_JOINS_S_HOR_TOP   (char)193
#define S_JOINS_S_HOR_BOT   (char)194

// double line joining
#define D_JOINS_D_VER_LEFT  (char)185
#define D_JOINS_D_VER_RIGHT (char)204
#define D_JOINS_D_HOR_TOP   (char)202
#define D_JOINS_D_HOR_BOT   (char)203

// single line joining double line
#define S_JOINS_D_VER_LEFT  (char)182
#define S_JOINS_D_VER_RIGHT (char)199
#define S_JOINS_D_HOR_TOP   (char)207
#define S_JOINS_D_HOR_BOT   (char)209

#endif

#if 0 

// single line
#define SINGLE_VERTICAL     (char)'|'
#define SINGLE_HORIZONTAL   (char)'-'
#define SINGLE_TOP_LEFT     (char)'+'
#define SINGLE_TOP_RIGHT    (char)'+'
#define SINGLE_BOT_LEFT     (char)'+'
#define SINGLE_BOT_RIGHT    (char)'+'

// double line
#define DOUBLE_VERTICAL     (char)'H'
#define DOUBLE_HORIZONTAL   (char)'='
#define DOUBLE_TOP_LEFT     (char)'H'
#define DOUBLE_TOP_RIGHT    (char)'H'
#define DOUBLE_BOT_LEFT     (char)'H'
#define DOUBLE_BOT_RIGHT    (char)'H'

// line intersections
#define SINGLES_CROSS       (char)'+'
#define DOUBLES_CROSS       (char)'H'
#define S_HOR_CROSS_D_VER   (char)'H'
#define S_VER_CROSS_D_HOR   (char)'+'

// single line joining
#define S_JOINS_S_VER_LEFT  (char)'+'
#define S_JOINS_S_VER_RIGHT (char)'+'
#define S_JOINS_S_HOR_TOP   (char)'+'
#define S_JOINS_S_HOR_BOT   (char)'+'

// double line joining
#define D_JOINS_D_VER_LEFT  (char)'H'
#define D_JOINS_D_VER_RIGHT (char)'H'
#define D_JOINS_D_HOR_TOP   (char)'H'
#define D_JOINS_D_HOR_BOT   (char)'H'

// single line joining double line
#define S_JOINS_D_VER_LEFT  (char)'H'
#define S_JOINS_D_VER_RIGHT (char)'H'
#define S_JOINS_D_HOR_TOP   (char)'+'
#define S_JOINS_D_HOR_BOT   (char)'+'

#endif


// single line
#define SINGLE_VERTICAL     (char)'|'
#define SINGLE_HORIZONTAL   (char)'-'
#define SINGLE_TOP_LEFT     (char)'+'
#define SINGLE_TOP_RIGHT    (char)'+'
#define SINGLE_BOT_LEFT     (char)'+'
#define SINGLE_BOT_RIGHT    (char)'+'

// double line
#define DOUBLE_VERTICAL     (char)'|'
#define DOUBLE_HORIZONTAL   (char)'-'
#define DOUBLE_TOP_LEFT     (char)'+'
#define DOUBLE_TOP_RIGHT    (char)'+'
#define DOUBLE_BOT_LEFT     (char)'+'
#define DOUBLE_BOT_RIGHT    (char)'+'

// line intersections
#define SINGLES_CROSS       (char)'+'
#define DOUBLES_CROSS       (char)'+'
#define S_HOR_CROSS_D_VER   (char)'+'
#define S_VER_CROSS_D_HOR   (char)'+'

// single line joining
#define S_JOINS_S_VER_LEFT  (char)'+'
#define S_JOINS_S_VER_RIGHT (char)'+'
#define S_JOINS_S_HOR_TOP   (char)'+'
#define S_JOINS_S_HOR_BOT   (char)'+'

// double line joining
#define D_JOINS_D_VER_LEFT  (char)'+'
#define D_JOINS_D_VER_RIGHT (char)'+'
#define D_JOINS_D_HOR_TOP   (char)'+'
#define D_JOINS_D_HOR_BOT   (char)'+'

// single line joining double line
#define S_JOINS_D_VER_LEFT  (char)'+'
#define S_JOINS_D_VER_RIGHT (char)'+'
#define S_JOINS_D_HOR_TOP   (char)'+'
#define S_JOINS_D_HOR_BOT   (char)'+'



// other symbols
#define UNDERSCORE          (char)95
//#define SYMBOL_ZERO       (char)248   // degree sign
#define SYMBOL_ZERO         (char)'o'
#define SYMBOL_ONE          (char)'1'
#define SYMBOL_DC           (char)'-'
#define SYMBOL_OVERLAP      (char)'?'

// full cells and half cells
#define CELL_FREE           (char)32
#define CELL_FULL           (char)219
#define HALF_UPPER          (char)223
#define HALF_LOWER          (char)220
#define HALF_LEFT           (char)221
#define HALF_RIGHT          (char)222

// the maximum number of variables in k-maps
#define MAXVARS   14
#define MAXCELLS  1000
#define MAXVALUES 100

static int Mvr_RelationFindPartition( Mvr_Relation_t * pMvr );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Prints the Karnough-map of the relation.]

  Description [Prints the relation. Uses the real variable names 
  if the node is given.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvr_RelationPrintKmap( FILE * pFile, Mvr_Relation_t * pMvr, char ** pVarNames )
{
	Vm_VarMap_t * pVm;
	DdManager * dd = pMvr->pMan->pDdLoc;
	DdNode * bCubeHor, * bCube, * bValueSet;
	DdNode * bCubeVer, * bTemp;
	DdNode ** pbCubes;
	DdNode * bCubeInputs;

	char Buffer[10];
	int fUserNames;

	int * pVarMap;
	int nVarsVer, nVarsHor;
	int nCellsVer, nCellsHor;
	int nSkipSpaces;
	int nOutputValues;
	int nInputs;

	int	CellWidth, PartSkip, IntervalSkip;
	int Divisor, DivisorPrev;
	int VertProd, VertRemain;
	int iValue;

	int VertVarValues[MAXVALUES];
	int HoriVarValues[MAXVALUES];
	int VertLineType[MAXCELLS];
	int HoriLineType[MAXCELLS];
	int HoriInterval;
	int VertInterval;
	int a, b, w, p, s, v, n, h, d, i;

	//////////////////////////////////////////////////////////////////////
	// check the relation's support
	Mvr_RelationCheckSupport( pMvr );
//PRB( pMvr->pMan->pDdLoc, pMvr->bRel );

	// get the MV var map
	pVm = Mvr_RelationReadVm( pMvr );
    if ( pVm->nVarsIn > MAXVARS )
    {
        fprintf( pFile, "Cannot print the K-map of the MV relation with %d variables.\n", pVm->nVarsIn );
        return;
    }

    // check if the relation is all-binary
    // in this case, we print the binary K-map
    if ( Vm_VarMapIsBinary(pVm) && Vm_VarMapReadVarsInNum(pVm) )
    {
        DdNode * bVar, * bOnSet, * bOffSet, * bInter, * bTemp;
        DdNode * bXVars[MAXVARS];

        // cofactor the relation
        bVar = dd->vars[ pMvr->pVmx->pBitsOrder[pVm->nVarsIn] ];
        bOffSet = Cudd_Cofactor( dd, pMvr->bRel, Cudd_Not(bVar) );  Cudd_Ref( bOffSet );
        bOnSet  = Cudd_Cofactor( dd, pMvr->bRel, bVar );            Cudd_Ref( bOnSet );

        // subtract the intersection from both cofactors
        bInter  = Cudd_bddAnd( dd, bOffSet, bOnSet );               Cudd_Ref( bInter );
        bOffSet = Cudd_bddAnd( dd, bTemp = bOffSet, Cudd_Not(bInter) );       Cudd_Ref( bOffSet );
        Cudd_RecursiveDeref( dd, bTemp );
        bOnSet = Cudd_bddAnd( dd, bTemp = bOnSet, Cudd_Not(bInter) );         Cudd_Ref( bOnSet );
        Cudd_RecursiveDeref( dd, bTemp );
        Cudd_RecursiveDeref( dd, bInter );

        for ( i = 0; i < pVm->nVarsIn; i++ )
            bXVars[i] = dd->vars[ pMvr->pVmx->pBitsOrder[i] ];
        Extra_PrintKMap( pFile, dd, bOnSet, bOffSet, pVm->nVarsIn, bXVars, -1, pVarNames );
        Cudd_RecursiveDeref( dd, bOffSet );
        Cudd_RecursiveDeref( dd, bOnSet );
        return;
    }

	// find the partitioning of variables into horizontal and vertical
	nVarsHor = Mvr_RelationFindPartition( pMvr );
	nVarsVer = pVm->nVarsIn - nVarsHor;

	// get the total number of cells vertically and horizontally
	nCellsHor = 1;
	for ( i = 0; i < nVarsHor; i++ )
		nCellsHor *= pVm->pValues[i];

	nCellsVer = 1;
	for ( i = nVarsHor; i < pVm->nVarsIn; i++ )
		nCellsVer *= pVm->pValues[i];

	// check if the number of cells is okay
	if ( nCellsHor > MAXCELLS-2 || nCellsVer > MAXCELLS-2 )
	{
		fprintf( pFile, "Mvr_RelationPrintKmap(): Cannot print K-map because of large total number of input values\n" );
		return;
	}

	// get other parameters
	nInputs       = pVm->nVarsIn;
	nOutputValues = Vm_VarMapReadValuesOutput(pVm);
	CellWidth     = nOutputValues + 3;
	PartSkip      = CellWidth/2;  // the number of spaces to skip before the first digit
	IntervalSkip  = CellWidth;    // the number of spaces to skip between the cells
	nSkipSpaces   = nVarsVer + 1;

	// get the input names if they are not given
	fUserNames = 1;
	if ( pVarNames == NULL )
	{
		fUserNames = 0;
		pVarNames = ALLOC( char *, nInputs + 1 );
		for ( i = 0; i <= nInputs; i++ )
		{
			sprintf( Buffer, "v%d", i );
			pVarNames[i] = util_strsav( Buffer );
		}
	}

	// get var map (currently, not used)
	pVarMap = ALLOC( int, nInputs );
	for ( i = 0; i < nInputs; i++ )
		pVarMap[i] = i;


	////////////////////////////////////////////////////////////////////
	// decide where the line will be bold and where it will be normal
	// the valuedness of the last var in the horizontal vars group
	if ( nVarsHor == 0 )
		HoriInterval = 1;
	else
		HoriInterval = pVm->pValues[ pVarMap[nVarsHor-1] ];
	for ( a = 0; a <= nCellsHor; a++ )
		if ( a % HoriInterval == 0 ) // should be bold
			HoriLineType[a] = 1;
		else // should be normal
			HoriLineType[a] = 0;
    if ( nVarsVer == 0 )
        VertInterval = 1;
    else
    {
	    // the valuedness of the last var in the vertical vars group
	    VertInterval = pVm->pValues[ pVarMap[nInputs-1] ];
	    for ( b = 0; b <= nCellsVer; b++ )
		    if ( b % VertInterval == 0 ) // should be bold
			    VertLineType[b] = 1;
		    else // should be normal
			    VertLineType[b] = 0;
    }

	////////////////////////////////////////////////////////////////////
	// print the title line
	fprintf( pFile, "MV Relation of Node <%s>.\n", pVarNames[nInputs] );

	////////////////////////////////////////////////////////////////////
	// print variable names
//	fprintf( pFile, "\n" );
	for ( w = 0; w < nVarsVer; w++ )
		fprintf( pFile, "%s ", pVarNames[ pVarMap[nVarsHor + w] ] );
	fprintf( pFile, "\\ " );
	for ( w = 0; w < nVarsHor; w++ )
		fprintf( pFile, "%s ", pVarNames[ pVarMap[w] ] );
	fprintf( pFile, "\n" );

	////////////////////////////////////////////////////////////////////
	// print horizontal digits
    if ( nVarsHor )
    {
	    Divisor = nCellsHor/pVm->pValues[ pVarMap[0] ];
	    DivisorPrev = -1;

	    for ( d = 0; d < nVarsHor; d++ )
	    {
		    for ( p = 0; p < nSkipSpaces+PartSkip; p++, printf(" ") );

		    for ( n = 0; n < nCellsHor; n++ )
		    {
			    if ( DivisorPrev == -1 )
				    fprintf( pFile, "%d", n/Divisor );
			    else
				    fprintf( pFile, "%d", (n%DivisorPrev)/Divisor );

			    for ( p = 0; p < IntervalSkip-1; p++, printf(" ") );
		    }

		    DivisorPrev = Divisor;
		    if ( d < nVarsHor-1 )
			    Divisor /= pVm->pValues[ pVarMap[1+d] ];
		    fprintf( pFile, "\n" );
	    }
    }

	////////////////////////////////////////////////////////////////////
	// print the upper line
	for ( p = 0; p < nSkipSpaces; p++, printf(" ") );
	fprintf( pFile, "%c", DOUBLE_TOP_LEFT );
	for ( s = 0; s < nCellsHor; s++ )
	{
		for ( p = 0; p < IntervalSkip-1; p++, fprintf( pFile, "%c", DOUBLE_HORIZONTAL ) );

		if ( s != nCellsHor-1 )
			if ( HoriLineType[s+1] )
				fprintf( pFile, "%c", D_JOINS_D_HOR_BOT );
			else
				fprintf( pFile, "%c", S_JOINS_D_HOR_BOT );
	}
	fprintf( pFile, "%c", DOUBLE_TOP_RIGHT );
	fprintf( pFile, "\n" );

	////////////////////////////////////////////////////////////////////
	// print the map
	pbCubes = Vmx_VarMapEncodeMap( dd, pMvr->pVmx );
	bCubeInputs = Vmx_VarMapCharCubeInput( dd, pMvr->pVmx );  Cudd_Ref( bCubeInputs );

	for ( v = 0; v < nCellsVer; v++ )
	{
        if ( nVarsVer )
        {
		    // print horizontal digits
		    VertProd = nCellsVer/pVm->pValues[ pVarMap[nVarsHor] ];
		    VertRemain = v;

		    for ( n = 0; n < nVarsVer; n++ )
		    {
			    VertVarValues[n] = VertRemain / VertProd;
			    VertRemain       = VertRemain % VertProd;

			    fprintf( pFile, "%d", VertVarValues[n] );
			    if ( n < nVarsVer-1 )
				    VertProd /= pVm->pValues[ pVarMap[nVarsHor+n+1] ];
		    }
        }
	    fprintf( pFile, " ");

		// find vertical cube
		bCubeVer = b1;  Cudd_Ref( bCubeVer );
		for ( n = 0; n < nVarsVer; n++ )
		{
			iValue = pVm->pValuesFirst[ pVarMap[nVarsHor+n] ] + VertVarValues[n];
			bCubeVer = Cudd_bddAnd( dd, bTemp = bCubeVer, pbCubes[iValue] );  Cudd_Ref( bCubeVer );
			Cudd_RecursiveDeref( dd, bTemp );
		}

		// print text line
		fprintf( pFile, "%c", DOUBLE_VERTICAL );
		for ( h = 0; h < nCellsHor; h++ )
		{
            if ( nVarsHor )
            {
			    Divisor = nCellsHor/pVm->pValues[ pVarMap[0] ];
			    DivisorPrev = -1;
			    for ( d = 0; d < nVarsHor; d++ )
			    {
				    if ( DivisorPrev == -1 )
					    HoriVarValues[d] = h/Divisor;
				    else
					    HoriVarValues[d] = (h%DivisorPrev)/Divisor;
				    DivisorPrev = Divisor;
				    if ( d < nVarsHor-1 )
					    Divisor /= pVm->pValues[ pVarMap[1+d] ];
			    }
			    // find vertical cube
			    bCubeHor = b1;  Cudd_Ref( bCubeHor );
			    for ( n = 0; n < nVarsHor; n++ )
			    {
				    iValue = pVm->pValuesFirst[ pVarMap[n] ] + HoriVarValues[n];
				    bCubeHor = Cudd_bddAnd( dd, bTemp = bCubeHor, pbCubes[iValue] ); Cudd_Ref( bCubeHor );
				    Cudd_RecursiveDeref( dd, bTemp );
			    }
            }
            else
            {
   			    bCubeHor = b1;  Cudd_Ref( bCubeHor );
            }

			bCube     = Cudd_bddAnd( dd, bCubeHor, bCubeVer );   Cudd_Ref( bCube );
			bValueSet = Cudd_bddAndAbstract( dd, pMvr->bRel, bCube, bCubeInputs );  Cudd_Ref( bValueSet );
			Cudd_RecursiveDeref( dd, bCube );
			Cudd_RecursiveDeref( dd, bCubeHor );
//PRB( dd, bValueSet );

			// go through all possible values of this set
			fprintf( pFile, " ");
			for ( i = 0; i < nOutputValues; i++ )
			{
				iValue = pVm->pValuesFirst[nInputs] + i;
				if ( Cudd_bddLeq( dd, pbCubes[iValue], bValueSet ) )
					fprintf( pFile, "%d", i % 10 );
				else
					fprintf( pFile, "%c", '-' );
			}
			Cudd_RecursiveDeref( dd, bValueSet );

			fprintf( pFile, " ");
			if ( h != nCellsHor-1 )
				if ( HoriLineType[h+1] )
					fprintf( pFile, "%c", DOUBLE_VERTICAL );
				else
					fprintf( pFile, "%c", SINGLE_VERTICAL );
		}
		fprintf( pFile, "%c", DOUBLE_VERTICAL );
		fprintf( pFile, "\n" );
		Cudd_RecursiveDeref( dd, bCubeVer );

		// print separator line
		if ( v != nCellsVer-1 )
		{
			for ( p = 0; p < nSkipSpaces; p++, printf(" ") );
			if ( VertLineType[v+1] )
			{
				fprintf( pFile, "%c", D_JOINS_D_VER_RIGHT );
				for ( s = 0; s < nCellsHor; s++ )
				{
					for ( p = 0; p < IntervalSkip-1; p++, fprintf( pFile, "%c", DOUBLE_HORIZONTAL ) );
					if ( s != nCellsHor-1 )
						if ( HoriLineType[s+1] )
							fprintf( pFile, "%c", DOUBLES_CROSS );
						else
							fprintf( pFile, "%c", S_VER_CROSS_D_HOR );
				}
				fprintf( pFile, "%c", D_JOINS_D_VER_LEFT );
			}
			else
			{
				fprintf( pFile, "%c", S_JOINS_D_VER_RIGHT );
				for ( s = 0; s < nCellsHor; s++ )
				{
					for ( p = 0; p < IntervalSkip-1; p++, fprintf( pFile, "%c", SINGLE_HORIZONTAL ) );
					if ( s != nCellsHor-1 )
						if ( HoriLineType[s+1] )
							fprintf( pFile, "%c", S_HOR_CROSS_D_VER );
						else
							fprintf( pFile, "%c", SINGLES_CROSS );
				}
				fprintf( pFile, "%c", S_JOINS_D_VER_LEFT );
			}
			fprintf( pFile, "\n" );
		}
	}
	Vmx_VarMapEncodeDeref( dd, pMvr->pVmx, pbCubes );
	Cudd_RecursiveDeref( dd, bCubeInputs );
	
	////////////////////////////////////////////////////////////////////
	// print the lower line
	for ( p = 0; p < nSkipSpaces; p++, printf(" ") );
	fprintf( pFile, "%c", DOUBLE_BOT_LEFT );
	for ( s = 0; s < nCellsHor; s++ )
	{
		for ( p = 0; p < IntervalSkip-1; p++, fprintf( pFile, "%c", DOUBLE_HORIZONTAL ) );
		if ( s != nCellsHor-1 )
			if ( HoriLineType[s+1] )
				fprintf( pFile, "%c", D_JOINS_D_HOR_TOP );
			else
				fprintf( pFile, "%c", S_JOINS_D_HOR_TOP );
	}
	fprintf( pFile, "%c", DOUBLE_BOT_RIGHT );
	fprintf( pFile, "\n" );

	////////////////////////////////////////////////////////////////////
	// deref 
	FREE( pVarMap );
	if ( !fUserNames )
	{
		for ( i = 0; i <= pVm->nVarsIn; i++ )
			FREE( pVarNames[i] );
		FREE( pVarNames );
	}
}

/**Function*************************************************************

  Synopsis    [Create a good partition of MV variables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mvr_RelationFindPartition( Mvr_Relation_t * pMvr )
{
	Vm_VarMap_t * pVm;
	int nValueProdHor;
	int nValueProd;
    int nOutputValues;
    int HorLimit;
	int i;

    // the number of output values
    nOutputValues = Vm_VarMapReadValuesOutput( pMvr->pVmx->pVm );
    // the limit on the number of horizontal cells
    HorLimit = 80 / (nOutputValues + 3) - 1;

	pVm = Mvr_RelationReadVm( pMvr );
	assert( pVm->nVarsIn >= 0 );
	if ( pVm->nVarsIn == 0 )
		return 0;
	if ( pVm->nVarsIn < 3 )
		return 1;

	// get the product of all values
	nValueProd = 1;
	for ( i = 0; i < pVm->nVarsIn; i++ )
		nValueProd *= pVm->pValues[i];

	nValueProdHor = 1;
	for ( i = 0; i < pVm->nVarsIn; i++ )
	{
		nValueProdHor *= pVm->pValues[i];
		if ( nValueProdHor > HorLimit )
			return i;
		if ( nValueProdHor > 2*(nValueProd / nValueProdHor) )
			return i;
		if ( i == pVm->nVarsIn - 1 )
			return i;
	}
	return -1;
}



////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


