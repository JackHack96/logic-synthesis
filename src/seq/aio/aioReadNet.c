/**CFile****************************************************************

  FileName    [ioReadNet.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Creates the network structure from the node fanin/fanout information.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: aioReadNet.c,v 1.2 2004/03/10 03:18:46 alanmi Exp $]

***********************************************************************/

#include "aioInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////
extern int Aio_AutoReadNextStateTable( Aio_Read_t * p, Aut_Auto_t * pAut );

static int  Aio_ReadNetworkName( Aio_Read_t * p, Aut_Auto_t * pAut );
static int  Aio_ReadNetworkCis( Aio_Read_t * p, int Line, Aut_Var_t ** pVars, int * pnVars );
static int Aio_AutoReadResetTable( Aio_Read_t * p, Aut_Auto_t * pAut );
static int Aio_AutoReadOutputTable( Aio_Read_t * p, Aut_Auto_t * pAut );

static Aut_Var_t ** Aio_DeriveVars( Aio_Read_t * p, int * nVars );
static Vmx_VarMap_t * Aio_DeriveVmx( Aio_Read_t * p, Aut_Var_t ** pVars, int nVars );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Compiles the automaton from information derived by preparsing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aut_Auto_t * Aio_ReadNetworkStructure( Aio_Read_t * p )
{
    Vmx_VarMap_t * pVmx;
    Aut_Auto_t * pAut;
    Aut_Var_t ** pVars;
    int nVars;

    // derive the information about variables
    pVars = Aio_DeriveVars( p, &nVars );

    // create the variable map
    pVmx = Aio_DeriveVmx( p, pVars, nVars );

    // allocate the empty automaton
    pAut = Aut_AutoAlloc( pVmx, p->pManDd );
    Aut_AutoSetVars( pAut, pVars );
    Aut_AutoSetVarNum( pAut, nVars );
    p->pAut = pAut;
    
    // read the network name
    if ( Aio_ReadNetworkName( p, pAut ) )
        goto failure;

    if( p->fEmpty )
        return pAut;

    // read the latch reset table
    if ( Aio_AutoReadResetTable( p, pAut ) )
        goto failure;

    // read the output node
    if ( Aio_AutoReadOutputTable( p, pAut ) )
        goto failure;

    // read the next state node
    if ( Aio_AutoReadNextStateTable( p, pAut ) )
        goto failure;

    return pAut;

failure:
    Aut_AutoFree( pAut );
    return NULL;
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aio_ReadNetworkName( Aio_Read_t * p, Aut_Auto_t * pAut )
{
    char * pTemp;
    assert( strncmp( p->pLines[p->LineModel], ".model", 6 ) == 0 );
    // skip the ".model" word and get the network name
    pTemp = strtok( p->pLines[p->LineModel] + 7, " \t" );
    // assign the name
    Aut_AutoSetName( pAut, Extra_FileNameGeneric(pTemp) );
    // get the next token
    pTemp = strtok( NULL, " \t" );
    if ( pTemp != NULL )
    {
        p->LineCur = p->LineModel;
        sprintf( p->sError, "Trailing symbols on .model line (%s).", pTemp );
        Aio_ReadPrintErrorMessage( p );
        return 1;
    }
    return 0;
}


/**Function*************************************************************

  Synopsis    [Reads the reset table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aio_AutoReadResetTable( Aio_Read_t * p, Aut_Auto_t * pAut )
{
    Aio_Latch_t * pIoLatch;
    Aio_Node_t * pIoNode;
    char * pLine, * pTemp;
    int Latch = 0;
    Aio_Var_t * pVar;
    Aut_State_t * pState;
    char * pName;
    int n;
    Aut_State_t ** pInit;

    // get the IO latch pointer
    pIoLatch = p->pAioLatches + Latch;
    pIoNode = &pIoLatch->AioNode;

    // set the current parsing line
    p->LineCur = pIoNode->LineNames;

    // read the reset table
    // get the .res line of the latch
    pLine = p->pLines[ pIoNode->LineNames ];
    // get the first word on the line
    pTemp = strtok( pLine, " \t" );
    assert( strncmp( pTemp, ".r", 2 ) == 0 );
    // read the fanins names, one by one
    pTemp = strtok( NULL, " \t" );
    // make sure the last signal is the output
    assert( strcmp( pTemp, pIoLatch->pOutputName ) == 0 );


    ///////////////////////////////////////////////
    // create the table mapping state names into their number
    pVar = NULL;
    if ( !st_lookup( p->tName2Var, pTemp, (char **)&pVar ) ) 
    {
        p->LineCur = 0;
        sprintf( p->sError, "There is no .mv line for current state variable %s.", pTemp );
        Aio_ReadPrintErrorMessage( p );
        return 1;
    }
    for ( n = 0; n < pVar->nValues; n++ )
    {
        pState = Aut_AutoFindState( pAut, pVar->pValueNames[n] );
        if ( pState )
        {
            p->LineCur = 0;
            sprintf( p->sError, "The state name %s is repeated on the .mv line.", pVar->pValueNames[n] );
            Aio_ReadPrintErrorMessage( p );
            return 1;
        }
        pState = Aut_StateAlloc( pAut );
        pName = Aut_AutoRegisterName( pAut, pVar->pValueNames[n] );
        Aut_StateSetName( pState, pName );
        Aut_AutoStateAddLast( pState );
    }
    ///////////////////////////////////////////////

    // read the reset table
    if ( pIoNode->nLines )
    {
        pTemp = p->pLines[pIoNode->LineTable];
        while ( *pTemp == ' ' )
            pTemp++;
        if ( *pTemp == '(' ) // add many init state support!!!
            pTemp = strtok( pTemp + 1, " \n\t)" );
        else
            pTemp = strtok( pTemp, " \n\t" );

//        if ( !st_lookup( p->tName2Num, pTemp, (char **)&pState ) )
        pState = Aut_AutoFindState( pAut, pTemp );
        if ( pState == NULL )
        {
            p->LineCur = 0;
            sprintf( p->sError, "The initial state %s in the .reset line is not in .mv line.", pTemp );
            Aio_ReadPrintErrorMessage( p );
            return 1;
        }
        Aut_AutoSetInitNum( pAut, 1 );
        pInit = Aut_AutoReadInitAll( pAut );
        pInit[0] = pState;
    }
    return 0;
}


/**Function*************************************************************

  Synopsis    [Reads the output table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aio_AutoReadOutputTable( Aio_Read_t * p, Aut_Auto_t * pAut )
{
    Aio_Node_t * pIoNode1;
    Aio_Node_t * pIoNode2;
    char * pTemp;
    Aut_State_t * pState;

    // set the current parsing line
    pIoNode1 = p->pAioNodes + 0;
    pIoNode2 = p->pAioNodes + 1;

    if ( pIoNode1->nValues == 2 && pIoNode2->nValues != 2 )
    {
        p->pIoNodeO = pIoNode1;
        p->pIoNodeN = pIoNode2;
    }
    else if ( pIoNode2->nValues == 2 && pIoNode1->nValues != 2 )
    {
        p->pIoNodeO = pIoNode2;
        p->pIoNodeN = pIoNode1;
    }
    else if ( strcmp(pIoNode2->pOutput, "ns") == 0 || strcmp(pIoNode2->pOutput, "NS") == 0 )
    {
        p->pIoNodeO = pIoNode1;
        p->pIoNodeN = pIoNode2;
    }
    else if ( strcmp(pIoNode1->pOutput, "ns") == 0 || strcmp(pIoNode1->pOutput, "NS") == 0 )
    {
        p->pIoNodeO = pIoNode2;
        p->pIoNodeN = pIoNode1;
    }
    else
    {
        p->LineCur = 0;
        sprintf( p->sError, "Cannot detect the output node." );
        Aio_ReadPrintErrorMessage( p );
        return 1;
    }

    if ( p->pIoNodeO->nLines )
    {
        // check the case when the node is a constant
        pTemp = p->pLines[ p->pIoNodeO->LineNames ];
        pTemp = strtok( pTemp, " \n\t" );
        pTemp = strtok( NULL, " \n\t" ); 
        pTemp = strtok( NULL, " \n\t" ); 
        if ( pTemp == NULL )
        {   // this is the constant node
            pTemp = p->pLines[p->pIoNodeO->LineTable];
            while ( *pTemp == ' ' )
                pTemp++;

            if ( *pTemp == '1' || p->pIoNodeO->Default == 1 )
            {
                Aut_AutoForEachState( pAut, pState )
                    Aut_StateSetAcc( pState, 1 );
            }
        }
        else
        {

            int FlagValue;
            if ( p->pIoNodeO->Default == 1 )
            {
                Aut_AutoForEachState( pAut, pState )
                    Aut_StateSetAcc( pState, 1 );
                FlagValue = 0;
            }
            else
            {
                Aut_AutoForEachState( pAut, pState )
                    Aut_StateSetAcc( pState, 0 );
                FlagValue = 1;
            }

            pTemp = p->pLines[p->pIoNodeO->LineTable];
            while ( *pTemp == ' ' )
                pTemp++;
            if ( *pTemp == '(' ) // add many init state support!!!
            {
                int fBreakFlag = 0;

                pTemp = strtok( pTemp + 1, " \n\t,)" );
                while ( pTemp )
                {
            //        if ( !st_lookup( p->tName2Num, pTemp, (char **)&pState ) )
                    pState = Aut_AutoFindState( pAut, pTemp );
                    if ( pState == NULL )
                    {
                        p->LineCur = 0;
                        sprintf( p->sError, "The state %s of the output node is not in .mv line.", pTemp );
                        Aio_ReadPrintErrorMessage( p );
                        return 1;
                    }
                    Aut_StateSetAcc( pState, FlagValue );

                    if ( fBreakFlag )
                        break;

                    pTemp = strtok( NULL, " \n\t," );

                    if ( pTemp[ strlen(pTemp) - 1 ] == ')' )
                    {
                        pTemp[ strlen(pTemp) - 1 ] = 0; 
                        fBreakFlag = 1; 
                    }
                }
            }
            else
            {
                pTemp = strtok( pTemp, " \n\t" );
        //        if ( !st_lookup( p->tName2Num, pTemp, (char **)&pState ) )
                pState = Aut_AutoFindState( pAut, pTemp );
                if ( pState == NULL )
                {
                    p->LineCur = 0;
                    sprintf( p->sError, "The state %s of the output node is not in .mv line.", pTemp );
                    Aio_ReadPrintErrorMessage( p );
                    return 1;
                }
                Aut_StateSetAcc( pState, FlagValue );
            }
        }
    }
    else
    {
        if ( p->pIoNodeO->LineTable > 0 )
        {
            pTemp = p->pLines[p->pIoNodeO->LineTable];
            while ( *pTemp == ' ' )
                pTemp++;
        }
        else 
            pTemp = NULL;

        if ( pTemp && *pTemp == '1' || p->pIoNodeO->Default == 1 )
        {
            Aut_AutoForEachState( pAut, pState )
                Aut_StateSetAcc( pState, 1 );
        }
    }
    return 0;
}


/**Function*************************************************************

  Synopsis    [Reads the list of primary inputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aio_ReadNetworkCis( Aio_Read_t * p, int Line, Aut_Var_t ** pVars, int * pnVars )
{
    char * pTemp;
    Aut_Var_t * pVar;
    // make sure this is indeed the .inputs line
    assert( strncmp( p->pLines[p->pDotInputs[Line]], ".inputs", 7 ) == 0 );
    // skip the ".inputs" word
    pTemp = strtok( p->pLines[p->pDotInputs[Line]], " \t" );
    // read the PI names one by one
    while ( pTemp = strtok( NULL, " \t" ) )
    {
        pVar = Aut_VarAlloc();
        Aut_VarSetName( pVar, util_strsav( pTemp ) );
        pVars[ (*pnVars)++ ] = pVar;
    }
    return 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aut_Var_t ** Aio_DeriveVars( Aio_Read_t * p, int * pnVars )
{
    Aut_Var_t ** pVars;
    int nVars, i, v;
    int nValues;
    char ** pValueNames;
    Aio_Var_t * pIoVar;

    nVars = 0;
    pVars = ALLOC( Aut_Var_t *, 1000 );

    // read the names
    for ( i = 0; i < p->nDotInputs; i++ )
        Aio_ReadNetworkCis( p, i, pVars, &nVars );

    // set the values
    for ( i = 0; i < nVars; i++ )
    {
        nValues = 2;
        if ( p->tName2Values )
            st_lookup( p->tName2Values, Aut_VarReadName(pVars[i]), (char**)&nValues );
        Aut_VarSetValueNum( pVars[i], nValues );
        if ( p->nValuesMax < nValues )
            p->nValuesMax = nValues;
        // set the value names
        if ( p->tName2Var )
        {
            if ( st_lookup( p->tName2Var, Aut_VarReadName(pVars[i]), (char**)&pIoVar ) )
            {
                pValueNames = ALLOC( char *, pIoVar->nValues );
                for ( v = 0; v < pIoVar->nValues; v++ )
                    pValueNames[v] = util_strsav(pIoVar->pValueNames[v]);
                Aut_VarSetValueNames( pVars[i], pValueNames );
            }
        }
    }

    *pnVars = nVars;
    return pVars;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vmx_VarMap_t * Aio_DeriveVmx( Aio_Read_t * p, Aut_Var_t ** pVars, int nVars )
{
    Vm_VarMap_t * pVm;
    Vmx_VarMap_t * pVmx;
    int * pValues;
    int i;

    pValues = ALLOC( int, nVars );
    for ( i = 0; i < nVars; i++ )
        pValues[i] = Aut_VarReadValueNum(pVars[i]);

    pVm  = Vm_VarMapLookup ( Fnc_ManagerReadManVm(Mv_FrameReadMan(p->pMvsis)),  nVars,  0, pValues );
    pVmx = Vmx_VarMapLookup( Fnc_ManagerReadManVmx(Mv_FrameReadMan(p->pMvsis)), pVm,   -1, NULL );
    FREE( pValues );

    return pVmx;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


