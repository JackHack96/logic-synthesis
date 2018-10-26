
#include "nova.h"

/***********************************************************************
*                   Auxiliary routines                                 *
***********************************************************************/


myindex(s,t)   /* returns index of t in s, -1 if none */
char s[];
char t[];

{
    int i,j,k;
    
    for (i=0; s[i] != '\0'; i++) {
        for (j=i, k=0; t[k]!='\0' && s[j]==t[k]; j++,k++)
             ;
        if (t[k] == '\0')
             return(i);
    }
    return(-1);
}


/* computes the upper ceiling of the log base 2 of its argument */
mylog2(arg)   
int arg;

{
    int i, value;

    for (i=1, value=0; i < arg ; i = i << 1 , value++) ;

    return(value);
  
}


/* computes the upper ceiling of the log base 2 of its argument 
   if arg is <= 1, it is defined as 1                              */
special_log2(arg)   
int arg;

{
    int i, value;

    if (arg <= 1) value = 1;

    if (arg > 1) {
	for (i=1, value=0; i < arg ; i = i << 1 , value++) ;
    }

    return(value);
  
}


minpow2(arg)     /* computes the minimum power of 2 >= than its argument */
int arg;

{
    int value;

    for (value=1; value < arg ; value = value << 1) ;

    return(value);
  
}


/* computes the hamming-distance between the cubes "arg1" and "arg2" 
   of "length" size */
hamm_dist(arg1,arg2,length)
char *arg1;       
char *arg2;
int length;

{
    int i,distance;

    distance = 0;

    for ( i=0; i < length; i++) {
        if ( ((arg1[i] == ONE) && (arg2[i] == ZERO)) ||
             ((arg1[i] == ZERO) && (arg2[i] == ONE)) ) {
            distance++;
        }
    }

    return(distance);

}


perimeter(dim)  /* computes the perimeter of an hyperface of dimension "dim" */
int dim;

{
    int i,power;

    power = 1;

    for (i = 0; i < dim-1; i++) {
	power = power * 2;
    }

    return( dim * power );
}


myatoi(s)       /* converts s to integer */
char s[];

{
    int i,n;

    for (i=0; s[i]==' ' || s[i]=='\n' || s[i]=='\t' || s[i]=='.' ||
	      s[i]=='i' || s[i]=='o' || s[i]=='p'; i++) {
	;  /* skips white space and the keys '.i' , '.o' , '.p' */
    }

    for (n = 0; s[i] >= '0' && s[i] <= '9'; i++) {
	n = 10 * n + s[i] - '0';
    }

    return(n);
}



size()

{
    FILE *fp, *fopen();
    char line[MAXLINE], *fgets();
    int min_size;
    /* note : min_inputs,min_ouputs,min_products have been defined as global 
       variables */

    if ( (fp = fopen(temp4,"r")) == NULL ) {
	fprintf(stderr,"size: can't open temp4\n");
	exit(-1);
    } else {
	  /* skips the first line ( comment ) 
          fgets(line,MAXLINE,fp );*/ 

	  if ( fgets(line,MAXLINE,fp ) != (char *) 0  &&
	       myindex(line,".i") != -1 ) {
              min_inputs = myatoi(line);
	  }
	  if (VERBOSE) printf("\nmin_inputs = %d\n",min_inputs);

	  if ( fgets(line,MAXLINE,fp ) != (char *) 0  &&
	       myindex(line,".o") != -1 ) {
              min_outputs = myatoi(line);
	  }
	  if (VERBOSE) printf("min_outputs = %d\n",min_outputs);

	  if ( fgets(line,MAXLINE,fp ) != (char *) 0  &&
	       myindex(line,".p") != -1 ) {
              min_products = myatoi(line);
	  }
	  if (VERBOSE) printf("min_products = %d\n",min_products);
    }

    fclose(fp);

    min_size = ( 2*min_inputs + min_outputs ) * min_products;

    if (VERBOSE) printf("min_size = %d\n", min_size);

    return (min_size);

}


