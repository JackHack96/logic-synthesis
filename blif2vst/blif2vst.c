/*
 * $Header: /users/pchong/CVS/sis/blif2vst/blif2vst.c,v 1.1.1.1 2004/02/07 10:13:56 pchong Exp $
 * $Source: /users/pchong/CVS/sis/blif2vst/blif2vst.c,v $
 * $Log: blif2vst.c,v $
 * Revision 1.1.1.1  2004/02/07 10:13:56  pchong
 * imported
 *
 * Revision 1.10  1994/06/14  15:47:29  archiadm
 * error in: printf of output vectors
 *
 * Revision 1.9  1994/06/11  16:06:05  archiadm
 * update 11/6
 *
 * Revision 1.8  1994/05/23  21:14:20  archiadm
 * new improvements
 *
 * Revision 1.8  1994/05/20  18:05:11  archiadm
 * new improvements
 *
 * Revision 1.7  1994/05/09  10:17:39  archiadm
 * minor bug fixes
 *
 * Revision 1.6  1994/05/06  15:45:13  Rob
 * bug fix in the initial help, id has been added
 *
 * Revision 1.5  1994/05/04  20:56:25  Rob
 * First released version
 *
 * Revision 1.4  1994/05/04  09:14:19  Rob
 * Fixed the library parser and the blif file parser
 *
 *
 *   Blif2Vst  version 0.0
 *    BY RAMBALDI rOBERTO.
 *        Apr. 6 1994.
 */


#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>


#define MAXTOKENLEN 256
#define MAXNAMELEN 32


/*           ASCII seq:   (  )  *  +  ,    :  ;  <  =  >       *
 *           code         40 41 42 43 44   58 59 60 61 62      */
#define isBLK(c) ( (((c)=='\t') || ((c)==' ') || ((c)=='\r')) )
#define isREM(c) ( ((c)=='#') )
#define isDQ(c)  ( ((c)=='"') )
#define isCNT(c) ( ((c)=='\\') )
#define isEOL(c) ( ((c)=='\n') )
#define isSTK(c) ( ((c)=='(') || ((c)==')') || isEOL(c) || ((c)=='='))

/* structure that contains all the info extracted from the library *
 * All the pins, input, outp[ut and clock (if needed)              *
 * This because SIS should be case sensitive and VHDL is case      *
 * insensitive, so we must keep the library's names of gates and   *
 * pins. In the future this may be useful for further checks that  *
 * in this version are not performed.                              */


#ifndef DATE
#define DATE "Nodate";
#endif
static char rcsid[] = "$Id: blif2vst.c,v 1.1.1.1 2004/02/07 10:13:56 pchong Exp $";
static char build_date[] = DATE;


struct Ports{
    char             name[MAXNAMELEN];
};

struct Cell {
    char         name[MAXNAMELEN];
    int          npins;
    char         used;
    struct Ports *formals;
    struct Cell  *next;
};

struct Instance {
    struct Cell     *what;
    struct Ports    *actuals;
    struct Instance *next;
};
	

/* list of formal names, is used in GetPort when multiple   *
 * definition are given, as    a,b,c : IN BIT;              */
struct TMPstruct {
    char   name[MAXNAMELEN];
    int    num;
    struct TMPstruct *next;
};

struct BITstruct {
   char   name[MAXNAMELEN+10];
   struct BITstruct *next;
};
    
struct VECTstruct {
    char   name[MAXNAMELEN];
    int    start;
    int    end;
    char   dir;
    struct VECTstruct *next;
};

struct TYPEterms {
    struct BITstruct  *BITs;
    struct VECTstruct *VECTs;
};

struct MODELstruct {
    char   name[MAXNAMELEN];
    struct TYPEterms   *Inputs;
    struct TYPEterms   *Outputs;
    struct TYPEterms   *Internals;
    struct Instance    *Net;
    struct MODELstruct *next;
};




FILE *In;
FILE *Out;
int  line;            /* line parsed                         */

struct Cell *LIBRARY;
char VDD[MAXNAMELEN]={ 0 },VSS[MAXNAMELEN]={ 0 };
char DEBUG;
char ADDPOWER;
char INSTANCE;

#if defined(linux)
void AddBIT(struct BITstruct **,char *);
#endif


/*[]-------==[ Procedures to exit or to display warnings ]==---------------[]*/


/* -=[ CloseAll ]=-                                 *
 * Closes all the files and flushes the memory      *
 *		                                    */
void CloseAll()
{

    if (In!=stdin) (void)fclose(In);
    if (Out!=stdout) (void)fclose(Out);

}


/* -=[ Error ]=-                                    *
 * Displays an error message, and then exits        *
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


/* -=[ Warning ]=-                                  *
 * Puts a message on stderr, writes the current     *
 * line and then sends the current token back       *
 *                                                  *
 * Inputs :                                         *
 *      name = message                              */
void Warning(name)
char *name;
{
    if (DEBUG) (void)fprintf(stderr,"*parse warning* Line %u : %s\n",line,name);

}


/* -=[ SyntaxError ]=-                              *
 * sends to stderr a message and then exits         *
 *                                                  *
 * Input :                                          *
 *     name = message to print                      *
 *     obj  = token that generates the error        */ 
void SyntaxError(msg,obj)
char *msg;
char *obj;
{

    (void)fprintf(stderr,"\n*Error* Line %u : %s\n",line,msg);
    (void)fprintf(stderr,"*Error* Line %u : object of the error : %s\n",line,obj);
    Error("Could not continue!");

}


