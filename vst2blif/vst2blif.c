static char rcsid[] = "$Id: vst2blif.c,v 1.1.1.1 2004/02/07 10:14:13 pchong Exp $";
#ifndef DATE
#define DATE "Compile date not supplied"
#endif
static char build_date[] = DATE;
/*
 * $Header: /users/pchong/CVS/sis/vst2blif/vst2blif.c,v 1.1.1.1 2004/02/07 10:14:13 pchong Exp $
 * $Revision: 1.1.1.1 $
 * $Source: /users/pchong/CVS/sis/vst2blif/vst2blif.c,v $
 * $Log: vst2blif.c,v $
 * Revision 1.1.1.1  2004/02/07 10:14:13  pchong
 * imported
 *
 *
 *    BY rAMBALDI rOBERTO.
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>


#define MAXTOKENLEN 256
#define MAXNAMELEN 32

enum TOKEN_STATES { tZERO, tTOKEN, tREM1, tREM2, tSTRING, tEOF };


/*                                                                     *
 * These are the test used in the tokenizer, please refer to the graph *
 * included in the documentation doc.                                  *
 *                                                                     */

/*           ASCII seq:   (  )  *  +  ,    :  ;  <  =  >       *
 *           code         40 41 42 43 44   58 59 60 61 62      */
#define isSTK(c) (( ((c)=='[') || ((c)==']') || ((c)=='|') || ((c)=='#') || \
		    ( ((c)>=':') && ((c)<='>') ) || \
		    ( ((c)>='(') && ((c)<=',') ) ||  ((c)=='&') ))
#define isBLK(c) ( (((c)=='\t') || ((c)==' ') || ((c)=='\r')) )
#define isREM(c) ( ((c)=='-') )
#define isDQ(c)  ( ((c)=='"') )
#define isEOL(c) ( ((c)=='\n') )
#define isEOF(c) ( ((c)=='\0') )
#define isEOT(c) ( (isBLK((c)) || isEOL((c))) )


/* structure that contains all the info extracted from the library *
 * All the pins, input, outp[ut and clock (if needed)              *
 * This because SIS should be case sensitive and VHDL is case      *
 * insensitive, so we must keep the library's names of gates and   *
 * pins. In the future this may be useful for further checks that  *
 * in this version are not performed.                              */

/* list of inputs */
struct PortName{
    char             name[MAXNAMELEN];
    int              notused;  /* this flag is used to check if a  *
				* formal terminal has been already *
				* connected.                       */
    struct PortName  *next;
};


struct Ports{
    char   name[MAXNAMELEN];
};

struct Cell {
    char             name[MAXNAMELEN];
    int              npins;
    char             type;
    struct SIGstruct *io;
    struct Ports     *formals;
    struct Cell      *next;
};

struct LibCell {
    char           name[MAXNAMELEN];
    int            npins;
    char           clk[MAXNAMELEN];
    struct Ports   *formals;
    struct LibCell *next;
};

struct Instance {
    char            name[MAXNAMELEN];
    struct Cell     *what;
    struct Ports    *actuals;
    struct Instance *next;
};
	

struct BITstruct {
   char   name[MAXNAMELEN+10];
   char   dir;
   struct BITstruct *next;
};


struct SIGstruct {
    char name[MAXNAMELEN];
    char dir;
    int  start;
    int  end;
    struct SIGstruct *next;
};


struct TERMstruct {
    char   name[MAXNAMELEN];
    int    start;
    int    end;
};

struct ENTITYstruct {
    char   name[MAXNAMELEN];
    struct Cell        *Components;
    struct Cell        *EntityPort;
    struct SIGstruct   *Internals;
    struct Instance    *Net;
};


struct LibCell *ScanLibrary();

FILE *In;
FILE *Out;
struct LibCell *LIBRARY;
int  line;            /* line parsed                         */

int SendTokenBack;   /* Used to send the token back to the  *
                       * input stream in case of a missing   *
                       * keyword,                            */
char VDD[MAXNAMELEN]={ 0 };
char VSS[MAXNAMELEN]={ 0 };
int  INIT;
int  LINKAGE;
int  LOWERCASE;
char CLOCK[MAXNAMELEN];
int  WARNING;
int  INSTANCE;
struct Cell *cells;


/*===========================================================================*

  Utilities.
  CloseAll   : closes all the files ads flushes the memory
  Error      : display an error message an then exit
  KwrdCmp    : compares keywords --> case insensitive

 *===========================================================================*/

/* -=[ CloseAll ]=-                                 *
 * Closes all the files and flushes the used memory *
 *		                                    */
void CloseAll()
{

    if (In!=stdin) (void)fclose(In);
    if (Out!=stdout) (void)fclose(Out);

    free(cells);

}


/* -=[ Error ]=-                                    *
 * Displays an error message, and then exit         *
 *                                                  *
 * Input :                                          *
 *     msg  = message to printout before exiting    */
void Error(Msg)
char *Msg;
{
    (void)fprintf(stderr,"*** Error : %s\n",Msg);
    CloseAll();
    exit(1);
}

/* -=[ KwrdCmp ]=-                                  *
 * Compares to strings, without taking care of the  *
 * case.                                            *
 *                                                  *
 * Inoput :                                         *
 *      name = first string, tipically the token    *
 *      keywrd = second string, tipically a keyword *
 * Output :                                         *
 *      int  = 1 if the strings matches            *
 *              0 if they don't match               */
int KwrdCmp(name,keywrd)
char *name;
char *keywrd;
{
    int  t;
    int  len;

    if ( (len=strlen(name)) != (strlen(keywrd))) return 0;
    if (!len) return 0;
       /* if length 0 exit */
    for(t=0; (t<len); name++, keywrd++, t++){
	   /* if they're not equal */
	if (toupper(*name)!=toupper(*keywrd))
	    return 0;
	/* EoL, exit */
	if (*keywrd=='\0') break;
    }
    return 1;
}

/* -=[ CheckArgs ]=-                                *
 * Gets the options from the command line, open     *
 * the input and output file and read params from   *
 * the library file.                                *
 *                                                  *
 * Input :                                          * 
 *     argc,argv = usual cmdline arcguments         */
void CheckArgs(argc,argv)
int  argc;
char **argv;
{
    char *s;
    char c;
    int help;
    int NoPower;

    extern char *optarg;
    extern int optind;

    s=&( argv[0][strlen(argv[0])-1] );
    while( (s>= &(argv[0][0])) && (*s!='/') ) {  s--; }
     
    (void)fprintf(stderr,"\t\t      Vst Converter v1.5\n");
    (void)fprintf(stderr,"\t\t      by Roberto Rambaldi\n");
    (void)fprintf(stderr,"\t\tD.E.I.S. Universita' di Bologna\n\n");
    help=0;  INSTANCE=1; NoPower=0; LOWERCASE=1; INIT='3';
    while( (c=getopt(argc,argv,"s:S:d:D:c:C:i:I:l:L:hHvVuUnN$") )>0 ){
        switch (toupper(c)) {
          case 'S':
            (void)strncpy(VSS,optarg,MAXNAMELEN);
            break;
          case 'D':
            (void)strncpy(VDD,optarg,MAXNAMELEN);
              break;
          case 'H':
            help=1;
            break;
          case 'C':
            (void)strncpy(CLOCK,optarg,MAXNAMELEN);
            break;
          case 'I':
            INIT= *optarg;
            if ((INIT<'0') || (INIT>'3')) {
                (void)fprintf(stderr,"Wrong latch init value");
                help=1;
            }
            break;
          case 'L':
            if (KwrdCmp(optarg,"IN")) {
                LINKAGE='i';
            } else {
                if  (KwrdCmp(optarg,"OUT")) {
                    LINKAGE='o';
                } else {
		    (void)fprintf(stderr,"\tUnknow direction for a port of type linkage\n");
                    help=1;
                }
            }
            break;
          case 'U':
            LOWERCASE=0;
            break;
          case 'N':
            NoPower=1;
            break;
          case '$':
            help=1;
            (void)fprintf(stderr,"\n\tID = %s\n",rcsid);
            (void)fprintf(stderr,"\tCompiled on %s\n\n",build_date);
            break;
        }
    }     


    if (!help) {
        if (LOWERCASE) 
            for(s=&(CLOCK[0]); *s!='\0'; s++) (void)tolower(*s);
        else 
            for(s=&(CLOCK[0]); *s!='\0'; s++) (void)toupper(*s);

        if (optind>=argc) {
            (void)fprintf(stderr,"No Library file specified\n\n");
            help=1;
        } else {
            LIBRARY=(struct LibCell *)ScanLibrary(argv[optind]);
            if (++optind>=argc){
                In=stdin; Out=stdout;
            } else {
                if ((In=fopen(argv[optind],"rt"))==NULL) {
                    (void)fprintf(stderr,"Couldn't read input file");
                    help=1;
                }
                if (++optind>=argc) { Out=stdout; }
                else {
                    if ((Out=fopen(argv[optind],"wt"))==NULL) {
                        (void)fprintf(stderr,"Could'n make opuput file");
                        help=1;
                    }
                }
            }
        }
	
        if (NoPower) {
            VDD[0]='\0'; VSS[0]='\0';
        } else {
            if (!VDD[0]) (void)strcpy(VDD,"VDD");
            if (!VSS[0]) (void)strcpy(VSS,"VSS");
        }
    }

    if (help) {
        (void)fprintf(stderr,"\tUsage: vst2blif [options] <library> [infile [outfile]]\n");
        (void)fprintf(stderr,"\t\t if outfile is not given will be used stdout, if also\n");
        (void)fprintf(stderr,"\t\t infile is not given will be used stdin instead.\n");
        (void)fprintf(stderr,"\t<library>\t is the name of the library file to use\n");
        (void)fprintf(stderr,"\tOptions :\n\t-s <name>\t <name> will be used for VSS net\n");
        (void)fprintf(stderr,"\t-d <name>\t <name> will be used for VDD net\n");
        (void)fprintf(stderr,"\t-c <name>\t .clock <name>  will be added to the blif file\n");
        (void)fprintf(stderr,"\t-i <value>\t default value for latches, must be between 0 and 3\n");
        (void)fprintf(stderr,"\t-l <in/out>\t sets the direction for linkage ports\n");
        (void)fprintf(stderr,"\t\t\t the default value is \"in\"\n");
        (void)fprintf(stderr,"\t-u\t\t converts all names to uppercase\n");
        (void)fprintf(stderr,"\t-n\t\t no VSS or VDD to skip.\n");
        (void)fprintf(stderr,"\t-h\t\t prints these lines");
        (void)fprintf(stderr,"\n\tIf no VDD or VSS nets are given VDD and VSS will be used\n");
        exit(0);
    }
}

