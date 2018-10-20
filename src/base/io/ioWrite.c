/**CFile****************************************************************

  FileName    [ioWrite.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Various network writing procedures.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ioWrite.c,v 1.21 2004/10/19 05:01:43 satrajit Exp $]

***********************************************************************/

#include "ioInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define  IO_WRITE_LINE_LENGTH    78    // the output line length

static void Io_WriteNetworkNodeFanins( FILE * pFile, Ntk_Node_t * pNode );

static void Io_WriteNetworkNodeBlif( FILE * pFile, Vm_VarMap_t * pVm, Cvr_Cover_t * pCover );
static void Io_WriteNetworkNodeBlifMvs( FILE * pFile, Vm_VarMap_t * pVm, Cvr_Cover_t * pCover );
static void Io_WriteNetworkNodeBlifMv( FILE * pFile, Vm_VarMap_t * pVm, Cvr_Cover_t * pCover );

static void Io_WriteNetworkNodeBlifCover( FILE * pFile, Vm_VarMap_t * pVm, Mvc_Cover_t * Cover, int Value );
static void Io_WriteNetworkNodeBlifMvsCover( FILE * pFile, Vm_VarMap_t * pVm, Mvc_Cover_t * Cover, int Value, int Default, int nOutputValues );
static void Io_WriteNetworkNodeBlifMvCover( FILE * pFile, Vm_VarMap_t * pVm, Mvc_Cover_t * Cover, int Value );

static void Io_WriteNetworkLatch( FILE * pFile, Ntk_Latch_t * pLatch, int FileType );
static void Io_WriteNetworkBuffer( FILE * pFile, Ntk_Node_t * pInput, Ntk_Node_t * pOutput, int FileType );
static void Io_WriteNetworkArrivalInfo( FILE * pFile, Ntk_Network_t * pNet );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Write the network into a BLIF file with the given name.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_WriteNetwork( Ntk_Network_t * pNet, char * FileName, int FileType, bool fWriteLatches )
{
    Ntk_Network_t * pNetExdc;
    FILE * pFile;
    pFile = fopen( FileName, "w" );
    if ( pFile == NULL )
    {
        fprintf( Ntk_NetworkReadMvsisOut(pNet), "Io_WriteNetwork(): Cannot open the output file.\n" );
        return;
    }
    // write the model name
    fprintf( pFile, ".model %s\n", Ntk_NetworkReadName(pNet) );
    // write the spec line name
    if ( FileType == IO_FILE_BLIF_MV && Ntk_NetworkReadSpec(pNet) )
        fprintf( pFile, ".spec %s\n", Ntk_NetworkReadSpec(pNet) );
    // write the network
    Io_WriteNetworkOne( pFile, pNet, FileType, fWriteLatches );
    // write EXDC network if it exists
    pNetExdc = Ntk_NetworkReadNetExdc( pNet );
    if ( pNetExdc )
    {
        fprintf( pFile, "\n" );
        fprintf( pFile, ".exdc\n" );
        Io_WriteNetworkOne( pFile, pNetExdc, FileType, 0 );
    }
    // finalize the file
    fprintf( pFile, ".end\n" );
    fclose( pFile );
}