/*[]-------==[ General Procedures ]==--------------------------------------[]*/


/* -=[ KwrdCmp ]=-                                  *
 * Compares two strings, it is case-insensitive,    *
 * it also skips initial and final duouble-quote    *
 *                                                  *
 * Input :                                          *
 *      name = first string, tipically the token    *
 *      keywrd = second string, tipically a keyword *
 * Output :                                         *
 *      char = 1 if the strings match               *
 *              0 if they do not match              */
char KwrdCmp(name,keywrd)
char *name;
char *keywrd;
{
    int  t;
    int  len;
    char *n;
    
    len=strlen(name);
    if (*name=='"') {
	n=name+1;
	len-=2;
    } else {
	n=name;
    }
    if ( len != (strlen(keywrd))) return 0;
    if (!len) return 0;
       /* if length 0 exit */
    for(t=0; (t<len); n++, keywrd++, t++){
	   /* if they're not equal */
	if (toupper(*n)!=toupper(*keywrd))
	    return 0;
	/* EoL, exit */
	if ((*n=='\0') || (*n=='"')) break;
    }
    return 1;
}



/* -=[ WhatGate ]=-                                 *
 * Returns a pointer to an element of the list      *
 * of gates that matches up the name given, if      *
 * there is not a match a null pointer is returned  *
 *                                                  *
 * Input :                                          *
 *     name = name to match                         *
 * Ouput :                                          *
 *     (void *) a pointer                           */
struct Cell *WhatGate(name)
char *name;
{
    struct Cell  *ptr;

    for(ptr=LIBRARY; ptr!=NULL ; ptr=ptr->next)
	if ( KwrdCmp(ptr->name,name) ) return ptr;

    (void)fprintf(stderr,"Instance %s not found in library!\n",name);
    Error("could not continue");
    return (struct Cell *)NULL;
}


void ReleaseBit(ptr)
struct BITstruct *ptr;
{
    struct BITstruct *tmp;
    struct BITstruct *ptr2;

    ptr2=ptr;
    while(ptr2!=NULL){
	tmp=ptr2->next;
	free((char *)ptr2);
	ptr2=tmp;
    }
}


struct Cell *NewCell(name,ports)
char *name;
struct BITstruct *ports;
{
    struct Cell      *tmp;
    struct BITstruct *Bptr;
    struct Ports     *Pptr;
    unsigned int     j;
    int    num;
    
    if ( (tmp=(struct Cell *)calloc(1,sizeof(struct Cell)))== NULL ) {
	Error("Allocation Error or not enought memory !");
    }

    if (ports!=NULL){
	j=1;
	Bptr=ports;
	while(Bptr->next!=NULL){
	    j++; Bptr=Bptr->next;
	}
	num=j;
	
	if ( (tmp->formals=(struct Ports *)calloc(1,j*sizeof(struct Ports)))==NULL){
	    Error("Allocation Error or not enought memory !");
	}
	(void)strcpy(tmp->name,name);
	tmp->npins=j;
	tmp->next=NULL;
	
	Bptr=ports;
	Pptr=tmp->formals;
	j=0;
	while(Bptr!=NULL){
	    if (j>num) Error("__NewCell error ...");
	    (void)strcpy(Pptr->name,Bptr->name);
	    Bptr=Bptr->next;
	    Pptr++; j++;
	}
	ReleaseBit(ports);
    } else {
	tmp->formals=NULL;
    }
    return tmp;
}



struct Instance *NewInstance(cell)
struct Cell *cell;
{
    struct Instance  *tmp;
    unsigned i;

    if ( (tmp=(struct Instance *)calloc(1,sizeof(struct Instance)))== NULL ) {
	Error("Allocation Error or not enought memory !");
    }
    if (cell!=NULL){
	i=cell->npins*sizeof(struct Ports);
	if ( (tmp->actuals=(struct Ports *)calloc(1,i))==NULL){
	    Error("Allocation Error or not enought memory !");
	}
    } else {
	tmp->actuals=NULL;
    }
    tmp->what=cell;
    tmp->next=NULL;

    return tmp;
}


struct TYPEterms *NewTYPE()
{
    struct TYPEterms *TYPEptr;

    TYPEptr=(struct TYPEterms *)calloc(1,sizeof(struct TYPEterms));
    TYPEptr->BITs=(struct BITstruct *)calloc(1,sizeof(struct BITstruct));
    TYPEptr->VECTs=(struct VECTstruct *)calloc(1,sizeof(struct VECTstruct));
    if ( (TYPEptr==NULL) || (TYPEptr->BITs==NULL) || (TYPEptr->VECTs==NULL) ) {
	Error("Allocation Error or not enought memory !");
    }
    return TYPEptr;
}

struct MODELstruct *NewModel()
{
    struct MODELstruct *LocModel;
    
    if ((LocModel=(struct MODELstruct *)calloc(1,sizeof (struct MODELstruct))) ==NULL) {
	Error("Not enought memory or allocation error");
    }
    
    LocModel->Inputs=NewTYPE();
    LocModel->Outputs=NewTYPE();
    LocModel->Internals=NewTYPE();
    LocModel->Net=NewInstance((struct Cell *)NULL);
    
    return LocModel;
}