/* -=[ GetNextToken ]=-                             *
 * Tokenizer, see the graph to understand how it    *
 * works.                                           *
 *                                                  *
 * Inputs :                                         *
 *     tok = pointer to the final buffer, which is  *
 *           a copy of the internal Token, it's     *
 * *indirectly* uses SendTokenBack as input and     *
 * line as output                                   */
void GetNextToken(tok)
char *tok;
{
    static char init=0;
    static enum TOKEN_STATES state;
    static char sentback;
    static char str;
    static char Token[MAXTOKENLEN];
    char   *t;
    int    num;
    int   TokenReady;
    char   c;
    
    if (!init) {
	state=tZERO;
	init=1;
	line=0;
	SendTokenBack=0;
    }

    t=&(Token[0]);
    num=0;
    TokenReady=0;
    str=0;

    if (SendTokenBack) {
	SendTokenBack=0;
	(void)strcpy(tok,Token);
	return;
    }

    do {
	if (sentback) {
	    c=sentback;
	} else {
	    c=fgetc(In);
	    if (feof(In)) state=tEOF;
	    if (c=='\n') line++;
	}

	switch (state){
	  case tZERO:
            /*******************/
	    /*    ZERO state   */
            /*******************/
	    num=0;
	    sentback='\0';
	    if (isSTK(c)) {
		*t=c; t++;
		TokenReady=1;
	    } else {
		if isREM(c) {
		    *t=c;
		    t++; num++;
		    sentback='\0';
		    state=tREM1;
		} else {
		    if isDQ(c) {
			sentback='\0';
			state=tSTRING;
		    } else {
			if isEOF(c) {
			    state=tEOF;
			    sentback=1;
			    TokenReady=0;
			} else {
			    if isEOT(c) {
				state=tZERO;
				/* stay in tZERO */
			    } else {
				sentback=c;
				state=tTOKEN;
			    }
			}
		    }
		}
	    }
	    break;
	  case tTOKEN:
            /*******************/
	    /*   TOKEN  state  */
            /*******************/
	    TokenReady=1;
	    sentback=c;
	    if (isSTK(c)) {
		state=tZERO;
	    } else {
		if isREM(c) {
		    state=tREM1;
		} else {
		    if isDQ(c) {
			sentback='\0';
			state=tSTRING;
		    } else {
			if isEOF(c) {
			    sentback=1;
			    state=tEOF;
			} else {
			    if isEOT(c) {
				sentback='\0';
				state=tZERO;
			    } else {
				sentback='\0';
				if (num>=(MAXTOKENLEN-1)){
				    sentback=c;
				    (void)fprintf(stderr,"*Parse Warning* Line %u: token too long !\n",line);
				} else {                                    
				    if (LOWERCASE) *t=tolower(c);
                                    else *t=toupper(c);
				    t++; num++;
				    TokenReady=0;
				    /* fprintf(stderr,"."); */
				}
				state=tTOKEN;
			    }
			}
		    }
		}
	    }
	    break;
	  case tREM1:
            /*******************/
	    /*    REM1 state   */
            /*******************/
	    TokenReady=1;
	    sentback=c;
	    if (isSTK(c)) {
		state=tZERO;
	    } else {
		if isREM(c) {
		    sentback='\0';
		    state=tREM2;    /* it's a remmark. */
		    TokenReady=0;
		} else {
		    if isDQ(c) {
			sentback='\0';
			state=tSTRING;
		    } else {
			if isEOF(c) {
			    state=tEOF;
			    sentback=1;
			    TokenReady=0;
			} else {
			    if isEOT(c) {
				sentback='\0';
				/* there's no need to parse an EOT */
				state=tZERO;
			    } else {
				state=tTOKEN;
			    }
			}
		    }
		}
	    }
	    break;
	  case tREM2:
            /*******************/
	    /*    REM2 state   */
            /*******************/
	    sentback='\0';
	    TokenReady=0;
	    if isEOL(c) {
		num=0;
		t=&(Token[0]);
		state=tZERO;
	    } else {
		if isEOF(c) {
		    state=tEOF;
		    sentback=1;
		    TokenReady=0;
		} else {
		    state=tREM2;
		}
	    }
	    break;
	  case tSTRING:
            /*******************/
	    /*  STRING  state  */
            /*******************/
	    if (!str) {
		*t='"'; t++; num++; /* first '"' */
		str=1;
	    }
	    sentback='\0';
	    TokenReady=1;
	    if isDQ(c) {
		*t=c; t++; num++;
		state=tZERO;   /* There's no sentback char so this     *
				* double quote is the last one *ONLY*  */
	    } else {
		if isEOF(c) {
		    state=tEOF;    /* this is *UNESPECTED* ! */
		    sentback=1;
		    (void)fprintf(stderr,"*Parse Warning* Line %u: unespected Eof\n",line);
		} else {
		    sentback='\0';
		    if (num>=MAXTOKENLEN-2){
			sentback=c;
			(void)fprintf(stderr,"*Parse Warning* Line %u: token too long !\n",line);
		    } else {                                
			if (LOWERCASE) *t=tolower(c);
			else *t=toupper(c);
			t++; num++;
			TokenReady=0;
			state=tSTRING;
		    }
		}
	    }
	    break;
	  case tEOF:
            /*******************/
	    /*    EOF  state   */
            /*******************/
	    t=&(Token[0]);
	    TokenReady=1;
	    state=tEOF;
	    break;
	}
    } while(!TokenReady);
    *(t)='\0';
    (void)strcpy(tok,Token);
    return ;
}

/*====================================================================*


  ReleaseBit : deallocates an entire BITstruct structure
  ReleaseSIG : deallocates an entire SIGstruct structure
  NewCell    : allocates memory for a new cell


  ====================================================================*/

void ReleaseBit(ptr)
struct BITstruct *ptr;
{
    struct BITstruct *tmp;

    while(ptr!=NULL){
	tmp=ptr->next;
	free(ptr);
	ptr=tmp;
    }
}


void ReleaseSIG(ptr)
struct SIGstruct *ptr;
{
    struct SIGstruct *tmp;

    while(ptr!=NULL){
	tmp=ptr->next;
	free(ptr);
	ptr=tmp;
    }
}



void AddBIT(BITptr,name,dir)
struct BITstruct **BITptr;
char   *name;
char   dir;
{

   (*BITptr)->next=(struct BITstruct *)calloc(1,sizeof(struct BITstruct));
   if ( (*BITptr)->next==NULL) {
	Error("Allocation Error or not enought memory !");
    }
   (*BITptr)=(*BITptr)->next;
   (void)strcpy((*BITptr)->name,name);
   (*BITptr)->dir=dir;
   (*BITptr)->next=NULL;
}

