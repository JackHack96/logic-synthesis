/**CFile****************************************************************

  FileName    [lang.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Implicit language solving flow.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: lang.c,v 1.8 2005/06/02 03:34:21 alanmi Exp $]

***********************************************************************/
 
#include "langInt.h"

#ifndef WIN32
#define _unlink unlink
#endif

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// these commands to input/output automata
static int Lang_CommandAutoReadAut    ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Lang_CommandAutoWriteAut   ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Lang_CommandAutoStgExtract ( Mv_Frame_t * pMvsis, int argc, char ** argv );

// these commands transform automata
static int Lang_CommandAutoSupport    ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Lang_CommandAutoComplete   ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Lang_CommandAutoComplement ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Lang_CommandAutoDeterminize( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Lang_CommandAutoMinimize   ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Lang_CommandAutoProduct    ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Lang_CommandAutoPrefixClose( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Lang_CommandAutoProgressive( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Lang_CommandAutoReduce     ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Lang_CommandAutoReencode   ( Mv_Frame_t * pMvsis, int argc, char ** argv );

// these commands only modify the topmost automaton in the stack
static int Lang_CommandAutoCheck      ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Lang_CommandAutoPsa        ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Lang_CommandAutoReorder    ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Lang_CommandAutoTest       ( Mv_Frame_t * pMvsis, int argc, char ** argv );

// these command only work on the stack
static int Lang_CommandAutoSwap       ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Lang_CommandAutoDup        ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Lang_CommandAutoPop        ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Lang_CommandAutoHistory    ( Mv_Frame_t * pMvsis, int argc, char ** argv );

// the temporary place for the stack of automata
static Lang_Stack_t * pStack = NULL;

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lang_Init( Mv_Frame_t * pMvsis )
{
#ifndef BALM
    Cmd_CommandAdd( pMvsis, "Sequential implicit synthesis", "_iread_aut",    Lang_CommandAutoReadAut,     0 ); 
    Cmd_CommandAdd( pMvsis, "Sequential implicit synthesis", "_iwrite_aut",   Lang_CommandAutoWriteAut,    0 ); 
    Cmd_CommandAdd( pMvsis, "Sequential implicit synthesis", "_istg_extract", Lang_CommandAutoStgExtract,  0 ); 

    Cmd_CommandAdd( pMvsis, "Sequential implicit synthesis", "_isupport",     Lang_CommandAutoSupport,     0 ); 
    Cmd_CommandAdd( pMvsis, "Sequential implicit synthesis", "_icomplete",    Lang_CommandAutoComplete,    0 ); 
    Cmd_CommandAdd( pMvsis, "Sequential implicit synthesis", "_icomplement",  Lang_CommandAutoComplement,  0 ); 
    Cmd_CommandAdd( pMvsis, "Sequential implicit synthesis", "_ideterminize", Lang_CommandAutoDeterminize, 0 ); 
    Cmd_CommandAdd( pMvsis, "Sequential implicit synthesis", "_iminimize",    Lang_CommandAutoMinimize,    0 ); 
    Cmd_CommandAdd( pMvsis, "Sequential implicit synthesis", "_iproduct",     Lang_CommandAutoProduct,     0 ); 
    Cmd_CommandAdd( pMvsis, "Sequential implicit synthesis", "_iprefix",      Lang_CommandAutoPrefixClose, 0 ); 
    Cmd_CommandAdd( pMvsis, "Sequential implicit synthesis", "_iprogressive", Lang_CommandAutoProgressive, 0 ); 
    Cmd_CommandAdd( pMvsis, "Sequential implicit synthesis", "_ireduce",      Lang_CommandAutoReduce,      0 ); 
    Cmd_CommandAdd( pMvsis, "Sequential implicit synthesis", "_ireencode",    Lang_CommandAutoReencode,    0 ); 

    Cmd_CommandAdd( pMvsis, "Sequential implicit synthesis", "_icheck",       Lang_CommandAutoCheck,       0 ); 
    Cmd_CommandAdd( pMvsis, "Sequential implicit synthesis", "_ipsa",         Lang_CommandAutoPsa,         0 ); 
    Cmd_CommandAdd( pMvsis, "Sequential implicit synthesis", "_ireorder",     Lang_CommandAutoReorder,     0 ); 
    Cmd_CommandAdd( pMvsis, "Sequential implicit synthesis", "_itest",        Lang_CommandAutoTest,        0 ); 

    Cmd_CommandAdd( pMvsis, "Sequential implicit synthesis", "_iswap",        Lang_CommandAutoSwap,        0 ); 
    Cmd_CommandAdd( pMvsis, "Sequential implicit synthesis", "_idup",         Lang_CommandAutoDup,         0 ); 
    Cmd_CommandAdd( pMvsis, "Sequential implicit synthesis", "_ipop",         Lang_CommandAutoPop,         0 ); 
    Cmd_CommandAdd( pMvsis, "Sequential implicit synthesis", "_istack",       Lang_CommandAutoHistory,     0 ); 
    assert( pStack == NULL );
    pStack = Lang_StackStart( 100 );
#endif
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lang_End()
{
    Lang_Auto_t * pAut;
    while ( !Lang_StackIsEmpty( pStack ) )
    {
        pAut = Lang_StackPop( pStack );
        Lang_AutoFree( pAut );
    }
    Lang_StackFree( pStack );
    pStack = NULL;
}




/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Lang_CommandAutoReadAut( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Lang_Auto_t * pAut;
    FILE * pOut, * pErr, * pFile;
    char * FileNameIn;
    int fVerbose;
    int c;

    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    fVerbose = 0;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "hv" ) ) != EOF )
    {
        switch ( c )
        {
			case 'v':
				fVerbose ^= 1;
				break;
			case 'h':
				goto usage;
			default:
				goto usage;
        }
    }

    // get the file names
    if ( argc < 2 )
        goto usage;

    FileNameIn  = argv[util_optind];

    // check that the input file is okay
    if ( (pFile = fopen( FileNameIn, "r" )) == NULL )
    {
        fprintf( pErr, "Cannot open input file \"%s\".\n", FileNameIn );
        goto usage;
    }
    fclose( pFile );

    pAut = Lang_AutoRead( pMvsis, FileNameIn );
    if ( pAut == NULL )
    {
        fprintf( pErr, "Converting FSM KISS file into an automaton KISS file has failed.\n" );
        return 1;
    }
    Lang_AutoAssignHistory( pAut, argc, argv );

    Lang_StackPush( pStack, pAut );
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: _iread_aut <file_in>\n");
	fprintf( pErr, "       reads the automaton and pushes it into the stack\n" );
	fprintf( pErr, " <file_in>  : the input file with the automaton\n" );
//    fprintf( pErr, "        -v  : toggles verbose [default = %s]\n", (fVerbose? "yes": "no") );
	return 1;					// error exit 
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Lang_CommandAutoWriteAut( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Lang_Auto_t * pAut;
    FILE * pOut, * pErr;
    char * FileNameOut;
    int fVerbose;
    int c;

    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    fVerbose = 0;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "hv" ) ) != EOF )
    {
        switch ( c )
        {
			case 'v':
				fVerbose ^= 1;
				break;
			case 'h':
				goto usage;
			default:
				goto usage;
        }
    }

    // get the file names
    if ( argc < 2 )
        goto usage;

    FileNameOut  = argv[util_optind];

    pAut = Lang_StackPop( pStack );
    if ( pAut == NULL )
		goto usage;
    Lang_StackPush( pStack, pAut );

    Lang_AutoWrite( pAut, FileNameOut );
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: _iwrite_aut <file_out>\n");
	fprintf( pErr, "       write the top automaton in the stack into the file\n" );
	fprintf( pErr, " <file_out> : the output file with the resulting automaton\n" );