void AddVECT(VECTptr,name,a,b)
struct VECTstruct **VECTptr;
char *name;
int a;
int b;
{
   (*VECTptr)->next=(struct VECTstruct *)calloc(1,sizeof(struct VECTstruct));
   if ( (*VECTptr)->next==NULL) {
	Error("Allocation Error or not enought memory !");
    }
   (*VECTptr)=(*VECTptr)->next;
   (void)strcpy((*VECTptr)->name,name);
   (*VECTptr)->start=a;
   (*VECTptr)->end=b;
   (*VECTptr)->next=NULL;
}

void AddBIT(BITptr,name)
struct BITstruct **BITptr;
char   *name;
{

   (*BITptr)->next=(struct BITstruct *)calloc(1,sizeof(struct BITstruct));
   if ( (*BITptr)->next==NULL) {
	Error("Allocation Error or not enought memory !");
    }
   (*BITptr)=(*BITptr)->next;
   (void)strcpy((*BITptr)->name,name);
   (*BITptr)->next=NULL;
}

/*
void AddTMP(TMPptr)
struct TMPstruct **TMPptr;
{

   (*TMPptr)->next=(struct TMPstruct *)calloc(1,sizeof(struct TMPstruct));
   if ( (*TMPptr)->next==NULL) {
	Error("Allocation Error or not enough memory !");
    }
   (*TMPptr)=(*TMPptr)->next;

}
*/

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

/*
struct BITstruct *IsKnown(name,model)
char *name;
struct MODELstruct *model;
{
    struct BITstruct *ptr;

    if ( (ptr=IsHere(name,(model->Inputs)->BITs))!=NULL ) return ptr;
    if ( (ptr=IsHere(name,(model->Outputs)->BITs))!=NULL ) return ptr;
    if ( (ptr=IsHere(name,(model->Internals)->BITs))!=NULL ) return ptr;
    return (struct BITstruct *)NULL;

}
*/




/* -=[ GetToken ]=-                                 *
 *                                                  *
 * Output :                                         *
 *      tok = filled with the new token             */
void GetToken(tok)
char *tok;
{
    enum states { tZERO, tLONG, tEOF, tSTRING, tCONT, tREM };
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
	line=0;
    }

    t=&(TOKEN[0]);
    num=0;
    str=0;

    do{
	if (sentback){
	    c=sentback;
	} else {
	    c=fgetc(In);
	    if (c=='\n') line++;
	}
	if (feof(In)) next=tEOF;
	ready=0;
	sentback='\0';

	switch (next) {
	  case tZERO:
#if defined(DBG)
	    (void)fprintf(stderr,"\n> ZERO, c=%c  token=%s",c,TOKEN);
#endif
	    if isBLK(c) {
		next=tZERO;
	    } else {
		if isSTK(c) {
		    *t=c; t++;
		    next=tZERO;
		    ready=1;
		} else {
		    if isREM(c) {
			num=0;
			next=tREM;
		    } else {
			if isCNT(c) {
			    next=tCONT;
			} else {
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
	    if (DEBUG) (void)fprintf(stderr,"\n-> LONG, c=%c  token=%s",c,TOKEN);
	    if (isBLK(c) || isSTK(c)) {
		sentback=c;
		ready=1;
		next=tZERO;
	    } else {
		if isDQ(c) {
		    ready=1;
		    next=tSTRING;
		} else {
		    if isREM(c) {
			ready=1;
			next=tREM;
		    } else {
			if isCNT(c) {
			    ready=1;
			    sentback=c;
			} else {
			    *t=c;
			    t++;
			    num++;
			    next=tLONG;
			    if ( (ready=(num>=MAXTOKENLEN-1)) )
			    (void)fprintf(stderr,"Sorry, exeeded max name len of %u",num+1);
			}
		    }
		}
	    }
	    break;
	  case tSTRING:
#if defined(DBG)
	    (void)fprintf(stderr,"\n--> STRING, c=%c  token=%s",c,TOKEN);
#endif
	    if (!str) {
		*t='"'; t++; num++;
		str=1;
	    }
		*t=c; t++; num++;
	    if (c=='"') {   /* last dblquote */
		ready=1;
		next=tZERO;
		break;
	    }
	    next=tSTRING;
	    if ( (ready=(num>=MAXTOKENLEN-1)) )
		(void)fprintf(stderr,"Sorry, exeeded max name len of %u",num+1);
	    break;
	  case tREM:
	    if (DEBUG) (void)fprintf(stderr,"\n---> REM, c=%c  token=%s",c,TOKEN);
	    next=tREM;
	    if isEOL(c) {
		sentback=c; /* in this case EOL must be given to the caller */ 
		next = tZERO;
	    }
	    break;  
	  case tCONT:
#if defined(DBG)
	    (void)fprintf(stderr,"\n----> CONT, c=%c  token=%s",c,TOKEN);
#endif
	    if isEOL(c) {
		sentback=0; /* EOL must be skipped */
		next = tZERO;
	    } else {
		next=tCONT;
	    }
	    break;
	  case tEOF:
#if defined(DBG)
	    (void)fprintf(stderr,"\n-----> EOF, c=%c  token=%s",c,TOKEN);
#endif
	    next=tEOF;
	    ready=1;
	    sentback=c;
	    *t=c;
	    break;
	  default :
#if defined(DBG)
	    (void)fprintf(stderr,"\n-------> DEFAULT, c=%c  token=%s",c,TOKEN);
#endif
	      next=tZERO;
	}
    } while(!ready);
    *t='\0';
    (void)strcpy(tok,&(TOKEN[0]));
}


void PrintGates(cell)
struct Cell *cell;
{
    struct Ports *ptr;
    int    j;
    
    while(cell->next!=NULL){
	cell=cell->next;
	if (DEBUG) (void)fprintf(stderr,"Cell name: %s, num pins : %d\n",cell->name,cell->npins);
	ptr=cell->formals;
	for(j=0; j<cell->npins; j++, ptr++){
	    if (DEBUG) (void)fprintf(stderr,"\tpin %d : %s\n",j,ptr->name);
	}
    }
}

/*[]------------------[]*/

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
			} else {
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
		if(DEBUG) (void)fprintf(stderr,"<%c>\n",c);
		str=1;
	    }
	    *t=c; t++; num++;
	    if (c=='"') {   /* last dblquote */
		    ready=1;
		    next=tZERO;
		    if (DEBUG) fprintf(stderr,"STRING : %s\n",TOKEN);
		    break;
		}
	    next=tSTRING;
	    if ( (ready=(num>=MAXTOKENLEN-1)) )
		(void)fprintf(stderr,"Sorry, exeeded max name len of %u",num+1);
	    break;
	  case tEOF:
	    next=tEOF;
	    ready=1;
	    sentback=c;
	    *t=c;
	    if (DEBUG) (void)fprintf(stderr,"EOF\n");
	    break;
	  case tREM:
	    next=tREM;
	    if (c=='\n'){
		sentback=c;
		next=tZERO;
	    }
	    break;
	  default :
	      next=tZERO;
	}
    } while(!ready);
    *t='\0';
    (void)strcpy(tok,&(TOKEN[0]));
}