void AddSIG(SIGptr,name,dir,start,end)
struct SIGstruct **SIGptr;
char   *name;
char   dir;
int    start;
int    end;
{

   (*SIGptr)->next=(struct SIGstruct *)calloc(1,sizeof(struct SIGstruct));
   if ( (*SIGptr)->next==NULL) {
	Error("Allocation Error or not enought memory !");
    }
   (*SIGptr)=(*SIGptr)->next;
   (void)strcpy((*SIGptr)->name,name);
   (*SIGptr)->dir=dir;
   (*SIGptr)->start=start;
   (*SIGptr)->end=end;
   (*SIGptr)->next=NULL;
#ifdef DEBUG
   (void)fprintf(stderr,"\n\t\tAdded SIGNAL <%s>, dir = %c, start =%d, end =%d",(*SIGptr)->name,(*SIGptr)->dir,(*SIGptr)->start,(*SIGptr)->end);
#endif
}



/* -=[ Warning ]=-                                  *
 * Puts a message on stderr, write the current line *
 * and then sends the current token back            *
 *                                                  *
 * Inputs :                                         *
 *      name = message                              */
void Warning(name)
char *name;
{
    if (WARNING) (void)fprintf(stderr,"*parse warning* Line %u : %s\n",line,name);
    SendTokenBack=1;
}

/* -=[ VstError ]=-                                 *
 * sends to stderr a message and then gets tokens   *
 * until a gicen one is reached                     *
 *                                                  *
 * Input :                                          *
 *     name = message to print                      *
 *     next = token to reach                        */ 
void VstError(name,next)
char *name;
char *next;
{
    char *w;
    char LocalToken[MAXTOKENLEN];

    w=&(LocalToken[0]);
    (void)fprintf(stderr,"*Error* Line %u : %s\n",line,name);
    (void)fprintf(stderr,"*Error* Line %u : skipping text until the keyword %s is reached\n",line,next);
    SendTokenBack=1;
    do{
	GetNextToken(w);
	if (feof(In))
	    Error("Unespected Eof!");
    } while( !KwrdCmp(w,next) );
}

/* -=[ DecNumber ]=-                                *
 * checks if a token is a decimal number            *
 *                                                  *
 * Input  :                                         *
 *     string = token to check                      *
 * Output :                                         *
 *     int = converted integer, or 0 if the string  *
 *           is not a number                        *
 * REMMARK : strtol() can be used...                */
int DecNumber(string)
char *string;
{
    char msg[50];
    char *s;

    for(s=string; *s!='\0'; s++)
	if (!isdigit(*s)) {
	    (void)sprintf(msg,"*Error Line %u : Expected decimal integer number \n",line);
	    Error(msg);
	}
    return atoi(string);
}


/*=========================================================================*


  Genlib scan
  ^^^^^^^^^^^

  PrintGates  : outputs the gates read if in DEBUG mode
  GetLibToken : tokenizer for the genlib file
  IsHere      : formals check
  ScanLibrary : parses the genlib files and builds the data structure.
  WhatGate    : checks for a cell into the library struct.

 *=========================================================================*/

/* -=[ PrintGates ]=-                               *
 * A kind debugger procedure ...                    *
 *                                                  *
 * Input  :                                         *
 *      cell = library file                         */
void PrintGates(cell)
struct LibCell *cell;
{
    struct Ports *ptr;
    int    j;
    
    while(cell->next!=NULL){
	if (cell->clk[0]) {
	    (void)fprintf(stderr,"Latch name: %s, num pins : %d\n",cell->name,cell->npins);
	} else {
	    (void)fprintf(stderr,"Cell name: %s, num pins : %d\n",cell->name,cell->npins);
	}
	ptr=cell->formals;
	for(j=0; j<cell->npins; j++, ptr++){
	    (void)fprintf(stderr,"\tpin %d : %s\n",j,ptr->name);
	}
	if (cell->clk[0]) {
	    (void)fprintf(stderr,"\tclock : %s\n",cell->clk);
	}
	cell=cell->next;
    }
}

/* -=[ GetTypeOfCell ]=-                            *
 * Scans the genlib data structure to find the      *
 * type of the cell                                 *
 *                                                  *
 * Input  :                                         *
 *      cell = library file                         *
 *      name = name of cell to match                *
 * Output :                                         *
 *      type of cell                                */
char GetTypeOfCell(cell,name)
struct LibCell *cell;
char *name;
{
    while(cell->next!=NULL){
	if (KwrdCmp(cell->name,name)) {
	    if (cell->clk[0]) {
		return 'L';   /* Library celly, type = L(atch) */
	    } else {
		return 'G';   /* Library celly, type = G(ate)  */
	    }
	}
	cell=cell->next;
    }
    return 'S';   /* type = S(ubcircuit) */
}

/* -=[ GetLibToken ]=-                              *
 * Tokenizer to scan the library file               *
 *                                                  *
 * Input  :                                         *
 *      Lib = library file                          *
 * Output :                                         *
 *      tok = filled with the new token             */
void GetLibToken(Lib,tok)
FILE *Lib;
char *tok;
{
    enum states { tZERO, tLONG, tEOF, tSTRING, tREM };
    static enum states next;
    static char init=0;
    static char sentback;
    static char TOKEN[MAXTOKENLEN];
    static char str;
    char   ready;
    int    num;
    char   *t;
    char   c;

    if (!init){
	sentback=0;
	init=1;
	next=tZERO;
	str=0;
    }

    t=&(TOKEN[0]);
    num=0;
    str=0;

    do{
	if (sentback){
	    c=sentback;
	} else {
	    c=fgetc(Lib);
	}
	if (feof(Lib)) next=tEOF;
	ready=0;
	sentback='\0';

	switch (next) {
	  case tZERO:
	    if ((c==' ') || (c=='\r') || (c=='\t')){
		next=tZERO;
	    } else {
		if (c=='#') {
                    next=tREM;
                } else {
		    if ( ((c>=0x27) && (c<=0x2b)) || (c=='=') || (c==';') || (c=='\n') || (c=='!')){
			*t=c; t++;
			next=tZERO;
			ready=1;
		    } else {
			if (c=='"') {
			    num=0;
			    next=tSTRING;
			} else
			    {
				num=0;
				next=tLONG;
				ready=0;
				sentback=c;
			    }
		    }
		}
	    }
	    break;
	  case tLONG:
	    if ((c==' ') || (c=='\r') || (c=='\t')){
		ready=1;
		next=tZERO;
	    } else {
		if ( ((c>=0x27) && (c<=0x2b)) || (c=='=') || (c==';') || (c=='\n') || (c=='!')){
		    next=tZERO;
		    ready=1;
		    sentback=c;
		} else {
		    if (c=='"') {
			ready=1;
			next=tSTRING;
		    } else {
                        if (c=='#') {
                            ready=1;
                            next=tREM;
			} else {
			    *t=c;
			    t++; num++;
			    next=tLONG;
			    if ( (ready=(num>=MAXTOKENLEN-1)) )
			    (void)fprintf(stderr,"Sorry, exeeded max name len of %u",num+1);
			}
		    }
		}
		   
	    }
	    break;
	  case tSTRING:
	    if (!str) {
		*t='"'; t++; num++;
		str=1;
#ifdef DEBUG
                (void)fprintf(stderr,"<%c>\n",c);
#endif
	    }
	    *t=c; t++; num++;
	    if (c=='"') {   /* last dblquote */
		ready=1;
		next=tZERO;
#ifdef DEBUG
		(void)fprintf(stderr,"STRING : %s\n",TOKEN);
#endif
		break;
	    }
	    next=tSTRING;
	    if ( (ready=(num>=MAXTOKENLEN-1)) )
		(void)fprintf(stderr,"Sorry, exeeded max name len of %u",num+1);
	    break;
	  case tREM:
	    next=tREM;
	    if (c=='\n'){
		sentback=c;
		next=tZERO;
	    }
	    break;
	  case tEOF:
	    next=tEOF;
	    ready=1;
	    sentback=c;
	    *t=c;
#ifdef DEBUG
	    (void)fprintf(stderr,"EOF\n");
#endif
	    break;
	  default :
	      next=tZERO;
	}
    } while(!ready);
    *t='\0';
    (void)strcpy(tok,&(TOKEN[0]));
}

/* -=[ IsHere ]=-                                   * 
 * Check is a name has been already used in the     *
 * expression. Here we look only at names, 'unused' * 
 * flag here is not useful.                         *
 *                                                  *
 * Input  :                                         *
 *     ptr  = pointer to list of formals            *
 *     name = name to check                         *
 * Output :                                         *
 *     pointer = if 'name' is used                  *
 *     NULL    = if used                            */
struct BITstruct *IsHere(name,ptr)
char *name;
struct BITstruct *ptr;
{
    struct BITstruct *BITptr;

