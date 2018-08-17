/**
 * vst2blif version 1.1
 *
 * Original version by Rambaldi Roberto, reworked by Matteo Iervasi
 *
 * Copyright 1994-2018
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include "vst2blif.h"

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

//struct LibCell *scan_library();

FILE           *In;
FILE           *Out;
struct LibCell *LIBRARY;
int            line; // line parsed

int         SendTokenBack; // used to send the token back to the input stream in case of a missing keywork
char        VDD[MAXNAMELEN] = {0};
char        VSS[MAXNAMELEN] = {0};
int         INIT;
int         LINKAGE;
int         LOWERCASE;
char        CLOCK[MAXNAMELEN];
int         WARNING;
int         INSTANCE;
struct Cell *cells;

void close_all() {

    if (In != stdin) (void) fclose(In);
    if (Out != stdout) (void) fclose(Out);

    free(cells);
}

void print_error(char *msg) {
    (void) fprintf(stderr, "*** print_error : %s\n", msg);
    close_all();
    exit(1);
}

int kwrd_cmp(char *name, char *keywrd) {
    int t;
    int len;

    if ((len = strlen(name)) != (strlen(keywrd))) return 0;
    if (!len) return 0;
    /* if length 0 exit */
    for (t = 0; (t < len); name++, keywrd++, t++) {
        /* if they're not equal */
        if (toupper(*name) != toupper(*keywrd))
            return 0;
        /* EoL, exit */
        if (*keywrd == '\0') break;
    }
    return 1;
}

void check_args(int argc, char **argv) {
    char *s;
    char c;
    int  help;
    int  NoPower;

    extern char *optarg;
    extern int  optind;

    s = &(argv[0][strlen(argv[0]) - 1]);
    while ((s >= &(argv[0][0])) && (*s != '/')) { s--; }

    (void) fprintf(stderr, "\t\t      Vst Converter v1.5\n");
    (void) fprintf(stderr, "\t\t      by Roberto Rambaldi\n");
    (void) fprintf(stderr, "\t\tD.E.I.S. Universita' di Bologna\n\n");
    help      = 0;
    INSTANCE  = 1;
    NoPower   = 0;
    LOWERCASE = 1;
    INIT      = '3';
    while ((c = getopt(argc, argv, "s:S:d:D:c:C:i:I:l:L:hHvVuUnN$")) > 0) {
        switch (toupper(c)) {
            case 'S':(void) strncpy(VSS, optarg, MAXNAMELEN);
                break;
            case 'D':(void) strncpy(VDD, optarg, MAXNAMELEN);
                break;
            case 'H':help = 1;
                break;
            case 'C':(void) strncpy(CLOCK, optarg, MAXNAMELEN);
                break;
            case 'I':INIT = *optarg;
                if ((INIT < '0') || (INIT > '3')) {
                    (void) fprintf(stderr, "Wrong latch init value");
                    help = 1;
                }
                break;
            case 'L':
                if (kwrd_cmp(optarg, "IN")) {
                    LINKAGE = 'i';
                } else {
                    if (kwrd_cmp(optarg, "OUT")) {
                        LINKAGE = 'o';
                    } else {
                        (void) fprintf(stderr, "\tUnknow direction for a port of type linkage\n");
                        help = 1;
                    }
                }
                break;
            case 'U':LOWERCASE = 0;
                break;
            case 'N':NoPower = 1;
                break;
            case '$':help = 1;
                break;
        }
    }


    if (!help) {
        if (LOWERCASE)
            for (s = &(CLOCK[0]); *s != '\0'; s++) (void) tolower(*s);
        else
            for (s = &(CLOCK[0]); *s != '\0'; s++) (void) toupper(*s);

        if (optind >= argc) {
            (void) fprintf(stderr, "No Library file specified\n\n");
            help = 1;
        } else {
            LIBRARY = (struct LibCell *) scan_library(argv[optind]);
            if (++optind >= argc) {
                In  = stdin;
                Out = stdout;
            } else {
                if ((In = fopen(argv[optind], "rt")) == NULL) {
                    (void) fprintf(stderr, "Couldn't read input file");
                    help = 1;
                }
                if (++optind >= argc) { Out = stdout; }
                else {
                    if ((Out = fopen(argv[optind], "wt")) == NULL) {
                        (void) fprintf(stderr, "Could'n make opuput file");
                        help = 1;
                    }
                }
            }
        }

        if (NoPower) {
            VDD[0] = '\0';
            VSS[0] = '\0';
        } else {
            if (!VDD[0]) (void) strcpy(VDD, "VDD");
            if (!VSS[0]) (void) strcpy(VSS, "VSS");
        }
    }

    if (help) {
        (void) fprintf(stderr, "\tUsage: vst2blif [options] <library> [infile [outfile]]\n");
        (void) fprintf(stderr, "\t\t if outfile is not given will be used stdout, if also\n");
        (void) fprintf(stderr, "\t\t infile is not given will be used stdin instead.\n");
        (void) fprintf(stderr, "\t<library>\t is the name of the library file to use\n");
        (void) fprintf(stderr, "\tOptions :\n\t-s <name>\t <name> will be used for VSS net\n");
        (void) fprintf(stderr, "\t-d <name>\t <name> will be used for VDD net\n");
        (void) fprintf(stderr, "\t-c <name>\t .clock <name>  will be added to the blif file\n");
        (void) fprintf(stderr, "\t-i <value>\t default value for latches, must be between 0 and 3\n");
        (void) fprintf(stderr, "\t-l <in/out>\t sets the direction for linkage ports\n");
        (void) fprintf(stderr, "\t\t\t the default value is \"in\"\n");
        (void) fprintf(stderr, "\t-u\t\t converts all names to uppercase\n");
        (void) fprintf(stderr, "\t-n\t\t no VSS or VDD to skip.\n");
        (void) fprintf(stderr, "\t-h\t\t prints these lines");
        (void) fprintf(stderr, "\n\tIf no VDD or VSS nets are given VDD and VSS will be used\n");
        exit(0);
    }
}