/* -=[ ScanLibrary ]=-                              *
 * Scans the library to get the names of the cells  *
 * the output pins and the clock signals of latches *
 *                                                  *
 * Input :                                          *
 *     LibName = the name of library file           */
struct Cell *ScanLibrary(LibName)
char *LibName;
{
    enum states { sZERO, sPIN, sCLOCK, sADDCELL } next;
    FILE *Lib;
    struct Cell      *cell;
    struct Cell      first;
    struct BITstruct *tmpBIT;
    struct BITstruct firstBIT;
    char LocalToken[MAXTOKENLEN];
    char tmp[MAXNAMELEN];
    char name[MAXNAMELEN];
    char latch;
    char *s;
    

    if ((Lib=fopen(LibName,"rt"))==NULL)
	Error("Couldn't open library file");


    first.next=NewCell("dummy",(struct BITstruct *)NULL);
    firstBIT.name[0]='\0'; firstBIT.next=NULL;
    cell=first.next;
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
		if (DEBUG) (void)fprintf(stderr,"Gate name: %s\n",name);
	    } else {
		if (KwrdCmp(s,"LATCH")) {
		    latch=1;
		    GetLibToken(Lib,s);
		    (void)strcpy(name,s);
		    GetLibToken(Lib,s);   /* area */
		    next=sPIN;
		    if (DEBUG) (void)fprintf(stderr,"Latch name: %s\n",name);
		}
	    }
	    break;
	  case sPIN:
	    if ( !( ((*s>=0x27) && (*s<=0x2b)) || (*s=='=') || (*s=='!')|| (*s==';')) ){
		(void)strncpy(tmp,s,5);
		if (KwrdCmp(tmp,"CONST") && !isalpha(*(s+6)))
		    /* if the expression has a constant value we must */
		    /* skip it, because there are no inputs           */
		    break;
		/* it's an operand so get its name */
		if (DEBUG) (void)fprintf(stderr,"\tpin read : %s\n",s);
		if (IsHere(s,&firstBIT)==NULL) {
		    if (DEBUG) (void)fprintf(stderr,"\tunknown pin : %s\n",s);
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
		if (DEBUG) (void)fprintf(stderr,"\tpin : %s\n",s);
		AddBIT(&tmpBIT,s);
		next=sADDCELL;
	    } else {
		next=sCLOCK;
	    }
	    break;
	  case sADDCELL:
	    if (DEBUG) (void)fprintf(stderr,"\tadding cell to library\n");
	    cell->next=NewCell(name,firstBIT.next);
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
    if (DEBUG) (void)fprintf(stderr,"eond of lib");
    PrintGates(first.next);
    return first.next;
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
    char help;

    extern char *optarg;
    extern int optind;

    s=&( argv[0][strlen(argv[0])-1] );
    while( (s>= &(argv[0][0])) && (*s!='/') ) {  s--; }
    (void)fprintf(stderr,"\t\t      Blif Converter v1.0\n");
    (void)fprintf(stderr,"\t\t      by Roberto Rambaldi\n");
    (void)fprintf(stderr,"\t\tD.E.I.S. Universita' di Bologna\n\n");
    help=0; ADDPOWER=0; INSTANCE=1;
    while( (c=getopt(argc,argv,"s:S:d:D:IihHvVnN$") )>0 ){
	switch (toupper(c)) {
	  case 'I':
	    INSTANCE=0;
	    break;
	  case 'S':
	    (void)strncpy(VSS,optarg,MAXNAMELEN);
	    break;
	  case 'D':
	    (void)strncpy(VDD,optarg,MAXNAMELEN);
	      break;
	  case 'V':
	    DEBUG=1;
	    break;
	  case 'H':
	    help=1;
	    break;
	  case 'N':
	    ADDPOWER=1;
	    break;
	  case '$':
	    help=1;
	    (void)fprintf(stderr,"\n\tID = %s\n",rcsid);
	    (void)fprintf(stderr,"\tCompiled on %s\n\n",build_date);
	    break;
	  default:
	    (void)fprintf(stderr,"\t *** unknown options");
	    help=1;
	    break;
	}
    }	    

    if (!help) {
        if (optind>=argc) {
	    if (!help) (void)fprintf(stderr,"No Library file specified\n\n");
	    help=1;
	} else {
	    LIBRARY=ScanLibrary(argv[optind]);
	    if (++optind>=argc){
		In=stdin; Out=stdout;
	    } else {
		if ((In=fopen(argv[optind],"rt"))==NULL) Error("Couldn't read input file");
		if (++optind>=argc) { Out=stdout; }
		else if ((Out=fopen(argv[optind],"wt"))==NULL) Error("Could'n make opuput file");
	    }
	}
	
	if (!ADDPOWER) {
	    VDD[0]='\0'; VSS[0]='\0';
	} else {
	    if (VDD[0]=='\0') (void)strcpy(VDD,"vdd");
	    if (VSS[0]=='\0') (void)strcpy(VSS,"vss");
	}
    }

    if (help) {
	(void)fprintf(stderr,"\tUsage: %s [options] <library> [infile [outfile]]\n",s);
	(void)fprintf(stderr,"\t       if outfile is not given will be used stdout, if also\n");
	(void)fprintf(stderr,"\t       infile is not given will be used stdin instead.\n");
	(void)fprintf(stderr,"\t<library>\t is the name of the library file to use\n");
	(void)fprintf(stderr,"\tOptions :\n\t-s <name>\t <name> will be used for VSS net\n");
	(void)fprintf(stderr,"\t-i \t\t <name> skip instance names\n");
	(void)fprintf(stderr,"\t-d <name>\t <name> will be used for VDD net\n");
	(void)fprintf(stderr,"\t-n\t\t no VSS or VDD to skip.\n");
	(void)fprintf(stderr,"\t-h\t\t prints these lines");
	(void)fprintf(stderr,"\n\tIf no VDD or VSS nets are given VDD and VSS will be used\n");
	exit(0);
    }
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
    char *s;

    for(s=string; *s!='\0'; s++)
	if (!isdigit(*s)) {
	    SyntaxError("Expected decimal integer number",string);
	}
    return atoi(string);
}