//    fprintf( pErr, "        -v  : toggles verbose [default = %s]\n", (fVerbose? "yes": "no") );
	return 1;					// error exit 
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Lang_CommandAutoStgExtract( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Lang_Auto_t * pAut;
    Ntk_Network_t * pNet;
    FILE * pOut, * pErr;
    int fVerbose;
    int fLongNames;
    int c;

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    fLongNames = 0;
    fVerbose = 0;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "lhv" ) ) != EOF )
    {
        switch ( c )
        {
			case 'l':
				fLongNames ^= 1;
				break;
			case 'v':
				fVerbose ^= 1;
				break;
			case 'h':
				goto usage;
			default:
				goto usage;
        }
    }

    if ( pNet == NULL )
    {
        fprintf( pErr, "Empty network.\n" );
        return 1; 
    }

    if ( Ntk_NetworkReadLatchNum(pNet) == 0 )
    {
        fprintf( pErr, "The current network does not have latches.\n" );
        return 1;
    }

    pAut = Lang_ExtractStg( pNet, fLongNames );
    if ( pAut == NULL )
    {
        fprintf( pErr, "Extracting STG from the network has failed.\n" );
        return 1;
    }
    Lang_AutoAssignHistory( pAut, argc, argv );

    Lang_StackPush( pStack, pAut );
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: _istg_extract [-l]\n");
	fprintf( pErr, "       extracts STG from current network and pushes it into the stack\n" );
    fprintf( pErr, "        -l  : toggles the use of long names [default = %s]\n", (fLongNames? "yes": "no") );
//    fprintf( pErr, "        -v  : toggles verbose [default = %s]\n", (fVerbose? "yes": "no") );
	return 1;					// error exit 
}



/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Lang_CommandAutoSupport( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Lang_Auto_t * pAut, * pAutR;
    FILE * pOut, * pErr;
    char * pInputString;
    char * pInputStringCopy;
    int fVerbose, fKeep;
    int c, RetValue;

    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    fKeep = 0;
    fVerbose = 0;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "khv" ) ) != EOF )
    {
        switch ( c )
        {
			case 'k':
				fKeep ^= 1;
				break;
			case 'v':
				fVerbose ^= 1;
				break;
			case 'h':
				goto usage;
			default:
				goto usage;
        }
    }

    if ( util_optind == argc )
    {
        fprintf( pErr, "A comma-separated list of IO vars should be given on the command line.\n" );
        return 1;
    }

    // get the string of input variables
    pInputString = argv[util_optind];

    // get the topmost automaton in the stack
    pAut = Lang_StackPop( pStack );
    if ( pAut == NULL )
		goto usage;
    // decide what to do with the automaton
    if ( fKeep )
    {
        pAutR = Lang_AutoDup( pAut );
        Lang_StackPush( pStack, pAut );
    }
    else
        pAutR = pAut;
    // perform operation on this automaton
    pInputStringCopy = util_strsav( pInputString );
    RetValue = Lang_AutoSupport( pAutR, pInputStringCopy );
    FREE( pInputStringCopy );
    if( RetValue == 0 )
    {
        if ( fKeep )
            Lang_AutoFree( pAutR );
        fprintf( pErr, "Support computation has failed.\n" );
        return 1;
    }
    // add history to the new automaton
    Lang_AutoAssignHistory( pAutR, argc, argv );
    // push the result into the stack
    Lang_StackPush( pStack, pAutR );

    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: _isupport [-k] <input_order>\n");
	fprintf( pErr, "       changes the input variables of the top automaton in the stack\n" );
    fprintf( pErr, "        -k  : keep the argument automaton in the stack [default = %s]\n", (fKeep? "yes": "no") );
	fprintf( pErr, " <input_order> : the resulting list of inputs (comma-separated, no spaces)\n" );