    BITptr=ptr;
    while(BITptr->next!=NULL){
        BITptr=BITptr->next;
        if (KwrdCmp(name,BITptr->name)) return BITptr;
    }
    return (struct BITstruct *)NULL;
}

struct LibCell *NewLibCell(name,ports,latch)
char *name;
struct BITstruct *ports;
int latch;
{
    struct LibCell      *tmp;
    struct BITstruct *bptr;
    struct Ports     *pptr;
    int    j;
    int    num;
    
    if ( (tmp=(struct LibCell *)calloc(1,sizeof(struct LibCell)))== NULL ) {
	Error("Allocation Error or not enought memory !");
    }

    for(bptr=ports, num=1; bptr->next!=NULL; num++, bptr=bptr->next);
    if (latch ) num--;
	
    if ( (tmp->formals=(struct Ports *)calloc(1,num*sizeof(struct Ports)))==NULL){
	    Error("Allocation Error or not enought memory !");
	}

    (void)strcpy(tmp->name,name);
    tmp->next=NULL;
    tmp->npins=num; num--;
    for(bptr=ports->next, pptr=tmp->formals, j=0; (j<num) && (bptr!=NULL)
	                                  ; pptr++, j++, bptr=bptr->next ){
	if (j>num) Error("(NewLibCell) error ...");
#ifdef DEBUG
	(void)fprintf(stderr,"(NewLibCell):adding %s\n",bptr->name);
#endif
	(void)strcpy(pptr->name,bptr->name);
    }
    (void)strcpy(pptr->name,ports->name); /* output is the last one */
    if (latch) {
	(void)strcpy(tmp->clk,bptr->name);
#ifdef DEBUG
	(void)fprintf(stderr,"(NewLibCell):clock %s\n",bptr->name);
#endif
    } else {
	tmp->clk[0]='\0';
    }
    ReleaseBit(ports);
    return tmp;
}


/* -=[ ScanLibrary ]=-                              *
 * Scans the library to get the namesof the cells   *
 * the output pins and the clock signals of latches *
 *                                                  *
 * Input :                                          *
 *     LibName = the name of library file           */
struct LibCell *ScanLibrary(LibName)
char *LibName;
{
    enum states { sZERO, sPIN, sCLOCK, sADDCELL } next;
    FILE *Lib;
    struct LibCell      *cell;
    struct LibCell      first;
    struct BITstruct *tmpBIT;
    struct BITstruct firstBIT;
    char LocalToken[MAXTOKENLEN];
    char name[MAXNAMELEN];
    int  latch;
    char *s;
    

    if ((Lib=fopen(LibName,"rt"))==NULL)
	Error("Couldn't open library file");


/*    first.next=NewLibCell("_dummy_",(struct BITstruct *)NULL,LIBRARY); */
    firstBIT.name[0]='\0'; firstBIT.next=NULL;
    cell=&first;
    s=&(LocalToken[0]);
    latch=0; next=sZERO;
    (void)fseek(Lib,0L,SEEK_SET);
    tmpBIT=&firstBIT;
    do {
	GetLibToken(Lib,s);
	switch (next) {
	  case sZERO:
	    next=sZERO;
	    if (KwrdCmp(s,"GATE")) {
		latch=0;
		GetLibToken(Lib,s);
		(void)strcpy(name,s);
		GetLibToken(Lib,s);   /* area */
		next=sPIN;
#ifdef DEBUG
		(void)fprintf(stderr,"Gate name: %s\n",name);
#endif
	    } else {
		if (KwrdCmp(s,"LATCH")) {
		    latch=1;
		    GetLibToken(Lib,s);
		    (void)strcpy(name,s);
		    GetLibToken(Lib,s);   /* area */
		    next=sPIN;
#ifdef DEBUG
		    (void)fprintf(stderr,"Latch name: %s\n",name);
#endif
		}
	    }
	    break;
	  case sPIN:
	    if ( !( ((*s>=0x27) && (*s<=0x2b)) || (*s=='=') || (*s=='!')|| (*s==';')) ){
/*
		(void)strncpy(tmp,s,5);
		if (KwrdCmp(tmp,"CONST") && !isalpha(*(s+6)))
*/
		    /* if the expression has a constant value we must */
		    /* skip it, because there are no inputs           */
/*
		    break;
*/
		/* it's an operand so get its name */
#ifdef DEBUG
		(void)fprintf(stderr,"\tpin read : %s\n",s);
#endif
		if (IsHere(s,&firstBIT)==NULL) {
#ifdef DEBUG
		    (void)fprintf(stderr,"\tunknown pin : %s --> added!\n",s);
#endif
		    AddBIT(&tmpBIT,s);
		}
	    }
	    if (*s==';') {
		if (latch) {
		    next=sCLOCK;
		} else {
		    next=sADDCELL;
		}
	    } else {
		next=sPIN;
	    }
	    break;
	  case sCLOCK:
	    if (KwrdCmp(s,"CONTROL")){
		GetLibToken(Lib,s);
#ifdef DEBUG
		(void)fprintf(stderr,"\tcontrol pin : %s\n",s);
#endif
		AddBIT(&tmpBIT,s);
		next=sADDCELL;
	    } else {
		next=sCLOCK;
	    }
	    break;
	  case sADDCELL:
#ifdef DEBUG
	      (void)fprintf(stderr,"\tadding cell to library\n");
#endif
	    cell->next=NewLibCell(name,firstBIT.next,latch);
	    tmpBIT=&firstBIT; firstBIT.next=NULL;
	    cell=cell->next;
	    next=sZERO;
	    break;
	}
    } while (!feof(Lib));

    if ((first.next)->next==NULL) {
	(void)sprintf("Library file %s does *NOT* contains gates !",LibName);
	Error("could not continue with an empy library");
    }
#ifdef DEBUG
    (void)fprintf(stderr,"end of lib\n");
    PrintGates(first.next);
#endif
    return (struct LibCell *)first.next;
}


/* -=[ WhatGate ]=-                                 *
 * Returns a pointer to an element of the list      *
 * of gates that matches up the name given, if      *
 * ther isn't a match a null pointer is returned    *
 *                                                  *
 * Input :                                          *
 *     name = name to match                         *
 *     LIBRARY = genlib data struct                 *
 * Ouput :                                          *
 *     (void *) a pointer                           */
struct LibCell *WhatLibGate(name,LIBRARY)
char *name;
struct LibCell *LIBRARY;
{
    struct LibCell  *ptr;

    for(ptr=LIBRARY; ptr!=NULL ; ptr=ptr->next)
	if ( KwrdCmp(ptr->name,name) ) return ptr;

    return (struct LibCell *)NULL;
}

/*=========================================================================*



 *=========================================================================*/

struct Cell *NewCell(name,Bports,Fports,Genlib)
char *name;
struct SIGstruct *Bports;
struct BITstruct *Fports;
struct LibCell *Genlib;
{
    struct Cell      *tmp;
    struct BITstruct *Fptr;
    struct Ports     *Pptr;
    struct Ports     *Lpptr;
    struct LibCell   *Lptr;
    int    j;
    int    num;
    char   t;
 
    if ( (tmp=(struct Cell *)calloc(1,sizeof(struct Cell)))== NULL ) {
	Error("Allocation Error or not enought memory !");
    }

    if (Bports!=NULL){
	j=1;
	Fptr=Fports;
	while(Fptr->next!=NULL){
	    j++; Fptr=Fptr->next;
	}
	num=j;
	t=GetTypeOfCell(Genlib,name);
	if (t=='L') j++;
	
	if ( (tmp->formals=(struct Ports *)calloc(1,j*sizeof(struct Ports)))==NULL){
	    Error("Allocation Error or not enought memory !");
	}
	(void)strcpy(tmp->name,name);
	tmp->npins=j;
	tmp->next=NULL;
	tmp->type=t;

	if (t=='S') {
	    /*                                           *
	     * Is a subcircuit, no ordering is necessary *
	     *                                           */
	    Fptr=Fports;
	    Pptr=tmp->formals;
	    j=0;
	    while(Fptr!=NULL){
		if (j>num) Error("(NewCell) error ...");
#ifdef DEBUG
		(void)fprintf(stderr,"(NewCell): adding %s\n",Fptr->name);
#endif
		(void)strcpy(Pptr->name,Fptr->name);
		Fptr=Fptr->next;
		Pptr++; j++;
	    }
	    ReleaseBit(Fports->next);
	    tmp->io=Bports->next;
	    Bports->next=NULL;
	} else {
	    /*                                            *
	     * Is a library cell, let's order the signals *
	     * to simplify the final output               * 
	     *                                            */	     
	    Lptr=WhatLibGate(name,Genlib);
	    Pptr=tmp->formals;
	    if (t=='L') num--;
	    for(j=1, Lpptr=Lptr->formals; j<num; j++, Lpptr++, Pptr++) {
		Fptr=Fports->next;
#ifdef DEBUG
		(void)fprintf(stderr,"\npin %s \n",Lpptr->name);
#endif
		while(Fptr!=NULL){
#ifdef DEBUG
		    (void)fprintf(stderr,"is %s ?\n",Fptr->name);
#endif
		    if (KwrdCmp(Fptr->name,Lpptr->name)) {
			(void)strcpy(Pptr->name,Fptr->name);
			break;
		    }
		    Fptr=Fptr->next;
		}
		if (Fptr==NULL) (void) Error("Mismatch between COMPONENT declaration and library one");
	    }
	    if (t=='L') {
		Fptr=Fports->next;
		while(Fptr!=NULL){
		    if (KwrdCmp(Fptr->name,Lptr->clk)) {
			(void)strcpy(Pptr->name,Fptr->name);
			break;
		    }
		    Fptr=Fptr->next;
		}
		if (Fptr==NULL) (void) Error("Mismatch between COMPONENT declaration and libray one");
	    }
	    ReleaseBit(Fports->next);
	    tmp->io=Bports->next;
	    Bports->next=NULL;
	    
	}

    } else {
	tmp->formals=NULL;
    }
    return tmp;
}


