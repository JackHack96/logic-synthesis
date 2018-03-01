/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sred/main.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:11 $
 *
 */
#define define_extern
#include "reductio.h"


main(argc,argv)
int argc;
char **argv;

{
   /*         INCOMPLETELY SPECIFIED
        FINITE STATE MACHINES DECOMPOSITION

                 Tiziano Villa
          CSELT Labs - Torino (Italy)

		  VERSION 1.0
                                                */

   procargs (argc, argv);

   yyparse();
   if (errorcount) exit (1);
  
   label();
 
   if (isymb  && !osymb) obincompa(); 
   if (isymb  &&  osymb)  incompat();  
   if (!isymb && !osymb) iobincomp();  
   if (!isymb &&  osymb) ibincompa();  

   if (! strcmp (coloring_algo, "fast")) {
	   fast_coloring();
   }
   else {
       espresso_coloring ();
   }

   mxcomptbl();

   solution();

   if (do_print_classes) {
	   print_classes ();
   }
   reduced();

   makeout(); 

   exit (0);
}