/* converts the integer "value" into the binary coded string "binary"
   of length "length" */
itob(binary,value,length)  
char *binary;
int value;
int length;

{

    int i;

    for (i = length - 1; i >= 0; i-- ) {

	if ( (value % 2) == 1 ) {
	    binary[i] = ONE;
	} else {
	    binary[i] = ZERO;
	}

	value = value / 2; 

    }

    binary[length] = '\0';

}


/* converts the binary coded string "binary" of length "length"
   in the integer "value" */
btoi(binary,length)  
char *binary;
int length;

{

    int i,value;

    value = 0;
    for (i = length - 1; i >= 0; i-- ) {

	if (binary[i] == ONE) {
	    value = value + power(2,length - 1 - i);
	}

    }

    return(value);

}


/* replaces all characters from a pound sign to end of line with blanks */
comment(string,length)
char *string;
int length;

{
    int i,j;

    for (i = 0; i < length; i++) {
	if (string[i] == '#') {
	    for (j = i; j < length; j++) {
		string[j] = ' ';
	    }
	    break;
	}
    }
}



/* replaces tabs with blanks */
tabs(string,length)
char *string;
int length;

{
    int i;

    for (i = 0; i < length; i++) {
	if (string[i] == '\t') {
	    string[i] = ' ';
	}
    }
}



/* removes trailing blanks and tabs */
trailblanks(string,length)
char *string;
int length;

{
    int i;

    i = length;

    while (--i >= 0) {
	if (string[i] != ' ' && string[i] != '\t' && string[i] != '\n') {
	    break;
	}
	string[i+1] = '\0';
    }

}



p_incl_s(p,s)  /* returns 1 (otherwise 0) iff set p includes set s -
		  it does not check whether the inclusion is proper  */
char *p;
char *s;

{

    int j;

    for (j=0; p[j] != '\0'; j++) {
        if (p[j] == ZERO  && s[j] == ONE) {
            return(0);
        } 
    }

    return(1);

}



face1_incl_face2(face1,face2,dim)  /* returns 1 (otherwise 0) iff face1 includes
				      properly face2                          */
char *face1;
char *face2;
int dim;

{

    int j;
    BOOLEAN inclusion,proper;

    inclusion = TRUE;
    proper = FALSE;

    for (j=0; j<dim; j++) {

        if (face1[j] == ZERO && face2[j] == ZERO) {
        } 
        if (face1[j] == ZERO && face2[j] == ONE) {
            inclusion = FALSE;
	    break;
        } 
        if (face1[j] == ZERO && face2[j] == 'x') {
            inclusion = FALSE;
	    break;
	}
        if (face1[j] == ONE && face2[j] == ZERO) {
            inclusion = FALSE;
	    break;
        } 
        if (face1[j] == ONE && face2[j] == ONE) {
        } 
        if (face1[j] == ONE && face2[j] == 'x') {
            inclusion = FALSE;
	    break;
	}
        if (face1[j] == 'x' && face2[j] == ZERO) {
	    proper = TRUE;
	}
        if (face1[j] == 'x' && face2[j] == ONE) {
	    proper = TRUE;
	}
        if (face1[j] == 'x' && face2[j] == 'x') {
	}

    }

    return( inclusion && proper );

}



face1_incl2_face2(face1,face2,dim) /* returns 1 (otherwise 0) iff face1 includes
				      (also not properly) face2 */
char *face1;
char *face2;
int dim;

{

    int j;
    BOOLEAN inclusion;

    inclusion = TRUE;

    for (j=0; j<dim; j++) {

        if (face1[j] == ZERO && face2[j] == ZERO) {
        } 
        if (face1[j] == ZERO && face2[j] == ONE) {
            inclusion = FALSE;
	    break;
        } 
        if (face1[j] == ZERO && face2[j] == 'x') {
            inclusion = FALSE;
	    break;
	}
        if (face1[j] == ONE && face2[j] == ZERO) {
            inclusion = FALSE;
	    break;
        } 
        if (face1[j] == ONE && face2[j] == ONE) {
        } 
        if (face1[j] == ONE && face2[j] == 'x') {
            inclusion = FALSE;
	    break;
	}
        if (face1[j] == 'x' && face2[j] == ZERO) {
	}
        if (face1[j] == 'x' && face2[j] == ONE) {
	}
        if (face1[j] == 'x' && face2[j] == 'x') {
	}

    }

    return( inclusion );
}