/* -=[ WhatGate ]=-                                 *
 * Returns a pointer to an element of the list      *
 * of gates that matches up the name given, if      *
 * ther isn't a match a null pointer is returned    *
 *                                                  *
 * Input :                                          *
 *     name = name to match                         *
 *     LIBRARY = components data struct             *
 * Ouput :                                          *
 *     (void *) a pointer                           */
struct Cell *WhatGate(name,LIBRARY)
char *name;
struct Cell *LIBRARY;
{
    struct Cell  *ptr;

    for(ptr=LIBRARY; ptr!=NULL ; ptr=ptr->next)
	if ( KwrdCmp(ptr->name,name) ) return ptr;

    return (struct Cell *)NULL;
}


/* -=[ GetPort ]=-                                 *
 * Gets the port definition of an ENTITY or of a   *
 * COMPONENT.                                      *
 *                                                 */
struct Cell *GetPort(Cname)
char *Cname;
{
    enum states { sFORMAL, sCONN, sANOTHER, sDIR, sTYPE, sVECTOR, sWAIT } next;
    struct SIGstruct BITstart;
    struct BITstruct FORMstart;
    struct SIGstruct *BITptr;
    struct BITstruct *FORMptr;
    struct BITstruct *TMPptr;
    struct BITstruct TMPstart;
    char *w;
    char LocalToken[MAXTOKENLEN];
    char tmp[MAXNAMELEN];
    char dir;
    int  Cont;
    int  Token;
    int  start;
    int  end;
    int  j;
    int  num;
    

    w=&(LocalToken[0]);

    GetNextToken(w);
    if ( *w!='(' ) Warning("expected '('");

    BITptr=&BITstart;
    FORMptr=&FORMstart;
    TMPstart.next=NULL;

    dir = '\0';
    Token=1; start=0; end=0;
    next=sFORMAL;
    Cont=1; num=0;
    do{
	/* name of the port */
	if (Token) GetNextToken(w);
	else Token=1;
	switch (next) {
	  case sFORMAL:
#ifdef DEBUG
	      (void)fprintf(stderr,"\n\n** getting formal : %s",w);
#endif
	    (void)strcpy(TMPstart.name,w);
	    TMPptr=&TMPstart;
	    next=sCONN;
	    break;
	  case sCONN:
	    next=sDIR;
	    if (*w!=':') {
		if (*w==',') {
		    next=sANOTHER;
		} else {
		    Warning("Expected ':' or ',' ",line);
		    Token=0;
		}
	    }
	    break;
	  case sANOTHER:
#ifdef DEBUG
	      (void)fprintf(stderr,"\n*** another input : %s",w); 
#endif
	    if (TMPptr->next==NULL) {
		AddBIT(&TMPptr,w,1);
	    } else {
		TMPptr=TMPptr->next;
		(void)strcpy(TMPptr->name,w);
	    }
	    num++;
	    next=sCONN;
	    break;
	  case sDIR:
#ifdef DEBUG
	      (void)fprintf(stderr,"\n\tdirection = %s",w);
#endif
	    if (KwrdCmp(w,"IN")){
		dir='i';
	    } else {
		if (KwrdCmp(w,"OUT")) {
		    dir='o';
		} else {
		    if (KwrdCmp(w,"INOUT")){
			dir='u';
		    } else {
			if (KwrdCmp(w,"LINKAGE")){
			    dir=LINKAGE;
			} else {
			    (void)fprintf(stderr,"* Error * Line %u : unknown direction of a port\n",line);
			    Error("Could not continue");
			}
		    }
		}
	    }
	    next=sTYPE;
	    break;
	  case sTYPE:
	    if (KwrdCmp(w,"BIT")) {
		next=sWAIT;
		start=end;
	    } else {
		if (KwrdCmp(w,"BIT_VECTOR")) {
#ifdef DEBUG
		    (void)fprintf(stderr,"\n\tvector, ");
#endif
		    next=sVECTOR;
		}
	    }
	    break;
	  case sVECTOR:
	    if (*w!='(') {
		Warning("Expected '('");
	    } else {
		GetNextToken(w);
	    }
	    start=DecNumber(w);
	    GetNextToken(w);
	    if (!KwrdCmp(w,"TO") && !KwrdCmp(w,"DOWNTO")){
		Warning("Expected keword TO or DOWNTO");
	    } else {
		GetNextToken(w);
	    }
	    end=DecNumber(w);
	    GetNextToken(w);
	    if (*w!=')') {
		Warning("Expected ')'");
		Token=1;
	    }
	    next=sWAIT;
#ifdef DEBUG
	      (void)fprintf(stderr," from %d to %d",start,end);
#endif
	    break;
	  case sWAIT:
	    TMPptr=&TMPstart;
	    /* if (num>0) num--; */
#ifdef DEBUG
	      (void)fprintf(stderr,"\nPower?");
#endif
	    if (!(KwrdCmp(TMPptr->name,VDD) || KwrdCmp(TMPptr->name,VSS))){
#ifdef DEBUG
		(void)fprintf(stderr," no\n");
#endif

	    while(num>=0){
#ifdef DEBUG
		(void)fprintf(stderr,"\n\n%d)working on %s",num,TMPptr->name);
#endif
		if (start==end) {
#ifdef DEBUG
		    (void)fprintf(stderr,"\nadded bit & formal of name : %s",TMPptr->name);
#endif
		    AddSIG(&BITptr,TMPptr->name,dir,0,0);
		    AddBIT(&FORMptr,TMPptr->name,dir);
		} else {
		    if (start>end){
			for(j=start; j>=end; j--) {
			    (void)sprintf(tmp,"%s[%d]",TMPptr->name,j);
#ifdef DEBUG
			    (void)fprintf(stderr,"\nadded formal of name : %s",tmp);
#endif
			    AddBIT(&FORMptr,tmp,dir);
			}
#ifdef DEBUG
			(void)fprintf(stderr,"\nadded vector of name : %s[%d..%d]",TMPptr->name,start,end);
#endif
		    } else {
			for(j=start; j<=end; j++) {
			    (void)sprintf(tmp,"%s[%d]",TMPptr->name,j);
#ifdef DEBUG
			    (void)fprintf(stderr,"\nadded formal of name : %s",tmp);
#endif
			    AddBIT(&FORMptr,tmp,dir);
			}
#ifdef DEBUG
			(void)fprintf(stderr,"\nadded vector of name : %s[%d..%d]",TMPptr->name,start,end);
#endif
		    }
		    AddSIG(&BITptr,TMPptr->name,dir,start,end);
		}
		TMPptr=TMPptr->next;
		num--;
	    }

	}
	    num=0;
	    if (*w==';') {
		    next=sFORMAL;
		} else {
		    if (*w!=')') {
		    VstError("Missing ')' or ';'","END");
		} else {
		    GetNextToken(w);  /* ; */
		    if (*w != ';') {
			Warning("Missing ';'");
		    } else {
			GetNextToken(w);  /* end */
		    }
		    if (!KwrdCmp(w,"END")){
			VstError("Missing END keyword","END");
		    }
		}
		Cont=0;
	    }
	    break;
	}

    } while (Cont);
    ReleaseBit(TMPstart.next);
    return NewCell(Cname,&BITstart,&FORMstart,LIBRARY);
}


/* -=[ GetEntity ]=-                                *
 * parses the entity statement                      */
void GetEntity(Entity)
struct Cell **Entity;
{
    char *w;
    char LocalToken[MAXTOKENLEN];
    char name[MAXTOKENLEN];
    int  num;

    w=&(LocalToken[0]);