//    fprintf( pErr, "        -v  : toggles verbose [default = %s]\n", (fVerbose? "yes": "no") );
	return 1;					// error exit 
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Lang_CommandAutoComplete( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Lang_Auto_t * pAut, * pAutR;
    FILE * pOut, * pErr;
    int fVerbose, fKeep, fAccepting, nInputs;
    int c, nStates;

    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    fKeep = 0;
    fVerbose = 0;
    fAccepting = 0;
    nInputs = -1;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "kiahv" ) ) != EOF )
    {
        switch ( c )
        {
			case 'k':
				fKeep ^= 1;
				break;
            case 'i':
                nInputs = atoi(argv[util_optind]);
                util_optind++;
                if ( nInputs < 0 ) 
                    goto usage;
                break;
			case 'a':
				fAccepting ^= 1;
				break;
			case 'v':
				fVerbose ^= 1;
				break;
			case 'h':
				goto usage;
			default:
				goto usage;
        }
    }

    // get the topmost automaton in the stack
    pAut = Lang_StackPop( pStack );
    if ( pAut == NULL )
		goto usage;
    // if the number of inputs is not given, complete w.r.t. all inputs
    if ( nInputs == -1 )
        nInputs = pAut->nInputs;
    // get the automaton for the transformation
    if ( fKeep )
    {
        pAutR = Lang_AutoDup( pAut );
        Lang_StackPush( pStack, pAut );
    }
    else
        pAutR = pAut;
    // perform operation on this automaton
    nStates = Lang_AutoComplete( pAutR, nInputs, fAccepting, 0 );
    if( nStates == 0 )
    {
        fprintf( pErr, "The automaton is already complete and is left unchanged.\n" );
        return 1;
    }
    // add history to the new automaton
    Lang_AutoAssignHistory( pAutR, argc, argv );
    // push the result into the stack
    Lang_StackPush( pStack, pAutR );

    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: _icomplete [-k] [-i num] [-a]\n");
	fprintf( pErr, "       completes the automaton by adding the don't-care state\n" );
    fprintf( pErr, "    -k      : keep the argument automaton in the stack [default = %s]\n", (fKeep? "yes": "no") );
	fprintf( pErr, "    -i num  : the number of inputs to consider for completion [default = all].\n" );
	fprintf( pErr, "              (if the number of inputs is less than all inputs,\n" );
	fprintf( pErr, "              the completion is performed in the FSM sense)\n" );
    fprintf( pErr, "    -a      : toggles the don't-care state type [default = %s]\n", (fAccepting? "accepting": "non-accepting") );
//    fprintf( pErr, "        -v  : toggles verbose [default = %s]\n", (fVerbose? "yes": "no") );
	return 1;					// error exit 
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Lang_CommandAutoComplement( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Lang_Auto_t * pAut, * pAutR;
    FILE * pOut, * pErr;
    int fVerbose, fKeep;
    int c, RetValue;

    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    fKeep = 0;
    fVerbose = 0;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "khv" ) ) != EOF )
    {
        switch ( c )
        {
			case 'k':
				fKeep ^= 1;
				break;
			case 'v':
				fVerbose ^= 1;
				break;
			case 'h':
				goto usage;
			default:
				goto usage;
        }
    }

    // get the topmost automaton in the stack
    pAut = Lang_StackPop( pStack );
    if ( pAut == NULL )
		goto usage;
    // get the automaton for the transformation
    if ( fKeep )
    {
        pAutR = Lang_AutoDup( pAut );
        Lang_StackPush( pStack, pAut );
    }
    else
        pAutR = pAut;
    // perform operation on this automaton
    RetValue = Lang_AutoComplement( pAutR );
    if( RetValue == 0 )
    {
        if ( fKeep )
            Lang_AutoFree( pAutR );
        fprintf( pErr, "Complementation has failed.\n" );
        return 1;
    }
    // add history to the new automaton
    Lang_AutoAssignHistory( pAutR, argc, argv );
    // push the result into the stack
    Lang_StackPush( pStack, pAutR );
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: _icomplement [-k]\n");
	fprintf( pErr, "       complement the top automaton in the stack\n" );
    fprintf( pErr, "        -k  : keep the argument automaton in the stack [default = %s]\n", (fKeep? "yes": "no") );
