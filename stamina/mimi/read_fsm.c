/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/stamina/mimi/read_fsm.c,v $
 * $Author: pchong $
 * $Revision: 1.2 $
 * $Date: 2005/03/08 01:07:23 $
 *
 */
#include <stdio.h>
#include "user.h"
#include "util.h"
#include "struct.h"
#include "global.h"

char *item[5];
void line_parser();
NLIST **hash_initial();
NLIST *install();
STATE *install_state();
static FILE *fp_temp;

read_fsm(fp_input)
FILE *fp_input;
{
	register i;
	char line[MAXSTRLEN];
	NLIST **state_hash;		/* a hash table stores state name */
	char pstate_name[50];	/* the name of present state */ 
	char nstate_name[50];	/* the name of next state */
	STATE *pstate_ptr;	/* pointer to STATE structure */
	STATE *nstate_ptr;	/* pointer to STATE structure */
	EDGE *edge_ptr;		/* pointer to EDGE structure */
	EDGE *search_edge;
	EDGE *p_star_ptr;
	int p_star = 0;		/* flag indicating p_state is a star */
	int n_star = 0;		/* flag indicating n_state is a star */
	static  int edge_index = 0;	/* a counter when saving edges  */
	static char buffer[] = "/tmp/STMXXXXXX";
	int sfd;

	p_star_ptr=NIL(EDGE);

	/***
	  *** allocate memory for hash_table: "state_hash" with 
       *** size of number of states.
	  *** note that the hashsize = num_st.
	 ***/

	sfd = mkstemp(buffer);
	if(sfd == -1) {
	  panic("cannot open temp file");
	}
	fp_temp = fdopen(sfd, "w+");
	unlink(buffer);
	
	/* write information to the file */

	user.stat.reset = 0;

	while ( fgets(line, MAXSTRLEN, fp_input ) != NULL ) {
		fputs(line,fp_temp);
		line_parser(line);
	    if ((item[0] == NULL ) || (item[0][0] == '#'))
			continue;	/* ignore blank lines */

		if (item[0][0] == '.') {
			switch (item[0][1]) {
			case 's':
				num_st = atoi(item[1]);
				if ( (state_hash = hash_initial(num_st)) == NIL(NLIST *) )  {
					panic("ALLOC hash");
				}
				
				/***
	  			*** allocate memory for **states, and **edges.
	   			***/

				if ( (states = ALLOC(STATE *, num_st)) == NIL(STATE *) )  {
					panic("ALLOC state");
				}

				break;
			case 'r':
	    		if ((strcmp(item[1], "*") != 0) 
					&& (strcmp(item[1],"ANY") != 0)) {
   	    			if (!(install_state(item[1],state_hash,num_st)))
					panic("install fail");
				}
				user.stat.reset = 1;
				break;
			case 'p':
				num_product = atoi(item[1]);
				if ( (edges = ALLOC(EDGE *, num_product )) == NIL(EDGE *) )  {
					panic("ALLOC edge");
				}
				break;
			case 'i':
				num_pi = atoi(item[1]);
				break;
			case 'o':
				num_po = atoi(item[1]);
				break;
			case 'e':
				item[0] = NULL;
			default:
				break;
			}
			if (item[0] == NULL)
				break;
			else
				continue;
		}
		
	    (void) sprintf (pstate_name, "%s", item[1]);
	    (void) sprintf (nstate_name, "%s", item[2]);

	    /*
	     * taking care of "stars": 
	     *                        a "*" either in the present state
	     *                        or in the next state field will be 
 	     *                        considered as a don't care state 
 	     *                        on that transition edge.
	     */

	    if ((strcmp(pstate_name, "*") != 0) 
			&& (strcmp(pstate_name,"ANY") != 0))  {
   	        pstate_ptr = install_state(pstate_name,state_hash,num_st);
			if ( pstate_ptr == NIL(STATE) )  {
		    	panic("failed to install a state.");
			}
	    } else {
			p_star = 1;		/* flag on */
	    }

	    if ((strcmp(nstate_name, "*") != 0 )
			&& (strcmp(nstate_name,"ANY") != 0))  {
	        nstate_ptr = install_state(nstate_name,state_hash,num_st);
			if ( nstate_ptr == NIL(STATE) )  {
		    	panic("failed to install a state");
			}
	    } else {
			n_star = 1;		/* flag on */
	    }

	    /***
	     *** allocate the memory for a EDGE.
	     ***/

	    if ( (edge_ptr = ALLOC (EDGE, 1)) == NIL(EDGE) )  {
			panic("ALLOC edge");
	    }

		edge_ptr->next = NIL(EDGE);
	    edges[edge_index] = edge_ptr;   /* save the address of new edge*/
	    edge_index++;

	    if (!(edge_ptr->input =ALLOC ( char, num_pi + 1 ))) {
			panic("ALLOC input");
	    }

	    (void) strcpy (edge_ptr->input, item[0]);

	    if (!(edge_ptr->output=ALLOC( char, num_po + 1))) {
			panic("ALLOC edge output");
	    }
	    (void) strcpy (edge_ptr->output, item[3]);
		edge_ptr->n_star  = n_star;
		edge_ptr->p_star = p_star;

	    if ( p_star != 0 )  {	/* p_state is a star on this edge */
			edge_ptr->p_state = NIL(STATE); /* a null pointer */
			p_star = 0;		/* reset flag p_star to 0 */
			if (p_star_ptr) {
				search_edge = p_star_ptr;
				while (search_edge->next)
					search_edge=search_edge->next;
				search_edge->next=edge_ptr;
			}
			else
				p_star_ptr = edge_ptr;
			/* I don't need this kind information */
	    } else {
			edge_ptr->p_state = pstate_ptr;
			if (pstate_ptr->edge != NIL(EDGE)) {
				search_edge = pstate_ptr->edge;
				while (search_edge->next != NIL(EDGE))
					search_edge = search_edge->next;
				search_edge->next = edge_ptr;
			}
			else
				pstate_ptr->edge = edge_ptr;
	    }

	    if ( n_star !=0 )  {	/* n_state is a star on this edge */
			edge_ptr->n_state = NIL(STATE); /* a null pointer */
			n_star = 0;		/* reset flag n_star to 0 */
	    } else {
			edge_ptr->n_state = nstate_ptr;
	    }
	} 

	if (access_n_state() != num_st) {
		num_st=access_n_state();
/*
		(void) fprintf(stderr,"Incorrect .s statement %d\n",num_st);
*/
	}
/*
	for (i=0; i<num_st; i++)
		if (!states[i]->edge)
			printf("### Total redundant state\n");
*/
	/* Expand current state don't care */

	while (p_star_ptr) {
		for (i=0; i<num_st; i++) {
			search_edge=states[i]->edge;
			if (!search_edge) {
				if (!(search_edge=ALLOC(EDGE,1)))
					panic("read_fsm2");
				MEMCPY(search_edge,p_star_ptr,sizeof(EDGE));
				search_edge->p_state=states[i];
				search_edge->next = NIL(EDGE);
			}
			else {
				while (search_edge->next)
					search_edge=search_edge->next;
				if (!(search_edge->next=ALLOC(EDGE,1)))
					panic("read_fsm2");
				MEMCPY(search_edge->next,p_star_ptr,sizeof(EDGE));
				search_edge->next->p_state=states[i];
				search_edge->next->next = NIL(EDGE);
			}
		}
		p_star_ptr=p_star_ptr->next;
	}

#ifdef DEBUG
	(void) printf("XXXXXXXXXXXXXXXXXXXXXXX HASH XXXXXXXXXXXXXXXXXXXXXXX\n");
	(void) hash_dump(state_hash, num_st);
	(void) printf("XXXXXXXXXXXXXXXXXXXXXXX HASH XXXXXXXXXXXXXXXXXXXXXXX\n");
#endif

	/***
	 *** dump the states.
	 ***/

#ifdef DEBUG
	(void) printf("XXXXXXXXXXXXXXXXXXXXXXX STATE XXXXXXXXXXXXXXXXXXXXXXX\n");
	(void) dump_states();
	(void) printf("XXXXXXXXXXXXXXXXXXXXXXX STATE XXXXXXXXXXXXXXXXXXXXXXX\n");
#endif

	/***
	 *** dump the edges.
	 ***/

#ifdef DEBUG
	(void) printf("XXXXXXXXXXXXXXXXXXXXXXX EDGE XXXXXXXXXXXXXXXXXXXXXXX\n");
	(void) dump_edges();
	(void) printf("XXXXXXXXXXXXXXXXXXXXXXX EDGE XXXXXXXXXXXXXXXXXXXXXXX\n");
#endif
}

/*
 *   parse input line. 
 */

void
line_parser(line_buf)
register char *line_buf;
{
    register char ch;
	int count;
 
    count = 0;
	while (ch = *line_buf)  {
            /*
             * Note that the value of return character '\n' is 10
             * which is smaller than the value of space ' ',32, .
             * And note that 'x' is a charater, but "x" is a string
             * of single charater.
             * This note is only for my own reference.
             */

	    if (ch <= ' ')  { 
                line_buf++;
                continue;
            }

            item[count] = line_buf;
            count++;

            line_buf++;
        
            while ( (ch = *line_buf) && ch > ' ' )
                line_buf++;
            if (ch != 0)  { 
                *line_buf = '\0';
                 line_buf++;
            }
        }

        item[count] = 0;
}

flushout()
{
	char line[MAXSTRLEN];
	FILE *out;

	if (user.oname)
		out = fopen(user.oname,"w");
	else
		out = stdout;
	fflush(fp_temp);
	rewind(fp_temp);
	while (fgets(line,MAXSTRLEN,fp_temp) != NULL) {
		fprintf(out,"%s",line);
	}
}