    /* name of the entity = name of the model */
    GetNextToken(w);
    (void)strcpy(name,w);
    GetNextToken(w);
    if (!KwrdCmp(w,"IS")) Warning("expected syntax: ENTITY <name> IS");

    /* GENERIC CLAUSE */
    GetNextToken(w);
    if (KwrdCmp(w,"GENERIC")) {
	GetNextToken(w);
	if (*w!='(') Warning("expected '(' after GENERIC keyword");
	num=1;
	do {
	    GetNextToken(w);
	    if (*w=='(') num++;
	    else {
		if (*w==')') num--;
	    }
	} while (num!=0);
	GetNextToken(w);
	if (*w!=';') Warning("expected ';'");
	GetNextToken(w);
    }

    /* PORT CLAUSE */
    if (KwrdCmp(w,"PORT")) {
	(*Entity)=GetPort(name);
    } else {
	Warning("no inputs or outputs in this entity ?!");
    }

    GetNextToken(w);
    if (!KwrdCmp(w,name))
	Warning("<name> after END differs from <name> after ENTITY");

    GetNextToken(w);
    if (*w!=';') Warning("expected ';'");
}

/* -=[ GetComponent ]=-                             *
 * Parses the component statement                   */
void GetComponent(cell)
struct Cell **cell;
{
    char *w;
    char LocalToken[MAXTOKENLEN];
    char name[MAXNAMELEN];

    w=&(LocalToken[0]);
    /* component name */
    GetNextToken(w);
    (void)strcpy(name,w);
#ifdef DEBUG
    (void)fprintf(stderr,"\nParsing component %s\n",name);
#endif

    /* A small checks may be done here ... next time */

    /* PORT CLAUSE */
    GetNextToken(w);
    if (KwrdCmp(w,"PORT")) {
	(*cell)->next=GetPort(name);
	(*cell)=(*cell)->next;
    } else {
	Warning("no inputs or outputs in this component ?!");
    }


    /* END CLAUSE */
    GetNextToken(w);
    if (!KwrdCmp(w,"COMPONENT")) {
	Warning("COMPONENT keyword missing !");
    }

    GetNextToken(w);
    if (*w!=';') Warning("expected ';'");

}

/* -=[ GetSignal ]=-                                 *
 * Skips the signal definitions                      */
void GetSignal(Internals)
struct SIGstruct **Internals;
{
    struct SIGstruct *SIGptr;
    struct SIGstruct *SIGstart;
    char *w;
    char LocalToken[MAXTOKENLEN];
    char name[MAXNAMELEN];
    char dir;
    int vect;
    int  start;
    int  end;


    SIGstart=(struct SIGstruct *)calloc(1,sizeof(struct SIGstruct));
    if (SIGstart==NULL) {
	Error("Allocation Error or not enought memory !"); 
    }
    SIGptr=SIGstart;

    w=&(LocalToken[0]);

    dir = '\0';

    do {
	GetNextToken(w);
#ifdef DEBUG
	(void)fprintf(stderr,"\n\n** getting signal %s",w);
#endif
	AddSIG(&SIGptr,w,'*',999,999);
	GetNextToken(w);
    } while (*w==',');

    if (*w!=':') {
	Warning("Expected ':'");
    } else {
	GetNextToken(w);
    }
	
    start=0; end=0;
    vect=0;
    if (*(w+3)=='_') {
	*(w+3)='\0';
	vect=1;
    }
    if (KwrdCmp(w,"BIT")) {
	dir='b';
    } else {
	if (KwrdCmp(w,"MUX")) {
	    dir='m';
	} else {
	    if (KwrdCmp(w,"WOR")) {
		dir='w';
	    }
	}
    }
    if (vect && !KwrdCmp(w+4,"VECTOR")) {
	(void)fprintf(stderr," Unknown signal type : %s\n",w);
	Error("could not continue.");
    }

    if (vect) {
	GetNextToken(w);
	if (*w!='(') {
	    Warning("Expected '('");
	} else {
	    GetNextToken(w);
	}
	start=DecNumber(w);
	GetNextToken(w);
	if (!KwrdCmp(w,"TO") && !KwrdCmp(w,"DOWNTO")){
	    Warning("Expected keword TO or DOWNTO");
	} else {
	    GetNextToken(w);
	}
	
	end=DecNumber(w);
	GetNextToken(w);
	if (*w!=')') {
	    Warning("Expected ')'");
	}
#ifdef DEBUG
	(void)fprintf(stderr," from %d to %d",start,end);
#endif
    }
    
    GetNextToken(w);
    if (dir!='b') {
	if (!KwrdCmp(w,"BUS")) {
	    Warning(" Keyword 'BUS' expected");
	} else {
	    GetNextToken(w);
	}
    }

    SIGptr=SIGstart;
    while (SIGptr->next!=NULL) {
	SIGptr=SIGptr->next;
	if (vect) {
#ifdef DEBUG
	    (void)fprintf(stderr,"\nadded vector of name : %s[%d..%d]",name,start,end);
#endif
	    AddSIG(Internals,SIGptr->name,dir,start,end);
	} else {
#ifdef DEBUG
	    (void)fprintf(stderr,"\nadded bit of name : %s",name);
#endif
	    AddSIG(Internals,SIGptr->name,dir,0,0);
	}
    }
    ReleaseSIG(SIGstart);
    
    if (*w!=';') {
	Warning("expected ';'");
    }

}


void FillTerm(TERM,Entity,which,WhatCell)
struct TERMstruct   *TERM;
struct ENTITYstruct *Entity;
int    which;
struct Cell      *WhatCell;
{
    struct Cell      *cell;
    struct SIGstruct *Sptr;

#ifdef DEBUG
    (void)fprintf(stderr,"filling dimension of %s\n",TERM->name);
#endif
    if (which) {
#ifdef DEBUG
	(void)fprintf(stderr,"searchin into component %s\n",WhatCell->name);
#endif
	cell=Entity->Components;
	while(cell!=NULL){
	    if (!strcmp(cell->name,WhatCell->name)) {
		Sptr=cell->io;
		while (Sptr!=NULL){
		    if (KwrdCmp(TERM->name,Sptr->name)){
			TERM->start=Sptr->start;
			TERM->end=Sptr->end;
			return;
		    }
		    Sptr=Sptr->next;
		}
	    }
	    cell=cell->next;
	}
    } else {
	Sptr=(Entity->EntityPort)->io;
	while (Sptr!=NULL){
#ifdef DEBUG
	    (void)fprintf(stderr,"searchin into io\n");
#endif
	    if (KwrdCmp(TERM->name,Sptr->name)){
		TERM->start=Sptr->start;
		TERM->end=Sptr->end;
		return;
	    }
	    Sptr=Sptr->next;
	}
	Sptr=Entity->Internals;
	while (Sptr!=NULL){
#ifdef DEBUG
	    (void)fprintf(stderr,"searchin into internals\n");
#endif
	    if (KwrdCmp(TERM->name,Sptr->name)){
		TERM->start=Sptr->start;
		TERM->end=Sptr->end;
		return;
	    }
	    Sptr=Sptr->next;
	}
	if (KwrdCmp(TERM->name,VSS) || KwrdCmp(TERM->name,VDD)) {
	    TERM->start=0; TERM->end=0;
	    return;
	}
	(void)fprintf(stderr,"Signal name %s not declared",TERM->name);
	Error("Could not continue");
    }
}


/* -=[ GetName ]=-                                   *
 * Gets a name of an actual terminal, that can be    *
 * a single token or 3 tokens long (if it is an      *
 * element of a vector )                             *
 *                                                   *
 * Output :                                          *
 *     name = the name read                          */
void GetName(TERM,Entity,which,WhatCell)
struct TERMstruct   *TERM;
struct ENTITYstruct *Entity;
int   which;
struct Cell      *WhatCell;
{
    char *w;
    char LocalToken[MAXTOKENLEN];

    TERM->start=-1; TERM->end=-1;
    w=&(LocalToken[0]);
    do {
	GetNextToken(w);
#ifdef DEBUG
	(void)fprintf(stderr,"Parsing %s\n",w);
#endif
    } while ((*w==',') || (*w=='&'));
    (void)strcpy(TERM->name,w);
    GetNextToken(w);
#ifdef DEBUG
    (void)fprintf(stderr,"then  %s\n",w);
#endif
    if (*w!='(') {
	FillTerm(TERM,Entity,which,WhatCell);
	if ((TERM->start==TERM->end) && (TERM->start==0)) {
	    TERM->start=-1; TERM->end=-1;
	}
	SendTokenBack=1;
	/* Don't lose this token */
	return;
    } else {
#ifdef DEBUG
	(void)fprintf(stderr,"got (\n");
#endif
    }
    GetNextToken(w);
    TERM->start=DecNumber(w);
    GetNextToken(w);
    if (*w!=')') {
	if (!KwrdCmp(w,"TO") && !KwrdCmp(w,"DOWNTO")){
	    Error("expected ')' or 'TO' or 'DOWNTO', could not continue");
	} else {
	    GetNextToken(w);
	    TERM->end=DecNumber(w);
	    GetNextToken(w);
	    if (*w!=')') {
		Warning("Expected ')'");
		SendTokenBack=1;
	    }
	}
    } else {
#ifdef DEBUG
	(void)fprintf(stderr,"--> just an element of index %d\n",TERM->start);
#endif
	TERM->end=TERM->start;
    }
}