//    fprintf( pErr, "        -v  : toggles verbose [default = %s]\n", (fVerbose? "yes": "no") );
	return 1;					// error exit 
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Lang_CommandAutoDeterminize( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Lang_Auto_t * pAut, * pAutR;
    FILE * pOut, * pErr;
    int fLongNames;
    int fVerbose, fKeep, fAllAccepting;
    int fComplement, fComplete;
    int c;

    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    fKeep = 0;
    fVerbose = 0;
    fComplete = 0;
    fComplement = 0;
    fLongNames = 0;
    fAllAccepting = 0;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "klciahv" ) ) != EOF )
    {
        switch ( c )
        {
			case 'k':
				fKeep ^= 1;
				break;
			case 'l':
				fLongNames ^= 1;
				break;
			case 'c':
				fComplete ^= 1;
				break;
			case 'i':
				fComplement ^= 1;
				break;
			case 'a':
				fAllAccepting ^= 1;
				break;
			case 'v':
				fVerbose ^= 1;
				break;
			case 'h':
				goto usage;
			default:
				goto usage;
        }
    }

    // get the topmost automaton in the stack
    pAut = Lang_StackPop( pStack );
    if ( pAut == NULL )
		goto usage;
    // perform operation on this automaton
    pAutR = Lang_AutoDeterminize( pAut, fAllAccepting, fLongNames );
    if( pAutR == NULL )
    {
        fprintf( pErr, "Determinization has failed.\n" );
        return 1;
    }
    // add history to the new automaton
    Lang_AutoAssignHistory( pAutR, argc, argv );
    if ( fComplete )
        Lang_AutoComplete( pAutR, pAutR->nInputs, 0, 0 );
    if ( fComplement )
        Lang_AutoComplement( pAutR );
    // decide what to do with the original one
    if ( fKeep )
        Lang_StackPush( pStack, pAut );
    else
        Lang_AutoFree( pAut );
    // push the result into the stack
    Lang_StackPush( pStack, pAutR );
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: _ideterminize [-k] [-c] [-i] [-a] [-l]\n");
	fprintf( pErr, "       reads the topmost automaton, determinizes it, and pushed it back\n" );
    fprintf( pErr, "        -k  : keep the argument automaton in the stack [default = %s]\n", (fKeep? "yes": "no") );
    fprintf( pErr, "        -c  : complete the automaton after determinization [default = %s]\n", (fComplete? "yes": "no") );
    fprintf( pErr, "        -i  : complement the automaton after determinization [default = %s]\n", (fComplement? "yes": "no") );
    fprintf( pErr, "        -a  : require all states in a subset to be accepting [default = %s]\n", (fAllAccepting? "yes": "no") );
    fprintf( pErr, "        -l  : use long names for the subsets of states [default = %s]\n", (fLongNames? "yes": "no") );
//    fprintf( pErr, "        -v  : toggles verbose [default = %s]\n", (fVerbose? "yes": "no") );
	return 1;					// error exit 
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Lang_CommandAutoMinimize( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Lang_Auto_t * pAut, * pAutR;
    FILE * pOut, * pErr;
    int fVerbose, fKeep;
    int c;
    int fUseTable;

    fprintf( stdout, "Debugging of this command is not finished.\n" );
    return 1;

    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    fKeep = 0;
    fUseTable = 1;
    fVerbose = 0;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "kahv" ) ) != EOF )
    {
        switch ( c )
        {
			case 'k':
				fKeep ^= 1;
				break;
			case 'a':
				fUseTable ^= 1;
				break;
			case 'v':
				fVerbose ^= 1;
				break;
			case 'h':
				goto usage;
			default:
				goto usage;
        }
    }

    // get the topmost automaton in the stack
    pAut = Lang_StackPop( pStack );
    if ( pAut == NULL )
		goto usage;
    // perform operation on this automaton
    pAutR = Lang_AutoMinimize( pAut );
    if( pAutR == NULL )
    {
        fprintf( pErr, "Minimization has failed.\n" );
        return 1;
    }
    // add history to the new automaton
    Lang_AutoAssignHistory( pAutR, argc, argv );
    // decide what to do with the original one
    if ( fKeep )
        Lang_StackPush( pStack, pAut );
    else
        Lang_AutoFree( pAut );
    // push the result into the stack
    Lang_StackPush( pStack, pAutR );
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: _iminimize [-k]\n");
	fprintf( pErr, "       reads the topmost automaton, minimizes it, and pushed it back\n" );
    fprintf( pErr, "        -k  : keep the argument automaton in the stack [default = %s]\n", (fKeep? "yes": "no") );