void get_next_token(char *tok) {
    static char              init = 0;
    static enum TOKEN_STATES state;
    static char              sentback;
    static char              str;
    static char              Token[MAXTOKENLEN];
    char                     *t;
    int                      num;
    int                      TokenReady;
    char                     c;

    if (!init) {
        state         = tZERO;
        init          = 1;
        line          = 0;
        SendTokenBack = 0;
    }

    t          = &(Token[0]);
    num        = 0;
    TokenReady = 0;
    str        = 0;

    if (SendTokenBack) {
        SendTokenBack = 0;
        (void) strcpy(tok, Token);
        return;
    }

    do {
        if (sentback) {
            c = sentback;
        } else {
            c = fgetc(In);
            if (feof(In)) state = tEOF;
            if (c == '\n') line++;
        }

        switch (state) {
            case tZERO:
                /*******************/
                /*    ZERO state   */
                /*******************/
                num      = 0;
                sentback = '\0';
                if (isSTK(c)) {
                    *t = c;
                    t++;
                    TokenReady = 1;
                } else {
                    if isREM(c) {
                        *t = c;
                        t++;
                        num++;
                        sentback = '\0';
                        state    = tREM1;
                    } else {
                        if isDQ(c) {
                            sentback = '\0';
                            state    = tSTRING;
                        } else {
                            if isEOF(c) {
                                state      = tEOF;
                                sentback   = 1;
                                TokenReady = 0;
                            } else {
                                if isEOT(c) {
                                    state = tZERO;
                                    /* stay in tZERO */
                                } else {
                                    sentback = c;
                                    state    = tTOKEN;
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
                TokenReady = 1;
                sentback   = c;
                if (isSTK(c)) {
                    state = tZERO;
                } else {
                    if isREM(c) {
                        state = tREM1;
                    } else {
                        if isDQ(c) {
                            sentback = '\0';
                            state    = tSTRING;
                        } else {
                            if isEOF(c) {
                                sentback = 1;
                                state    = tEOF;
                            } else {
                                if isEOT(c) {
                                    sentback = '\0';
                                    state    = tZERO;
                                } else {
                                    sentback = '\0';
                                    if (num >= (MAXTOKENLEN - 1)) {
                                        sentback = c;
                                        (void) fprintf(stderr, "*Parse warning* Line %u: token too long !\n", line);
                                    } else {
                                        if (LOWERCASE) *t = tolower(c);
                                        else *t = toupper(c);
                                        t++;
                                        num++;
                                        TokenReady = 0;
                                        /* fprintf(stderr,"."); */
                                    }
                                    state    = tTOKEN;
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
                TokenReady = 1;
                sentback   = c;
                if (isSTK(c)) {
                    state = tZERO;
                } else {
                    if isREM(c) {
                        sentback   = '\0';
                        state      = tREM2;    /* it's a remmark. */
                        TokenReady = 0;
                    } else {
                        if isDQ(c) {
                            sentback = '\0';
                            state    = tSTRING;
                        } else {
                            if isEOF(c) {
                                state      = tEOF;
                                sentback   = 1;
                                TokenReady = 0;
                            } else {
                                if isEOT(c) {
                                    sentback = '\0';
                                    /* there's no need to parse an EOT */
                                    state    = tZERO;
                                } else {
                                    state = tTOKEN;
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
                sentback   = '\0';
                TokenReady = 0;
                if isEOL(c) {
                    num   = 0;
                    t     = &(Token[0]);
                    state = tZERO;
                } else {
                    if isEOF(c) {
                        state      = tEOF;
                        sentback   = 1;
                        TokenReady = 0;
                    } else {
                        state = tREM2;
                    }
                }
                break;
            case tSTRING:
                /*******************/
                /*  STRING  state  */
                /*******************/
                if (!str) {
                    *t = '"';
                    t++;
                    num++; /* first '"' */
                    str = 1;
                }
                sentback   = '\0';
                TokenReady = 1;
                if isDQ(c) {
                    *t = c;
                    t++;
                    num++;
                    state = tZERO;   /* There's no sentback char so this     *
				* double quote is the last one *ONLY*  */
                } else {
                    if isEOF(c) {
                        state    = tEOF;    /* this is *UNESPECTED* ! */
                        sentback = 1;
                        (void) fprintf(stderr, "*Parse warning* Line %u: unespected Eof\n", line);
                    } else {
                        sentback = '\0';
                        if (num >= MAXTOKENLEN - 2) {
                            sentback = c;
                            (void) fprintf(stderr, "*Parse warning* Line %u: token too long !\n", line);
                        } else {
                            if (LOWERCASE) *t = tolower(c);
                            else *t = toupper(c);
                            t++;
                            num++;
                            TokenReady = 0;
                            state      = tSTRING;
                        }
                    }
                }
                break;
            case tEOF:
                /*******************/
                /*    EOF  state   */
                /*******************/
                t          = &(Token[0]);
                TokenReady = 1;
                state      = tEOF;
                break;
        }
    } while (!TokenReady);
    *(t) = '\0';
    (void) strcpy(tok, Token);
    return;
}

/*====================================================================*


  releaseBIT : deallocates an entire BITstruct structure
  releaseSIG : deallocates an entire SIGstruct structure
  new_cell    : allocates memory for a new cell


  ====================================================================*/

void releaseBIT(struct BITstruct *ptr) {
    struct BITstruct *tmp;

    while (ptr != NULL) {
        tmp = ptr->next;
        free(ptr);
        ptr = tmp;
    }
}


void releaseSIG(struct SIGstruct *ptr) {
    struct SIGstruct *tmp;

    while (ptr != NULL) {
        tmp = ptr->next;
        free(ptr);
        ptr = tmp;
    }
}

void addBIT(struct BITstruct **BITptr, char *name, char dir) {

    (*BITptr)->next = (struct BITstruct *) calloc(1, sizeof(struct BITstruct));
    if ((*BITptr)->next == NULL) {
        print_error("Allocation print_error or not enought memory !");
    }
    (*BITptr) = (*BITptr)->next;
    (void) strcpy((*BITptr)->name, name);
    (*BITptr)->dir  = dir;
    (*BITptr)->next = NULL;
}

void addSIG(struct SIGstruct **SIGptr, char *name, char dir, int start, int end) {

    (*SIGptr)->next = (struct SIGstruct *) calloc(1, sizeof(struct SIGstruct));
    if ((*SIGptr)->next == NULL) {
        print_error("Allocation print_error or not enought memory !");
    }
    (*SIGptr) = (*SIGptr)->next;
    (void) strcpy((*SIGptr)->name, name);
    (*SIGptr)->dir   = dir;
    (*SIGptr)->start = start;
    (*SIGptr)->end   = end;
    (*SIGptr)->next  = NULL;
#ifdef DEBUG
    (void)fprintf(stderr,"\n\t\tAdded SIGNAL <%s>, dir = %c, start =%d, end =%d",(*SIGptr)->name,(*SIGptr)->dir,(*SIGptr)->start,(*SIGptr)->end);
#endif
}

void warning(char *msg) {
    if (WARNING) (void) fprintf(stderr, "*parse warning* Line %u : %s\n", line, msg);
    SendTokenBack = 1;
}

void vst_error(char *name, char *next) {
    char *w;
    char LocalToken[MAXTOKENLEN];

    w = &(LocalToken[0]);
    (void) fprintf(stderr, "*print_error* Line %u : %s\n", line, name);
    (void) fprintf(stderr, "*print_error* Line %u : skipping text until the keyword %s is reached\n", line, next);
    SendTokenBack = 1;
    do {
        get_next_token(w);
        if (feof(In))
            print_error("Unespected Eof!");
    } while (!kwrd_cmp(w, next));
}

int dec_number(char *string) {
    char msg[50];
    char *s;

    for (s = string; *s != '\0'; s++)
        if (!isdigit(*s)) {
            (void) sprintf(msg, "*print_error Line %u : Expected decimal integer number \n", line);
            print_error(msg);
        }
    return atoi(string);
}


/*=========================================================================*


  Genlib scan
  ^^^^^^^^^^^

  print_gates  : outputs the gates read if in DEBUG mode
  get_lib_token : tokenizer for the genlib file
  is_here      : formals check
  scan_library : parses the genlib files and builds the data structure.
  what_gate    : checks for a cell into the library struct.

 *=========================================================================*/

void print_gates(struct LibCell *cell) {
    struct Ports *ptr;
    int          j;

    while (cell->next != NULL) {
        if (cell->clk[0]) {
            (void) fprintf(stderr, "Latch name: %s, num pins : %d\n", cell->name, cell->npins);
        } else {
            (void) fprintf(stderr, "Cell name: %s, num pins : %d\n", cell->name, cell->npins);
        }
        ptr    = cell->formals;
        for (j = 0; j < cell->npins; j++, ptr++) {
            (void) fprintf(stderr, "\tpin %d : %s\n", j, ptr->name);
        }
        if (cell->clk[0]) {
            (void) fprintf(stderr, "\tclock : %s\n", cell->clk);
        }
        cell = cell->next;
    }
}

char get_type_of_cell(struct LibCell *cell, char *name) {
    while (cell->next != NULL) {
        if (kwrd_cmp(cell->name, name)) {
            if (cell->clk[0]) {
                return 'L';   /* Library celly, type = L(atch) */
            } else {
                return 'G';   /* Library celly, type = G(ate)  */
            }
        }
        cell = cell->next;
    }
    return 'S';   /* type = S(ubcircuit) */
}

void get_lib_token(FILE *Lib, char *tok) {
    enum states {
        tZERO, tLONG, tEOF, tSTRING, tREM
    };
    static enum states next;
    static char        init = 0;
    static char        sentback;
    static char        TOKEN[MAXTOKENLEN];
    static char        str;
    char               ready;
    int                num;
    char               *t;
    char               c;

    if (!init) {
        sentback = 0;
        init     = 1;
        next     = tZERO;
        str      = 0;
    }

    t   = &(TOKEN[0]);
    num = 0;
    str = 0;

    do {
        if (sentback) {
            c = sentback;
        } else {
            c = fgetc(Lib);
        }
        if (feof(Lib)) next = tEOF;
        ready    = 0;
        sentback = '\0';

        switch (next) {
            case tZERO:
                if ((c == ' ') || (c == '\r') || (c == '\t')) {
                    next = tZERO;
                } else {
                    if (c == '#') {
                        next = tREM;
                    } else {
                        if (((c >= 0x27) && (c <= 0x2b)) || (c == '=') || (c == ';') || (c == '\n') || (c == '!')) {
                            *t = c;
                            t++;
                            next  = tZERO;
                            ready = 1;
                        } else {
                            if (c == '"') {
                                num  = 0;
                                next = tSTRING;
                            } else {
                                num      = 0;
                                next     = tLONG;
                                ready    = 0;
                                sentback = c;
                            }
                        }
                    }
                }
                break;
            case tLONG:
                if ((c == ' ') || (c == '\r') || (c == '\t')) {
                    ready = 1;
                    next  = tZERO;
                } else {
                    if (((c >= 0x27) && (c <= 0x2b)) || (c == '=') || (c == ';') || (c == '\n') || (c == '!')) {
                        next     = tZERO;
                        ready    = 1;
                        sentback = c;
                    } else {
                        if (c == '"') {
                            ready = 1;
                            next  = tSTRING;
                        } else {
                            if (c == '#') {
                                ready = 1;
                                next  = tREM;
                            } else {
                                *t = c;
                                t++;
                                num++;
                                next       = tLONG;
                                if ((ready = (num >= MAXTOKENLEN - 1)))
                                    (void) fprintf(stderr, "Sorry, exeeded max name len of %u", num + 1);
                            }
                        }
                    }

                }
                break;
            case tSTRING:
                if (!str) {
                    *t = '"';
                    t++;
                    num++;
                    str = 1;
#ifdef DEBUG
                    (void)fprintf(stderr,"<%c>\n",c);
#endif
                }
                *t         = c;
                t++;
                num++;
                if (c == '"') {   /* last dblquote */
                    ready = 1;
                    next  = tZERO;
#ifdef DEBUG
                    (void)fprintf(stderr,"STRING : %s\n",TOKEN);
#endif
                    break;
                }
                next       = tSTRING;
                if ((ready = (num >= MAXTOKENLEN - 1)))
                    (void) fprintf(stderr, "Sorry, exeeded max name len of %u", num + 1);
                break;
            case tREM: next = tREM;
                if (c == '\n') {
                    sentback = c;
                    next     = tZERO;
                }
                break;
            case tEOF: next = tEOF;
                ready    = 1;
                sentback = c;
                *t = c;
#ifdef DEBUG
                (void)fprintf(stderr,"EOF\n");
#endif
                break;
            default : next = tZERO;
        }
    } while (!ready);
    *t = '\0';
    (void) strcpy(tok, &(TOKEN[0]));
}

struct BITstruct *is_here(char *name, struct BITstruct *ptr) {
    struct BITstruct *BITptr;

    BITptr = ptr;
    while (BITptr->next != NULL) {
        BITptr = BITptr->next;
        if (kwrd_cmp(name, BITptr->name)) return BITptr;
    }
    return (struct BITstruct *) NULL;
}

struct LibCell *new_lib_cell(char *name, struct BITstruct *ports, int latch) {
    struct LibCell   *tmp;
    struct BITstruct *bptr;
    struct Ports     *pptr;
    int              j;
    int              num;

    if ((tmp = (struct LibCell *) calloc(1, sizeof(struct LibCell))) == NULL) {
        print_error("Allocation print_error or not enought memory !");
    }

    for (bptr = ports, num = 1; bptr->next != NULL; num++, bptr = bptr->next);
    if (latch) num--;

    if ((tmp->formals = (struct Ports *) calloc(1, num * sizeof(struct Ports))) == NULL) {
        print_error("Allocation print_error or not enought memory !");
    }

    (void) strcpy(tmp->name, name);
    tmp->next  = NULL;
    tmp->npins = num;
    num--;
    for (bptr = ports->next, pptr = tmp->formals, j = 0; (j < num) && (bptr != NULL); pptr++, j++, bptr = bptr->next) {
        if (j > num) print_error("(new_lib_cell) error ...");
#ifdef DEBUG
        (void)fprintf(stderr,"(new_lib_cell):adding %s\n",bptr->name);
#endif
        (void) strcpy(pptr->name, bptr->name);
    }
    (void) strcpy(pptr->name, ports->name); /* output is the last one */
    if (latch) {
        (void) strcpy(tmp->clk, bptr->name);
#ifdef DEBUG
        (void)fprintf(stderr,"(new_lib_cell):clock %s\n",bptr->name);
#endif
    } else {
        tmp->clk[0] = '\0';
    }
    releaseBIT(ports);
    return tmp;
}

struct LibCell *scan_library(char *LibName) {
    enum states {
        sZERO, sPIN, sCLOCK, sADDCELL
    }                next;
    FILE             *Lib;
    struct LibCell   *cell;
    struct LibCell   first;
    struct BITstruct *tmpBIT;
    struct BITstruct firstBIT;
    char             LocalToken[MAXTOKENLEN];
    char             name[MAXNAMELEN];
    int              latch;
    char             *s;


    if ((Lib = fopen(LibName, "rt")) == NULL)
        print_error("Couldn't open library file");


/*    first.next=new_lib_cell("_dummy_",(struct BITstruct *)NULL,LIBRARY); */
    firstBIT.name[0] = '\0';
    firstBIT.next = NULL;
    cell  = &first;
    s     = &(LocalToken[0]);
    latch = 0;
    next  = sZERO;
    (void) fseek(Lib, 0L, SEEK_SET);
    tmpBIT = &firstBIT;
    do {
        get_lib_token(Lib, s);
        switch (next) {
            case sZERO: next = sZERO;
                if (kwrd_cmp(s, "GATE")) {
                    latch = 0;
                    get_lib_token(Lib, s);
                    (void) strcpy(name, s);
                    get_lib_token(Lib, s);   /* area */
                    next = sPIN;
#ifdef DEBUG
                    (void)fprintf(stderr,"Gate name: %s\n",name);
#endif
                } else {
                    if (kwrd_cmp(s, "LATCH")) {
                        latch = 1;
                        get_lib_token(Lib, s);
                        (void) strcpy(name, s);
                        get_lib_token(Lib, s);   /* area */
                        next = sPIN;
#ifdef DEBUG
                        (void)fprintf(stderr,"Latch name: %s\n",name);
#endif
                    }
                }
                break;
            case sPIN:
                if (!(((*s >= 0x27) && (*s <= 0x2b)) || (*s == '=') || (*s == '!') || (*s == ';'))) {
/*
		(void)strncpy(tmp,s,5);
		if (kwrd_cmp(tmp,"CONST") && !isalpha(*(s+6)))
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
                    if (is_here(s, &firstBIT) == NULL) {
#ifdef DEBUG
                        (void)fprintf(stderr,"\tunknown pin : %s --> added!\n",s);
#endif
                        addBIT(&tmpBIT, s, NULL);
                    }
                }
                if (*s == ';') {
                    if (latch) {
                        next = sCLOCK;
                    } else {
                        next = sADDCELL;
                    }
                } else {
                    next = sPIN;
                }
                break;
            case sCLOCK:
                if (kwrd_cmp(s, "CONTROL")) {
                    get_lib_token(Lib, s);
#ifdef DEBUG
                    (void)fprintf(stderr,"\tcontrol pin : %s\n",s);
#endif
                    addBIT(&tmpBIT, s, NULL);
                    next = sADDCELL;
                } else {
                    next = sCLOCK;
                }
                break;
            case sADDCELL:
#ifdef DEBUG
                (void)fprintf(stderr,"\tadding cell to library\n");
#endif
                cell->next = new_lib_cell(name, firstBIT.next, latch);
                tmpBIT = &firstBIT;
                firstBIT.next = NULL;
                cell = cell->next;
                next = sZERO;
                break;
        }
    } while (!feof(Lib));

    if ((first.next)->next == NULL) {
        (void) sprintf("Library file %s does *NOT* contains gates !", LibName);
        print_error("could not continue with an empy library");
    }
#ifdef DEBUG
                                                                                                                            (void)fprintf(stderr,"end of lib\n");
    print_gates(first.next);
#endif
    return (struct LibCell *) first.next;
}

struct LibCell *what_lib_gate(char *name, struct LibCell *LIBRARY) {
    struct LibCell *ptr;

    for (ptr = LIBRARY; ptr != NULL; ptr = ptr->next)
        if (kwrd_cmp(ptr->name, name)) return ptr;

    return (struct LibCell *) NULL;
}

/*=========================================================================*



 *=========================================================================*/

struct Cell *new_cell(char *name, struct SIGstruct *Bports, struct BITstruct *Fports, struct LibCell *Genlib) {
    struct Cell      *tmp;
    struct BITstruct *Fptr;
    struct Ports     *Pptr;
    struct Ports     *Lpptr;
    struct LibCell   *Lptr;
    int              j;
    int              num;
    char             t;

    if ((tmp = (struct Cell *) calloc(1, sizeof(struct Cell))) == NULL) {
        print_error("Allocation print_error or not enought memory !");
    }

    if (Bports != NULL) {
        j    = 1;
        Fptr = Fports;
        while (Fptr->next != NULL) {
            j++;
            Fptr = Fptr->next;
        }
        num  = j;
        t    = get_type_of_cell(Genlib, name);
        if (t == 'L') j++;

        if ((tmp->formals = (struct Ports *) calloc(1, j * sizeof(struct Ports))) == NULL) {
            print_error("Allocation print_error or not enought memory !");
        }
        (void) strcpy(tmp->name, name);
        tmp->npins = j;
        tmp->next  = NULL;
        tmp->type  = t;

        if (t == 'S') {
            /*                                           *
	     * Is a subcircuit, no ordering is necessary *
	     *                                           */
            Fptr = Fports;
            Pptr = tmp->formals;
            j    = 0;
            while (Fptr != NULL) {
                if (j > num) print_error("(new_cell) error ...");
#ifdef DEBUG
                (void)fprintf(stderr,"(new_cell): adding %s\n",Fptr->name);
#endif
                (void) strcpy(Pptr->name, Fptr->name);
                Fptr = Fptr->next;
                Pptr++;
                j++;
            }
            releaseBIT(Fports->next);
            tmp->io      = Bports->next;
            Bports->next = NULL;
        } else {
            /*                                            *
	     * Is a library cell, let's order the signals *
	     * to simplify the final output               *
	     *                                            */
            Lptr = what_lib_gate(name, Genlib);
            Pptr = tmp->formals;
            if (t == 'L') num--;
            for (j = 1, Lpptr = Lptr->formals; j < num; j++, Lpptr++, Pptr++) {
                Fptr = Fports->next;
#ifdef DEBUG
                (void)fprintf(stderr,"\npin %s \n",Lpptr->name);
#endif
                while (Fptr != NULL) {
#ifdef DEBUG
                    (void)fprintf(stderr,"is %s ?\n",Fptr->name);
#endif
                    if (kwrd_cmp(Fptr->name, Lpptr->name)) {
                        (void) strcpy(Pptr->name, Fptr->name);
                        break;
                    }
                    Fptr = Fptr->next;
                }
                if (Fptr == NULL) (void) print_error("Mismatch between COMPONENT declaration and library one");
            }
            if (t == 'L') {
                Fptr = Fports->next;
                while (Fptr != NULL) {
                    if (kwrd_cmp(Fptr->name, Lptr->clk)) {
                        (void) strcpy(Pptr->name, Fptr->name);
                        break;
                    }
                    Fptr = Fptr->next;
                }
                if (Fptr == NULL) (void) print_error("Mismatch between COMPONENT declaration and libray one");
            }
            releaseBIT(Fports->next);
            tmp->io      = Bports->next;
            Bports->next = NULL;

        }

    } else {
        tmp->formals = NULL;
    }
    return tmp;
}

struct Cell *what_gate(char *name, struct Cell *LIBRARY) {
    struct Cell *ptr;

    for (ptr = LIBRARY; ptr != NULL; ptr = ptr->next)
        if (kwrd_cmp(ptr->name, name)) return ptr;

    return (struct Cell *) NULL;
}

struct Cell *get_port(char *Cname) {
    enum states {
        sFORMAL, sCONN, sANOTHER, sDIR, sTYPE, sVECTOR, sWAIT
    }                next;
    struct SIGstruct BITstart;
    struct BITstruct FORMstart;
    struct SIGstruct *BITptr;
    struct BITstruct *FORMptr;
    struct BITstruct *TMPptr;
    struct BITstruct TMPstart;
    char             *w;
    char             LocalToken[MAXTOKENLEN];
    char             tmp[MAXNAMELEN];
    char             dir;
    int              Cont;
    int              Token;
    int              start;
    int              end;
    int              j;
    int              num;


    w = &(LocalToken[0]);

    get_next_token(w);
    if (*w != '(') warning("expected '('");

    BITptr  = &BITstart;
    FORMptr = &FORMstart;
    TMPstart.next = NULL;

    dir   = '\0';
    Token = 1;
    start = 0;
    end   = 0;
    next  = sFORMAL;
    Cont  = 1;
    num   = 0;
    do {
        /* name of the port */
        if (Token) get_next_token(w);
        else Token = 1;
        switch (next) {
            case sFORMAL:
#ifdef DEBUG
                (void)fprintf(stderr,"\n\n** getting formal : %s",w);
#endif
                (void) strcpy(TMPstart.name, w);
                TMPptr = &TMPstart;
                next   = sCONN;
                break;
            case sCONN: next = sDIR;
                if (*w != ':') {
                    if (*w == ',') {
                        next = sANOTHER;
                    } else {
                        char s[40];
                        sprintf(s, "Expected ':' or ',' %d", line);
                        warning(s);
                        Token = 0;
                    }
                }
                break;
            case sANOTHER:
#ifdef DEBUG
                (void)fprintf(stderr,"\n*** another input : %s",w);
#endif
                if (TMPptr->next == NULL) {
                    addBIT(&TMPptr, w, 1);
                } else {
                    TMPptr = TMPptr->next;
                    (void) strcpy(TMPptr->name, w);
                }
                num++;
                next = sCONN;
                break;
            case sDIR:
#ifdef DEBUG
                (void)fprintf(stderr,"\n\tdirection = %s",w);
#endif
                if (kwrd_cmp(w, "IN")) {
                    dir = 'i';
                } else {
                    if (kwrd_cmp(w, "OUT")) {
                        dir = 'o';
                    } else {
                        if (kwrd_cmp(w, "INOUT")) {
                            dir = 'u';
                        } else {
                            if (kwrd_cmp(w, "LINKAGE")) {
                                dir = LINKAGE;
                            } else {
                                (void) fprintf(stderr, "* print_error * Line %u : unknown direction of a port\n", line);
                                print_error("Could not continue");
                            }
                        }
                    }
                }
                next = sTYPE;
                break;
            case sTYPE:
                if (kwrd_cmp(w, "BIT")) {
                    next  = sWAIT;
                    start = end;
                } else {
                    if (kwrd_cmp(w, "BIT_VECTOR")) {
#ifdef DEBUG
                        (void)fprintf(stderr,"\n\tvector, ");
#endif
                        next = sVECTOR;
                    }
                }
                break;
            case sVECTOR:
                if (*w != '(') {
                    warning("Expected '('");
                } else {
                    get_next_token(w);
                }
                start = dec_number(w);
                get_next_token(w);
                if (!kwrd_cmp(w, "TO") && !kwrd_cmp(w, "DOWNTO")) {
                    warning("Expected keword TO or DOWNTO");
                } else {
                    get_next_token(w);
                }
                end  = dec_number(w);
                get_next_token(w);
                if (*w != ')') {
                    warning("Expected ')'");
                    Token = 1;
                }
                next = sWAIT;
#ifdef DEBUG
                (void)fprintf(stderr," from %d to %d",start,end);
#endif
                break;
            case sWAIT: TMPptr = &TMPstart;
                /* if (num>0) num--; */
#ifdef DEBUG
                (void)fprintf(stderr,"\nPower?");
#endif
                if (!(kwrd_cmp(TMPptr->name, VDD) || kwrd_cmp(TMPptr->name, VSS))) {
#ifdef DEBUG
                    (void)fprintf(stderr," no\n");
#endif

                    while (num >= 0) {
#ifdef DEBUG
                        (void)fprintf(stderr,"\n\n%d)working on %s",num,TMPptr->name);
#endif
                        if (start == end) {
#ifdef DEBUG
                            (void)fprintf(stderr,"\nadded bit & formal of name : %s",TMPptr->name);
#endif
                            addSIG(&BITptr, TMPptr->name, dir, 0, 0);
                            addBIT(&FORMptr, TMPptr->name, dir);
                        } else {
                            if (start > end) {
                                for (j = start; j >= end; j--) {
                                    (void) sprintf(tmp, "%s[%d]", TMPptr->name, j);
#ifdef DEBUG
                                    (void)fprintf(stderr,"\nadded formal of name : %s",tmp);
#endif
                                    addBIT(&FORMptr, tmp, dir);
                                }
#ifdef DEBUG
                                (void)fprintf(stderr,"\nadded vector of name : %s[%d..%d]",TMPptr->name,start,end);
#endif
                            } else {
                                for (j = start; j <= end; j++) {
                                    (void) sprintf(tmp, "%s[%d]", TMPptr->name, j);
#ifdef DEBUG
                                    (void)fprintf(stderr,"\nadded formal of name : %s",tmp);
#endif
                                    addBIT(&FORMptr, tmp, dir);
                                }
#ifdef DEBUG
                                (void)fprintf(stderr,"\nadded vector of name : %s[%d..%d]",TMPptr->name,start,end);
#endif
                            }
                            addSIG(&BITptr, TMPptr->name, dir, start, end);
                        }
                        TMPptr = TMPptr->next;
                        num--;
                    }

                }
                num = 0;
                if (*w == ';') {
                    next = sFORMAL;
                } else {
                    if (*w != ')') {
                        vst_error("Missing ')' or ';'", "END");
                    } else {
                        get_next_token(w);  /* ; */
                        if (*w != ';') {
                            warning("Missing ';'");
                        } else {
                            get_next_token(w);  /* end */
                        }
                        if (!kwrd_cmp(w, "END")) {
                            vst_error("Missing END keyword", "END");
                        }
                    }
                    Cont = 0;
                }
                break;
        }

    } while (Cont);
    releaseBIT(TMPstart.next);
    return new_cell(Cname, &BITstart, &FORMstart, LIBRARY);
}

void get_entity(struct Cell **Entity) {
    char *w;
    char LocalToken[MAXTOKENLEN];
    char name[MAXTOKENLEN];
    int  num;

    w = &(LocalToken[0]);

    /* name of the entity = name of the model */
    get_next_token(w);
    (void) strcpy(name, w);
    get_next_token(w);
    if (!kwrd_cmp(w, "IS")) warning("expected syntax: ENTITY <name> IS");

    /* GENERIC CLAUSE */
    get_next_token(w);
    if (kwrd_cmp(w, "GENERIC")) {
        get_next_token(w);
        if (*w != '(') warning("expected '(' after GENERIC keyword");
        num = 1;
        do {
            get_next_token(w);
            if (*w == '(') num++;
            else {
                if (*w == ')') num--;
            }
        } while (num != 0);
        get_next_token(w);
        if (*w != ';') warning("expected ';'");
        get_next_token(w);
    }

    /* PORT CLAUSE */
    if (kwrd_cmp(w, "PORT")) {
        (*Entity) = get_port(name);
    } else {
        warning("no inputs or outputs in this entity ?!");
    }

    get_next_token(w);
    if (!kwrd_cmp(w, name))
        warning("<name> after END differs from <name> after ENTITY");

    get_next_token(w);
    if (*w != ';') warning("expected ';'");
}

void get_component(struct Cell **cell) {
    char *w;
    char LocalToken[MAXTOKENLEN];
    char name[MAXNAMELEN];

    w = &(LocalToken[0]);
    /* component name */
    get_next_token(w);
    (void) strcpy(name, w);
#ifdef DEBUG
    (void)fprintf(stderr,"\nParsing component %s\n",name);
#endif

    /* A small checks may be done here ... next time */

    /* PORT CLAUSE */
    get_next_token(w);
    if (kwrd_cmp(w, "PORT")) {
        (*cell)->next = get_port(name);
        (*cell) = (*cell)->next;
    } else {
        warning("no inputs or outputs in this component ?!");
    }


    /* END CLAUSE */
    get_next_token(w);
    if (!kwrd_cmp(w, "COMPONENT")) {
        warning("COMPONENT keyword missing.sh !");
    }

    get_next_token(w);
    if (*w != ';') warning("expected ';'");

}

void get_signal(struct SIGstruct **Internals) {
    struct SIGstruct *SIGptr;
    struct SIGstruct *SIGstart;
    char             *w;
    char             LocalToken[MAXTOKENLEN];
    char             name[MAXNAMELEN];
    char             dir;
    int              vect;
    int              start;
    int              end;


    SIGstart = (struct SIGstruct *) calloc(1, sizeof(struct SIGstruct));
    if (SIGstart == NULL) {
        print_error("Allocation print_error or not enought memory !");
    }
    SIGptr = SIGstart;

    w = &(LocalToken[0]);

    dir = '\0';

    do {
        get_next_token(w);
#ifdef DEBUG
        (void)fprintf(stderr,"\n\n** getting signal %s",w);
#endif
        addSIG(&SIGptr, w, '*', 999, 999);
        get_next_token(w);
    } while (*w == ',');

    if (*w != ':') {
        warning("Expected ':'");
    } else {
        get_next_token(w);
    }

    start = 0;
    end   = 0;
    vect  = 0;
    if (*(w + 3) == '_') {
        *(w + 3) = '\0';
        vect = 1;
    }
    if (kwrd_cmp(w, "BIT")) {
        dir = 'b';
    } else {
        if (kwrd_cmp(w, "MUX")) {
            dir = 'm';
        } else {
            if (kwrd_cmp(w, "WOR")) {
                dir = 'w';
            }
        }
    }
    if (vect && !kwrd_cmp(w + 4, "VECTOR")) {
        (void) fprintf(stderr, " Unknown signal type : %s\n", w);
        print_error("could not continue.");
    }

    if (vect) {
        get_next_token(w);
        if (*w != '(') {
            warning("Expected '('");
        } else {
            get_next_token(w);
        }
        start = dec_number(w);
        get_next_token(w);
        if (!kwrd_cmp(w, "TO") && !kwrd_cmp(w, "DOWNTO")) {
            warning("Expected keword TO or DOWNTO");
        } else {
            get_next_token(w);
        }

        end = dec_number(w);
        get_next_token(w);
        if (*w != ')') {
            warning("Expected ')'");
        }
#ifdef DEBUG
        (void)fprintf(stderr," from %d to %d",start,end);
#endif
    }

    get_next_token(w);
    if (dir != 'b') {
        if (!kwrd_cmp(w, "BUS")) {
            warning(" Keyword 'BUS' expected");
        } else {
            get_next_token(w);
        }
    }

    SIGptr = SIGstart;
    while (SIGptr->next != NULL) {
        SIGptr = SIGptr->next;
        if (vect) {
#ifdef DEBUG
            (void)fprintf(stderr,"\nadded vector of name : %s[%d..%d]",name,start,end);
#endif
            addSIG(Internals, SIGptr->name, dir, start, end);
        } else {
#ifdef DEBUG
            (void)fprintf(stderr,"\nadded bit of name : %s",name);
#endif
            addSIG(Internals, SIGptr->name, dir, 0, 0);
        }
    }
    releaseSIG(SIGstart);

    if (*w != ';') {
        warning("expected ';'");
    }

}


void fill_term(struct TERMstruct *TERM, struct ENTITYstruct *Entity, int which, struct Cell *WhatCell) {
    struct Cell      *cell;
    struct SIGstruct *Sptr;

#ifdef DEBUG
    (void)fprintf(stderr,"filling dimension of %s\n",TERM->name);
#endif
    if (which) {
#ifdef DEBUG
        (void)fprintf(stderr,"searchin into component %s\n",WhatCell->name);
#endif
        cell = Entity->Components;
        while (cell != NULL) {
            if (!strcmp(cell->name, WhatCell->name)) {
                Sptr = cell->io;
                while (Sptr != NULL) {
                    if (kwrd_cmp(TERM->name, Sptr->name)) {
                        TERM->start = Sptr->start;
                        TERM->end   = Sptr->end;
                        return;
                    }
                    Sptr = Sptr->next;
                }
            }
            cell = cell->next;
        }
    } else {
        Sptr = (Entity->EntityPort)->io;
        while (Sptr != NULL) {
#ifdef DEBUG
            (void)fprintf(stderr,"searchin into io\n");
#endif
            if (kwrd_cmp(TERM->name, Sptr->name)) {
                TERM->start = Sptr->start;
                TERM->end   = Sptr->end;
                return;
            }
            Sptr = Sptr->next;
        }
        Sptr = Entity->Internals;
        while (Sptr != NULL) {
#ifdef DEBUG
            (void)fprintf(stderr,"searchin into internals\n");
#endif
            if (kwrd_cmp(TERM->name, Sptr->name)) {
                TERM->start = Sptr->start;
                TERM->end   = Sptr->end;
                return;
            }
            Sptr = Sptr->next;
        }
        if (kwrd_cmp(TERM->name, VSS) || kwrd_cmp(TERM->name, VDD)) {
            TERM->start = 0;
            TERM->end   = 0;
            return;
        }
        (void) fprintf(stderr, "Signal name %s not declared", TERM->name);
        print_error("Could not continue");
    }
}

void get_name(struct TERMstruct *TERM, struct ENTITYstruct *Entity, int which, struct Cell *WhatCell) {
    char *w;
    char LocalToken[MAXTOKENLEN];

    TERM->start = -1;
    TERM->end   = -1;
    w = &(LocalToken[0]);
    do {
        get_next_token(w);
#ifdef DEBUG
        (void)fprintf(stderr,"Parsing %s\n",w);
#endif
    } while ((*w == ',') || (*w == '&'));
    (void) strcpy(TERM->name, w);
    get_next_token(w);
#ifdef DEBUG
    (void)fprintf(stderr,"then  %s\n",w);
#endif
    if (*w != '(') {
        fill_term(TERM, Entity, which, WhatCell);
        if ((TERM->start == TERM->end) && (TERM->start == 0)) {
            TERM->start = -1;
            TERM->end   = -1;
        }
        SendTokenBack = 1;
        /* Don't lose this token */
        return;
    } else {
#ifdef DEBUG
        (void)fprintf(stderr,"got (\n");
#endif
    }
    get_next_token(w);
    TERM->start = dec_number(w);
    get_next_token(w);
    if (*w != ')') {
        if (!kwrd_cmp(w, "TO") && !kwrd_cmp(w, "DOWNTO")) {
            print_error("expected ')' or 'TO' or 'DOWNTO', could not continue");
        } else {
            get_next_token(w);
            TERM->end = dec_number(w);
            get_next_token(w);
            if (*w != ')') {
                warning("Expected ')'");
                SendTokenBack = 1;
            }
        }
    } else {
#ifdef DEBUG
        (void)fprintf(stderr,"--> just an element of index %d\n",TERM->start);
#endif
        TERM->end = TERM->start;
    }
}


void change_internal(struct SIGstruct *Intern, char *name) {
    struct SIGstruct *Sptr;

#ifdef DEBUG
    (void)fprintf(stderr,"\n ** scanning into internals --> Int=%p",Intern);
#endif
    Sptr = Intern;
    while (Sptr != NULL) {
#ifdef DEBUG
        (void)fprintf(stderr,"\t <%s>",Sptr->name);
#endif
        if (!strcmp(Sptr->name, name)) {
            char i;
            i = *name;
            i = ((i <= 'z') && (i >= 'a') ? i + ('A' - 'a') : i);
            *name = i;
#ifdef DEBUG
            (void)fprintf(stderr,"\n *** %s : e' un segnale interno\n",name);
#endif
            return;
        }
        Sptr = Sptr->next;
    }
}

struct Instance *get_instance(char *name, struct ENTITYstruct *Entity) {
    struct Cell       *cell;
    struct Instance   *INST;
    struct TERMstruct FORMterm;
    struct TERMstruct ACTterm;
    struct SIGstruct  *ACTtrms;
    struct SIGstruct  *ACTptr;
    struct Ports      *Cptr;
    struct Ports      *Aptr;
    char              *w;
    char              LocalToken[MAXTOKENLEN];
    int               j;
    char              Iname[MAXTOKENLEN + 10];
    int               incF, incA;
    int               iiF;
    int               iF, iA;

    w = &(LocalToken[0]);
    get_next_token(w);  /* : */
    get_next_token(w);
    cell = what_gate(w, Entity->Components);
#ifdef DEBUG
    (void)fprintf(stderr,"\n=========================\nParsing instance %s of component %s\n==========================\n",name,cell->name);
#endif

    INST = (struct Instance *) calloc(1, sizeof(struct Instance));
    if (INST == NULL) {
        print_error("Allocation print_error or not enought memory !");
    }
    (void) strcpy(INST->name, name);
    INST->what = what_gate(w, Entity->Components);
    INST->actuals =
            (struct Ports *) calloc(1, ((INST->what)->npins) * sizeof(struct Ports));
    if (INST->actuals == NULL) {
        print_error("Allocation print_error or not enought memory !");
    }

    ACTtrms = (struct SIGstruct *) calloc(1, sizeof(struct SIGstruct));
    if (ACTtrms == NULL) {
        print_error("Allocation print_error or not enought memory !");
    }


    get_next_token(w);
    if (!kwrd_cmp(w, "PORT")) {
        warning("PORT keyword missing.sh");
    }

    get_next_token(w);
    if (!kwrd_cmp(w, "MAP"))
        warning("MAP keyword missing.sh");

    get_next_token(w);
    if (*w != '(')
        warning("Expexcted '('");

    do {
        get_name(&FORMterm, Entity, 1, cell);
#ifdef DEBUG
        (void)fprintf(stderr,"\t formal : %s\n",FORMterm.name);
#endif

        get_next_token(w);
        if (*w != '=') {
            if (*w != '>') {
                warning("Expected '=>'");
                SendTokenBack = 1;
            }
        } else {
            get_next_token(w);
            if (*w != '>') {
                warning("Expected '=>'");
                SendTokenBack = 1;
            }
        }

        ACTptr = ACTtrms;
        do {
            get_name(&ACTterm, Entity, 0, cell);
#ifdef DEBUG
            (void)fprintf(stderr,"\t actual : %s .. %s\n",ACTterm.name,w);
#endif
            addSIG(&ACTptr, ACTterm.name, '\0', ACTterm.start, ACTterm.end);
            change_internal(Entity->Internals, ACTptr->name);
#ifdef DEBUG
            (void)fprintf(stderr,"\nafter GetInstance: %s %s\n",ACTterm.name,ACTptr->name);
#endif
            get_next_token(w);
        } while (*w == '&');

        if ((*w != ',') && (*w != ')')) {
            print_error("Expected ')' or ','");
        }
#ifdef DEBUG
        (void)fprintf(stderr,"----> %s\n",w);
#endif
        if (FORMterm.start == FORMterm.end) {
#ifdef DEBUG
            if (DEBUG) (void)fprintf(stderr,"after if (FORMterm.start==FORMterm.end) {: %s %s\n",ACTterm.name,ACTptr->name);
#endif
            Cptr   = (INST->what)->formals;
            Aptr   = INST->actuals;
            for (j = 0; j < (INST->what)->npins; j++, Cptr++, Aptr++) {
                if (kwrd_cmp(Cptr->name, FORMterm.name)) break;
            }
            if ((ACTterm.start != ACTterm.end) || ((ACTtrms->next)->next != NULL)) {
                warning("Actual vector's dimension differs to formal's one");
            }
#ifdef DEBUG
            (void)fprintf(stderr,"value: %d %s\n",ACTptr->start,ACTptr->name);
#endif
            if (ACTptr->start != -1) {
                if (isupper((ACTptr->name)[0])) {
                    (void) sprintf(Aptr->name, "%s_%d_", ACTptr->name, ACTterm.start);
                } else {
                    (void) sprintf(Aptr->name, "%s[%d]", ACTptr->name, ACTterm.start);
                }
            } else {
                (void) strcpy(Aptr->name, ACTterm.name);
            }
        } else {
#ifdef DEBUG
            (void)fprintf(stderr,"\t\tthey are vectors --> formal from %d to %d\n",FORMterm.start,FORMterm.end);
#endif
            incF   = (FORMterm.start > FORMterm.end ? -1 : 1);
            if (incF < 0) {
                /*
		 * Downto
		 */
                iiF = FORMterm.end;
            } else {
                /*
		 *  To
		 */
                iiF = FORMterm.start;
            }
            ACTptr = ACTtrms;
            incA   = 0;
            ACTptr->end = 0;
            iA = 0;
            iF = FORMterm.start;
            do {
                /*
		 *  Let's look if we need a new element...
		 */
                if (iA == (ACTptr->end + incA)) {
                    if (ACTptr->next == NULL) {
                        print_error("Wrong vector size in assignement");
                    }
                    ACTptr = ACTptr->next;
                    incA   = (ACTptr->start > ACTptr->end ? -1 : 1);
                    iA     = ACTptr->start;
#ifdef DEBUG
                    (void)fprintf(stderr,"ACTUAL changed!\ncurent : <%s> from %d to %d\n",ACTptr->name,ACTptr->start,ACTptr->end);
#endif
                }

                /*
		 *  let's make the connection
		 */
                (void) sprintf(Iname, "%s[%d]", FORMterm.name, iiF);
                Cptr   = (INST->what)->formals;
                Aptr   = INST->actuals;
                for (j = 0; j < (INST->what)->npins; j++, Cptr++, Aptr++) {
#ifdef DEBUG
                    (void)fprintf(stderr,"(%d)\tAptr->name=<%s>\tCptr->name=<%s>\tIname=<%s>\n",j,Aptr->name,Cptr->name,Iname);
#endif
                    if (kwrd_cmp(Cptr->name, Iname)) break;
                }
                if (ACTptr->start < 0) {
                    /*
		     * ACTual is a BIT
		     */
                    if (isupper((ACTptr->name)[0])) {
                        (void) sprintf(Aptr->name, "%s", ACTptr->name);
                    } else {
                        (void) sprintf(Aptr->name, "%s", ACTptr->name);
                    }
                } else {
                    /*
		     * ACTual is a VECTOR
		     */
                    if (isupper((ACTptr->name)[0])) {
                        (void) sprintf(Aptr->name, "%s_%d_", ACTptr->name, iA);
                    } else {
                        (void) sprintf(Aptr->name, "%s[%d]", ACTptr->name, iA);
                    }
                }
#ifdef DEBUG
                (void)fprintf(stderr,"%d)--> added ACTconn : <%s>\n",iF,Aptr->name);
#endif
                iF += incF;
                iA += incA;
                iiF++;
            } while (iF != (FORMterm.end + incF));
            releaseSIG(ACTtrms->next);
#ifdef DEBUG
            (void)fprintf(stderr,"---->release done<----\n ");
#endif
/*
	    for(iF=FORMterm.start, iA=ACTterm.start; iF!=FORMterm.end+incF; iF+=incF, iA+=incA) {
		sprintf(Iname,"%s[%d]",FORMterm.name,iF);
		Cptr=(INST->what)->formals;
		Aptr=INST->actuals;
		for(j=0; j<(INST->what)->npins; j++, Cptr++, Aptr++){
		    if (kwrd_cmp(Cptr->name,Iname)) break;
		}
		if (isupper(ACTterm.name[0])) {
		    sprintf(Aptr->name,"%s_%d_",ACTterm.name,ACTterm.start);
		} else {
		    sprintf(Aptr->name,"%s[%d]",ACTterm.name,iA);
		}
	    }
	    if (iA!=ACTterm.end+incA) {
		warning("Actual vector's dimension differs to formal's one");
	    }
*/
        }
    } while (*w != ')');

    free(ACTtrms);
    get_next_token(w);
    if (*w != ';') {
        warning("expected ';'");
    }
    return INST;
}

void get_architecture(struct ENTITYstruct *ENTITY) {
    struct Cell      *COMPOptr;
    struct SIGstruct *SIGptr;
    struct Instance  *INSTptr;
    struct Cell      COMPOstart;
    struct SIGstruct SIGstart;
    struct Instance  INSTstart;
    char             *w;
    char             LocalToken[MAXTOKENLEN];
    char             name[MAXTOKENLEN];
    char             msg[MAXTOKENLEN];


    SIGptr = &SIGstart;
    SIGstart.next = NULL;
    COMPOptr = &COMPOstart;
    COMPOstart.next = NULL;
    SIGptr = &SIGstart;

    w = &(LocalToken[0]);
    /* type of architecture... */
    get_next_token(w);

    get_next_token(w);
    if (!kwrd_cmp(w, "OF"))
        warning("expected syntax: ARCHITECTURE <type> OF <name> IS");
    get_next_token(w);
    (void) strcpy(name, w);
    get_next_token(w);
    if (!kwrd_cmp(w, "IS"))
        warning("expected syntax: ENTITY <name> IS");

    /* Components and signals: before a 'BEGIN' only sturcture *
     * COMPONENT and SIGNAL are allowed                        */
    do {
        get_next_token(w);
        if (kwrd_cmp(w, "COMPONENT")) {
            get_component(&COMPOptr);
        } else {
            if (kwrd_cmp(w, "SIGNAL")) {
                get_signal(&SIGptr);
            } else {
                if (kwrd_cmp(w, "BEGIN")) break;
                else {
                    (void) sprintf(msg, "%s unknown, skipped", w);
                    warning(msg);
                    SendTokenBack = 0; /* as we said we must skip it */
                }
            }
        }
        if (feof(In)) print_error("Unespected EoF");
    } while (1);

    ENTITY->Internals  = SIGstart.next;
    ENTITY->Components = COMPOstart.next;
    /* NETLIST */
    INSTptr = &INSTstart;
    do {
        get_next_token(w);
        if (kwrd_cmp(w, "END")) {
            break;
        } else {
            INSTptr->next = get_instance(w, ENTITY);
            INSTptr = INSTptr->next;
        }
        if (feof(In)) print_error("Unespected EoF");
    } while (1);

    ENTITY->Net = INSTstart.next;

    /* name of kind of architecture */
    get_next_token(w);
    if (*w != ';') {
        /* End of architecture last ';' */
        get_next_token(w);
        if (*w != ';') warning("extected ';'");
    }
}

void print_signals(char *msg, char typ, struct SIGstruct *Sptr) {
    int i;
    int incr;

    (void) fputs(msg, Out);
    while (Sptr != NULL) {
        if ((Sptr->dir == typ) && (!kwrd_cmp(Sptr->name, CLOCK))) {
            if (Sptr->start == Sptr->end) {
                (void) fprintf(Out, "%s ", Sptr->name);
            } else {
                incr   = (Sptr->start < Sptr->end ? 1 : -1);
                for (i = Sptr->start; i != Sptr->end; i += incr) {
                    (void) fprintf(Out, "%s[%d] ", Sptr->name, i);
                }
            }
        }
        Sptr = Sptr->next;
    }
    (void) fputs("\n", Out);
}

void print_ordered_signals(struct Ports *Sptr, struct LibCell *Lptr) {
    struct Ports *Lpptr;
    int          i;

    i     = 0;
    Lpptr = Lptr->formals;
    while (i < Lptr->npins) {
        (void) fprintf(Out, "%s=%s ", Lpptr->name, Sptr->name);
        Sptr++;
        i++;
        Lpptr++;
    }
    if (Lptr->clk[0]) (void) fputs(Sptr->name, Out);
}


void print_sls(struct ENTITYstruct *Entity) {
    struct Instance *Iptr;
    struct Ports    *Aptr;
    struct Ports    *Fptr;
    int             j;

    (void) fprintf(Out, "# *--------------------------------------*\n");
    (void) fprintf(Out, "# |    File created by Vst2Blif v 1.1    |\n");
    (void) fprintf(Out, "# |                                      |\n");
    (void) fprintf(Out, "# |         by Roberto Rambaldi          |\n");
    (void) fprintf(Out, "# |    D.E.I.S. Universita' di Bologna   |\n");
    (void) fprintf(Out, "# *--------------------------------------*/\n\n");

    (void) fprintf(Out, "\n.model %s\n", (Entity->EntityPort)->name);
    print_signals(".input ", 'i', (Entity->EntityPort)->io);
    print_signals(".output ", 'o', (Entity->EntityPort)->io);
    if (CLOCK[0]) {
        (void) fprintf(Out, ".clock %s", CLOCK);
    }
    (void) fputs("\n\n", Out);

    Iptr = Entity->Net;
    while (Iptr != NULL) {
        switch ((Iptr->what)->type) {
            case 'S' : (void) fprintf(Out, ".subckt %s ", (Iptr->what)->name);
                Aptr   = Iptr->actuals;
                Fptr   = (Iptr->what)->formals;
                for (j = 0, Aptr++, Fptr++; j < (Iptr->what)->npins - 1; j++, Aptr++, Fptr++) {
                    (void) fprintf(Out, "%s=%s ", Fptr->name, Aptr->name);
                }
                break;
            case 'L': (void) fprintf(Out, ".latch %s ", (Iptr->what)->name);
                print_ordered_signals(Iptr->actuals, what_lib_gate((Iptr->what)->name, LIBRARY));
                (void) fprintf(Out, " %c", INIT);
                break;
            case 'G': (void) fprintf(Out, ".gate %s ", (Iptr->what)->name);
                print_ordered_signals(Iptr->actuals, what_lib_gate((Iptr->what)->name, LIBRARY));
                break;
        }
        (void) fputs("\n", Out);
        Iptr = Iptr->next;
    }
    (void) fputs("\n\n", Out);


}

void parse_file() {
    struct ENTITYstruct LocalENTITY;
    char                *w;
    char                LocalToken[MAXTOKENLEN];
    int                 flag;

    w = &(LocalToken[0]);
    do {

        /* ENTITY CLAUSE */
        flag = 0;
        do {
            get_next_token(w);
            if ((flag = kwrd_cmp(w, "ENTITY"))) {
                get_entity(&(LocalENTITY.EntityPort));
            } else {
                if (*w == '\0') break;
                vst_error("No Entity ???", "ENTITY");
                /* After this call surely flag will be true *
		 * in any other cases the program will stop *
		 * so this point will be never reached ...  */
            }
        } while (!flag);

        /* ARCHITECTURE CLAUSE */
        flag = 0;
        do {
            get_next_token(w);
            if ((flag = kwrd_cmp(w, "ARCHITECTURE"))) {
                get_architecture(&LocalENTITY);
            } else {
                if (*w == '\0') break;
                vst_error("No Architecture ???", "ARCHITECTURE");
                /* it's the same as the previous one         */
            }
        } while (!flag);
        /* end of the model */

    } while (!feof(In));

    print_sls(&LocalENTITY);

}

int main(int argc, char **argv) {

    check_args(argc, argv);

    parse_file();

    close_all();

    exit(0);
}