power(x,n)       /* raise x to n-th power; n > 0 */
int x,n;

{ 
    int i,p;

    p = 1;
    for (i=1; i<=n; ++i) 
	p = p * x;
    return(p);
}



char *a_inter_b(a,b,constr_card,dim) /* returns the intersection of faces a and
                                        b ; checks also if it has dimension 
					large enough to contain constr->card  */
char *a;
char *b;
int constr_card;
int dim;

{

    int j,inter_dim;
    char *new_code;

    if ( (new_code = (char *) calloc((unsigned)dim+1,sizeof(char))) == (char *) 0) {
        fprintf(stderr,"Insufficient memory for new_code in a_inter_b");
        exit(-1);
    }

    new_code[dim] = '\0';

    inter_dim = 1;

    for (j=0; j<dim; j++) {

        if (a[j] == ZERO && b[j] == ZERO) {
            new_code[j] = ZERO;
        } 
        if (a[j] == ZERO && b[j] == ONE) {
            free_mem(new_code);
            new_code = (char *) 0;
	    break;
        } 
        if (a[j] == ZERO && b[j] == 'x') {
            new_code[j] = ZERO;
	}
        if (a[j] == ONE && b[j] == ZERO) {
            free_mem(new_code);
            new_code = (char *) 0;
	    break;
        } 
        if (a[j] == ONE && b[j] == ONE) {
            new_code[j] = ONE;
        } 
        if (a[j] == ONE && b[j] == 'x') {
            new_code[j] = ONE;
	}
        if (a[j] == 'x' && b[j] == ZERO) {
            new_code[j] = ZERO;
	}
        if (a[j] == 'x' && b[j] == ONE) {
            new_code[j] = ONE;
	}
        if (a[j] == 'x' && b[j] == 'x') {
            new_code[j] = 'x';
	    inter_dim = 2 * inter_dim;
	}

    }


    /* checks if the dimension of the intersection between a and b is larger
       (i.e. can contain) than a constraint of cardinality "constr_card"    */
    if ( inter_dim < constr_card ) {
       free_mem(new_code);
       new_code = (char *) 0;
    }

    return(new_code);

}



int dim_fathers_inter(a,b,dim)     /* returns the dimension of the intersection
                                      of faces a and b */
char *a;
char *b;
int dim;

{

    int j,inter_dim;

    inter_dim = 0;

    for (j = 0; j < dim; j++) {

        if (a[j] == ZERO && b[j] == ONE) {
            inter_dim = -1;
	    break;
        } 
        if (a[j] == ONE && b[j] == ZERO) {
            inter_dim = -1;
	    break;
        } 
        if (a[j] == 'x' && b[j] == 'x') {
	    inter_dim = inter_dim + 1;
	}

    }

    return(inter_dim);

}



int dim_face(a,dim)     /* returns the dimension of face a */
char *a;
int dim;

{

    int j,inter_dim;

    inter_dim = 0;

    for (j = 0; j < dim; j++) {

        if (a[j] == 'x') {
	    inter_dim = inter_dim + 1;
	}

    }

    return(inter_dim);

}



char *c1_inter_c2(a,b,dim) /* returns the intersection of constraints a and b */
char *a;
char *b;
int dim;