//    fprintf( pErr, "        -a  : toggles minimization using table/other [default = %s]\n", (fUseTable? "table": "other") );
//    fprintf( pErr, "        -v  : toggles verbose [default = %s]\n", (fVerbose? "yes": "no") );
	return 1;					// error exit 
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Lang_CommandAutoProduct( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Lang_Auto_t * pAut1, * pAut2, * pAutR;
    FILE * pOut, * pErr;
    int fLongNames;
    int fVerbose, fKeep;
    int c;

    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    fKeep = 0;
    fVerbose = 0;
    fLongNames = 0;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "klhv" ) ) != EOF )
    {
        switch ( c )
        {
			case 'k':
				fKeep ^= 1;
				break;
			case 'l':
				fLongNames ^= 1;
				break;
			case 'v':
				fVerbose ^= 1;
				break;
			case 'h':
				goto usage;
			default:
				goto usage;
        }
    }
    
    // get the topmost automaton in the stack
    pAut1 = Lang_StackPop( pStack );
    if ( pAut1 == NULL )
		goto usage;
    pAut2 = Lang_StackPop( pStack );
    if ( pAut2 == NULL )
		goto usage;
    // perform operation on this automaton
    pAutR = Lang_AutoProduct( pAut1, pAut2, fLongNames, 0 );
    if( pAutR == NULL )
    {
        fprintf( pErr, "Product computation has failed.\n" );
        return 1;
    }
    // add history to the new automaton
    Lang_AutoAssignHistory( pAutR, argc, argv );
    // decide what to do with the original one
    if ( fKeep )
    {
        Lang_StackPush( pStack, pAut2 );
        Lang_StackPush( pStack, pAut1 );
    }
    else
    {
        Lang_AutoFree( pAut2 );
        Lang_AutoFree( pAut1 );
    }
    // push the result into the stack
    Lang_StackPush( pStack, pAutR );

    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: _iproduct [-k] [-l]\n");
	fprintf( pErr, "       reads two automata from the stack, computes the product, and pushed it back\n" );
    fprintf( pErr, "        -k  : keep the argument automata in the stack [default = %s]\n", (fKeep? "yes": "no") );
    fprintf( pErr, "        -l  : use long names for the subsets of states [default = %s]\n", (fLongNames? "yes": "no") );
//    fprintf( pErr, "        -v  : toggles verbose [default = %s]\n", (fVerbose? "yes": "no") );
	return 1;					// error exit 
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Lang_CommandAutoPrefixClose( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Lang_Auto_t * pAut, * pAutR;
    FILE * pOut, * pErr;
    int fVerbose, fKeep;
    int c, RetValue;

    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    fKeep = 0;
    fVerbose = 0;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "khv" ) ) != EOF )
    {
        switch ( c )
        {
			case 'k':
				fKeep ^= 1;
				break;
			case 'v':
				fVerbose ^= 1;
				break;
			case 'h':
				goto usage;
			default:
				goto usage;
        }
    }

    // get the topmost automaton in the stack
    pAut = Lang_StackPop( pStack );
    if ( pAut == NULL )
		goto usage;
    // get the automaton for the transformation
    if ( fKeep )
    {
        pAutR = Lang_AutoDup( pAut );
        Lang_StackPush( pStack, pAut );
    }
    else
        pAutR = pAut;
    // perform operation on this automaton
    RetValue = Lang_AutoPrefixClose( pAutR );
    if( RetValue == 0 )
    {
        if ( fKeep )
            Lang_AutoFree( pAutR );
        fprintf( pErr, "Prefix close computation has failed.\n" );
        return 1;
    }
    // add history to the new automaton
    Lang_AutoAssignHistory( pAutR, argc, argv );
    // push the result into the stack
    Lang_StackPush( pStack, pAutR );
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: _iprefix [-k]\n");
	fprintf( pErr, "       reads the topmost automaton, makes it prefix closed, and pushes it back\n" );
    fprintf( pErr, "        -k  : keep the argument automaton in the stack [default = %s]\n", (fKeep? "yes": "no") );
//    fprintf( pErr, "        -v  : toggles verbose [default = %s]\n", (fVerbose? "yes": "no") );
	return 1;					// error exit 
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Lang_CommandAutoProgressive( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Lang_Auto_t * pAut, * pAutR;
    FILE * pOut, * pErr;
    int fVerbose, fKeep;
    int nInputs, c, RetValue;

    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    fKeep = 0;
    fVerbose = 0;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "kihv" ) ) != EOF )
    {
        switch ( c )
        {
			case 'k':
				fKeep ^= 1;
				break;
            case 'i':
                nInputs = atoi(argv[util_optind]);
                util_optind++;
                if ( nInputs < 0 ) 
                    goto usage;
                break;
			case 'v':
				fVerbose ^= 1;
				break;
			case 'h':
				goto usage;
			default:
				goto usage;
        }
    }

    if ( nInputs == -1 )
    {
        fprintf( pErr, "The number of FSM inputs should be given on the command line.\n" );
        goto usage;
    }


    // get the topmost automaton in the stack
    pAut = Lang_StackPop( pStack );
    if ( pAut == NULL )
		goto usage;
    // get the automaton for the transformation
    if ( fKeep )
    {
        pAutR = Lang_AutoDup( pAut );
        Lang_StackPush( pStack, pAut );
    }
    else
        pAutR = pAut;
    // perform operation on this automaton
    RetValue = Lang_AutoProgressive( pAutR, nInputs, fVerbose );
    if( RetValue == 0 )
    {
        if ( fKeep )
            Lang_AutoFree( pAutR );
        fprintf( pErr, "Progressive computation has failed.\n" );
        return 1;
    }
    // add history to the new automaton
    Lang_AutoAssignHistory( pAutR, argc, argv );
    // push the result into the stack
    Lang_StackPush( pStack, pAutR );
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: _iprogressive [-k] [-i num]\n");
	fprintf( pErr, "       reads the topmost automaton, makes it progressive, and pushes it back\n" );
	fprintf( pErr, "       leaves only accepting and complete states that are reachable\n" );
    fprintf( pErr, "        -k  : keep the argument automaton in the stack [default = %s]\n", (fKeep? "yes": "no") );
	fprintf( pErr, "    -i num  : the number of inputs to consider for completion [default = all].\n" );
