/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/stamina/mimi/global.h,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:15 $
 *
 */
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

extern STATE **states;		/* array of pointers to states. */
extern EDGE  **edges;		/* array of pointers to edges.  */
extern char b_file[];

extern int num_pi;		/* number of primary inputs     */
extern int num_po;		/* number of primary outputs    */
extern int num_product; 	/* number of product terms      */
extern int num_st;		/* number of states             */
/* extern int code_length;		/* the encoding length. The dufault 
				   value is the minimum encoding 
				   length. User can specify the 
				   encoding length by using the option
				   -l following an integer      */
