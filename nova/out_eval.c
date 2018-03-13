/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/nova/out_eval.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:10 $
 *
 */
#include "inc/nova.h"

out_performance(label)
char *label;

{

     ORDER_RELATION *curptr;
     int i,cover_count,ycover_count;
     int codei_covers_codej();

     cover_count = ycover_count = 0;

     for ( i = 0; i < statenum; i++) {
         for ( curptr = order_graph[i] ; curptr != (ORDER_RELATION *) 0;
	                                 curptr = curptr->next  ) {
             cover_count++;
	     if (strcmp(label,"exact") == 0) {
               if (codei_covers_codej(states[i].exact_code,states[curptr->index].exact_code) == 1) {
                   ycover_count++;
               }
	     }
	     if (strcmp(label,"exbest") == 0) {
               if (codei_covers_codej(states[i].exbest_code,states[curptr->index].exbest_code) == 1) {
                   ycover_count++;
               } 
	     }
         }
     }
     printf("\n\nOutput constraints satisfaction = %d\n", ycover_count);
     printf("Output constraints unsatisfaction = %d\n", cover_count - ycover_count);

}



weighted_outperf(net_num,label)
int net_num;
char *label;

{

     BOOLEAN ROW;
     int i,j,row_count,weighted_gain,weighted_loss;

     weighted_gain = 0;
     weighted_loss = 0;
     for (i = 0; i < net_num; i++) {
	 ROW = TRUE;
	 row_count = 0;
	 for (j = 0; j < net_num; j++) {
	     if (order_array[i][j] == ONE) {
		 row_count++;
		 if (strcmp(label,"exact") == 0) {
                   if (codei_covers_codej(states[j].exact_code,states[i].exact_code) != 1) {
	               ROW = FALSE;
		   }
		 }
		 if (strcmp(label,"exbest") == 0) {
                   if (codei_covers_codej(states[j].exbest_code,states[i].exbest_code) != 1) {
	               ROW = FALSE;
		   }
		 }
             }
	 }
	 if (ROW == TRUE && row_count > 0) {
	     weighted_gain += gain_array[i];
	 } 
	 if (ROW == FALSE && row_count > 0) {
	     weighted_loss += gain_array[i];
	 }
     }
     printf("Weighted_gain = %d\n", weighted_gain);
     printf("Weighted_loss = %d\n", weighted_loss);

}



int codei_covers_codej(p,s)  /* returns 1 iff set p includes properly set s */
char *p;
char *s;

{

    int j;
    BOOLEAN cover;

    cover = FALSE;

    for (j=0; p[j] != '\0'; j++) {
        if (p[j] == ONE  && s[j] == ZERO) cover = TRUE;
        if (p[j] == ZERO  && s[j] == ONE) {
	    cover = FALSE;
            return(cover);
        } 
    }

    return(cover);

}



int symbmin_card()

{

     FILE *fopen(), *fpin;
     char line[MAXLINE], string[MAXLINE];
     int symb_card;

     if ((fpin = fopen(temp10, "r")) == NULL) {
	 fprintf(stderr,"fopen: can't read file temp10\n");
	 exit(-1);
     }
     while (fgets(line, MAXLINE, fpin) != NULL) {
         if (myindex(line,".p") >= 0) {
	     sscanf(line, "%s %d", string, &symb_card);
             break;
         }
     }
     fclose(fpin);

     symbmin_products = symb_card;

     return(symb_card);

}