{

    int j,num_ones;
    char *new_code;

    if ( (new_code = (char *) calloc((unsigned)dim+1,sizeof(char))) == (char *) 0) {
        fprintf(stderr,"Insufficient memory for new_code in c1_inter_c2");
        exit(-1);
    }

    new_code[dim] = '\0';

    num_ones = 0;

    for (j=0; j<dim; j++) {

        if (a[j] == ZERO && b[j] == ZERO) {
            new_code[j] = ZERO;
        } 
        if (a[j] == ZERO && b[j] == ONE) {
            new_code[j] = ZERO;
        } 
        if (a[j] == ONE && b[j] == ZERO) {
            new_code[j] = ZERO;
        } 
        if (a[j] == ONE && b[j] == ONE) {
            new_code[j] = ONE;
	    num_ones++;
        } 

    }

    if ( num_ones >= 1 ) {
	return(new_code);
    } else {
        free_mem(new_code);
	return( (char *) 0 );
    }

}



int card_c1c2(a,b,dim) /* returns #(intersection of constraints a and b) */
char *a;
char *b;
int dim;

{

    int j,num_ones;

    num_ones = 0;

    for (j=0; j<dim; j++) {

        if (a[j] == ONE && b[j] == ONE) {
	    num_ones++;
        } 

    }

    return(num_ones);

}



int card_c1(a,dim) /* returns #(constraint a) */
char *a;
int dim;

{

    int j,num_ones;

    num_ones = 0;

    for (j=0; j<dim; j++) {

        if (a[j] == ONE) {
	    num_ones++;
        } 

    }

    return(num_ones);

}



/* ************************************************************************** */
/*                ADDED FOR THE SYMBOLIC MINIMIZATION LOOP                    */
/* ************************************************************************** */

dfs(old_source,new_source)
int old_source;
int new_source;

{
    ORDER_RELATION *curptr;
    int path_is_new();

    reached[new_source] = 1;

    for ( curptr = order_graph[new_source]; curptr != (ORDER_RELATION *) 0 ;
					    curptr = curptr->next )        {
	if ( reached[curptr->index] == 0 ) {
	    if (path_is_new(old_source,curptr->index) == 0) {
                new_order_path(old_source,curptr->index);
	    }
            dfs(old_source,curptr->index);
	}
    }

}



relation_is_new(row,column)
int row;
int column;

{
    ORDER_RELATION *curptr;

    for ( curptr = order_graph[row]; curptr != (ORDER_RELATION *) 0 ;
					    curptr = curptr->next )        {
	if ( curptr->index == column ) {
            return(1);
	}
    }
    return(0);

}



path_is_new(row,column)
int row;
int column;

{
    ORDER_PATH *curptr;

    for ( curptr = path_graph[row]; curptr != (ORDER_PATH *) 0 ;
					    curptr = curptr->next )        {
	if ( curptr->dest == column ) {
            return(1);
	}
    }
    return(0);

}



/******************************************************************************
*                    Copies string t[0,statenum-1] to string s                *
******************************************************************************/

strcar(s,t)
char s[],t[];

{
    int i;

    for (i = 0; i < statenum; i++) {
	s[i] = t[i];
    }
    s[statenum] = '\0';

}




/******************************************************************************
*                    s[index] = '0' , otherwise s[i] = '~'                    *
******************************************************************************/

stroff(s,index)
char s[];
int index;

{
    int i;

    for (i = 0; i < statenum; i++) {
	s[i] = '0';
    }
    s[index] = '0';
    s[statenum] = '\0';

}

/******************************************************************************
*                    s[index] = '-' , otherwise s[i] = '~'                    *
******************************************************************************/

strdc(s,index)
char s[];
int index;

{
    int i;

    for (i = 0; i < statenum; i++) {
	s[i] = '0';
    }
    s[index] = '-';
    s[statenum] = '\0';

}

stron(s,index)
char s[];
int index;

{
    int i;

    for (i = 0; i < statenum; i++) {
	s[i] = '0';
    }
    s[index] = '1';
    s[statenum] = '\0';

}



save_in_code(simboli,simbnum)
SYMBLEME *simboli;
int simbnum;

{

    int i;

    for ( i = 0; i < simbnum; i++ ) {
	strcpy(simboli[i].code,simboli[i].best_code);
    }

}