/**Function*************************************************************

  Synopsis    [Write one network.]

  Description [Writes a network composed of PIs, POs, internal nodes,
  and latches. The following rules are used to print the names of 
  internal nodes:
  ]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_WriteNetworkOne( FILE * pFile, Ntk_Network_t * pNet, int FileType, bool fWriteLatches )
{
    Ntk_Node_t * pFanout, * pFanin, * pNode;
    Ntk_Latch_t * pLatch;
    Ntk_Pin_t * pPin;

    // write the PIs
    fprintf( pFile, ".inputs" );
    Io_WriteNetworkCis( pFile, pNet, fWriteLatches );
    fprintf( pFile, "\n" );

    // write the POs
    fprintf( pFile, ".outputs" );
    Io_WriteNetworkCos( pFile, pNet, fWriteLatches );
    fprintf( pFile, "\n" );
    
    // write arrival times
    Io_WriteNetworkArrivalInfo( pFile, pNet );

    // write the .mv lines
    if ( FileType == IO_FILE_BLIF_MV )
    {
//        fprintf( pFile, "\n" );
        Ntk_NetworkForEachCi( pNet, pNode )
            if ( Ntk_NodeReadValueNum(pNode) > 2 )
                fprintf( pFile, ".mv %s %d\n", Ntk_NodeReadName(pNode), Ntk_NodeReadValueNum(pNode) );
//        fprintf( pFile, "\n" );
        Ntk_NetworkForEachCo( pNet, pNode )
            if ( Ntk_NodeReadValueNum(pNode) > 2 )
                fprintf( pFile, ".mv %s %d\n", Ntk_NodeReadName(pNode), Ntk_NodeReadValueNum(pNode) );
        // write all the .mv directives for the internal nodes on top of the file
//        fprintf( pFile, "\n" );
        Ntk_NetworkForEachNode( pNet, pNode )
        {
            // if an internal node has a single CO fanout, 
            // the name of this node is the same as CO name
            // and the node's .mv is already written above as the CO's .mv
            if ( Ntk_NodeHasCoName(pNode) )
                continue;
            // otherwise, write the .mv directive if it is non-binary
            if ( Ntk_NodeReadValueNum(pNode) > 2 )
                fprintf( pFile, ".mv %s %d\n", Ntk_NodeGetNameLong(pNode), Ntk_NodeReadValueNum(pNode) );
        }
    }

    // write the latches
    if ( fWriteLatches )
    {
        if ( Ntk_NetworkReadLatchNum( pNet ) )
            fprintf( pFile, "\n" );
        // write only those latches that have both input/output belonging to the latch
        // this is used as a hack to simplify writing network parts in Mvn_CommandNetLatchSplit
        Ntk_NetworkForEachLatch( pNet, pLatch )
            if ( Ntk_NodeBelongsToLatch(pLatch->pInput) && Ntk_NodeBelongsToLatch(pLatch->pOutput) )
                Io_WriteNetworkLatch( pFile, pLatch, FileType );
        if ( Ntk_NetworkReadLatchNum( pNet ) )
            fprintf( pFile, "\n" );
    }

    // write each internal node
    Ntk_NetworkForEachNode( pNet, pNode )
    {
        // write the internal node
        Io_WriteNetworkNode( pFile, pNode, FileType, 0 );
        // if an internal node has only one CO fanout,
        // the CO name is used for the internal node, and in this way, 
        // the CO is written into the file

        // however, if an internal node has more than one CO fanout
        // it is written as an intenal node with its own name
        // in this case, the COs should be written as MV buffers
        // driven by the given internal node
        if ( Ntk_NodeIsCoFanin(pNode) && !Ntk_NodeHasCoName(pNode) )
        {
            Ntk_NodeForEachFanout( pNode, pPin, pFanout ) 
                if ( Ntk_NodeIsCo(pFanout) )
                    Io_WriteNetworkBuffer( pFile, pNode, pFanout, FileType ); 
        }
    }

    // some PO nodes may have a fanin, which is a PI
    // such PO nodes are written here, because they are not written
    // in the above loop, which interates through the internal nodes
    Ntk_NetworkForEachCo( pNet, pNode )
    {
        pFanin = Ntk_NodeReadFaninNode( pNode, 0 );
        if ( Ntk_NodeIsCi(pFanin) )
            Io_WriteNetworkBuffer( pFile, pFanin, pNode, FileType );
    }
}


/**Function*************************************************************

  Synopsis    [Writes the primary input list in a nice sort of way.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_WriteNetworkCis( FILE * pFile, Ntk_Network_t * pNet, bool fWriteLatches )
{
    Ntk_Node_t * pNode;
    int LineLength;
    int AddedLength;
    int NameCounter;

    LineLength  = 7;
    NameCounter = 0;
    Ntk_NetworkForEachCi( pNet, pNode )
    {
        if ( !fWriteLatches || !Ntk_NodeBelongsToLatch(pNode) )
        {
            // get the line length after this name is written
            AddedLength = strlen(Ntk_NodeReadName(pNode)) + 1;
            if ( NameCounter && LineLength + AddedLength + 3 > IO_WRITE_LINE_LENGTH )
            { // write the line extender
                fprintf( pFile, " \\\n" );
                // reset the line length
                LineLength  = 0;
                NameCounter = 0;
            }
            fprintf( pFile, " %s", Ntk_NodeReadName(pNode) );
            LineLength += AddedLength;
            NameCounter++;
        }
    }
}

/**Function*************************************************************

  Synopsis    [Writes the primary input list in a nice sort of way.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_WriteNetworkCisSpecial( FILE * pFile, Ntk_Network_t * pNet )
{
    Ntk_Node_t * pNode;
    int LineLength;
    int AddedLength;
    int NameCounter;

    LineLength  = 7;
    NameCounter = 0;
    Ntk_NetworkForEachNodeSpecial( pNet, pNode )
    {
        if ( Ntk_NodeIsCi(pNode) )
        {
            // get the line length after this name is written
            AddedLength = strlen(Ntk_NodeReadName(pNode)) + 1;
            if ( NameCounter && LineLength + AddedLength + 3 > IO_WRITE_LINE_LENGTH )
            { // write the line extender
                fprintf( pFile, " \\\n" );
                // reset the line length
                LineLength  = 0;
                NameCounter = 0;
            }
            fprintf( pFile, " %s", Ntk_NodeReadName(pNode) );
            LineLength += AddedLength;
            NameCounter++;
        }
    }
}

/**Function*************************************************************

  Synopsis    [Writes the primary input list in a nice sort of way.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_WriteNetworkCos( FILE * pFile, Ntk_Network_t * pNet, bool fWriteLatches )
{
    Ntk_Node_t * pNode;
    int LineLength;
    int AddedLength;
    int NameCounter;

    LineLength  = 8;
    NameCounter = 0;
    Ntk_NetworkForEachCo( pNet, pNode )
    {
        if ( !fWriteLatches || !Ntk_NodeBelongsToLatch(pNode) )
        {
            // get the line length after this name is written
            AddedLength = strlen(Ntk_NodeReadName(pNode)) + 1;
            if ( NameCounter && LineLength + AddedLength + 3 > IO_WRITE_LINE_LENGTH )
            { // write the line extender
                fprintf( pFile, " \\\n" );
                // reset the line length
                LineLength  = 0;
                NameCounter = 0;
            }
            fprintf( pFile, " %s", Ntk_NodeReadName(pNode) );
            LineLength += AddedLength;
            NameCounter++;
        }
    }
}


/**Function*************************************************************

  Synopsis    [Write the latch into a file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_WriteNetworkLatch( FILE * pFile, Ntk_Latch_t * pLatch, int FileType )
{
    Ntk_Node_t * pLatchInput;
    Ntk_Node_t * pLatchOutput;
    Ntk_Node_t * pLatchNode;
    int nValues, Reset;
    int i;

    pLatchInput  = Ntk_LatchReadInput( pLatch );
    pLatchOutput = Ntk_LatchReadOutput( pLatch );
    pLatchNode   = Ntk_LatchReadNode( pLatch );
    nValues      = Ntk_NodeReadValueNum(pLatchOutput);
    Reset        = Ntk_LatchReadReset(pLatch);

    // write the reset line
    if ( FileType != IO_FILE_BLIF )
    {
        if ( pLatchNode )
        {
            Ntk_NodeSetName( pLatchNode, Ntk_NodeReadName(pLatchOutput) );
            Io_WriteNetworkNode( pFile, pLatchNode, FileType, 1 );
            Ntk_NodeSetName( pLatchNode, NULL );
        }
        else
        { // write the latch reset node based on the reset value
            fprintf( pFile, ".reset %s\n", Ntk_NodeReadName(pLatchOutput) );
            if ( FileType == IO_FILE_BLIF_MVS )
            {
                fprintf( pFile, " " );
                for ( i = 0; i < nValues; i++ )
                    if ( i == Reset )
                        fprintf( pFile, "%d", i % 10 );
                    else
                        fprintf( pFile, "%c", '-' );
                fprintf( pFile, "\n" );
            }
            else
                fprintf( pFile, "%d\n", Reset );
        }
    }
    // write the latch line
    fprintf( pFile, ".latch %s %s", Ntk_NodeReadName(pLatchInput), Ntk_NodeReadName(pLatchOutput) );
    // write the reset value if it is BLIF
    if ( FileType == IO_FILE_BLIF )
        fprintf( pFile, "  %d", Reset );
    fprintf( pFile, "\n" );
}


/**Function*************************************************************

  Synopsis    [Write the node into a file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_WriteNetworkNode( FILE * pFile, Ntk_Node_t * pNode, int FileType, int fLatch )
{
    Cvr_Cover_t * pCover;
    Vm_VarMap_t * pVm;

    // write the .names line
    if ( fLatch )
        fprintf( pFile, ".reset" );
    else if ( FileType == IO_FILE_BLIF_MV )
        fprintf( pFile, ".table" );
    else
        fprintf( pFile, ".names" );

    Io_WriteNetworkNodeFanins( pFile, pNode );
    fprintf( pFile, "\n" );

    // write the cubes
    pCover = Fnc_FunctionGetCvr( Ntk_NodeReadMan(pNode), Ntk_NodeReadFunc(pNode) );
    pVm    = Fnc_FunctionReadVm( Ntk_NodeReadFunc(pNode) );
    assert( pVm = Cvr_CoverReadVm(pCover) );
    if ( FileType == IO_FILE_BLIF )
        Io_WriteNetworkNodeBlif( pFile, pVm, pCover );
    else if ( FileType == IO_FILE_BLIF_MVS )
        Io_WriteNetworkNodeBlifMvs( pFile, pVm, pCover );
    else if ( FileType == IO_FILE_BLIF_MV )
        Io_WriteNetworkNodeBlifMv( pFile, pVm, pCover );
}

/**Function*************************************************************

  Synopsis    [Writes the primary input list in a nice sort of way.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_WriteNetworkNodeFanins( FILE * pFile, Ntk_Node_t * pNode )
{
    Ntk_Node_t * pFanin;
    Ntk_Pin_t * pPin;
    int LineLength;
    int AddedLength;
    int NameCounter;
    char * pName;

    LineLength  = 6;
    NameCounter = 0;
    Ntk_NodeForEachFanin( pNode, pPin, pFanin )
    {
        // get the fanin name
        pName = Ntk_NodeGetNameLong(pFanin);
        // get the line length after the fanin name is written
        AddedLength = strlen(pName) + 1;
        if ( NameCounter && LineLength + AddedLength + 3 > IO_WRITE_LINE_LENGTH )
        { // write the line extender
            fprintf( pFile, " \\\n" );
            // reset the line length
            LineLength  = 0;
            NameCounter = 0;
        }
        fprintf( pFile, " %s", pName );
        LineLength += AddedLength;
        NameCounter++;
    }

    // get the output name
    pName = Ntk_NodeGetNameLong(pNode);
    // get the line length after the output name is written
    AddedLength = strlen(pName) + 1;
    if ( NameCounter && LineLength + AddedLength > 75 )
    { // write the line extender
        fprintf( pFile, " \\\n" );
        // reset the line length
        LineLength  = 0;
        NameCounter = 0;
    }
    fprintf( pFile, " %s", pName );
}



/**Function*************************************************************

  Synopsis    [Write the buffer.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_WriteNetworkBuffer( FILE * pFile, Ntk_Node_t * pInput, Ntk_Node_t * pOutput, int FileType )
{
    int nValues;
    int i, k;
    assert( Ntk_NodeReadValueNum(pInput) == Ntk_NodeReadValueNum(pOutput) );
    if ( FileType == IO_FILE_BLIF_MV )
        fprintf( pFile, ".table" );
    else
        fprintf( pFile, ".names" );
    fprintf( pFile, " %s", Ntk_NodeGetNameLong(pInput) );
    fprintf( pFile, " %s", Ntk_NodeGetNameLong(pOutput) );
    fprintf( pFile, "\n" );
    if ( FileType == IO_FILE_BLIF )
        fprintf( pFile, "1 1\n" );
    else if ( FileType == IO_FILE_BLIF_MV )
        fprintf( pFile, "- =%s\n", Ntk_NodeGetNameLong(pInput) );
    else if ( FileType == IO_FILE_BLIF_MVS )
    {
        nValues = Ntk_NodeReadValueNum(pInput);
        for ( i = 0; i < nValues; i++ )
        {
            fprintf( pFile, "    " );
            for ( k = 0; k < nValues; k++ )
                fprintf( pFile, "%c", (k != i)? '-': '0' + k % 10 );
            fprintf( pFile, "  " );
            for ( k = 0; k < nValues; k++ )
                fprintf( pFile, "%c", (k != i)? '-': '0' + k % 10 );
            fprintf( pFile, "\n" );
        }
    }
}


/**Function*************************************************************

  Synopsis    [Write the cover as BLIF.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_WriteNetworkNodeBlif( FILE * pFile, Vm_VarMap_t * pVm, Cvr_Cover_t * pCover )
{
    Mvc_Cover_t ** pIsets;
    Mvc_Cover_t * Cover;
    int nValues, fPhase;

    pIsets  = Cvr_CoverReadIsets( pCover );
    nValues = Vm_VarMapReadValuesOutput(pVm);
    if ( pIsets[1] == NULL )
    {
        fPhase = 0;
        Cover = pIsets[0];
    }
    else
    {
        fPhase = 1;
        Cover = pIsets[1];
    }
    // at this point, we may need to print both covers
    // if the node is incompletely specified (not a standard BLIF)
    // so far, we are simply printing the positive phase

    // check if the cover is a constant
    if ( Vm_VarMapReadVarsNum(pVm) == 1 )
    {
        if ( ( fPhase == 0 && Mvc_CoverReadCubeNum(Cover) == 0 ) || 
             ( fPhase == 1 && Mvc_CoverReadCubeNum(Cover) == 1 ) )
            fprintf( pFile, " 1\n" );
        return;
    }

    Io_WriteNetworkNodeBlifCover( pFile, pVm, Cover, fPhase );
}

/**Function*************************************************************

  Synopsis    [Write the cover as BLIF.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_WriteNetworkNodeBlifCover( FILE * pFile, Vm_VarMap_t * pVm, Mvc_Cover_t * Cover, int Value )
{
    Mvc_Cube_t * pCube;
    int v;

    //int * pValues      = Vm_VarMapReadValuesArray(pVm);
    //int * pValuesFirst = Vm_VarMapReadValuesFirstArray(pVm);
    int nVarsIn        = Vm_VarMapReadVarsInNum(pVm);

    Mvc_CoverForEachCube( Cover, pCube ) 
    {
        // write the inputs
        for ( v = 0; v < nVarsIn; v++ )
            if ( Mvc_CubeBitValue(  pCube,  2*v ) )
            {
                if ( Mvc_CubeBitValue(  pCube,  2*v + 1 ) )
                    fprintf( pFile, "%c", '-' );
                else 
                    fprintf( pFile, "%c", '0' );
            }
            else 
            {
                if ( Mvc_CubeBitValue(  pCube,  2*v + 1 ) )
                    fprintf( pFile, "%c", '1' );
                else 
                    fprintf( pFile, "%c", '?' );
            }
        // write the output
        fprintf( pFile, " %d\n", Value );
    }
}




/**Function*************************************************************

  Synopsis    [Write the cover as BLIF-MVS.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_WriteNetworkNodeBlifMvs( FILE * pFile, Vm_VarMap_t * pVm, Cvr_Cover_t * pCover )
{
    Mvc_Cover_t ** pIsets;
    int nValues, Default, i;

    pIsets  = Cvr_CoverReadIsets( pCover );
    nValues = Vm_VarMapReadValuesOutput(pVm);
    Default = Cvr_CoverReadDefault( pCover );

    for ( i = 0; i < nValues; i++ )
        if ( pIsets[i] )
            Io_WriteNetworkNodeBlifMvsCover( pFile, pVm, pIsets[i], i, Default, nValues );
}

/**Function*************************************************************

  Synopsis    [Write the cover as BLIF-MVS.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_WriteNetworkNodeBlifMvsCover( FILE * pFile, Vm_VarMap_t * pVm, Mvc_Cover_t * Cover, int Value, int Default, int nOutputValues )
{
    Mvc_Cube_t * pCube;
    int i, v;

    int * pValues      = Vm_VarMapReadValuesArray(pVm);
    int * pValuesFirst = Vm_VarMapReadValuesFirstArray(pVm);
    int nVarsIn        = Vm_VarMapReadVarsInNum(pVm);

    Mvc_CoverForEachCube( Cover, pCube )
    {
        // write space
        fprintf( pFile, "   " );
        // write the variables
        for ( i = 0; i < nVarsIn; i++ )
        {
            // write space
            fprintf( pFile, " " );
            // write the input literal
            for ( v = 0; v < pValues[i]; v++ )
                if ( Mvc_CubeBitValue( pCube,  pValuesFirst[i] + v) ) 
                    fprintf( pFile, "%d", v % 10 );
                else
                    fprintf( pFile, "%c", '-' );
        }

        // write space
        fprintf( pFile, "  " );
        // write the output literal
        for ( v = 0; v < nOutputValues; v++ )
            if ( v == Value )
                fprintf( pFile, "%d", v % 10 );
            else if ( v == Default )
                fprintf( pFile, "%c", 'D' );
            else
                fprintf( pFile, "%c", '-' );
        // write new line
        fprintf( pFile, "\n" );
    }
}



/**Function*************************************************************

  Synopsis    [Write the cover as BLIF-MV.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_WriteNetworkNodeBlifMv( FILE * pFile, Vm_VarMap_t * pVm, Cvr_Cover_t * pCover )
{
    Mvc_Cover_t ** pIsets;
    int nValues, Default, i;

    nValues = Vm_VarMapReadValuesOutput(pVm);
    pIsets  = Cvr_CoverReadIsets( pCover );
    Default = Cvr_CoverReadDefault( pCover );

    if ( Default >= 0 )
        fprintf( pFile, ".default %d\n", Default );

    for ( i = 0; i < nValues; i++ )
        if ( pIsets[i] )
            Io_WriteNetworkNodeBlifMvCover( pFile, pVm, pIsets[i], i );
}


/**Function*************************************************************

  Synopsis    [Write the cover as BLIF-MV.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_WriteNetworkNodeBlifMvCover( FILE * pFile, Vm_VarMap_t * pVm, Mvc_Cover_t * Cover, int Value )
{
    Mvc_Cube_t * pCube;
    int Counter, FirstValue;
    int i, v;

    int * pValues      = Vm_VarMapReadValuesArray(pVm);
    int * pValuesFirst = Vm_VarMapReadValuesFirstArray(pVm);
    int nVarsIn        = Vm_VarMapReadVarsInNum(pVm);

    Mvc_CoverForEachCube( Cover, pCube ) 
    {
        // write the variables
        for ( i = 0; i < nVarsIn; i++ )
        {
            // count the number of values in this literal
            Counter  = 0;
            for ( v = 0; v < pValues[i]; v++ )
                if ( Mvc_CubeBitValue( pCube,  pValuesFirst[i] + v) ) 
                    Counter++;
            assert( Counter > 0 );

            if ( Counter == pValues[i] )
                fprintf( pFile, "- " );
            else if ( Counter == 1 )
            {
                for ( v = 0; v < pValues[i]; v++ )
                    if ( Mvc_CubeBitValue( pCube,  pValuesFirst[i] + v) ) 
                        fprintf( pFile, "%d ", v );
            }
            else
            {
                fprintf( pFile, "(" );
                FirstValue = 1;
                for ( v = 0; v < pValues[i]; v++ )
                    if ( Mvc_CubeBitValue( pCube,  pValuesFirst[i] + v) ) 
                    {
                        if ( FirstValue )
                            FirstValue = 0;
                        else
                            fprintf( pFile, "," );
                        fprintf( pFile, "%d", v );
                    }
                fprintf( pFile, ") " );
            }
        }
        // write the output
        if ( nVarsIn > 0 )
            fprintf( pFile, " " );
        fprintf( pFile, "%d\n", Value );
    }
}

/**Function*************************************************************

  Synopsis    [Write the arrival times]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_WriteNetworkArrivalInfo( FILE * pFile, Ntk_Network_t * pNet )
{
    Ntk_Node_t * pNode;
    
    fprintf( pFile, ".default_input_arrival %g %g\n", 
            pNet->dDefaultArrTimeRise,
            pNet->dDefaultArrTimeFall );

    Ntk_NetworkForEachCi( pNet, pNode )
    {
        if (   (pNode->dArrTimeRise == pNet->dDefaultArrTimeRise)
                && (pNode->dArrTimeFall == pNet->dDefaultArrTimeFall) )
            continue;
        // Print only if this input has special arrival time
        fprintf( pFile, ".input_arrival %s %g %g\n", 
                Ntk_NodeGetNameLong(pNode),
                pNode->dArrTimeRise,
                pNode->dArrTimeFall );
    }
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