//    fprintf( pErr, "        -v  : toggles verbose [default = %s]\n", (fVerbose? "yes": "no") );
	return 1;					// error exit 
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Lang_CommandAutoReduce( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Lang_Auto_t * pAut, * pAutR;
    FILE * pOut, * pErr;
    int fVerbose, fKeep;
    int nInputs, c;

    fprintf( stdout, "This command is not implemented yet.\n" );
    return 1;


    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    fKeep = 0;
    fVerbose = 0;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "kihv" ) ) != EOF )
    {
        switch ( c )
        {
			case 'k':
				fKeep ^= 1;
				break;
            case 'i':
                nInputs = atoi(argv[util_optind]);
                util_optind++;
                if ( nInputs < 0 ) 
                    goto usage;
                break;
			case 'v':
				fVerbose ^= 1;
				break;
			case 'h':
				goto usage;
			default:
				goto usage;
        }
    }

    if ( nInputs == -1 )
    {
        fprintf( pErr, "The number of FSM inputs should be given on the command line.\n" );
        goto usage;
    }


    // get the topmost automaton in the stack
    pAut = Lang_StackPop( pStack );
    if ( pAut == NULL )
		goto usage;
    // perform operation on this automaton
    pAutR = Lang_AutoReduce( pAut, nInputs, fVerbose );
    if( pAutR == NULL )
    {
        fprintf( pErr, "Reduction has failed.\n" );
        return 1;
    }
    // add history to the new automaton
    Lang_AutoAssignHistory( pAutR, argc, argv );
    // decide what to do with the original one
    if ( fKeep )
        Lang_StackPush( pStack, pAut );
    else
        Lang_AutoFree( pAut );
    // push the result into the stack
    Lang_StackPush( pStack, pAutR );
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: _ireduce [-k] [-i num]\n");
	fprintf( pErr, "       reads the topmost automaton, reduces it, and pushes it back\n" );
    fprintf( pErr, "        -k  : keep the argument automaton in the stack [default = %s]\n", (fKeep? "yes": "no") );
	fprintf( pErr, "    -i num  : the number of inputs to consider for completion [default = all].\n" );
//    fprintf( pErr, "        -v  : toggles verbose [default = %s]\n", (fVerbose? "yes": "no") );
	return 1;					// error exit 
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Lang_CommandAutoReencode( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Lang_Auto_t * pAut, * pAutR;
    FILE * pOut, * pErr;
    int fVerbose, fKeep;
    int c, RetValue;

    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    fKeep = 0;
    fVerbose = 0;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "khv" ) ) != EOF )
    {
        switch ( c )
        {
			case 'k':
				fKeep ^= 1;
				break;
			case 'v':
				fVerbose ^= 1;
				break;
			case 'h':
				goto usage;
			default:
				goto usage;
        }
    }

    // get the topmost automaton in the stack
    pAut = Lang_StackPop( pStack );
    if ( pAut == NULL )
		goto usage;
    // decide what to do with the original one
    if ( fKeep )
    {
        pAutR = Lang_AutoDup( pAut );
        Lang_StackPush( pStack, pAut );
    }
    else
        pAutR = pAut;
    // perform operation on this automaton
    RetValue = Lang_AutoReencode( pAutR );
    if( RetValue == 0 )
    {
        if ( fKeep )
            Lang_AutoFree( pAutR );
        fprintf( pErr, "Reencoding has timed out.\n" );
        return 1;
    }
    // add history to the new automaton
    Lang_AutoAssignHistory( pAutR, argc, argv );
    // push the result into the stack
    Lang_StackPush( pStack, pAutR );
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: reencode [-k]\n");
	fprintf( pErr, "       reencodes the topmost automaton\n" );
    fprintf( pErr, "        -k  : keep the argument automaton in the stack [default = %s]\n", (fKeep? "yes": "no") );
//    fprintf( pErr, "        -v  : toggles verbose [default = %s]\n", (fVerbose? "yes": "no") );
	return 1;					// error exit 
}




/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Lang_CommandAutoCheck( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Au_Auto_t * pAut2a;
    Lang_Auto_t * pAut1, * pAut2;
    FILE * pOut, * pErr, * pFile;
    char * FileNameIn;
    int fVerbose;
    int fError;
    int fCheckFsm;
    int c;

    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    fCheckFsm = 0;
    fVerbose = 0;
    fError = 0;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "efhv" ) ) != EOF )
    {
        switch ( c )
        {
			case 'e':
				fError ^= 1;
				break;
			case 'f':
				fCheckFsm ^= 1;
				break;
			case 'v':
				fVerbose ^= 1;
				break;
			case 'h':
				goto usage;
			default:
				goto usage;
        }
    }

    if ( util_optind == argc ) // the file name is NOT given on the command line
    {
        // get the first automaton
        pAut1 = Lang_StackPop( pStack );
        if ( pAut1 == NULL )
		    goto usage;
        // get the second automaton
        pAut2 = Lang_StackPop( pStack );
        if ( pAut2 == NULL )
		    goto usage;
        Lang_StackPush( pStack, pAut2 );
        Lang_StackPush( pStack, pAut1 );

        Lang_AutoCheck( pOut, pAut1, pAut2, fError );
    }
    else 
    {
        assert( util_optind < argc );

        // get the first automaton
        pAut1 = Lang_StackPop( pStack );
        if ( pAut1 == NULL )
		    goto usage;
        Lang_StackPush( pStack, pAut1 );

        // get the second automaton from the file
        FileNameIn = argv[util_optind];

        // check that the input file is okay
        if ( (pFile = fopen( FileNameIn, "r" )) == NULL )
        {
            fprintf( pErr, "Cannot open input file \"%s\".\n", FileNameIn );
            goto usage;
        }
        fclose( pFile );

        // read the first automaton/FSM
        if ( !fCheckFsm )
            pAut2a = Au_AutoReadKiss( pMvsis, FileNameIn );
        else
        {
            if ( !Au_FsmReadKiss( pMvsis, FileNameIn, "_temp_.aut" ) )
            {
                fprintf( pErr, "Converting FSM KISS file 1 into an automaton KISS file has failed.\n" );
                return 1;
            }
            pAut2a = Au_AutoReadKiss( pMvsis, "_temp_.aut" );
            _unlink( "_temp_.aut" );
            if ( pAut2a )
            {
                FREE( pAut2a->pName );
                pAut2a->pName = util_strsav( FileNameIn );
//                Au_AutoComplete( pAut2a, pAut2a->nInputs, 0, 0 );
            }
        }
        if ( pAut2a == NULL )
        {
            fprintf( pErr, "Reading the automaton from file \"%s\" has failed.\n", FileNameIn );
            return 1;
        }
        pAut2 = Lang_AutomatonConvert( pAut2a, NULL );
        Au_AutoFree( pAut2a );

        Lang_AutoCheck( pOut, pAut1, pAut2, fError );
        Lang_AutoFree( pAut2 );
    }
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: _icheck [-f] <file_in>\n");
	fprintf( pErr, "       checks equivalence or containment of the automata behaviors\n" );