/* -=[ GetSignals ]=-                               */

void GetSignals(TYPEptr)
struct TYPEterms *TYPEptr;
{
    enum states { sZERO, sWAIT, sNUM, sEND } next;
    char *w;
    char LocalToken[MAXTOKENLEN];
    char name[MAXNAMELEN];
    char tmp[MAXNAMELEN+10];
    struct BITstruct  *BITptr;
    struct VECTstruct *VECTptr;
    char Token;
    int  num;

    w=&(LocalToken[0]);

    BITptr=TYPEptr->BITs;
    VECTptr=TYPEptr->VECTs;

    while(BITptr->next!=NULL) { 
/*	(void)fprintf(stderr,".. b) element used : %s, skip...\n",BITptr->name); */
	BITptr=BITptr->next; }
    while(VECTptr->next!=NULL) { 
/*	(void)fprintf(stderr,".. v) element used : %s, skip...\n",VECTptr->name); */
	VECTptr=VECTptr->next; }

    next = sZERO;
    Token=1;

    do {
	if (Token) GetToken(w);
	else Token=1;
	if ((*w=='\n') && (next!=sWAIT)) break;
	if (DEBUG) (void)fprintf(stderr,"next = %u\n",next);
	switch (next) {
	  case sZERO:
	    if (*w=='[') {
		*(w+strlen(w)-1)='\0';
		(void)sprintf(name,"intrnl%s",w+1);
	    } else {
		(void)strcpy(name,w);
	    }
	    if (DEBUG) (void)fprintf(stderr,"element name : %s\n",name);
	    next=sWAIT;
	    break;
	  case sWAIT:
	    if (DEBUG) (void)fprintf(stderr,"--sWAIT-- tok <%s>\n",w);
	    if (*w=='(') {
		/* is a vector */
		next=sNUM;
	    } else {
		/* it was a BIT type */
		if (DEBUG) (void)fprintf(stderr,"\t\t--> bit, added\n");
		AddBIT(&BITptr,name);
		Token=0;
		next=sZERO;
	    }
	    break;
	  case sNUM:
	    if (DEBUG) (void)fprintf(stderr,"\t\t is a vector, element number :%s\n",w); 
	    num=DecNumber(w);
	    AddVECT(&VECTptr,name,num,num);
	    (void)sprintf(tmp,"%s(%u)",name,num);
	    AddBIT(&BITptr,tmp);
	    next=sEND;
	    break;
	  case sEND:
	    if (*w!=')') {
	        Warning("closing ) expected !");
		Token=0;
	    }
	    next=sZERO;
	    break;
	}
	if (DEBUG) (void)fprintf(stderr,"next = %u\n",next);
    } while(!feof(In));

}


void OrderVectors(VECTptr)
struct VECTstruct *VECTptr;
{
    struct VECTstruct *actual;
    struct VECTstruct *ptr;
    struct VECTstruct *tmp;
    struct VECTstruct *prev;
    int    dir;
    int    last;
    
