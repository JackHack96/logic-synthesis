/**CFile****************************************************************

  FileName    [mio.h]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [File reading/writing for technology mapping.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 8, 2003.]

  Revision    [$Id: mio.h,v 1.8 2005/02/28 05:34:28 alanmi Exp $]

***********************************************************************/

#ifndef __MIO_H__
#define __MIO_H__

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "mv.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

typedef enum { MIO_PHASE_UNKNOWN, MIO_PHASE_INV, MIO_PHASE_NONINV } Mio_PinPhase_t;

typedef struct  Mio_LibraryStruct_t_      Mio_Library_t;
typedef struct  Mio_GateStruct_t_         Mio_Gate_t;
typedef struct  Mio_PinStruct_t_          Mio_Pin_t;

////////////////////////////////////////////////////////////////////////
///                       GLOBAL VARIABLES                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFITIONS                            ///
////////////////////////////////////////////////////////////////////////
 
#define Mio_LibraryForEachGate( Lib, Gate )                   \
    for ( Gate = Mio_LibraryReadGates(Lib);                   \
          Gate;                                               \
          Gate = Mio_GateReadNext(Gate) )
#define Mio_LibraryForEachGateSafe( Lib, Gate, Gate2 )        \
    for ( Gate = Mio_LibraryReadGates(Lib),                   \
          Gate2 = (Gate? Mio_GateReadNext(Gate): NULL);       \
          Gate;                                               \
          Gate = Gate2,                                       \
          Gate2 = (Gate? Mio_GateReadNext(Gate): NULL) )

#define Mio_GateForEachPin( Gate, Pin )                       \
    for ( Pin = Mio_GateReadPins(Gate);                       \
          Pin;                                                \
          Pin = Mio_PinReadNext(Pin) )
#define Mio_GateForEachPinSafe( Gate, Pin, Pin2 )              \
    for ( Pin = Mio_GateReadPins(Gate),                        \
          Pin2 = (Pin? Mio_PinReadNext(Pin): NULL);           \
          Pin;                                                \
          Pin = Pin2,                                         \
          Pin2 = (Pin? Mio_PinReadNext(Pin): NULL) )

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/*=== mioApi.c =============================================================*/
extern char *            Mio_LibraryReadName       ( Mio_Library_t * pLib );
extern int               Mio_LibraryReadGateNum    ( Mio_Library_t * pLib );
extern Mio_Gate_t *      Mio_LibraryReadGates      ( Mio_Library_t * pLib );
extern DdManager *       Mio_LibraryReadDd         ( Mio_Library_t * pLib );
extern Mio_Gate_t *      Mio_LibraryReadGateByName ( Mio_Library_t * pLib, char * pName );
extern Mvc_Cover_t *     Mio_LibraryReadMvcByName  ( Mio_Library_t * pLib, char * pName );    
extern Mio_Gate_t *      Mio_LibraryReadInv        ( Mio_Library_t * pLib );
extern float             Mio_LibraryReadDelayInvRise( Mio_Library_t * pLib );
extern float             Mio_LibraryReadDelayInvFall( Mio_Library_t * pLib );
extern float             Mio_LibraryReadDelayInvMax( Mio_Library_t * pLib );
extern float             Mio_LibraryReadDelayNand2Rise( Mio_Library_t * pLib );
extern float             Mio_LibraryReadDelayNand2Fall( Mio_Library_t * pLib );
extern float             Mio_LibraryReadDelayNand2Max( Mio_Library_t * pLib );
extern float             Mio_LibraryReadAreaInv    ( Mio_Library_t * pLib );
extern float             Mio_LibraryReadAreaNand2  ( Mio_Library_t * pLib );
extern char *            Mio_GateReadName          ( Mio_Gate_t * pGate );      
extern char *            Mio_GateReadOutName       ( Mio_Gate_t * pGate );      
extern double            Mio_GateReadArea          ( Mio_Gate_t * pGate );      
extern char *            Mio_GateReadForm          ( Mio_Gate_t * pGate );      
extern Mio_Pin_t *       Mio_GateReadPins          ( Mio_Gate_t * pGate );      
extern Mio_Library_t *   Mio_GateReadLib           ( Mio_Gate_t * pGate );      
extern Mio_Gate_t *      Mio_GateReadNext          ( Mio_Gate_t * pGate );      
extern int               Mio_GateReadInputs        ( Mio_Gate_t * pGate );      
extern double            Mio_GateReadDelayMax      ( Mio_Gate_t * pGate );      
extern Mvc_Cover_t *     Mio_GateReadMvc           ( Mio_Gate_t * pGate );      
extern Mvc_Cover_t *     Mio_GateReadMvcC          ( Mio_Gate_t * pGate );      
extern DdNode *          Mio_GateReadFunc          ( Mio_Gate_t * pGate );
extern char *            Mio_PinReadName           ( Mio_Pin_t * pPin );  
extern Mio_PinPhase_t    Mio_PinReadPhase          ( Mio_Pin_t * pPin );  
extern double            Mio_PinReadInputLoad      ( Mio_Pin_t * pPin );  
extern double            Mio_PinReadMaxLoad        ( Mio_Pin_t * pPin );  
extern double            Mio_PinReadDelayBlockRise ( Mio_Pin_t * pPin );  
extern double            Mio_PinReadDelayFanoutRise( Mio_Pin_t * pPin );  
extern double            Mio_PinReadDelayBlockFall ( Mio_Pin_t * pPin );  
extern double            Mio_PinReadDelayFanoutFall( Mio_Pin_t * pPin );  
extern double            Mio_PinReadDelayBlockMax  ( Mio_Pin_t * pPin );  
extern Mio_Pin_t *       Mio_PinReadNext           ( Mio_Pin_t * pPin );  
/*=== mioRead.c =============================================================*/
extern Mio_Library_t *   Mio_LibraryRead( Mv_Frame_t * pMvsis, char * FileName, char * ExcludeFile );
extern int               Mio_LibraryReadExclude( Mv_Frame_t * pMvsis, char * ExcludeFile, st_table * tExcludeGate );
/*=== mioFunc.c =============================================================*/
extern int               Mio_LibraryParseFormulas( Mio_Library_t * pLib );
/*=== mioUtils.c =============================================================*/
extern void              Mio_LibraryDelete( Mio_Library_t * pLib );
extern void              Mio_GateDelete( Mio_Gate_t * pGate );
extern void              Mio_PinDelete( Mio_Pin_t * pPin );
extern Mio_Pin_t *       Mio_PinDup( Mio_Pin_t * pPin );
extern void              Mio_WriteLibrary( FILE * pFile, Mio_Library_t * pLib );
extern Mio_Gate_t **     Mio_CollectRoots( Mio_Library_t * pLib, int nInputs, float tDelay, bool fSkipInv, int * pnGates );
extern void              Mio_DeriveTruthTable( Mio_Gate_t * pGate, unsigned uTruthsIn[][2], int nSigns, int nInputs, unsigned uTruthRes[] );
extern void              Mio_DeriveGateDelays( Mio_Gate_t * pGate, 
                            float ** ptPinDelays, int nPins, int nInputs, float tDelayZero, 
                            float * ptDelaysRes, float * ptPinDelayMax );
extern Mio_Gate_t *      Mio_GateCreatePseudo( int nInputs );

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
#endif
