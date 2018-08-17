
#include "inc/nova.h"

/***********************************************************************
*                   Auxiliary routines                                 *
***********************************************************************/

/* returns index of t in s, -1 if none */
int myindex(char s[], char t[]) {
    int i, j, k;

    for (
            i = 0;
            s[i] != '\0'; i++) {
        for (
                j = i, k = 0;
                t[k] != '\0' && s[j] == t[k]; j++, k++);
        if (t[k] == '\0')
            return (i);
    }
    return (-1);
}


/* computes the upper ceiling of the log base 2 of its argument */
int mylog2(int arg) {
    int i, value;

    for (
            i = 1, value = 0;
            i < arg;
            i = i << 1, value
                    ++);

    return (value);
}


/* computes the upper ceiling of the log base 2 of its argument 
   if arg is <= 1, it is defined as 1                              */
int special_log2(int arg) {
    int i, value;

    if (arg <= 1)
        value = 1;

    if (arg > 1) {
        for (
                i = 1, value = 0;
                i < arg;
                i = i << 1, value
                        ++);
    }

    return (value);

}

/* computes the minimum power of 2 >= than its argument */
int minpow2(int arg) {
    int value;

    for (
            value = 1;
            value < arg;
            value = value << 1
            );

    return (value);
}


/* computes the hamming-distance between the cubes "arg1" and "arg2" 
   of "length" size */
int hamm_dist(char *arg1, char *arg2, int length) {
    int i, distance;

    distance = 0;

    for (
            i = 0;
            i < length;
            i++) {
        if (((arg1[i] == ONE) && (arg2[i] == ZERO)) ||
            ((arg1[i] == ZERO) && (arg2[i] == ONE))) {
            distance++;
        }
    }

    return (distance);

}

/* computes the perimeter of an hyperface of dimension "dim" */
int perimeter(int dim) {
    int i, power;

    power = 1;

    for (
            i = 0;
            i < dim - 1; i++) {
        power = power * 2;
    }

    return (
            dim * power
    );
}

/* converts s to integer */
int myatoi(char s[]) {
    int i, n;

    for (
            i = 0;
            s[i] == ' ' || s[i] == '\n' || s[i] == '\t' || s[i] == '.' ||
            s[i] == 'i' || s[i] == 'o' || s[i] == 'p'; i++) { ;  /* skips white space and the keys '.i' , '.o' , '.p' */
    }

    for (
            n = 0;
            s[i] >= '0' && s[i] <= '9'; i++) {
        n = 10 * n + s[i] - '0';
    }

    return (n);
}

int size(void) {
    FILE *fp, *fopen();
    char line[MAXLINE], *fgets();
    int  min_size;
    /* note : min_inputs,min_ouputs,min_products have been defined as global 
       variables */

    if ((fp = fopen(temp4, "r")) == NULL) {
        fprintf(stderr, "size: can't open temp4\n");
        exit(-1);
    } else {
        /* skips the first line ( comment )
            fgets(line,MAXLINE,fp );*/

        if (fgets(line, MAXLINE, fp) != (char *) 0 &&
            myindex(line, ".i") != -1) {
            min_inputs = myatoi(line);
        }
        if (VERBOSE) printf("\nmin_inputs = %d\n", min_inputs);

        if (fgets(line, MAXLINE, fp) != (char *) 0 &&
            myindex(line, ".o") != -1) {
            min_outputs = myatoi(line);
        }
        if (VERBOSE) printf("min_outputs = %d\n", min_outputs);

        if (fgets(line, MAXLINE, fp) != (char *) 0 &&
            myindex(line, ".p") != -1) {
            min_products = myatoi(line);
        }
        if (VERBOSE) printf("min_products = %d\n", min_products);
    }

    fclose(fp);

    min_size = (2 * min_inputs + min_outputs) * min_products;

    if (VERBOSE) printf("min_size = %d\n", min_size);

    return (min_size);

}


/* converts the integer "value" into the binary coded string "binary"
   of length "length" */