void ChangInternal(Intern,name)
struct SIGstruct *Intern;
char *name;
{
    struct SIGstruct *Sptr;
    
#ifdef DEBUG
    (void)fprintf(stderr,"\n ** scanning into internals --> Int=%p",Intern);
#endif
    Sptr=Intern;
    while(Sptr!=NULL){
#ifdef DEBUG
	(void)fprintf(stderr,"\t <%s>",Sptr->name);
#endif
	if (!strcmp(Sptr->name,name)) {
	    char i;
	    i = *name;
	    i= ( (i<='z') && (i>='a') ? i+('A'-'a') : i);
	    *name=i;
#ifdef DEBUG
	    (void)fprintf(stderr,"\n *** %s : e' un segnale interno\n",name);
#endif
	    return;
	}
	Sptr=Sptr->next;
    }
}
/* -=[ GetInstance ]=-                              *
 * parses the netlist                               */
struct Instance *GetInstance(name,Entity)
char *name;
struct ENTITYstruct *Entity;
{
    struct Cell       *cell;
    struct Instance   *INST;
    struct TERMstruct FORMterm;
    struct TERMstruct ACTterm;
    struct SIGstruct  *ACTtrms;
    struct SIGstruct  *ACTptr;
    struct Ports      *Cptr;
    struct Ports      *Aptr;
    char   *w;
    char   LocalToken[MAXTOKENLEN];
    int    j;
    char   Iname[MAXTOKENLEN+10];
    int    incF,incA;
    int    iiF;
    int    iF,iA;

    w=&(LocalToken[0]);
    GetNextToken(w);  /* : */
    GetNextToken(w);
    cell=WhatGate(w,Entity->Components);
#ifdef DEBUG
    (void)fprintf(stderr,"\n=========================\nParsing instance %s of component %s\n==========================\n",name,cell->name);
#endif

    INST=(struct Instance *)calloc(1,sizeof(struct Instance));
    if (INST==NULL) { 
	Error("Allocation Error or not enought memory !"); 
    }
    (void)strcpy(INST->name,name);
    INST->what=WhatGate(w,Entity->Components);
    INST->actuals=
	(struct Ports *)calloc(1,((INST->what)->npins)*sizeof(struct Ports));
    if (INST->actuals==NULL){
	Error("Allocation Error or not enought memory !");
    }

    ACTtrms=(struct SIGstruct *)calloc(1,sizeof(struct SIGstruct));
    if (ACTtrms==NULL) {
	Error("Allocation Error or not enought memory !"); 
    }
	

    GetNextToken(w);
    if (!KwrdCmp(w,"PORT")) {
	Warning("PORT keyword missing","PORT");
    }

    GetNextToken(w);
    if (!KwrdCmp(w,"MAP"))
	Warning("MAP keyword missing");

    GetNextToken(w);
    if (*w!='(')
	Warning("Expexcted '('");

    do{
	GetName(&FORMterm,Entity,1,cell);
#ifdef DEBUG
	(void)fprintf(stderr,"\t formal : %s\n",FORMterm.name);
#endif

	GetNextToken(w);
	if (*w!='=') {
	    if (*w!='>') {
		Warning("Expected '=>'");
		SendTokenBack=1;
	    }
	} else {
	    GetNextToken(w);
	    if (*w!='>') {
		Warning("Expected '=>'");
		SendTokenBack=1;
	    }
	}

	ACTptr=ACTtrms;
	do {
	    GetName(&ACTterm,Entity,0,cell);
#ifdef DEBUG
	    (void)fprintf(stderr,"\t actual : %s .. %s\n",ACTterm.name,w);
#endif
	    AddSIG(&ACTptr,ACTterm.name,'\0',ACTterm.start,ACTterm.end);
	    ChangInternal(Entity->Internals,ACTptr->name);
#ifdef DEBUG
	    (void)fprintf(stderr,"\nafter GetInstance: %s %s\n",ACTterm.name,ACTptr->name);
#endif
	    GetNextToken(w);
	} while (*w=='&');

	if ((*w!=',') && (*w!=')')) {
	    Error("Expected ')' or ','");
	}
#ifdef DEBUG
	(void)fprintf(stderr,"----> %s\n",w);
#endif
	if (FORMterm.start==FORMterm.end) {
#ifdef DEBUG
	    if (DEBUG) (void)fprintf(stderr,"after if (FORMterm.start==FORMterm.end) {: %s %s\n",ACTterm.name,ACTptr->name);
#endif
	    Cptr=(INST->what)->formals;
	    Aptr=INST->actuals; 
	    for(j=0; j<(INST->what)->npins; j++, Cptr++, Aptr++){
		if (KwrdCmp(Cptr->name,FORMterm.name)) break;
	    }
	    if ((ACTterm.start!=ACTterm.end) || ((ACTtrms->next)->next!=NULL)) {
		Warning("Actual vector's dimension differs to formal's one");
	    }
#ifdef DEBUG
	    (void)fprintf(stderr,"value: %d %s\n",ACTptr->start,ACTptr->name);
#endif
	    if (ACTptr->start!=-1) {
		if (isupper((ACTptr->name)[0])) {
		    (void)sprintf(Aptr->name,"%s_%d_",ACTptr->name,ACTterm.start);
		} else {
		    (void)sprintf(Aptr->name,"%s[%d]",ACTptr->name,ACTterm.start);
		}
	    } else {
		(void)strcpy(Aptr->name,ACTterm.name);
	    }
	} else {
#ifdef DEBUG
	    (void)fprintf(stderr,"\t\tthey are vectors --> formal from %d to %d\n",FORMterm.start,FORMterm.end);
#endif
	    incF = (FORMterm.start>FORMterm.end ? -1 : 1 );
	    if (incF<0) {
		/*
		 * Downto
		 */
		iiF=FORMterm.end;
	    } else {
		/*
		 *  To
		 */
		iiF=FORMterm.start;
	    }
	    ACTptr=ACTtrms;
	    incA=0; ACTptr->end=0; iA=0;
	    iF=FORMterm.start;
	    do {
		/* 
		 *  Let's look if we need a new element...
		 */
		if (iA==(ACTptr->end+incA)) {
		    if (ACTptr->next==NULL) {
			Error("Wrong vector size in assignement");
		    }
		    ACTptr=ACTptr->next;
		    incA = (ACTptr->start > ACTptr->end ? -1 : 1);
		    iA=ACTptr->start;
#ifdef DEBUG
		    (void)fprintf(stderr,"ACTUAL changed!\ncurent : <%s> from %d to %d\n",ACTptr->name,ACTptr->start,ACTptr->end);
#endif
		}

		/*
		 *  let's make the connection
		 */
		(void)sprintf(Iname,"%s[%d]",FORMterm.name,iiF);
		Cptr=(INST->what)->formals;
		Aptr=INST->actuals;
		for(j=0; j<(INST->what)->npins; j++, Cptr++, Aptr++){
#ifdef DEBUG
		    (void)fprintf(stderr,"(%d)\tAptr->name=<%s>\tCptr->name=<%s>\tIname=<%s>\n",j,Aptr->name,Cptr->name,Iname);
#endif
		    if (KwrdCmp(Cptr->name,Iname)) break;
		}
		if (ACTptr->start<0) {
		    /*
		     * ACTual is a BIT
		     */
		    if (isupper( (ACTptr->name)[0])) {
			(void)sprintf(Aptr->name,"%s",ACTptr->name);
		    } else {
			(void)sprintf(Aptr->name,"%s",ACTptr->name);
		    }
		} else {
		    /*
		     * ACTual is a VECTOR
		     */
		    if (isupper( (ACTptr->name)[0])) {
			(void)sprintf(Aptr->name,"%s_%d_",ACTptr->name,iA);
		    } else {
			(void)sprintf(Aptr->name,"%s[%d]",ACTptr->name,iA);
		    }
		}
#ifdef DEBUG
		(void)fprintf(stderr,"%d)--> added ACTconn : <%s>\n",iF,Aptr->name);
#endif
		iF+=incF; iA+=incA; iiF++;
	    } while (iF!=(FORMterm.end+incF));
	    ReleaseSIG(ACTtrms->next);
#ifdef DEBUG
	    (void)fprintf(stderr,"---->release done<----\n ");
#endif
/*
	    for(iF=FORMterm.start, iA=ACTterm.start; iF!=FORMterm.end+incF; iF+=incF, iA+=incA) {
		sprintf(Iname,"%s[%d]",FORMterm.name,iF);
		Cptr=(INST->what)->formals;
		Aptr=INST->actuals;
		for(j=0; j<(INST->what)->npins; j++, Cptr++, Aptr++){
		    if (KwrdCmp(Cptr->name,Iname)) break;
		}
		if (isupper(ACTterm.name[0])) {
		    sprintf(Aptr->name,"%s_%d_",ACTterm.name,ACTterm.start);
		} else {
		    sprintf(Aptr->name,"%s[%d]",ACTterm.name,iA);
		}
	    }
	    if (iA!=ACTterm.end+incA) {
		Warning("Actual vector's dimension differs to formal's one");
	    }
*/
	}
    } while (*w!=')');