    actual=VECTptr;
    while (actual->next!=NULL) {
	actual=actual->next;
	last=actual->start;
/*	(void)fprintf(stderr,"vector under test : %s\n",actual->name);  */
	dir=0;
	prev=actual;
	ptr=actual->next;
	while (ptr!=NULL) {
/*	    (void)fprintf(stderr,"analizing element name = %s  number = %u\n",ptr->name,ptr->end); */
	    if (!strcmp(ptr->name,actual->name)) {
		/* same name, so ptr belogns to the vector of actual */
		if (ptr->end>actual->end) {
		    actual->end=ptr->end;
		} else {
		    if (ptr->start<actual->start) {
			actual->start=ptr->start;
		    }
		}
		if (ptr->end>last) dir++;
		else {
		    if (ptr->end==last) {
			Error("Two elements of an i/o vector with the same number not allowed!");
		    } else dir--;
		}
		/* release memory */
		tmp=ptr->next;
/*		(void)fprintf(stderr,"deleting element @%p, actual=%p  ..->next=%p\n",ptr,actual,actual->next); */
		if (ptr==actual->next) {
/*		    (void)fprintf(stderr,"updating actual's next"); */
		    actual->next=tmp;
		}
		free((char *)ptr);
		ptr=tmp;
		prev->next=tmp;
	    } else {
		prev=ptr;
		ptr=ptr->next;
	    }
	}
	actual->dir=dir>0;
    }
}



void PrintSignals(Model)
struct MODELstruct *Model;
{
    struct BITstruct  *BITptr;
    struct VECTstruct *VECTptr;
    
    BITptr=(Model->Inputs)->BITs;
    if (DEBUG) (void)fprintf(stderr,"\nINPUTS, BIT:");
    while(BITptr->next!=NULL){
	BITptr=BITptr->next;
	if (DEBUG) (void)fprintf(stderr,"\t%s",BITptr->name);
    }

    BITptr=(Model->Outputs)->BITs;
    if (DEBUG) (void)fprintf(stderr,"\nOUTPUTS, BIT:");
    while(BITptr->next!=NULL){
	BITptr=BITptr->next;
	if (DEBUG) (void)fprintf(stderr,"\t%s",BITptr->name);
    }


    VECTptr=(Model->Inputs)->VECTs;
    if (DEBUG) (void)fprintf(stderr,"\nINPUTS, VECT:");
    while(VECTptr->next!=NULL){
	VECTptr=VECTptr->next;
	if (VECTptr->dir) if (DEBUG) (void)fprintf(stderr,"\t%s [%d..%d]",VECTptr->name,VECTptr->start,VECTptr->end);
	else if (DEBUG) (void)fprintf(stderr,"\t%s [%d..%d]",VECTptr->name,VECTptr->end,VECTptr->start);
    }

    VECTptr=(Model->Outputs)->VECTs;
    if (DEBUG) (void)fprintf(stderr,"\nOUTPUTS, VECT:");
    while(VECTptr->next!=NULL){
	VECTptr=VECTptr->next;
	if (VECTptr->dir) if (DEBUG) (void)fprintf(stderr,"\t%s [%d..%d]",VECTptr->name,VECTptr->start,VECTptr->end);
	else if (DEBUG) (void)fprintf(stderr,"\t%s [%d..%d]",VECTptr->name,VECTptr->end,VECTptr->start);
    }
    if (DEBUG) (void)fprintf(stderr,"\n");
}


void PrintNet(cell)
struct Instance *cell;
{
    struct Ports *ptr1;
    struct Ports *ptr2;
    int    j;
    
    while(cell->next!=NULL){
	cell=cell->next;
	if (DEBUG) (void)fprintf(stderr,"Inst. connected to name: %s, num pins : %d\n",(cell->what)->name,(cell->what)->npins);
	ptr1=cell->actuals;
	ptr2=(cell->what)->formals;
	for(j=0; j<(cell->what)->npins; j++, ptr1++, ptr2++){
	    if (DEBUG) (void)fprintf(stderr,"\tpin %d : %s -> %s\n",j,ptr2->name,ptr1->name);
	}
    }
}

struct Instance *GetNames(name,type,Model)
char *name;
char type;
struct MODELstruct *Model;
{
    enum states { sZERO,  saWAIT, saNUM, saEND, sEQUAL,
		  saNAME, sfWAIT, sfNUM, sfEND, sADDINSTANCE } next;
    struct Ports     *Fptr;
    struct Ports     *Aptr;
    struct Instance  *Inst;
    struct Cell      *cell;
    struct BITstruct *Bptr;
    char *w;
    char LocalToken[MAXTOKENLEN];
    char FORMname[MAXNAMELEN];
    char ACTname[MAXNAMELEN];
    char tmp[MAXNAMELEN+10];
    char Token;
    int  num;
    int  j;
    int  exit;

    w=&(LocalToken[0]);
    next = sZERO;
    Token=1;
    exit=1;

    if (DEBUG) (void)fprintf(stderr,"Parsing instance with gate %s\n",name);
    cell=WhatGate(name);
    cell->used=1;
    Inst=NewInstance(cell);

    Bptr=(Model->Internals)->BITs;
    while(Bptr->next!=NULL){
	Bptr=Bptr->next;
    }

