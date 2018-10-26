/**
 * blif2vst version 1.1
 *
 * Original version by Rambaldi Roberto, reworked by Matteo Iervasi
 *
 * Copyright 1994-2018
 */
#include "blif2vst.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define isBLK(c) ((((c) == '\t') || ((c) == ' ') || ((c) == '\r')))
#define isREM(c) (((c) == '#'))
#define isDQ(c) (((c) == '"'))
#define isCNT(c) (((c) == '\\'))
#define isEOL(c) (((c) == '\n'))
#define isSTK(c) (((c) == '(') || ((c) == ')') || isEOL(c) || ((c) == '='))

FILE *In;
FILE *Out;
int  line; // parsed line

struct cell *LIBRARY;
char        VDD[MAX_NAME_LENGTH] = {0}, VSS[MAX_NAME_LENGTH] = {0};
char        DEBUG;
char        ADDPOWER;

int main(int argc, char **argv) {
    check_args(argc, argv);
    parse_file();
    close_all();

    return 0;
}

// Procedures to exit or to display warnings

void close_all() {
    if (In != stdin)
        (void) fclose(In);
    if (Out != stdout)
        (void) fclose(Out);
}

void error(char *msg) {
    (void) fprintf(stderr, "*** error : %s\n", msg);
    close_all();
    exit(1);
}

void warning(char *name) {
    if (DEBUG)
        (void) fprintf(stderr, "*parse warning* Line %u : %s\n", line, name);
}

void syntax_error(char *msg, char *obj) {
    (void) fprintf(stderr, "\n*error* Line %u : %s\n", line, msg);
    (void) fprintf(stderr, "*error* Line %u : object of the error : %s\n", line,
                   obj);
    error("Could not continue!");
}

// General procedures

char keyword_compare(char *name, char *keywrd) {
    int  t;
    int  len;
    char *n;

    len = (int) strlen(name);
    if (*name == '"') {
        n = name + 1;
        len -= 2;
    } else {
        n = name;
    }
    if (len != (strlen(keywrd)))
        return 0;
    if (!len)
        return 0;
    /* if length 0 exit */
    for (t = 0; (t < len); n++, keywrd++, t++) {
        /* if they're not equal */
        if (toupper(*n) != toupper(*keywrd))
            return 0;
        /* EoL, exit */
        if ((*n == '\0') || (*n == '"'))
            break;
    }
    return 1;
}

struct cell *match_gate(char *name) {
    struct cell *ptr;

    for (ptr = LIBRARY; ptr != NULL; ptr = ptr->next)
        if (keyword_compare(ptr->name, name))
            return ptr;

    (void) fprintf(stderr, "instance %s not found in library!\n", name);
    error("could not continue");
    return (struct cell *) NULL;
}

void release_bit(struct BITstruct *ptr) {
    struct BITstruct *tmp;
    struct BITstruct *ptr2;

    ptr2 = ptr;
    while (ptr2 != NULL) {
        tmp = ptr2->next;
        free((char *) ptr2);
        ptr2 = tmp;
    }
}

struct cell *new_cell(char *name, struct BITstruct *ports) {
    struct cell      *tmp;
    struct BITstruct *Bptr;
    struct ports     *Pptr;
    unsigned int     j;
    int              num;

    if ((tmp = (struct cell *) calloc(1, sizeof(struct cell))) == NULL) {
        error("Allocation error or not enought memory !");
    }

    if (ports != NULL) {
        j    = 1;
        Bptr = ports;
        while (Bptr->next != NULL) {
            j++;
            Bptr = Bptr->next;
        }
        num  = j;

        if ((tmp->formals = (struct ports *) calloc(1, j * sizeof(struct ports))) ==
            NULL) {
            error("Allocation error or not enought memory !");
        }
        (void) strcpy(tmp->name, name);
        tmp->npins = j;
        tmp->next  = NULL;

        Bptr = ports;
        Pptr = tmp->formals;
        j    = 0;
        while (Bptr != NULL) {
            if (j > num)
                error("__NewCell error ...");
            (void) strcpy(Pptr->name, Bptr->name);
            Bptr = Bptr->next;
            Pptr++;
            j++;
        }
        release_bit(ports);
    } else {
        tmp->formals = NULL;
    }
    return tmp;
}

struct instance *new_instance(struct cell *cell) {
    struct instance *tmp;
    unsigned        i;

    if ((tmp  = (struct instance *) calloc(1, sizeof(struct instance))) == NULL) {
        error("Allocation error or not enought memory !");
    }
    if (cell != NULL) {
        i = cell->npins * sizeof(struct ports);
        if ((tmp->actuals = (struct ports *) calloc(1, i)) == NULL) {
            error("Allocation error or not enought memory !");
        }
    } else {
        tmp->actuals = NULL;
    }
    tmp->what = cell;
    tmp->next = NULL;

    return tmp;
}

struct TYPEterms *new_type() {
    struct TYPEterms *TYPEptr;

