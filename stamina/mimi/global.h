
/*******************************************************************
 *                                                                 *
 *  Global.h   ---- this header file contains all global variables *
 *                   which are used by encoding program.           *
 *                                                                 *
 *  All global variables will be initially declared in main.c.     *
 *  So global.h won't be included in main.c.                       *
 *                                                                 *
 *******************************************************************/

/*************************Global Variables**************************/

STATE **states; /* array of pointers to states. */
EDGE **edges;   /* array of pointers to edges.  */
char b_file[];

int num_pi;      /* number of primary inputs     */
int num_po;      /* number of primary outputs    */
int num_product; /* number of product terms      */
int num_st;      /* number of states             */
/* int code_length;		/* the encoding length. The dufault
                                   value is the minimum encoding
                                   length. User can specify the
                                   encoding length by using the option
                                   -l following an integer      */
