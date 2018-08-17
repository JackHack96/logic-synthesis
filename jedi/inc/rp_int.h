
/*
 * Bill Lin
 * University of California, Berkeley
 * Comments to billlin@ic.Berkeley.EDU
 *
 * Copyright (c) 1989 Bill Lin, UC Berkeley CAD Group
 *     Permission is granted to do anything with this
 *     code except sell it or remove this message.
 */

/*
 * constants
 */
#define TRUE 1
#define FALSE 0
#define BUF_SIZE 100

/*
 * global variables
 */
FILE   *Fin;
FILE   *Fout;
FILE   *Ferr;
int    RangeSmall;
int    Verbose;
double OldCost;
double NewCost;
double StatesBegin;
double StatesEnd;
double Alpha;
double NewStatesPerUnit;