    TYPEptr = (struct TYPEterms *) calloc(1, sizeof(struct TYPEterms));
    TYPEptr->BITs  = (struct BITstruct *) calloc(1, sizeof(struct BITstruct));
    TYPEptr->VECTs = (struct VECTstruct *) calloc(1, sizeof(struct VECTstruct));
    if ((TYPEptr == NULL) || (TYPEptr->BITs == NULL) ||
        (TYPEptr->VECTs == NULL)) {
        error("Allocation error or not enought memory !");
    }
    return TYPEptr;
}

struct MODELstruct *new_model() {
    struct MODELstruct *LocModel;

    if ((LocModel = (struct MODELstruct *) calloc(
            1, sizeof(struct MODELstruct))) == NULL) {
        error("Not enought memory or allocation error");
    }

    LocModel->Inputs    = new_type();
    LocModel->Outputs   = new_type();
    LocModel->Internals = new_type();
    LocModel->Net       = new_instance((struct cell *) NULL);

    return LocModel;
}

void add_vect(struct VECTstruct **VECTptr, char *name, int a, int b) {
    (*VECTptr)->next = (struct VECTstruct *) calloc(1, sizeof(struct VECTstruct));
    if ((*VECTptr)->next == NULL) {
        error("Allocation error or not enought memory !");
    }
    (*VECTptr) = (*VECTptr)->next;
    (void) strcpy((*VECTptr)->name, name);
    (*VECTptr)->start = a;
    (*VECTptr)->end   = b;
    (*VECTptr)->next  = NULL;
}

void add_bit(struct BITstruct **BITptr, char *name) {
    (*BITptr)->next = (struct BITstruct *) calloc(1, sizeof(struct BITstruct));
    if ((*BITptr)->next == NULL) {
        error("Allocation error or not enought memory !");
    }
    (*BITptr) = (*BITptr)->next;
    (void) strcpy((*BITptr)->name, name);
    (*BITptr)->next = NULL;
}

/*
void AddTMP(TMPptr)
struct TMPstruct **TMPptr;
{

   (*TMPptr)->next=(struct TMPstruct *)calloc(1,sizeof(struct TMPstruct));
   if ( (*TMPptr)->next==NULL) {
        print_error("Allocation error or not enough memory !");
    }
   (*TMPptr)=(*TMPptr)->next;

}
*/

struct BITstruct *is_here(char *name, struct BITstruct *ptr) {
    struct BITstruct *BITptr;

    BITptr = ptr;
    while (BITptr->next != NULL) {
        BITptr = BITptr->next;
        if (keyword_compare(name, BITptr->name))
            return BITptr;
    }
    return (struct BITstruct *) NULL;
}

/*
struct BITstruct *IsKnown(name,model)
char *name;
struct MODELstruct *model;
{
    struct BITstruct *ptr;

    if ( (ptr=IsHere(name,(model->Inputs)->BITs))!=NULL ) return ptr;
    if ( (ptr=is_here(name,(model->Outputs)->BITs))!=NULL ) return ptr;
    if ( (ptr=is_here(name,(model->Internals)->BITs))!=NULL ) return ptr;
    return (struct BITstruct *)NULL;

}
*/