    free(ACTtrms);
    GetNextToken(w);
    if (*w!=';') {
	Warning("expected ';'");
    }
    return INST;
}

/* -=[ GetArchitecture ]=-                    *
 * parses the structure 'ARCHITECTURE'        */
void GetArchitecture(ENTITY)
struct ENTITYstruct *ENTITY;
{
    struct Cell      *COMPOptr;
    struct SIGstruct *SIGptr;
    struct Instance  *INSTptr;
    struct Cell      COMPOstart;
    struct SIGstruct SIGstart;
    struct Instance  INSTstart;
    char *w;
    char LocalToken[MAXTOKENLEN];
    char name[MAXTOKENLEN];
    char msg[MAXTOKENLEN];
    

    SIGptr=&SIGstart; SIGstart.next=NULL;
    COMPOptr=&COMPOstart; COMPOstart.next=NULL;
    SIGptr=&SIGstart;

    w=&(LocalToken[0]);
    /* type of architecture... */
    GetNextToken(w);

    GetNextToken(w);
    if (!KwrdCmp(w,"OF"))
	Warning("expected syntax: ARCHITECTURE <type> OF <name> IS");
    GetNextToken(w);
    (void)strcpy(name,w);
    GetNextToken(w);
    if (!KwrdCmp(w,"IS"))
	Warning("expected syntax: ENTITY <name> IS");

    /* Components and signals: before a 'BEGIN' only sturcture *
     * COMPONENT and SIGNAL are allowed                        */
    do{
	GetNextToken(w);
	if (KwrdCmp(w,"COMPONENT")){
	    GetComponent(&COMPOptr);
	} else {
	    if (KwrdCmp(w,"SIGNAL")){
		GetSignal(&SIGptr); 
	    } else {
		if (KwrdCmp(w,"BEGIN")) break;
		else {
		    (void)sprintf(msg,"%s unknown, skipped",w);
		    Warning(msg);
		    SendTokenBack=0; /* as we said we must skip it */
		}
	    }
	}
	if (feof(In)) Error("Unespected EoF");
    } while(1);

    ENTITY->Internals=SIGstart.next;
    ENTITY->Components=COMPOstart.next;
    /* NETLIST */
    INSTptr=&INSTstart;
    do{
	GetNextToken(w);
	if (KwrdCmp(w,"END")) {
	    break;
	} else {
	    INSTptr->next=GetInstance(w,ENTITY);
	    INSTptr=INSTptr->next;
	}
	if (feof(In)) Error("Unespected EoF");
    } while(1);
    
    ENTITY->Net=INSTstart.next;
    
    /* name of kind of architecture */
    GetNextToken(w);
    if (*w!=';') { 
	/* End of architecture last ';' */
	GetNextToken(w);
	if (*w!=';') Warning("extected ';'");
    }
}

void PrintSignals(msg,typ,Sptr)
char *msg;
char typ;
struct SIGstruct  *Sptr;
{
    int i;
    int incr;
    
    (void)fputs(msg,Out);
    while(Sptr!=NULL){
	if ((Sptr->dir==typ) && (!KwrdCmp(Sptr->name,CLOCK))) {
	    if (Sptr->start==Sptr->end) {
		(void)fprintf(Out,"%s ",Sptr->name);
	    } else {
		incr=( Sptr->start<Sptr->end ? 1 : -1 );
		for(i=Sptr->start; i!=Sptr->end; i+=incr) {
		    (void)fprintf(Out,"%s[%d] ",Sptr->name, i);
		}
	    }
	}
	Sptr=Sptr->next;
    }
    (void)fputs("\n",Out);
}

void PrintOrderedSignals(Sptr,Lptr)
struct Ports *Sptr;
struct LibCell *Lptr;
{
    struct Ports *Lpptr;
    int i;
    
    i=0; Lpptr=Lptr->formals;
    while(i<Lptr->npins){
	(void)fprintf(Out,"%s=%s ",Lpptr->name,Sptr->name);
	Sptr++; i++; Lpptr++;
    }
    if (Lptr->clk[0]) (void)fputs(Sptr->name,Out);
}


void PrintSls(Entity)
struct ENTITYstruct *Entity;
{
    struct Instance   *Iptr;
    struct Ports      *Aptr;
    struct Ports      *Fptr;
    int j;

    (void)fprintf(Out,"# *--------------------------------------*\n");
    (void)fprintf(Out,"# |    File created by Vst2Blif v 1.1    |\n");
    (void)fprintf(Out,"# |                                      |\n");
    (void)fprintf(Out,"# |         by Roberto Rambaldi          |\n");
    (void)fprintf(Out,"# |    D.E.I.S. Universita' di Bologna   |\n");
    (void)fprintf(Out,"# *--------------------------------------*/\n\n");

    (void)fprintf(Out,"\n.model %s\n",(Entity->EntityPort)->name);
    PrintSignals(".input ",'i',(Entity->EntityPort)->io);
    PrintSignals(".output ",'o',(Entity->EntityPort)->io);
    if (CLOCK[0]) {
	(void)fprintf(Out,".clock %s",CLOCK);
    }
    (void)fputs("\n\n",Out);
	
    Iptr=Entity->Net;
    while(Iptr!=NULL){
	switch ( (Iptr->what)->type ) {
	  case 'S' :
	      (void)fprintf(Out,".subckt %s ",(Iptr->what)->name);
	      Aptr=Iptr->actuals; Fptr=(Iptr->what)->formals;
	      for(j=0, Aptr++, Fptr++ ; j<(Iptr->what)->npins-1; j++, Aptr++,Fptr++){
		  (void)fprintf(Out,"%s=%s ",Fptr->name,Aptr->name);
	      }
	      break;
	    case 'L':
	      (void)fprintf(Out,".latch %s ",(Iptr->what)->name);
	      PrintOrderedSignals(Iptr->actuals,WhatLibGate((Iptr->what)->name,LIBRARY));
	      (void)fprintf(Out," %c",INIT);
	      break;
	    case 'G':
	      (void)fprintf(Out,".gate %s ",(Iptr->what)->name);
	      PrintOrderedSignals(Iptr->actuals,WhatLibGate((Iptr->what)->name,LIBRARY));
	      break;
	  }
	(void)fputs("\n",Out);
	Iptr=Iptr->next;
    }
    (void)fputs("\n\n",Out);


}


/* -=[ PARSE FILE ]=-                               *
 * switches between the two main states of          *
 * the program : the ENTITY prsing and the          *
 * ARCHITECTURE one.                                */  
void ParseFile()
{
    struct ENTITYstruct LocalENTITY;
    char *w;
    char LocalToken[MAXTOKENLEN];
    int  flag;

    w=&(LocalToken[0]);
    do {

	/* ENTITY CLAUSE */
	flag=0;
	do {
	    GetNextToken(w);
	    if ((flag=KwrdCmp(w,"ENTITY"))) {
		GetEntity(&(LocalENTITY.EntityPort));
	    } else {
		if (*w=='\0') break;
		VstError("No Entity ???","ENTITY");
	        /* After this call surely flag will be true *
		 * in any other cases the program will stop *
		 * so this point will be never reached ...  */
	    }
	} while(!flag);

	/* ARCHITECTURE CLAUSE */
	flag=0;
	do {
	    GetNextToken(w);
	    if ((flag=KwrdCmp(w,"ARCHITECTURE"))){
		GetArchitecture(&LocalENTITY);
	    } else {
		if (*w=='\0') break;
		VstError("No Architecture ???","ARCHITECTURE");
	        /* it's the same as the previous one         */
	    }
	} while (!flag);
	/* end of the model */

    } while (!feof(In));

    PrintSls(&LocalENTITY);

}

/* -=[ main ]=-                                     */
int main(argc,argv)
int  argc;
char **argv;
{

    CheckArgs(argc,argv);

    ParseFile();

    CloseAll();

    exit(0);
}