int itob(char *binary, int value, int length) {

    int i;

    for (
            i = length - 1;
            i >= 0; i--) {

        if ((value % 2) == 1) {
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
int btoi(char *binary, int length) {

    int i, value;

    value     = 0;
    for (
            i = length - 1;
            i >= 0; i--) {

        if (binary[i] == ONE) {
            value = value + power(2, length - 1 - i);
        }

    }

    return (value);

}


/* replaces all characters from a pound sign to end of line with blanks */
int comment(char *string, int length) {
    int i, j;

    for (
            i = 0;
            i < length;
            i++) {
        if (string[i] == '#') {
            for (
                    j = i;
                    j < length;
                    j++) {
                string[j] = ' ';
            }
            break;
        }
    }
}


/* replaces tabs with blanks */
int tabs(char *string, int length) {
    int i;

    for (
            i = 0;
            i < length;
            i++) {
        if (string[i] == '\t') {
            string[i] = ' ';
        }
    }
}


/* removes trailing blanks and tabs */
int trailblanks(char *string, int length) {
    int i;

    i = length;

    while (--i >= 0) {
        if (string[i] != ' ' && string[i] != '\t' && string[i] != '\n') {
            break;
        }
        string[i + 1] = '\0';
    }

}

/* returns 1 (otherwise 0) iff set p includes set s -
		  it does not check whether the inclusion is proper  */
int p_incl_s(char *p, char *s) {

    int j;

    for (
            j = 0;
            p[j] != '\0'; j++) {
        if (p[j] == ZERO && s[j] == ONE) {
            return (0);
        }
    }

    return (1);

}

/* returns 1 (otherwise 0) iff face1 includes
				      properly face2                          */
int face1_incl_face2(char *face1, char *face2, int dim) {

    int j;
    BOOLEAN inclusion, proper;

    inclusion = TRUE;
    proper    = FALSE;

    for (
            j = 0;
            j < dim;
            j++) {

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

    return (inclusion && proper);

}

/* returns 1 (otherwise 0) iff face1 includes
				      (also not properly) face2 */
int face1_incl2_face2(char *face1, char *face2, int dim) {

    int j;
    BOOLEAN inclusion;

    inclusion = TRUE;

    for (
            j = 0;
            j < dim;
            j++) {

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

    return (inclusion);
}

/* raise x to n-th power; n > 0 */
int power(int x, int n) {
    int i, p;

    p         = 1;
    for (
            i = 1;
            i <=
            n;
            ++i)
        p = p * x;
    return (p);
}

/* returns the intersection of faces a and
                                        b ; checks also if it has dimension
					large enough to contain constr->card  */
char *a_inter_b(char *a, char *b, int constr_card, int dim) {

    int  j, inter_dim;
    char *new_code;

    if ((new_code = (char *) calloc((unsigned) dim + 1, sizeof(char))) == (char *) 0) {
        fprintf(stderr, "Insufficient memory for new_code in a_inter_b");
        exit(-1);
    }

    new_code[dim] = '\0';

    inter_dim = 1;

    for (j = 0; j < dim; j++) {

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
    if (inter_dim < constr_card) {
        free_mem(new_code);
        new_code = (char *) 0;
    }

    return (new_code);

}

/* returns the dimension of the intersection
                                      of faces a and b */
int dim_fathers_inter(char *a, char *b, int dim) {

    int j, inter_dim;

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

    return (inter_dim);

}

/* returns the dimension of face a */
int dim_face(char *a, int dim) {

    int j, inter_dim;

    inter_dim = 0;

    for (j = 0; j < dim; j++) {

        if (a[j] == 'x') {
            inter_dim = inter_dim + 1;
        }

    }

    return (inter_dim);

}

/* returns the intersection of constraints a and b */
char *c1_inter_c2(char *a, char *b, int dim) {

    int  j, num_ones;
    char *new_code;

    if ((new_code = (char *) calloc((unsigned) dim + 1, sizeof(char))) == (char *) 0) {
        fprintf(stderr, "Insufficient memory for new_code in c1_inter_c2");
        exit(-1);
    }

    new_code[dim] = '\0';

    num_ones = 0;

    for (j = 0; j < dim; j++) {

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

    if (num_ones >= 1) {
        return (new_code);
    } else {
        free_mem(new_code);
        return ((char *) 0);
    }

}

/* returns #(intersection of constraints a and b) */
int card_c1c2(char *a, char *b, int dim) {

    int j, num_ones;

    num_ones = 0;

    for (j = 0; j < dim; j++) {

        if (a[j] == ONE && b[j] == ONE) {
            num_ones++;
        }

    }

    return (num_ones);

}

/* returns #(constraint a) */
int card_c1(char *a, int dim) {

    int j, num_ones;

    num_ones = 0;

    for (j = 0; j < dim; j++) {

        if (a[j] == ONE) {
            num_ones++;
        }

    }

    return (num_ones);

}



/* ************************************************************************** */
/*                ADDED FOR THE SYMBOLIC MINIMIZATION LOOP                    */
/* ************************************************************************** */

int dfs(int old_source, int new_source) {
    ORDER_RELATION *curptr;

    int path_is_new();

    reached[new_source] = 1;

    for (
            curptr = order_graph[new_source];
            curptr != (ORDER_RELATION *) 0;
            curptr = curptr->next
            ) {
        if (reached[curptr->index] == 0) {
            if (
                    path_is_new(old_source, curptr
                            ->index) == 0) {
                new_order_path(old_source, curptr
                        ->index);
            }
            dfs(old_source, curptr
                    ->index);
        }
    }

}


int relation_is_new(int row, int column) {
    ORDER_RELATION *curptr;

    for (
            curptr = order_graph[row];
            curptr != (ORDER_RELATION *) 0;
            curptr = curptr->next
            ) {
        if (curptr->index == column) {
            return (1);
        }
    }
    return (0);

}


int path_is_new(int row, int column) {
    ORDER_PATH *curptr;

    for (
            curptr = path_graph[row];
            curptr != (ORDER_PATH *) 0;
            curptr = curptr->next
            ) {
        if (curptr->dest == column) {
            return (1);
        }
    }
    return (0);

}


/******************************************************************************
*                    Copies string t[0,statenum-1] to string s                *
******************************************************************************/

int strcar(char s[], char t[]) {
    int i;

    for (
            i   = 0;
            i < statenum;
            i++) {
        s[i] = t[i];
    }
    s[statenum] = '\0';

}


/******************************************************************************
*                    s[index] = '0' , otherwise s[i] = '~'                    *
******************************************************************************/

int stroff(char s[], int index) {
    int i;

    for (
            i   = 0;
            i < statenum;
            i++) {
        s[i] = '0';
    }
    s[index]    = '0';
    s[statenum] = '\0';

}

/******************************************************************************
*                    s[index] = '-' , otherwise s[i] = '~'                    *
******************************************************************************/

int strdc(char s[], int index) {
    int i;

    for (
            i   = 0;
            i < statenum;
            i++) {
        s[i] = '0';
    }
    s[index]    = '-';
    s[statenum] = '\0';

}

int stron(char s[], int index) {
    int i;

    for (
            i   = 0;
            i < statenum;
            i++) {
        s[i] = '0';
    }
    s[index]    = '1';
    s[statenum] = '\0';

}


int save_in_code(SYMBLEME *simboli, int simbnum) {

    int i;

    for (
            i = 0;
            i < simbnum;
            i++) {
        strcpy(simboli[i]
                       .code, simboli[i].best_code);
    }

}