    do {
	if (Token) GetToken(w);
	else Token=1;
	if (*w=='\n') {
	    exit=0;
	    if (next!=sADDINSTANCE) {
		Warning("--------> Unexpected end of line!");
		next=sADDINSTANCE;
	    }
	}
	switch (next) {
	  case sZERO:
	    if (*w=='[') {
		*(w+strlen(w)-1)='\0';
		(void)sprintf(FORMname,"intrnl%s",w+1);
	    } else {
		(void)strcpy(FORMname,w);
	    }
	    if (DEBUG) (void)fprintf(stderr,"formal element name : %s\n",FORMname); 
	    next=sfWAIT;
	    break;
	  case sfWAIT:
	    if (*w=='(') {
		/* is a vector */
		next=sfNUM;
	    } else {
		/* it was a BIT type */
		Token=0;
		next=sEQUAL;
	    }
	    break;
	  case sfNUM:
	    if (DEBUG) (void)fprintf(stderr,"\t\t is a vector, element number :%s\n",w); 
	    num=DecNumber(w);
	    (void)sprintf(tmp,"%s(%u)",FORMname,num);
	    (void)strcpy(FORMname,tmp);
	    next=sfEND;
	    break;
	  case sfEND:
	    if (*w!=')') {
	        Warning("Closing ) expected !");
		Token=0;
	    }
	    next=sEQUAL;
	    break;
	  case sEQUAL:
	    next=saNAME;
	    if (*w!='=') {
		if (type) {
		    if (DEBUG) (void)fprintf(stderr,"clock signal\n");
		    (void)strcpy(ACTname,FORMname);
		    FORMname[0]='\0';
		    next=sADDINSTANCE;
		} else {
		    Warning("Expexted '=' !");
		    next=saNAME;
		}
		Token=0;
	    }
	    break;
	  case saNAME:
	    if (*w=='[') {
		*(w+strlen(w)-1)='\0';
		(void)sprintf(ACTname,"intrnl%s",w+1);
	    } else {
		(void)strcpy(ACTname,w);
	    }
	    if (DEBUG) (void)fprintf(stderr,"actual element name : %s\n",ACTname); 
	    next=saWAIT;
	    break;
	  case saWAIT:
	    if (*w=='(') {
		/* is a vector */
		next=saNUM;
	    } else {
		/* it was a BIT type */
		Token=0;
		next=sADDINSTANCE;
	    }
	    break;
	  case saNUM:
	    if (DEBUG) (void)fprintf(stderr,"\t\t is a vector, element number :%s\n",w); 
	    num=DecNumber(w);
	    (void)sprintf(tmp,"%s(%u)",ACTname,num);
	    (void)strcpy(ACTname,tmp);
	    next=saEND;
	    break;
	  case saEND:
	    if (*w!=')') {
	        Warning("Closing ) expected !");
		Token=0;
	    }
	    next=sADDINSTANCE;
	    break;
	  case sADDINSTANCE:
	    if ( (IsHere(ACTname,(Model->Inputs)->BITs)==NULL) &&
		(IsHere(ACTname,(Model->Outputs)->BITs)==NULL) &&
		(IsHere(ACTname,(Model->Internals)->BITs)==NULL) ) {
		AddBIT(&Bptr,ACTname);
		if (DEBUG) (void)fprintf(stderr,"\tadded to internals\n");
	    }
	    Aptr=Inst->actuals;
	    Fptr=cell->formals;
	    j=0; next=sZERO; 
	    Token=0;
	    if (DEBUG) (void)fprintf(stderr,"\tparsed pin : %s, %s\n",FORMname,ACTname);
	    if (FORMname[0]!='\0') {
		while(j<cell->npins){
		    if (KwrdCmp(Fptr->name,FORMname)){
			(void)strcpy(Aptr->name,ACTname);
			break;
		    }
		    Fptr++;
		    Aptr++; 
		    j++;
		}
	    } else {
		Aptr+=cell->npins-1;
		(void)strcpy(Aptr->name,ACTname);
		break;
	    }
	}
    } while(exit);
    return Inst;
}






void PrintVST(Model)
struct MODELstruct *Model;
{
    struct Cell       *Cptr;
    struct Instance   *Iptr;
    struct Ports      *Fptr;
    struct BITstruct  *Bptr;
    struct VECTstruct *Vptr;
    struct Ports      *Aptr;
    int    j;
    int    i;
    char   init;

    (void)fprintf(Out,"--[]--------------------------------------[]--\n");
    (void)fprintf(Out,"-- |    File created by Blif2Sls v1.0     | --\n");
    (void)fprintf(Out,"-- |                                      | --\n");
    (void)fprintf(Out,"-- |        by Roberto Rambaldi           | --\n");
    (void)fprintf(Out,"-- |   D.E.I.S. Universita' di Bologna    | --\n");
    (void)fprintf(Out,"--[]--------------------------------------[]--\n\n");

    (void)fprintf(Out,"\n\nENTITY %s IS\n    PORT(\n",Model->name);
    Bptr=(Model->Inputs)->BITs;
    init=0;
    while(Bptr->next!=NULL){
	Bptr=Bptr->next;
	if ( (Bptr->name)[strlen(Bptr->name)-1]!=')' ) {
	    if (init) (void)fputs(" ;\n",Out);
	    else init=1;
	    (void)fprintf(Out,"\t%s\t: in BIT",Bptr->name);
	}
    }

    Bptr=(Model->Outputs)->BITs;
    while(Bptr->next!=NULL){
	Bptr=Bptr->next;
	if ( (Bptr->name)[strlen(Bptr->name)-1]!=')' ) {
	    if (init) (void)fputs(" ;\n",Out);
	    else init=1;
	    (void)fprintf(Out,"\t%s\t: out BIT",Bptr->name);
	}
    }

