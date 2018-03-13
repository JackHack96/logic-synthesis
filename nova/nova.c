/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/nova/nova.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:10 $
 *
 */
#include "inc/nova.h"
#include "inc/decls.h"

/************************************************************************
*                                                                       *
*                            MAIN OF NOVA                               *
*                                                                       *
*   ENCODING OF FINITE STATE MACHINES REPRESENTED IN STRUCTURAL FORM    *
*                                                                       *
*                           TIZIANO VILLA                               * 
*  UC BERKELEY - DEPT. OF ELECTRICAL ENGINEERING AND COMPUTER SCIENCES  * 
*                                                                       * 
*                      VERSION 3.2 - January 1990                       *
************************************************************************/


main(argc,argv)
int argc;       /* number of command-lines arguments the program was invoked with */
char *argv[];   /* pointer to an array of character strings that contain the arguments , one per string */

{


    int infile;
    char command[MAXSTRING];
    long elapse_time;

    set_flags();

    infile = options(argc,argv);

    input_fsm(infile);

    if (!L_BOUND && !USER && !I_EXACT && !I_HYBRID  && !IO_HYBRID &&
	!IO_VARIANT && !I_GREEDY && !RANDOM && !ONEHOT && !I_ANNEAL) {
	I_GREEDY = TRUE;
    }

    if (L_BOUND) {      /* computes a lower bound on the # of product-terms
	                   needed to realize the fsm                         */
        lower_bound(); 
        elapse_time = util_cpu_time();
        if (VERBOSE) printf("#\n# \nCPU time = %2.1f seconds\n", elapse_time / 1000.0);
        exit(-1);      
    }

    mini();

    if (USER) { /* substitute user-given codes and analize performance */
	user_codes();
    }

    if (ONEHOT) { /* substitute one-hot codes and analize performance */
	onehot_codes();
    }

    if (I_EXACT) {  /* use exact encoding algorithm */

        if (ISYMB) {
		  if (VERBOSE)
                  printf("\n\nSYMBOLIC INPUTS CODING WITH EXACT ALGORITHM\n\n");

                  exact_graph(&inputnet,inputnum);

                  exact_code(inputnum);

                  exact_check(&inputnet,inputs,inputnum);

        }

        if (VERBOSE)
            printf("\n\nSTATES CODING WITH EXACT ALGORITHM\n\n");

        exact_graph(&statenet,statenum);

        exact_code(statenum);

        exact_check(&statenet,states,statenum);


        exact_rotation();

        exact_output();

    } 

    if (I_HYBRID) {

        if (ISYMB) {
	    if (VERBOSE)
               printf("\n\nSYMBOLIC INPUTS CODING WITH I_HYBRID ALGORITHM\n\n");

            ihybrid_code(&inputnet,inputs,"Inputnet",inputnum,inp_codelength);
	}

        if (VERBOSE) printf("\n\nSTATES CODING WITH I_HYBRID ALGORITHM\n\n");

        ihybrid_code(&statenet,states,"Statenet",statenum,st_codelength);
    }

    if (IO_HYBRID) {

        symbolic_loop();

        if (ISYMB) {
	    if (VERBOSE) 
              printf("\n\nSYMBOLIC INPUTS CODING WITH IO_HYBRID ALGORITHM\n\n");

            ihybrid_code(&inputnet,inputs,"Inputnet",inputnum,inp_codelength);
        } 
 
        if (VERBOSE) printf("\n\nSTATES CODING WITH IO_HYBRID ALGORITHM\n\n");

        iohybrid_code(&statenet,states,"Statenet",statenum,st_codelength);

        sprintf(command, "rm %s %s %s %s %s %s %s %s", temp6, temp7, temp81, temp82, temp83, temp9, temp10, constraints);
        system(command);
    }

    if (IO_VARIANT) {

        symbolic_loop();

        if (ISYMB) {
	    if (VERBOSE)
             printf("\n\nSYMBOLIC INPUTS CODING WITH IO_VARIANT ALGORITHM\n\n");

            ihybrid_code(&inputnet,inputs,"Inputnet",inputnum,inp_codelength);
        } 

        if (VERBOSE) printf("\n\nSTATES CODING WITH IO_VARIANT ALGORITHM\n\n");

        iovariant_code(&statenet,states,"Statenet",statenum,st_codelength);

        sprintf(command, "rm %s %s %s %s %s %s %s %s", temp6, temp7, temp81, temp82, temp83, temp9, temp10, constraints);
        system(command);
    }

    if (I_GREEDY) {  /* use greedy encoding algorithm */

        if (ISYMB) {
	    if (VERBOSE) 
               printf("\n\nSYMBOLIC INPUTS CODING WITH I_GREEDY ALGORITHM\n\n");

            precode(&inputnet);

            link_constr(inputnet,inputs,inputnum);

            log_approx(inputnet,inputs,inputnum,inp_codelength);

        }


        if (VERBOSE) printf("\n\nSTATES CODING WITH I_GREEDY ALGORITHM\n\n");

        precode(&statenet);

        link_constr(statenet,states,statenum);

        log_approx(statenet,states,statenum,st_codelength);


        rotation();

        log_output();

    }        

    if (I_ANNEAL) {
	if (VERBOSE) {
            printf("\nSIM ANNEAL:  cost = %d, ", cost_function);
            printf("moves = %d\n", num_moves);
	}

	if (ISYMB) {
	    if (VERBOSE)
            printf("\n\nSYMBOLIC INPUTS CODING WITH SIM ANNEAL ALGORITHM\n\n");
            anneal_code(inputnet,inputs,inputnum,inp_codelength);
	    if (VERBOSE) show_bestcode(inputs,inputnum);
	    save_in_code(inputs,inputnum);
	}

        if (VERBOSE) printf("\n\nSTATES CODING WITH SIM ANNEAL ALGORITHM\n\n");
	anneal_code(statenet,states,statenum,st_codelength);
	if (VERBOSE) {
	    printf("\n\nCodes returned by sim.-anneal.:\n");
	    show_bestcode(states,statenum);
	}
	save_in_code(states,statenum);

        rotation();
        log_output();

    }

    if (RANDOM) randomization();

    add_terminals(); /* temp5 copied to esp here */
    if (!ONEHOT) { 
        /* temp33 contains the best unminimized cover -
           input of previous version was temp3 my mistake - july 1994 */
        sprintf(command, "sis -o %s -t pla -T blif %s", dc, temp33);
        system(command);
    } else {
	read_script(esp);
        sprintf(command, "sis -f %s -o %s -t pla -T blif %s", readscript, dc, esp);
        system(command);
        sprintf(command, "rm %s", readscript); system(command);
	/* or replace esp by temp3 in the else clause */
    }
    sprintf(command, "rm %s %s %s %s %s %s", temp1, temp2, temp3, temp33, temp4, temp5);
    system(command);

    if (!SIS) nova_summ();

    if (ANALYSIS) ;

    if (SIS) {
	printf("#\n# .start_network\n");
        if (!ONEHOT) { 
	    sprintf(command, "sis -o %s -t pla -T blif %s", blif, esp);
	    system(command);
	} else {
	    sprintf(command, "cp %s %s", dc, blif); system(command);
	    /* or call sis to get blif from esp as in the above else clause */
        }
	nova_blif();
        sprintf(command, "rm %s %s", dc, blif); system(command);
	printf("# .end_network\n");
    }
    if (PLA_OUTPUT) {
	sprintf(command, "cat %s", esp); system(command);
    }
    sprintf(command, "rm %s", esp); system(command);

    elapse_time = util_cpu_time();
    printf("#\n# CPU time = %2.1f seconds\n", elapse_time / 1000.0);

    exit(0);

}         