void get_token(char *tok) {
    enum states {
        tZERO, tLONG, tEOF, tSTRING, tCONT, tREM
    };
    static enum states next;
    static char        init = 0;
    static char        sentback;
    static char        TOKEN[MAX_TOKEN_LENGTH];
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
        line     = 0;
    }

    t   = &(TOKEN[0]);
    num = 0;
    str = 0;

    do {
        if (sentback) {
            c = sentback;
        } else {
            c = (char) fgetc(In);
            if (c == '\n')
                line++;
        }
        if (feof(In))
            next = tEOF;
        ready    = 0;
        sentback = '\0';

        switch (next) {
            case tZERO:
#if defined(DBG)
                (void)fprintf(stderr, "\n> ZERO, c=%c  token=%s", c, TOKEN);
#endif
                if
                        isBLK(c) { next = tZERO; }
                else {
                    if
                            isSTK(c) {
                        *t = c;
                        t++;
                        next  = tZERO;
                        ready = 1;
                    } else {
                        if
                                isREM(c) {
                            num  = 0;
                            next = tREM;
                        } else {
                            if
                                    isCNT(c) { next = tCONT; }
                            else {
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
                if (DEBUG)
                    (void) fprintf(stderr, "\n-> LONG, c=%c  token=%s", c, TOKEN);
                if (isBLK(c) || isSTK(c)) {
                    sentback = c;
                    ready    = 1;
                    next     = tZERO;
                } else {
                    if
                            isDQ(c) {
                        ready = 1;
                        next  = tSTRING;
                    } else {
                        if
                                isREM(c) {
                            ready = 1;
                            next  = tREM;
                        } else {
                            if
                                    isCNT(c) {
                                ready    = 1;
                                sentback = c;
                            } else {
                                *t = c;
                                t++;
                                num++;
                                next       = tLONG;
                                if ((ready = (num >= MAX_TOKEN_LENGTH - 1)))
                                    (void) fprintf(stderr, "Sorry, exeeded max name len of %u",
                                                   num + 1);
                            }
                        }
                    }
                }
                break;
            case tSTRING:
#if defined(DBG)
                (void)fprintf(stderr, "\n--> STRING, c=%c  token=%s", c, TOKEN);
#endif
                if (!str) {
                    *t = '"';
                    t++;
                    num++;
                    str = 1;
                }
                *t         = c;
                t++;
                num++;
                if (c == '"') { /* last dblquote */
                    ready = 1;
                    next  = tZERO;
                    break;
                }
                next       = tSTRING;
                if ((ready = (num >= MAX_TOKEN_LENGTH - 1)))
                    (void) fprintf(stderr, "Sorry, exeeded max name len of %u", num + 1);
                break;
            case tREM:
                if (DEBUG)
                    (void) fprintf(stderr, "\n---> REM, c=%c  token=%s", c, TOKEN);
                next = tREM;
                if
                        isEOL(c) {
                    sentback = c; /* in this case EOL must be given to the caller */
                    next     = tZERO;
                }
                break;
            case tCONT:
#if defined(DBG)
                (void)fprintf(stderr, "\n----> CONT, c=%c  token=%s", c, TOKEN);
#endif
                if
                        isEOL(c) {
                    sentback = 0; /* EOL must be skipped */
                    next     = tZERO;
                } else {
                    next = tCONT;
                }
                break;
            case tEOF:
#if defined(DBG)
                (void)fprintf(stderr, "\n-----> EOF, c=%c  token=%s", c, TOKEN);
#endif
                next     = tEOF;
                ready    = 1;
                sentback = c;
                *t = c;
                break;
            default:
#if defined(DBG)
                (void)fprintf(stderr, "\n-------> DEFAULT, c=%c  token=%s", c, TOKEN);
#endif
                next = tZERO;
        }
    } while (!ready);
    *t = '\0';
    (void) strcpy(tok, &(TOKEN[0]));
}

void print_gates(struct cell *cell) {
    struct ports *ptr;
    int          j;

    while (cell->next != NULL) {
        cell = cell->next;
        if (DEBUG)
            (void) fprintf(stderr, "cell name: %s, num pins : %d\n", cell->name,
                           cell->npins);
        ptr    = cell->formals;
        for (j = 0; j < cell->npins; j++, ptr++) {
            if (DEBUG)
                (void) fprintf(stderr, "\tpin %d : %s\n", j, ptr->name);
        }
    }
}

void get_lib_token(FILE *lib, char *tok) {
    enum states {
        tZERO, tLONG, tEOF, tSTRING, tREM
    };
    static enum states next;
    static char        init = 0;
    static char        sentback;
    static char        TOKEN[MAX_TOKEN_LENGTH];
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
            c = (char) fgetc(lib);
        }
        if (feof(lib))
            next = tEOF;
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
                        if (((c >= 0x27) && (c <= 0x2b)) || (c == '=') || (c == ';') ||
                            (c == '\n') || (c == '!')) {
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
                    if (((c >= 0x27) && (c <= 0x2b)) || (c == '=') || (c == ';') ||
                        (c == '\n') || (c == '!')) {
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
                                if ((ready = (num >= MAX_TOKEN_LENGTH - 1)))
                                    (void) fprintf(stderr, "Sorry, exeeded max name len of %u",
                                                   num + 1);
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
                    if (DEBUG)
                        (void) fprintf(stderr, "<%c>\n", c);
                    str = 1;
                }
                *t         = c;
                t++;
                num++;
                if (c == '"') { /* last dblquote */
                    ready = 1;
                    next  = tZERO;
                    if (DEBUG)
                        fprintf(stderr, "STRING : %s\n", TOKEN);
                    break;
                }
                next       = tSTRING;
                if ((ready = (num >= MAX_TOKEN_LENGTH - 1)))
                    (void) fprintf(stderr, "Sorry, exeeded max name len of %u", num + 1);
                break;
            case tEOF:next = tEOF;
                ready    = 1;
                sentback = c;
                *t = c;
                if (DEBUG)
                    (void) fprintf(stderr, "EOF\n");
                break;
            case tREM:next = tREM;
                if (c == '\n') {
                    sentback = c;
                    next     = tZERO;
                }
                break;
            default:next = tZERO;
        }
    } while (!ready);
    *t = '\0';
    (void) strcpy(tok, &(TOKEN[0]));
}

struct cell *scan_library(char *lib_name) {
    enum states {
        sZERO, sPIN, sCLOCK, sADDCELL
    }                next;
    FILE             *Lib;
    struct cell      *cell;
    struct cell      first;
    struct BITstruct *tmpBIT;
    struct BITstruct firstBIT;
    char             LocalToken[MAX_TOKEN_LENGTH];
    char             tmp[MAX_NAME_LENGTH];
    char             name[MAX_NAME_LENGTH];
    char             latch;
    char             *s;

    if ((Lib = fopen(lib_name, "rt")) == NULL)
        error("Couldn't open library file");

    first.next = new_cell("dummy", (struct BITstruct *) NULL);
    firstBIT.name[0] = '\0';
    firstBIT.next = NULL;
    cell  = first.next;
    s     = &(LocalToken[0]);
    latch = 0;
    next  = sZERO;
    (void) fseek(Lib, 0L, SEEK_SET);
    tmpBIT = &firstBIT;
    do {
        get_lib_token(Lib, s);
        switch (next) {
            case sZERO:next = sZERO;
                if (keyword_compare(s, "GATE")) {
                    latch = 0;
                    get_lib_token(Lib, s);
                    (void) strcpy(name, s);
                    get_lib_token(Lib, s); /* area */
                    next = sPIN;
                    if (DEBUG)
                        (void) fprintf(stderr, "Gate name: %s\n", name);
                } else {
                    if (keyword_compare(s, "LATCH")) {
                        latch = 1;
                        get_lib_token(Lib, s);
                        (void) strcpy(name, s);
                        get_lib_token(Lib, s); /* area */
                        next = sPIN;
                        if (DEBUG)
                            (void) fprintf(stderr, "Latch name: %s\n", name);
                    }
                }
                break;
            case sPIN:
                if (!(((*s >= 0x27) && (*s <= 0x2b)) || (*s == '=') || (*s == '!') ||
                      (*s == ';'))) {
                    (void) strncpy(tmp, s, 5);
                    if (keyword_compare(tmp, "CONST") && !isalpha(*(s + 6)))
                        /* if the expression has a constant value we must */
                        /* skip it, because there are no inputs           */
                        break;
                    /* it's an operand so get its name */
                    if (DEBUG)
                        (void) fprintf(stderr, "\tpin read : %s\n", s);
                    if (is_here(s, &firstBIT) == NULL) {
                        if (DEBUG)
                            (void) fprintf(stderr, "\tunknown pin : %s\n", s);
                        add_bit(&tmpBIT, s);
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
                if (keyword_compare(s, "CONTROL")) {
                    get_lib_token(Lib, s);
                    if (DEBUG)
                        (void) fprintf(stderr, "\tpin : %s\n", s);
                    add_bit(&tmpBIT, s);
                    next = sADDCELL;
                } else {
                    next = sCLOCK;
                }
                break;
            case sADDCELL:
                if (DEBUG)
                    (void) fprintf(stderr, "\tadding cell to library\n");
                cell->next = new_cell(name, firstBIT.next);
                tmpBIT = &firstBIT;
                firstBIT.next = NULL;
                cell = cell->next;
                next = sZERO;
                break;
        }
    } while (!feof(Lib));

    if ((first.next)->next == NULL) {
        fprintf(stderr, "Library file %s does *NOT* contains gates !", lib_name);
        error("could not continue with an empy library");
    }
    if (DEBUG)
        (void) fprintf(stderr, "eond of lib");
    print_gates(first.next);
    return first.next;
}

void check_args(int argc, char **argv) {
    char *s;
    char c;
    char help;

    s = &(argv[0][strlen(argv[0]) - 1]);
    while ((s >= &(argv[0][0])) && (*s != '/')) {
        s--;
    }
    (void) fprintf(stderr, "\t\t      Blif Converter v1.0\n");
    (void) fprintf(stderr, "\t\t      by Roberto Rambaldi\n");
    (void) fprintf(stderr, "\t\tD.E.I.S. Universita' di Bologna\n\n");
    help      = 0;
    ADDPOWER  = 0;
    while ((c = (char) getopt(argc, argv, "s:S:d:D:IihHvVnN$")) > 0) {
        switch (toupper(c)) {
            case 'S':(void) strncpy(VSS, optarg, MAX_NAME_LENGTH);
                break;
            case 'D':(void) strncpy(VDD, optarg, MAX_NAME_LENGTH);
                break;
            case 'V':DEBUG = 1;
                break;
            case 'H':help = 1;
                break;
            case 'N':ADDPOWER = 1;
                break;
            case '$':help = 1;
                break;
            default:(void) fprintf(stderr, "\t *** unknown options");
                help = 1;
                break;
        }
    }

    if (!help) {
        if (optind >= argc) {
            fprintf(stderr, "No Library file specified\n\n");
            help = 1;
        } else {
            LIBRARY = scan_library(argv[optind]);
            if (++optind >= argc) {
                In  = stdin;
                Out = stdout;
            } else {
                if ((In = fopen(argv[optind], "rt")) == NULL)
                    error("Couldn't read input file");
                if (++optind >= argc) {
                    Out = stdout;
                } else if ((Out = fopen(argv[optind], "wt")) == NULL)
                    error("Could'n make opuput file");
            }
        }

        if (!ADDPOWER) {
            VDD[0] = '\0';
            VSS[0] = '\0';
        } else {
            if (VDD[0] == '\0')
                (void) strcpy(VDD, "vdd");
            if (VSS[0] == '\0')
                (void) strcpy(VSS, "vss");
        }
    }

    if (help) {
        (void) fprintf(stderr,
                       "\tUsage: %s [options] <library> [infile [outfile]]\n", s);
        (void) fprintf(
                stderr,
                "\t       if outfile is not given will be used stdout, if also\n");
        (void) fprintf(stderr,
                       "\t       infile is not given will be used stdin instead.\n");
        (void) fprintf(stderr,
                       "\t<library>\t is the name of the library file to use\n");
        (void) fprintf(
                stderr, "\tOptions :\n\t-s <name>\t <name> will be used for VSS net\n");
        (void) fprintf(stderr, "\t-i \t\t <name> skip instance names\n");
        (void) fprintf(stderr, "\t-d <name>\t <name> will be used for VDD net\n");
        (void) fprintf(stderr, "\t-n\t\t no VSS or VDD to skip.\n");
        (void) fprintf(stderr, "\t-h\t\t prints these lines");
        (void) fprintf(
                stderr,
                "\n\tIf no VDD or VSS nets are given VDD and VSS will be used\n");
        exit(0);
    }
}

int dec_number(char *string) {
    char *s;

    for (s = string; *s != '\0'; s++)
        if (!isdigit(*s)) {
            syntax_error("Expected decimal integer number", string);
        }
    return (int) strtol(string, NULL, 10);
}

void get_signals(struct TYPEterms *TYPEptr) {
    enum states {
        sZERO, sWAIT, sNUM, sEND
    }                 next;
    char              *w;
    char              LocalToken[MAX_TOKEN_LENGTH];
    char              name[MAX_NAME_LENGTH];
    char              tmp[MAX_NAME_LENGTH + 10];
    struct BITstruct  *BITptr;
    struct VECTstruct *VECTptr;
    char              Token;
    int               num;

    w = &(LocalToken[0]);

    BITptr  = TYPEptr->BITs;
    VECTptr = TYPEptr->VECTs;

    while (BITptr->next != NULL) {
        /*	(void)fprintf(stderr,".. b) element used : %s, skip...\n",BITptr->name);
         */
        BITptr = BITptr->next;
    }
    while (VECTptr->next != NULL) {
        /*	(void)fprintf(stderr,".. v) element used : %s,
         * skip...\n",VECTptr->name); */
        VECTptr = VECTptr->next;
    }

    next  = sZERO;
    Token = 1;

    do {
        if (Token)
            get_token(w);
        else
            Token = 1;
        if ((*w == '\n') && (next != sWAIT))
            break;
        if (DEBUG)
            (void) fprintf(stderr, "next = %u\n", next);
        switch (next) {
            case sZERO:
                if (*w == '[') {
                    *(w + strlen(w) - 1) = '\0';
                    (void) sprintf(name, "intrnl%s", w + 1);
                } else {
                    (void) strcpy(name, w);
                }
                if (DEBUG)
                    (void) fprintf(stderr, "element name : %s\n", name);
                next = sWAIT;
                break;
            case sWAIT:
                if (DEBUG)
                    (void) fprintf(stderr, "--sWAIT-- tok <%s>\n", w);
                if (*w == '(') {
                    /* is a vector */
                    next = sNUM;
                } else {
                    /* it was a BIT type */
                    if (DEBUG)
                        (void) fprintf(stderr, "\t\t--> bit, added\n");
                    add_bit(&BITptr, name);
                    Token = 0;
                    next  = sZERO;
                }
                break;
            case sNUM:
                if (DEBUG)
                    (void) fprintf(stderr, "\t\t is a vector, element number :%s\n", w);
                num = dec_number(w);
                add_vect(&VECTptr, name, num, num);
                (void) sprintf(tmp, "%s(%u)", name, num);
                add_bit(&BITptr, tmp);
                next = sEND;
                break;
            case sEND:
                if (*w != ')') {
                    warning("closing ) expected !");
                    Token = 0;
                }
                next = sZERO;
                break;
        }
        if (DEBUG)
            (void) fprintf(stderr, "next = %u\n", next);
    } while (!feof(In));
}

void order_vectors(struct VECTstruct *VECTptr) {
    struct VECTstruct *actual;
    struct VECTstruct *ptr;
    struct VECTstruct *tmp;
    struct VECTstruct *prev;
    int               dir;
    int               last;

    actual = VECTptr;
    while (actual->next != NULL) {
        actual = actual->next;
        last   = actual->start;
        /*	(void)fprintf(stderr,"vector under test : %s\n",actual->name);  */
        dir    = 0;
        prev   = actual;
        ptr    = actual->next;
        while (ptr != NULL) {
            /*	    (void)fprintf(stderr,"analizing element name = %s  number =
             * %u\n",ptr->name,ptr->end); */
            if (!strcmp(ptr->name, actual->name)) {
                /* same name, so ptr belogns to the vector of actual */
                if (ptr->end > actual->end) {
                    actual->end = ptr->end;
                } else {
                    if (ptr->start < actual->start) {
                        actual->start = ptr->start;
                    }
                }
                if (ptr->end > last)
                    dir++;
                else {
                    if (ptr->end == last) {
                        error("Two elements of an i/o vector with the same number not "
                              "allowed!");
                    } else
                        dir--;
                }
                /* release memory */
                tmp = ptr->next;
                /*		(void)fprintf(stderr,"deleting element @%p, actual=%p
                 * ..->next=%p\n",ptr,actual,actual->next); */
                if (ptr == actual->next) {
                    /*		    (void)fprintf(stderr,"updating actual's next"); */
                    actual->next = tmp;
                }
                free((char *) ptr);
                ptr = tmp;
                prev->next = tmp;
            } else {
                prev = ptr;
                ptr  = ptr->next;
            }
        }
        actual->dir = dir > 0;
    }
}

void print_signals(struct MODELstruct *model) {
    struct BITstruct  *BITptr;
    struct VECTstruct *VECTptr;

    BITptr = (model->Inputs)->BITs;
    if (DEBUG)
        (void) fprintf(stderr, "\nINPUTS, BIT:");
    while (BITptr->next != NULL) {
        BITptr = BITptr->next;
        if (DEBUG)
            (void) fprintf(stderr, "\t%s", BITptr->name);
    }

    BITptr = (model->Outputs)->BITs;
    if (DEBUG)
        (void) fprintf(stderr, "\nOUTPUTS, BIT:");
    while (BITptr->next != NULL) {
        BITptr = BITptr->next;
        if (DEBUG)
            (void) fprintf(stderr, "\t%s", BITptr->name);
    }

    VECTptr = (model->Inputs)->VECTs;
    if (DEBUG)
        (void) fprintf(stderr, "\nINPUTS, VECT:");
    while (VECTptr->next != NULL) {
        VECTptr = VECTptr->next;
        if (VECTptr->dir) {
            if (DEBUG)
                fprintf(stderr, "\t%s [%d..%d]", VECTptr->name, VECTptr->start, VECTptr->end);
            else if (DEBUG)
                fprintf(stderr, "\t%s [%d..%d]", VECTptr->name, VECTptr->end, VECTptr->start);
        }
    }

    VECTptr = (model->Outputs)->VECTs;
    if (DEBUG)
        (void) fprintf(stderr, "\nOUTPUTS, VECT:");
    while (VECTptr->next != NULL) {
        VECTptr = VECTptr->next;
        if (VECTptr->dir) {
            if (DEBUG)
                (void) fprintf(stderr, "\t%s [%d..%d]", VECTptr->name, VECTptr->start, VECTptr->end);
            else if (DEBUG)
                (void) fprintf(stderr, "\t%s [%d..%d]", VECTptr->name, VECTptr->end, VECTptr->start);
        }
    }
    if (DEBUG)
        (void) fprintf(stderr, "\n");
}

void print_net(struct instance *cell) {
    struct ports *ptr1;
    struct ports *ptr2;
    int          j;

    while (cell->next != NULL) {
        cell = cell->next;
        if (DEBUG)
            (void) fprintf(stderr, "Inst. connected to name: %s, num pins : %d\n",
                           (cell->what)->name, (cell->what)->npins);
        ptr1   = cell->actuals;
        ptr2   = (cell->what)->formals;
        for (j = 0; j < (cell->what)->npins; j++, ptr1++, ptr2++) {
            if (DEBUG)
                (void) fprintf(stderr, "\tpin %d : %s -> %s\n", j, ptr2->name,
                               ptr1->name);
        }
    }
}

struct instance *get_names(char *name, char type, struct MODELstruct *model) {
    enum states {
        sZERO,
        saWAIT,
        saNUM,
        saEND,
        sEQUAL,
        saNAME,
        sfWAIT,
        sfNUM,
        sfEND,
        sADDINSTANCE
    }                next;
    struct ports     *Fptr;
    struct ports     *Aptr;
    struct instance  *Inst;
    struct cell      *cell;
    struct BITstruct *Bptr;
    char             *w;
    char             LocalToken[MAX_TOKEN_LENGTH];
    char             FORMname[MAX_NAME_LENGTH];
    char             ACTname[MAX_NAME_LENGTH];
    char             tmp[MAX_NAME_LENGTH + 10];
    char             Token;
    int              num;
    int              j;
    int              exit;

    w     = &(LocalToken[0]);
    next  = sZERO;
    Token = 1;
    exit  = 1;

    if (DEBUG)
        (void) fprintf(stderr, "Parsing instance with gate %s\n", name);
    cell = match_gate(name);
    cell->used = 1;
    Inst = new_instance(cell);

    Bptr = (model->Internals)->BITs;
    while (Bptr->next != NULL) {
        Bptr = Bptr->next;
    }

    do {
        if (Token)
            get_token(w);
        else
            Token = 1;
        if (*w == '\n') {
            exit = 0;
            if (next != sADDINSTANCE) {
                warning("--------> Unexpected end of line!");
                next = sADDINSTANCE;
            }
        }
        switch (next) {
            case sZERO:
                if (*w == '[') {
                    *(w + strlen(w) - 1) = '\0';
                    (void) sprintf(FORMname, "intrnl%s", w + 1);
                } else {
                    (void) strcpy(FORMname, w);
                }
                if (DEBUG)
                    (void) fprintf(stderr, "formal element name : %s\n", FORMname);
                next = sfWAIT;
                break;
            case sfWAIT:
                if (*w == '(') {
                    /* is a vector */
                    next = sfNUM;
                } else {
                    /* it was a BIT type */
                    Token = 0;
                    next  = sEQUAL;
                }
                break;
            case sfNUM:
                if (DEBUG)
                    (void) fprintf(stderr, "\t\t is a vector, element number :%s\n", w);
                num = dec_number(w);
                (void) sprintf(tmp, "%s(%u)", FORMname, num);
                (void) strcpy(FORMname, tmp);
                next = sfEND;
                break;
            case sfEND:
                if (*w != ')') {
                    warning("Closing ) expected !");
                    Token = 0;
                }
                next = sEQUAL;
                break;
            case sEQUAL:next = saNAME;
                if (*w != '=') {
                    if (type) {
                        if (DEBUG)
                            (void) fprintf(stderr, "clock signal\n");
                        (void) strcpy(ACTname, FORMname);
                        FORMname[0] = '\0';
                        next = sADDINSTANCE;
                    } else {
                        warning("Expexted '=' !");
                        next = saNAME;
                    }
                    Token = 0;
                }
                break;
            case saNAME:
                if (*w == '[') {
                    *(w + strlen(w) - 1) = '\0';
                    (void) sprintf(ACTname, "intrnl%s", w + 1);
                } else {
                    (void) strcpy(ACTname, w);
                }
                if (DEBUG)
                    (void) fprintf(stderr, "actual element name : %s\n", ACTname);
                next = saWAIT;
                break;
            case saWAIT:
                if (*w == '(') {
                    /* is a vector */
                    next = saNUM;
                } else {
                    /* it was a BIT type */
                    Token = 0;
                    next  = sADDINSTANCE;
                }
                break;
            case saNUM:
                if (DEBUG)
                    (void) fprintf(stderr, "\t\t is a vector, element number :%s\n", w);
                num = dec_number(w);
                (void) sprintf(tmp, "%s(%u)", ACTname, num);
                (void) strcpy(ACTname, tmp);
                next = saEND;
                break;
            case saEND:
                if (*w != ')') {
                    warning("Closing ) expected !");
                    Token = 0;
                }
                next = sADDINSTANCE;
                break;
            case sADDINSTANCE:
                if ((is_here(ACTname, (model->Inputs)->BITs) == NULL) &&
                    (is_here(ACTname, (model->Outputs)->BITs) == NULL) &&
                    (is_here(ACTname, (model->Internals)->BITs) == NULL)) {
                    add_bit(&Bptr, ACTname);
                    if (DEBUG)
                        (void) fprintf(stderr, "\tadded to internals\n");
                }
                Aptr  = Inst->actuals;
                Fptr  = cell->formals;
                j     = 0;
                next  = sZERO;
                Token = 0;
                if (DEBUG)
                    (void) fprintf(stderr, "\tparsed pin : %s, %s\n", FORMname, ACTname);
                if (FORMname[0] != '\0') {
                    while (j < cell->npins) {
                        if (keyword_compare(Fptr->name, FORMname)) {
                            (void) strcpy(Aptr->name, ACTname);
                            break;
                        }
                        Fptr++;
                        Aptr++;
                        j++;
                    }
                } else {
                    Aptr += cell->npins - 1;
                    (void) strcpy(Aptr->name, ACTname);
                    break;
                }
        }
    } while (exit);
    return Inst;
}

void print_vst(struct MODELstruct *model) {
    struct cell       *Cptr;
    struct instance   *Iptr;
    struct ports      *Fptr;
    struct BITstruct  *Bptr;
    struct VECTstruct *Vptr;
    struct ports      *Aptr;
    int               j;
    int               i;
    char              init;

    (void) fprintf(Out, "--[]--------------------------------------[]--\n");
    (void) fprintf(Out, "-- |    File created by Blif2Sls v1.0     | --\n");
    (void) fprintf(Out, "-- |                                      | --\n");
    (void) fprintf(Out, "-- |        by Roberto Rambaldi           | --\n");
    (void) fprintf(Out, "-- |   D.E.I.S. Universita' di Bologna    | --\n");
    (void) fprintf(Out, "--[]--------------------------------------[]--\n\n");

    (void) fprintf(Out, "\n\nENTITY %s IS\n    PORT(\n", model->name);
    Bptr = (model->Inputs)->BITs;
    init = 0;
    while (Bptr->next != NULL) {
        Bptr = Bptr->next;
        if ((Bptr->name)[strlen(Bptr->name) - 1] != ')') {
            if (init)
                (void) fputs(" ;\n", Out);
            else
                init = 1;
            (void) fprintf(Out, "\t%s\t: in BIT", Bptr->name);
        }
    }

    Bptr = (model->Outputs)->BITs;
    while (Bptr->next != NULL) {
        Bptr = Bptr->next;
        if ((Bptr->name)[strlen(Bptr->name) - 1] != ')') {
            if (init)
                (void) fputs(" ;\n", Out);
            else
                init = 1;
            (void) fprintf(Out, "\t%s\t: out BIT", Bptr->name);
        }
    }

    Vptr = (model->Inputs)->VECTs;
    while (Vptr->next != NULL) {
        Vptr     = Vptr->next;
        if (init)
            (void) fputs(" ;\n", Out);
        else
            init = 1;
        if (Vptr->dir)
            (void) fprintf(Out, "\t%s\t: in BIT_VECTOR(%d to %d)", Vptr->name,
                           Vptr->start, Vptr->end);
        else
            (void) fprintf(Out, "\t%s\t: in BIT_VECTOR(%d downto %d)", Vptr->name,
                           Vptr->end, Vptr->start);
    }

    Vptr = (model->Outputs)->VECTs;
    while (Vptr->next != NULL) {
        Vptr     = Vptr->next;
        if (init)
            (void) fputs(" ;\n", Out);
        else
            init = 1;
        if (Vptr->dir)
            (void) fprintf(Out, "\t%s\t: out BIT_VECTOR(%d to %d)", Vptr->name,
                           Vptr->start, Vptr->end);
        else
            (void) fprintf(Out, "\t%s\t: out BIT_VECTOR(%d downto %d)", Vptr->name,
                           Vptr->end, Vptr->start);
    }
    if (ADDPOWER) {
        (void) fprintf(Out, " ;\n\t%s\t : in BIT;\n\t%s\t : in BIT", VSS, VDD);
    }

    (void) fprintf(Out,
                   " );\nEND %s;\n\n\nARCHITECTURE structural_from_SIS OF %s IS\n",
                   model->name, model->name);

    Cptr = LIBRARY;
    while (Cptr->next != NULL) {
        Cptr = Cptr->next;
        if (Cptr->used) {
            (void) fprintf(Out, "  COMPONENT %s\n    PORT (\n", Cptr->name);
            Fptr = Cptr->formals;
            (void) fprintf(Out, "\t%s\t: out BIT", Fptr->name);
            for (j = 1, Fptr++; j < Cptr->npins; j++, Fptr++) {
                (void) fprintf(Out, " ;\n\t%s\t: in BIT", Fptr->name);
            }
            if (ADDPOWER) {
                (void) fprintf(Out, " ;\n\t%s\t: in BIT ;\n\t%s\t: in BIT", VSS, VDD);
            }
            (void) fprintf(Out, " );\n  END COMPONENT;\n\n");
        }
    }

    Bptr = (model->Internals)->BITs;
    while (Bptr->next != NULL) {
        Bptr = Bptr->next;
        (void) fprintf(Out, "SIGNAL %s\t: BIT ;\n", Bptr->name);
    }

    (void) fputs("BEGIN\n", Out);

    j    = 0;
    Iptr = model->Net;
    while (Iptr->next != NULL) {
        Iptr = Iptr->next;
        (void) fprintf(Out, "  inst%d : %s\n  PORT MAP (\n", j, (Iptr->what)->name);
        j++;
        Aptr   = Iptr->actuals;
        Fptr   = (Iptr->what)->formals;
        for (i = 0; i < (Iptr->what)->npins - 1; i++, Aptr++, Fptr++) {
            (void) fprintf(Out, "\t%s => %s,\n", Fptr->name, Aptr->name);
        }
        if (ADDPOWER) {
            (void) fprintf(Out, "\t%s => %s,\n\t%s => %s,\n\t%s => %s);\n\n",
                           Fptr->name, Aptr->name, VSS, VSS, VDD, VDD);
        } else {
            (void) fprintf(Out, "\t%s => %s);\n\n", Fptr->name, Aptr->name);
        }
    }
    (void) fputs("\nEND structural_from_SIS;\n", Out);
}

void parse_file() {
    char               *w;
    char               LocalToken[MAX_TOKEN_LENGTH];
    struct MODELstruct *LocModel;
    struct instance    *Net;

    w        = &(LocalToken[0]);
    LocModel = new_model();

    if (DEBUG)
        (void) fprintf(stderr, "start of parsing\n");
    do {
        get_token(w);
    } while (strcmp(w, ".model") != 0);

    get_token(w);
    (void) strcpy(LocModel->name, w);
    do {
        do
            get_token(w);
        while (*w == '\n');
        if ((!strcmp(w, ".inputs")) || (!strcmp(w, ".clock"))) {
            get_signals(LocModel->Inputs);
        } else {
            if (!strcmp(w, ".outputs")) {
                get_signals(LocModel->Outputs);
            } else
                break;
        }
    } while (!feof(In));

    print_signals(LocModel);

    order_vectors((LocModel->Inputs)->VECTs);
    order_vectors((LocModel->Outputs)->VECTs);

    print_signals(LocModel);

    Net = LocModel->Net;
    do {
        if (!strcmp(w, ".gate")) {
            get_token(w);
            Net->next = get_names(w, 0, LocModel);
            Net = Net->next;
        } else {
            if (!strcmp(w, ".mlatch")) {
                get_token(w);
                Net->next = get_names(w, 1, LocModel);
                Net = Net->next;
            } else {
                if (!strcmp(w, ".end")) {
                    print_net(LocModel->Net);
                    if (DEBUG)
                        (void) fprintf(stderr, "fine");
                    break;
                } else {
                    /* syntax_error("Unknown keyword",w); */
                }
            }
        }
        do
            get_token(w);
        while (*w == '\n');
    } while (!feof(In));
    print_vst(LocModel);
}