//    fprintf( pErr, "        -e  : toggle printing error trace [default = %s]\n", (fError? "yes": "no") );
    fprintf( pErr, "        -f  : toggle checking FSMs/automata [default = %s]\n", (fCheckFsm? "FSMs": "automata") );
	fprintf( pErr, " <file_in>  : (optional) the input KISS file with the automaton or FSM\n" );
	fprintf( pErr, "              if the file is not given, two topmost automata from the stack are used\n" );
//    fprintf( pErr, "        -v  : toggles verbose [default = %s]\n", (fVerbose? "yes": "no") );
	return 1;					// error exit 
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Lang_CommandAutoPsa( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Lang_Auto_t * pAut;
    FILE * pOut, * pErr;
    int fVerbose;
    int nInputs, c;

    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    nInputs = -1;
    fVerbose = 0;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "ihv" ) ) != EOF )
    {
        switch ( c )
        {
            case 'i':
                nInputs = atoi(argv[util_optind]);
                util_optind++;
                if ( nInputs < 0 ) 
                    goto usage;
                break;
			case 'v':
				fVerbose ^= 1;
				break;
			case 'h':
				goto usage;
			default:
				goto usage;
        }
    }

    pAut = Lang_StackPop( pStack );
    if ( pAut == NULL )
		goto usage;
    Lang_StackPush( pStack, pAut );

    if ( nInputs == -1 )
        nInputs = pAut->nInputs;

    Lang_AutoPrintStats( pOut, pAut, nInputs );
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: _ipsa [-i num]\n");
	fprintf( pErr, "       prints statistics about the topmost automaton in the stack\n" );
	fprintf( pErr, "   -i num  : the number of FSM inputs [default = all].\n" );
//    fprintf( pErr, "        -v  : toggles verbose [default = %s]\n", (fVerbose? "yes": "no") );
	return 1;					// error exit 
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Lang_CommandAutoReorder( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Lang_Auto_t * pAut;
    FILE * pOut, * pErr;
    int fVerbose;
    int c;

    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    fVerbose = 0;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "hv" ) ) != EOF )
    {
        switch ( c )
        {
			case 'v':
				fVerbose ^= 1;
				break;
			case 'h':
				goto usage;
			default:
				goto usage;
        }
    }

    pAut = Lang_StackPop( pStack );
    if ( pAut == NULL )
		goto usage;
    Lang_StackPush( pStack, pAut );

    Cudd_ReduceHeap( pAut->dd, CUDD_REORDER_SYMM_SIFT, 10 );
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: _ireorder\n");
	fprintf( pErr, "       reorders the BDD manager of the topmost automaton\n" );
//    fprintf( pErr, "        -v  : toggles verbose [default = %s]\n", (fVerbose? "yes": "no") );
	return 1;					// error exit 
}



/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Lang_CommandAutoSwap( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Lang_Auto_t * pAut1, * pAut2;
    FILE * pOut, * pErr;
    int fVerbose, c;

    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    fVerbose = 0;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "hv" ) ) != EOF )
    {
        switch ( c )
        {
			case 'v':
				fVerbose ^= 1;
				break;
			case 'h':
				goto usage;
			default:
				goto usage;
        }
    }

    pAut1 = Lang_StackPop( pStack );
    if ( pAut1 == NULL )
		goto usage;
    pAut2 = Lang_StackPop( pStack );
    if ( pAut2 == NULL )
		goto usage;
    Lang_StackPush( pStack, pAut1 );
    Lang_StackPush( pStack, pAut2 );
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: _iswap\n");
	fprintf( pErr, "       swaps the two topmost entries in the stack\n" );