/******************************************************************************
*             Initialization of global variables                              *
******************************************************************************/

set_flags()

{

    LIST          = TRUE;
    DEBUG         = FALSE;
    NAMES         = FALSE;
    ISYMB         = FALSE;
    OSYMB         = FALSE;
    SHORT         = TRUE;
    PRTALL        = FALSE; 
    TYPEFR        = TRUE;
    IBITS_DEF     = TRUE;
    SBITS_DEF     = TRUE;
    DCARE         = FALSE;
    L_BOUND       = FALSE;
    RANDOM        = FALSE;
    USER          = FALSE;
    ONEHOT        = FALSE;
    I_ANNEAL      = FALSE;
    ANALYSIS      = FALSE;
    I_GREEDY      = FALSE;
    I_HYBRID      = FALSE;
    IO_HYBRID     = FALSE;
    IO_VARIANT    = FALSE;
    I_EXACT       = FALSE;
    COMPLEMENT    = FALSE; 
    POW2CONSTR    = FALSE;
    OUT_ALL       = FALSE;
    OUT_ONLY      = FALSE;
    ZERO_FL       = FALSE;
    INIT_FLAG     = FALSE;
    ILB           = FALSE;
    OB            = FALSE;
    SIS           = TRUE;
    VERBOSE       = FALSE;
    PLA_OUTPUT    = FALSE;

    cost_function = 0;
    num_moves = 10;
    rand_trials = -1;

}



