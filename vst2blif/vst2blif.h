#ifndef SIS_VST2BLIF_H
#define SIS_VST2BLIF_H

#define MAXTOKENLEN 256
#define MAXNAMELEN 32

/*
 * These are the test used in the tokenizer, please refer
 * to the graph included in the documentation
 */
enum TOKEN_STATES {
    tZERO, tTOKEN, tREM1, tREM2, tSTRING, tEOF
};

/*
 * Following are the structures containing all the info extracted from
 * the library (all the pins, the inputs, the outputs and, if needed, the clock)
 */

struct PortName {
    char            name[MAXNAMELEN];
    int             notused;  // this flag is used to check if a formal terminal has been already connected
    struct PortName *next;
};

struct Ports {
    char name[MAXNAMELEN];
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
    char             name[MAXNAMELEN + 10];
    char             dir;
    struct BITstruct *next;
};


struct SIGstruct {
    char             name[MAXNAMELEN];
    char             dir;
    int              start;
    int              end;
    struct SIGstruct *next;
};


struct TERMstruct {
    char name[MAXNAMELEN];
    int  start;
    int  end;
};

struct ENTITYstruct {
    char             name[MAXNAMELEN];
    struct Cell      *Components;
    struct Cell      *EntityPort;
    struct SIGstruct *Internals;
    struct Instance  *Net;
};

#endif //SIS_VST2BLIF_H