//    fprintf( pErr, "        -v  : toggles verbose [default = %s]\n", (fVerbose? "yes": "no") );
	return 1;					// error exit 
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Lang_CommandAutoDup( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Lang_Auto_t * pAut1, * pAut2;
    FILE * pOut, * pErr;
    int fVerbose;
    int c;

    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    fVerbose = 0;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "hv" ) ) != EOF )
    {
        switch ( c )
        {
			case 'v':
				fVerbose ^= 1;
				break;
			case 'h':
				goto usage;
			default:
				goto usage;
        }
    }

    pAut1 = Lang_StackPop( pStack );
    if ( pAut1 == NULL )
		goto usage;
    Lang_StackPush( pStack, pAut1 );

    pAut2 = Lang_AutoDup( pAut1 );
    if ( pAut2 == NULL )
    {
        fprintf( pErr, "Duplication of the automaton has failed.\n" );
        return 1;
    }
    Lang_StackPush( pStack, pAut2 );

    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: _idup\n");
	fprintf( pErr, "       duplicates the topmost entry in the stack\n" );
//    fprintf( pErr, "        -v  : toggles verbose [default = %s]\n", (fVerbose? "yes": "no") );
	return 1;					// error exit 
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Lang_CommandAutoPop( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Lang_Auto_t * pAut;
    FILE * pOut, * pErr;
    int fVerbose;
    int c;

    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    fVerbose = 0;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "hv" ) ) != EOF )
    {
        switch ( c )
        {
			case 'v':
				fVerbose ^= 1;
				break;
			case 'h':
				goto usage;
			default:
				goto usage;
        }
    }

    pAut = Lang_StackPop( pStack );
    if ( pAut == NULL )
		goto usage;
    Lang_AutoFree( pAut );

    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: _ipop\n");
	fprintf( pErr, "       pops and deletes the topmost automaton in the stack\n" );
//    fprintf( pErr, "        -v  : toggles verbose [default = %s]\n", (fVerbose? "yes": "no") );
	return 1;					// error exit 
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Lang_CommandAutoHistory( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Lang_Auto_t * pAutTemp[100];
    FILE * pOut, * pErr;
    int Level, c, fVerbose;
    int nEntries, nNameLen;

    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    fVerbose = 0;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "hv" ) ) != EOF )
    {
        switch ( c )
        {
			case 'v':
				fVerbose ^= 1;
				break;
			case 'h':
				goto usage;
			default:
				goto usage;
        }
    }

    // prints stats about the automata stack
    if ( Lang_StackIsEmpty( pStack ) )
    {
        printf( "The stack is empty.\n" );
        return 0;
    }

    // load the contents of the stack
    Level = 0;
    while ( !Lang_StackIsEmpty( pStack ) )
        pAutTemp[Level++] = Lang_StackPop( pStack );
    nEntries = Level;
    // push them back into the stack
    for ( Level--; Level >= 0; Level-- )
        Lang_StackPush( pStack, pAutTemp[Level] );

    // find the longest name of the automaton
    nNameLen = 0;
    for ( Level = 0; Level < nEntries; Level++ )
        if ( pAutTemp[Level]->pName )
            if ( nNameLen < (int)strlen(pAutTemp[Level]->pName) )
                 nNameLen = strlen(pAutTemp[Level]->pName);

    // print the contents of the stack
    for ( Level = 0; Level < nEntries; Level++ )
    {
        if ( Level == 0 )
            printf( "TOP : %*s : %s\n",        nNameLen, pAutTemp[Level]->pName, pAutTemp[Level]->pHistory );
        else
            printf( "%3d : %*s : %s\n", Level, nNameLen, pAutTemp[Level]->pName, pAutTemp[Level]->pHistory );
    }

    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: _istack\n");
	fprintf( pErr, "       prints the contents of the stack\n" );
//    fprintf( pErr, "        -v  : toggles verbose [default = %s]\n", (fVerbose? "yes": "no") );
	return 1;					// error exit 
}








/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Lang_CommandAutoTest( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Lang_Auto_t * pAut;
    FILE * pOut, * pErr, * pFile;
    char * FileNameIn;
    char * FileNameOut;
    int fVerbose;
    int c;

    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    fVerbose = 0;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "hv" ) ) != EOF )
    {
        switch ( c )
        {
			case 'v':
				fVerbose ^= 1;
				break;
			case 'h':
				goto usage;
			default:
				goto usage;
        }
    }

    // get the file names
    if ( argc < 3 )
        goto usage;

    FileNameIn  = argv[util_optind];
    FileNameOut = argv[util_optind + 1];

    // check that the input file is okay
    if ( (pFile = fopen( FileNameIn, "r" )) == NULL )
    {
        fprintf( pErr, "Cannot open input file \"%s\".\n", FileNameIn );
        goto usage;
    }
    fclose( pFile );

    pAut = Lang_AutoRead( pMvsis, FileNameIn );
    if ( pAut == NULL )
    {
        fprintf( pErr, "Reading the automaton from file \"%s\" has failed.\n", FileNameIn );
        return 1;
    }

    Lang_AutoWrite( pAut, FileNameOut );
    Lang_AutoFree( pAut );
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: _itest <file_in> <file_out>\n");
	fprintf( pErr, "       reads and writes the automaton from file\n" );
	fprintf( pErr, " <file_in>  : the input  KISS file with the input automaton\n" );
	fprintf( pErr, " <file_out> : the output KISS file with the resulting automaton\n" );
//    fprintf( pErr, "        -v  : toggles verbose [default = %s]\n", (fVerbose? "yes": "no") );
	return 1;					// error exit 
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