/*******************************************************************************
*               Routine for dealing with processor time reporting              *
*******************************************************************************/

#include <sys/times.h>

/*
    p_time -- return a floating point number which represents the
    elapsed processor time in seconds since the program started
*/

double p_time()

{
    struct tms buffer;
    double time;

    times(&buffer);
    time = (buffer.tms_utime + buffer.tms_stime) / 60.0 ;
    return( time );
}



temp_files()

{

    sprintf(temp1, "%s.temp1", file_name);
    sprintf(temp2, "%s.temp2", file_name);
    sprintf(temp3, "%s.temp3", file_name);
    sprintf(temp33, "%s.temp33", file_name);
    sprintf(temp4, "%s.temp4", file_name);
    sprintf(temp5, "%s.temp5", file_name);
    sprintf(temp6, "%s.temp6", file_name);
    sprintf(temp7, "%s.temp7", file_name);
    sprintf(temp81, "%s.temp81", file_name);
    sprintf(temp82, "%s.temp82", file_name);
    sprintf(temp83, "%s.temp83", file_name);
    sprintf(temp9, "%s.temp9", file_name);
    sprintf(temp10, "%s.temp10", file_name);
    sprintf(constraints, "%s.constraints", file_name);
    sprintf(summ, "%s.summ", file_name);
    sprintf(esp, "%s.esp", file_name);
    sprintf(readscript, "%s.readscript", file_name);
    sprintf(dc, "%s.dc", file_name);
    sprintf(blif, "%s.blif", file_name);

}



add_terminals()

{

    FILE *sfile,*dfile,*fopen();
    char line[MAXSTRING],linex[MAXSTRING],linexx[MAXSTRING];
    int i,len;

    line[0] = '\0'; linex[0] = '\0'; linexx[0] = '\0';

    if ((sfile = fopen(temp5,"r")) == NULL) {
	fprintf(stderr,"Error in opening temp5 in add_terminals\n");
	exit(-1);
    } else {
        if ((dfile = fopen(esp,"w")) == NULL) {
	    fprintf(stderr,"Error in opening esp in add_terminals\n");
	    exit(-1);
	} else {
	    while (fgets(line,sizeof(line),sfile)) {
		fputs(line,dfile);
		if (myindex(line,".p ") >= 0) {
	            if (ILB) {
                        len = strlen(input_names);
			/* notice: I copy len-1 chars because of char newline */
		        strncpy(line,input_names,len-1);
			line[len-1] = '\0';
		        for (i = 0; i < st_codelength; i++) {
		            sprintf(linex," st%d*",i);
		            strcat(line,linex);
		        }
		        fputs(line,dfile);
	                fputs("\n",dfile);
                    }
	            if (OB) {
		        sprintf(line,".ob st%d*",0);
		        for (i = 1; i < st_codelength; i++) {
		            sprintf(linex," st%d*",i);
		            strcat(line,linex);
		        }
                        len = strlen(output_names); 
			/* notice: I copy len-1 chars because of char newline */
		        for (i = 3; i < len-1; i++)  
		            linexx[i-3] = output_names[i]; 
			linexx[len-4] = '\0';
		        strcat(line,linexx);
		        fputs(line,dfile);
	                fputs("\n",dfile);
	            }
		}
	    }
            fclose(dfile);
	}
	fclose(sfile);
    }

}