    Vptr=(Model->Inputs)->VECTs;
    while(Vptr->next!=NULL){
	Vptr=Vptr->next;
	if (init) (void)fputs(" ;\n",Out);
	else init=1;
	if (Vptr->dir) (void)fprintf(Out,"\t%s\t: in BIT_VECTOR(%d to %d)",Vptr->name,Vptr->start,Vptr->end);
	else (void)fprintf(Out,"\t%s\t: in BIT_VECTOR(%d downto %d)",Vptr->name,Vptr->end,Vptr->start);
    }

    Vptr=(Model->Outputs)->VECTs;
    while(Vptr->next!=NULL){
	Vptr=Vptr->next;
	if (init) (void)fputs(" ;\n",Out);
	else init=1;
	if (Vptr->dir) (void)fprintf(Out,"\t%s\t: out BIT_VECTOR(%d to %d)",Vptr->name,Vptr->start,Vptr->end);
	else (void)fprintf(Out,"\t%s\t: out BIT_VECTOR(%d downto %d)",Vptr->name,Vptr->end,Vptr->start);

    }
    if (ADDPOWER) {
	(void)fprintf(Out," ;\n\t%s\t : in BIT;\n\t%s\t : in BIT",VSS,VDD);
    }

    (void)fprintf(Out," );\nEND %s;\n\n\nARCHITECTURE structural_from_SIS OF %s IS\n",Model->name,Model->name);


    Cptr=LIBRARY;
    while(Cptr->next!=NULL) {
	Cptr=Cptr->next;
	if (Cptr->used) {
	    (void)fprintf(Out,"  COMPONENT %s\n    PORT (\n",Cptr->name);
	    Fptr=Cptr->formals;
	    (void)fprintf(Out,"\t%s\t: out BIT",Fptr->name);
	    for(j=1, Fptr++; j<Cptr->npins; j++, Fptr++){
		(void)fprintf(Out," ;\n\t%s\t: in BIT",Fptr->name);
	    }
	    if (ADDPOWER) {
		(void)fprintf(Out," ;\n\t%s\t: in BIT ;\n\t%s\t: in BIT",VSS,VDD);
	    }
	    (void)fprintf(Out," );\n  END COMPONENT;\n\n");
	}
    }

    Bptr=(Model->Internals)->BITs;
    init=0;
    while(Bptr->next!=NULL){
	Bptr=Bptr->next;
	(void)fprintf(Out,"SIGNAL %s\t: BIT ;\n",Bptr->name);
    }

    (void)fputs("BEGIN\n",Out);

    j=0;
    Iptr=Model->Net;
    while(Iptr->next!=NULL){
	Iptr=Iptr->next;
	(void)fprintf(Out,"  inst%d : %s\n  PORT MAP (\n",j,(Iptr->what)->name);
	j++;
	Aptr=Iptr->actuals;
	Fptr=(Iptr->what)->formals;
	for(i=0; i<(Iptr->what)->npins-1; i++, Aptr++, Fptr++){
	    (void)fprintf(Out,"\t%s => %s,\n",Fptr->name,Aptr->name);
	}
	if (ADDPOWER) {
	    (void)fprintf(Out,"\t%s => %s,\n\t%s => %s,\n\t%s => %s);\n\n",Fptr->name,Aptr->name,VSS,VSS,VDD,VDD);
	} else {
	    (void)fprintf(Out,"\t%s => %s);\n\n",Fptr->name,Aptr->name);
	}
    }
    (void)fputs("\nEND structural_from_SIS;\n",Out);

}


/* -=[ PARSE FILE ]=-                               */
void ParseFile()
{
    char *w;
    char LocalToken[MAXTOKENLEN];
    struct MODELstruct *LocModel;
    struct Instance    *Net;


    w=&(LocalToken[0]);
    LocModel=NewModel();

    if (DEBUG) (void)fprintf(stderr,"start of parsing\n");
    do {
	GetToken(w);
    } while (strcmp(w,".model"));
    
    
    GetToken(w);
    (void)strcpy(LocModel->name,w);
    do {
	do GetToken(w); while(*w=='\n');
	if ( (!strcmp(w,".inputs")) || (!strcmp(w,".clock")) ) {
	    GetSignals(LocModel->Inputs);
	} else {
	    if ( !strcmp(w,".outputs") ) {
		GetSignals(LocModel->Outputs);
	    } else break;
	}
    } while (!feof(In));
    
    PrintSignals(LocModel);
    
    OrderVectors((LocModel->Inputs)->VECTs);
    OrderVectors((LocModel->Outputs)->VECTs);
    
    PrintSignals(LocModel);
    
    Net=LocModel->Net;
    do {
	if (!strcmp(w,".gate")) {
	    GetToken(w);
	    Net->next=GetNames(w,0,LocModel);
	    Net=Net->next;
	} else {
	    if (!strcmp(w,".mlatch")) {
		GetToken(w);
		    Net->next=GetNames(w,1,LocModel);
		Net=Net->next;
	    } else {
		if (!strcmp(w,".end")){
		    PrintNet(LocModel->Net);
		    if (DEBUG) (void)fprintf(stderr,"fine");
		    break;
		} else {
		    /* SyntaxError("Unknown keyword",w); */
		    }
	    }
	}
	do GetToken(w); while(*w=='\n');
    } while (!feof(In));
    PrintVST(LocModel); 
    
}

/* -=[ main ]=-                                     */
int main(argc,argv)
int  argc;
char **argv;
{

    CheckArgs(argc,argv);

    ParseFile();

    CloseAll();

    return 0;
}