read_script(sfile)
char *sfile;

{
    FILE *readscriptfile,*fopen();
    char string1[MAXSTRING];

    if ((readscriptfile = fopen(readscript,"w")) == NULL) {
        fprintf(stderr,"Error in opening readscript in read_script\n");
        exit(-1);
    } else {
	sprintf(string1,"read_pla -s %s\n", sfile);
	fputs(string1, readscriptfile);
        fclose(readscriptfile);
    }

}



nova_blif()

{

    FILE *blifile,*sfile,*fopen();
    char line[MAXSTRING],i_line[MAXSTRING],idc_line[MAXSTRING],o_line[MAXSTRING],
         s_line[2*MAXSTRING],l_line[MAXSTRING],w_line[2*MAXSTRING],
	 lorder_line[2*MAXSTRING],string[2];
    char *tokptr, *strptr;
    int i,j,input_length,input_max,output_max;
    BOOLEAN found=FALSE;

    if (!INIT_FLAG) strcpy(init_state,states[0].name);
    /*printf("init_state = %s\n", init_state);*/

    if (I_GREEDY || I_ANNEAL || RANDOM) {
        for (i = 0; i < statenum; i++) {
	    if (strcmp(states[i].name,init_state) == 0) {
	        strcpy(init_code,states[i].best_code);
                break;
	    }
        }
    }
    if (I_EXACT || I_HYBRID || IO_HYBRID || IO_VARIANT || ONEHOT || USER) {
        for (i = 0; i < statenum; i++) {
	    if (strcmp(states[i].name,init_state) == 0) {
	        strcpy(init_code,states[i].exbest_code);
                break;
	    }
        }
    } 
    if (ONEHOT) {
	for (i = 0; i < statenum; i++) {
	    if (init_code[i] == DASH) init_code[i] = ZERO;
	}
    }
    /*printf("init_code = %s\n", init_code);*/ 
    line[0] = '\0'; i_line[0] = '\0'; idc_line[0] = '\0'; o_line[0] = '\0'; s_line[0] = '\0';
    l_line[0] = '\0'; lorder_line[0] = '\0';

    if (ISYMB) {
	input_length = inp_codelength;
    } else {
	input_length = inputfield;
    }
    input_max = 1+ input_length + st_codelength;
    output_max = 1+ st_codelength + outputfield -1;

    if ((blifile = fopen(blif,"r")) == NULL) {
	fprintf(stderr,"Error in opening blif in nova_blif\n");
	exit(-1);
    } else {
        while (fgets(line,sizeof(line),blifile)) {

	    if (myindex(line,".inputs") >= 0) {
	        strptr = line;
		i = 1;
		while ( (tokptr = strtok(strptr," ")) != (char *) 0 ) {
		    if ( strchr (tokptr,'\\') != (char *) 0 ) {
		        if (i <= input_length +1) {
			    strncat(i_line,tokptr,strlen(tokptr)-1);
		            printf("%s\n", i_line);
			    i_line[0] = '\0';
		        }
                        fgets(line,sizeof(line),blifile);
			strptr = line;
			continue;
	            }
		    if (i == 1) {
			strcat(i_line,tokptr);
			strcat(i_line," ");
		    }
		    if (i > 1 && i < input_length+1) {
			strcat(i_line,tokptr);
			strcat(i_line," ");
		    }
		    if (i == input_length +1) {
			strcat(i_line,tokptr);
		        printf("%s\n", i_line);
		    }
		    if (i > input_length+1 && i < input_max) {
			strcat(s_line,tokptr);
			strcat(s_line," ");
		    }
		    if (i == input_max) {
			strncat(s_line,tokptr,strlen(tokptr)-1);
			strcat(s_line," ");
		    }
		    strptr = (char *) 0;
		    i++;
		}
                /* format input line for exdc network */
                /* notice: in a previous edition pi's were intentionally
                   left out of the .inputs line - and the comment was:  
                   format input line for exdc network with no pi's -
                   it seems an obvious error of unclear motivation
                   (fixed in sept. 1993)                       */
                strcpy(idc_line, i_line); strcat(idc_line," "); strcat(idc_line, s_line);
                strcat(idc_line, "\n");
		/*printf("%s\n", s_line);*/
	    } 

	    else  if (myindex(line,".outputs") >= 0) {
	        strptr = line;
		i = 1;
		while ( (tokptr = strtok(strptr," ")) != (char *) 0 ) {
		    if ( strchr (tokptr,'\\') != (char *) 0 ) {
		        if (i > st_codelength +1) {
			    strncat(o_line,tokptr,strlen(tokptr)-1);
		            printf("%s\n", o_line);
			    o_line[0] = '\0';
		        }
                        fgets(line,sizeof(line),blifile);
			strptr = line;
			continue;
	            }
		    if (i == 1) {
			strcat(o_line,tokptr);
			strcat(o_line," ");
		    }
		    if (i > 1 && i < st_codelength+1) {
			strcat(s_line,tokptr);
			strcat(s_line," ");
		    }
		    if (i == st_codelength +1) {
			strcat(s_line,tokptr);
		    }
		    if (i > st_codelength+1 && i < output_max) {
			strcat(o_line,tokptr);
			strcat(o_line," ");
		    }
		    if (i == output_max) {
			strncat(o_line,tokptr,strlen(tokptr)-1);
		        printf("%s\n", o_line);
		    }
		    strptr = (char *) 0;
		    i++;
		}
		/*printf("%s\n", s_line);*/

		strcpy(lorder_line,".latch_order");
		for (j = 1; j <= st_codelength; j++) {
                    strcpy(l_line,".latch");
		    strcpy(w_line,s_line);
	            strptr = w_line;
		    i = 1;
		    while ( (tokptr = strtok(strptr," ")) != (char *) 0 ) {
		        if (i == st_codelength+j) {
			    strcat(l_line," ");
			    strcat(l_line,tokptr);
		        }
		        strptr = (char *) 0;
		        i++;
		    }
		    strcpy(w_line,s_line);
	            strptr = w_line;
		    i = 1;
		    while ( (tokptr = strtok(strptr," ")) != (char *) 0 ) {
		        if (i == j) {
			    strcat(l_line," ");
			    strcat(l_line,tokptr);
			    strcat(lorder_line," ");
			    strcat(lorder_line,tokptr);
		        }
		        strptr = (char *) 0;
		        i++;
		    }
		    strcat(l_line," ");
	            string[0] = init_code[j-1]; string[1] = '\0';
		    strcat(l_line, string);
		    printf("%s\n", l_line);
		}
		printf("%s\n", lorder_line);

	    } 

	    else  if (myindex(line,".model") >= 0) {
		/* skip the .model line because outputted at the start */
	    }

            /* clause that outputs the dc set of the encoded fsm */
	    else if ((!ONEHOT && myindex(line,".end")  >= 0) ||
		      (ONEHOT && myindex(line,".exdc") >= 0)) {
                if ((sfile = fopen(dc,"r")) == NULL) {
	            fprintf(stderr,"Error in opening dc in nova_blif\n");
	            exit(-1);
                } else {
	            while (fgets(line,sizeof(line),sfile)) {
			/* the external dc net starts here */
		        if (myindex(line,".exdc") >= 0) {
		            found = TRUE;
		        }
		        if (found == TRUE) {
			    /* hack to avoid printing 1 empty line after .end */
		            if (myindex(line,".end") >= 0) {
				printf("%s", line);
                                break;
			    }
			    /* don't output .model twice and output idc_line as input
                               line (has no pi's) instead than current input line */
		            if ( (myindex(line,".model") < 0)  
                                 && (myindex(line,".inputs") < 0) ) printf("%s", line);
		            if (myindex(line,".inputs") >= 0) printf("%s", idc_line);
			}
	            }
	            fclose(sfile);
                }
		fclose(blifile); 
		return;
	    }

	    else printf("%s", line);

	}
        fclose(blifile);
    }

}



getname(name)
char *name;

{

    int i;

    i = 0;
    while(name[i] != '\0') {
	if (name[i] != '.') {
	    sh_filename[i] = name[i];
	} else break;
	i++;
    }
    sh_filename[i] = '\0';
    /*printf("sh_filename = %s\n", sh_filename);*/

}
