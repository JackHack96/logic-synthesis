

#include "inc/nova.h"

#undef ZERO
#undef ONE
#undef DASH

#include "../sis/include/espresso.h"
#include "sis/node/node.h"

#define NIL(type) ((type *)0)

/**********************************************************************/
/* outline of the procedure simulated annealing                       */
/*                                                                    */
/* INITIALIZE(state(0),temp(0),moves(0));                             */
/*                                                                    */
/* k = 0;                                                             */
/* state = state(0);                                                  */
/*                                                                    */
/* while (stopcriterion is unsatisfied) {                             */
/*     move_count = MOVES(k);                                         */
/*     temp = TEMPERATURE(k);                                         */
/*     for (l = 1; l <= move_count; l++) {                            */
/*         GENERATE(newstate from neighbours of oldstate);            */
/*         if (cost(newstate) <= cost(oldstate) oldstate = newstate;  */
/*          else if (exp( (cost(oldstate)-cost(newstate))/temp ) >    */
/*                                  random(0,1) oldstate = newstate;  */
/*     }                                                              */
/*     k++;                                                           */
/* }                                                                  */
/**********************************************************************/

#define DISPLACEMENT 1
#define INTERCHANGE 2

double        STATES_BEGIN, STATES_END;
double        ALPHA, START_TEMP;
int           init_cost;
int           codes_num;
int           num1, num2;
static int    do_single_expand = 1;
static int    new_cost         = 0;
static int    best_cost        = 0;
static int    accepted         = 0;
static int    attempted        = 0;
extern int num_moves;
static pcover constr_sf;
HYPERVERTEX   *vertices;

int anneal_code(CONSTRAINT *net, SYMBLEME *simboli, int node_num, int codelength) {
    /*
     * Solve face hypercube embedding for a given dimension of the cube
     */

    CONSTRAINT *constrptr;
    int        c1, c2, c3, start_cost, add, i, card, size;
    double     temp, update_temp(), find_starting_temp();
    int        cur_gen, max_gen;
    int        try
    ,          type;
    pset       constr, p, last;

    single_expand = do_single_expand;
    codes_num     = power(2, codelength);
    vert_alloc(codelength);

    if (cost_function == 1) {
        if (net == NIL(CONSTRAINT)) {
            constr_sf = sf_new(0, 1);
        } else {
            card = strlen(net->relation);
            define_cube_size(card);
            constr_sf      = sf_new(0, 2 * card);
            constr         = set_new(2 * card);
            for (constrptr = net; constrptr != (CONSTRAINT *) 0;
                 constrptr = constrptr->next) {
                for (i = 0; i < card; i++) {
                    switch (constrptr->relation[i]) {
                        case '0':PUTINPUT(constr, i, ZERO);
                            break;
                        case '1':PUTINPUT(constr, i, ONE);
                            break;
                        default:PUTINPUT(constr, i, TWO);
                            break;
                    }
                }
                add = 1;
                foreach_set(constr_sf, last, p) {
                    /*if (cdist0 (constr, p)) {PATCH*/
                    if (setp_implies(constr, p)) {
                        set_and(p, p, constr);
                        size = SIZE(p);
                        PUTSIZE(p, (size + 1));
                        add = 0;
                        break;
                    }
                }
                if (add) {
                    PUTSIZE(constr, 1);
                    sf_addset(constr_sf, constr);
                }
            }
            set_free(constr);
            if (DEBUG) {
                fprintf(stdout, "Constraints:\n");
                cprint(constr_sf);
            }
        }
        define_cube_size(codelength);
    }
    assign_initial_encoding(simboli, node_num, codelength);
    if (net == (CONSTRAINT *) 0) {
        if (DEBUG) {
            printf("Best codes (cost = %d):\n", best_cost);
            show_bestcode(simboli, node_num);
        }
        return best_cost;
    }
    START_TEMP = find_starting_temp(net, simboli, codelength);
    ALPHA      = 0.90;

    STATES_BEGIN = STATES_END = num_moves;
    c1           = init_cost - 30;
    c2           = init_cost - 20;
    c3           = init_cost - 10;
    max_gen      = STATES_BEGIN * node_num;
    temp         = START_TEMP;
    if (DEBUG) {
        printf("\n# Initial temperature %f, cost is %d, moves are %d\n", temp,
               init_cost, max_gen);
    }
    while (temp > 0.0 &&
           (c1 != init_cost || c2 != init_cost || c3 != init_cost)) {
        cur_gen    = 0;
        start_cost = init_cost;
        accepted   = attempted = 0;
        while (cur_gen < max_gen) {
            cur_gen++;
            try
                    = generate_state(simboli, &type);
            if (try)

                accept_or_reject_state(net, simboli, codelength, node_num, temp, type);
        }
        temp = update_temp(temp, &max_gen, node_num);
        if (DEBUG) {
            printf("\n# Changing temperature to %f, cost is %d, moves are %d\n", temp,
                   init_cost, max_gen);
        }
        c1 = c2;
        c2 = c3;
        c3 = start_cost;
    }
    if (DEBUG) {
        printf("# Final cost is %d\n", init_cost);
        printf("# Minimum cost is %d\n", best_cost);
        printf("Final codes :\n");
        show_code(simboli, node_num);
        if (best_cost < init_cost) {
            printf("Best codes :\n");
            show_bestcode(simboli, node_num);
        }
    }

    single_expand = 0;
    if (constr_sf != NIL(set_family_t)) {
        sf_free(constr_sf);
    }
    return best_cost;

} /* end of simulated_anneal */

/*
 * Assign nodes initial encodings prior to annealing
 */
assign_initial_encoding(SYMBLEME
*simboli,
int node_num,
int codelength
){
long time();

int i, new_code, increment, range, seed, trandom();

/* seed = (int) time(0); */
seed = 0;
srand  (seed);
srandom(seed);

/* random codes the states */
range     = minpow2(node_num);
new_code  = trandom(range);
increment = trandom(range) * 2 + 1;
for (
i = 0;
i<node_num;
i++) {
itob(simboli[i]
.code, new_code, codelength);
strcpy(simboli[i]
.best_code, simboli[i].code);
vertices[
code_index(simboli[i]
.code)].
label    = i;
new_code = (new_code + increment) % range;
}
/*if (DEBUG) printf("Initial codes :\n");
show_code(simboli,node_num);*/

} /* end of assign_initial_encoding */

int trandom(range) /* returns a random integer in the interval (0,range) */
        int range;
{
    int result, rand();

    result = rand() % range;
    return (result);
}

generate_state(simboli, type
)
SYMBLEME *simboli;
int      *type;
{
/*
 * randomly generate an interchange or a displacement
 */

double PRANDOM();

int stato1, stato2;

/* codes_num is the number of possible encodings  = 2**code_bits */
/* num1 e num2 sono due codici scelti a caso tra tutti i possibili */
num1 = PRANDOM(0, codes_num);
num2 = PRANDOM(0, codes_num);
if (num1 is num2 || num1 is codes_num || num2 is codes_num)
return (FALSE);
if (vertices[num1].label == -1 && vertices[num2].label == -1)
return FALSE;
if (vertices[num1].label == -1) {
*
type   = DISPLACEMENT;
/* swap the code of stato2 with that of num1 to find the new cost */
stato2 = vertices[num2].label;
strcpy(simboli[stato2]
.code, vertices[num1].code);
vertices[num1].
label = stato2;
vertices[num2].
label = -1;
return (TRUE);
} else if (vertices[num2].label == -1) {
*
type   = DISPLACEMENT;
/* swap the code of stato1 with that of num2 to find the new cost */
stato1 = vertices[num1].label;
strcpy(simboli[stato1]
.code, vertices[num2].code);
vertices[num1].
label = -1;
vertices[num2].
label = stato1;
return (TRUE);
} else {
*
type   = INTERCHANGE;
/* interchange the codes between stato1 and stato2 to find the new cost */
stato1 = vertices[num1].label;
stato2 = vertices[num2].label;
strcpy(simboli[stato1]
.code, vertices[num2].code);
strcpy(simboli[stato2]
.code, vertices[num1].code);
vertices[num1].
label = stato2;
vertices[num2].
label = stato1;
return (TRUE);
}

} /* end of generate_state */

double PRANDOM(left, right)int left, right;
{

    long divide, random_num;
#if !defined(__osf__)
#endif
    double number;

    random_num = random();
    divide     = 2147483647;
    number     = ((double) random_num) / divide;
    number     = number * abs(right - left) + left;

    return (number);

} /* end of PRANDOM */

double find_starting_temp(net, simboli, codelength)CONSTRAINT *net;
                                                   SYMBLEME *simboli;
                                                   int codelength;
{

    double delta_c, temp;
    int find_cost();
    /*
     *  Let's not accept a move which increases the initial cost by more than
     *   X percent.
     */

    init_cost = find_cost(net, simboli, codelength);
    best_cost = init_cost;
    delta_c   = 0.04 * (double) init_cost;
    temp      = delta_c / log(1.25);
    return (temp);

} /* end of find_starting_temp */

int find_cost(net, simboli, codelength)CONSTRAINT *net;
                                       SYMBLEME *simboli;
                                       int codelength;
{
    /*
     * find the cost of the given encoding as sum of the weights of the
     * unsatisfied constraints
     */

    CONSTRAINT *constrptr;
    char       *face;
    char       bit1;
    int        i, j, card, satisfied;
    int        sat_measure, unsat_measure;
    int        cost, count;
    static int call_count = 0;
    pcover F, D, R, codes;
    pset       p, last, pnew, p1, last1;

    call_count++;
    cost = 0;

    cost = init_cost;

    /* empty net : do nothing */
    if (net == (CONSTRAINT *) 0) {
        return 0;
    }

    sat_measure = unsat_measure = 0;

    card = strlen(net->relation);
    if (cost_function == 1) {
        pnew   = set_new(codelength * 2);
        /* create the codes as set family */
        codes  = sf_new(card, codelength * 2);
        for (j = 0; j < card; j++) {
            for (i = 0; i < codelength; i++) {
                switch (simboli[j].code[i]) {
                    case '1':PUTINPUT(pnew, i, ONE);
                        break;
                    case '0':PUTINPUT(pnew, i, ZERO);
                        break;
                    default:fprintf(stderr, "invalid code value\n");
                        exit(-1);
                }
            }
            sf_addset(codes, pnew);
        }
        foreach_set(constr_sf, last1, p1) {
            /* insert the codes in the appropriate set family */
            F = sf_new(card, codelength * 2);
            R = sf_new(card, codelength * 2);

            for (j = 0; j < card; j++) {
                set_copy(pnew, GETSET(codes, j));
                switch (GETINPUT(p1, j)) {
                    case ONE:sf_addset(F, pnew);
                        break;
                    case ZERO:sf_addset(R, pnew);
                        break;
                }
            }

            D = complement(cube2list(F, R));
            F = espresso(F, D, R);

            /* now count the literals */
            count = 0;
            foreach_set(F, last, p) { count += 2 * codelength - set_ord(p); }
            unsat_measure += SIZE(p1) * count;
            sf_free(F);
            if (cost_function == 1) {
                sf_free(D);
            }
            sf_free(R);
        }
        sf_free(codes);
        set_free(pnew);
    } else {
        /*shownet(net);
        exit(-1);*/

        if ((face = (char *) calloc((unsigned) codelength + 1, sizeof(char))) ==
            (char *) 0) {
        }

        /* scans all constraints */
        for (constrptr = net; constrptr != (CONSTRAINT *) 0;
             constrptr = constrptr->next) {

            /*printf("The constraint is %s\n", constrptr->relation);*/

            /* scans all boolean coordinates */
            for (i = 0; i < codelength; i++) {

                /* stores in bit1 the "i-th" bit of the best_code of the "j-th"
               symbleme ( which belongs to the current constraint )         */
                for (j = 0; j < card; j++) {
                    if (constrptr->relation[j] == '1') {
                        bit1 = simboli[j].code[i];
                    }
                }

                /* if there are two symblemes in the current constraint different
               on the i-th boolean coordinate face[i] gets a * (don't care)
               otherwise face[i] gets the common coordinate value            */
                for (j = 0; j < card; j++) {
                    if (constrptr->relation[j] == '1') {
                        if (bit1 != simboli[j].code[i]) {
                            face[i] = '*';
                            break;
                        } else {
                            face[i] = bit1;
                        }
                    }
                }
            }

            /*printf("the spanned face is %s\n", face);*/

            /* if the code of any other symbleme ( i.e. a symbleme not
               contained in constraint ) is covered by face , the current
               constraint has not been satisfied                             */
            for (j = 0; j < card; j++) {
                if (constrptr->relation[j] == '0') {
                    if (inclusion(simboli[j].code, face, codelength) == 1) {
                        /*printf("the symbleme %s (%d)  is covered by face
                         * %s\n",simboli[j].name, j, face);*/
                        satisfied = 0;
                        /*printf("UNSATISFIED\n");*/
                        break;
                    }
                }
            }
            /* no other constraint covered by face */
            if (j == card) {
                satisfied = 1;
                /*printf("SATISFIED\n");*/
            }
            if (satisfied == 0) {
                unsat_measure = unsat_measure + constrptr->weight;
            }
            if (satisfied == 1) {
                sat_measure = sat_measure + constrptr->weight;
            }
        }
    }

    if (DEBUG)
        printf("\nGain = %d ; Cost = %d\n", sat_measure, unsat_measure);
    cost = unsat_measure;

    return (cost);

} /* end of find_cost */

should_accept(change_in_cost, temp
)
int    change_in_cost;
double temp;
{

double R, Y, PRANDOM();

if (change_in_cost < 0)
return (TRUE);
else {
Y = exp(-(double) change_in_cost / temp);
R = PRANDOM(0, 1);
if (R < Y)
return (TRUE);
else
return (FALSE);
}

} /* end of should_accept */

accept_or_reject_state(net, simboli, codelength, node_num, temp,
                            type
)
CONSTRAINT *net;
SYMBLEME   *simboli;
int        codelength;
int        node_num;
double     temp;
int        type;
{

int find_cost();

int i, stato1, stato2;

new_cost = find_cost(net, simboli, codelength);
if (best_cost > new_cost) { /* save the best codes found so far */
best_cost = new_cost;
for (
i = 0;
i<node_num;
i++) {
strcpy(simboli[i]
.best_code, simboli[i].code);
}
}
attempted++;
if (
should_accept(new_cost
- init_cost, temp) is TRUE) {
accepted++;
if (DEBUG)
printf("Move accepted\n");
init_cost = new_cost;
} else {
if (type is DISPLACEMENT) {
if (vertices[num1].label == -1) {
/* swap the code of stato2 with that of num1 to undo last move */
stato2 = vertices[num2].label;
strcpy(simboli[stato2]
.code, vertices[num1].code);
vertices[num1].
label = stato2;
vertices[num2].
label = -1;
return (TRUE);
} else if (vertices[num2].label == -1) {
/* swap the code of stato1 with that of num2 to undo last move */
stato1 = vertices[num1].label;
strcpy(simboli[stato1]
.code, vertices[num2].code);
vertices[num1].
label = -1;
vertices[num2].
label = stato1;
return (TRUE);
}
} else if (type is INTERCHANGE) {
/* interchange the codes between stato1 and stato2 to undo last move */
stato1 = vertices[num1].label;
stato2 = vertices[num2].label;
strcpy(simboli[stato1]
.code, vertices[num2].code);
strcpy(simboli[stato2]
.code, vertices[num1].code);
vertices[num1].
label = stato2;
vertices[num2].
label = stato1;
}
}

return FALSE;

} /* end of accept_or_reject_state */

double update_temp(temp, new_states, node_num)double temp;
                                              int *new_states;
                                              int node_num;
{

    double new_temp, factor;

    if (sqrt(START_TEMP) < temp)
        new_temp = temp * ALPHA * ALPHA;
    else
        new_temp = ALPHA * temp;
    if (new_temp >= 1.0)
        factor   = log(new_temp) / log(START_TEMP);
    else
        factor = 0;
    *new_states = (int) ((STATES_END - (STATES_END - STATES_BEGIN) * factor) * 10 *
                         node_num);

    return (new_temp);

} /* end of update_temp */

/**************************************************************************
 * Fills the field "code" of "vertices" with the boolean coordinates of    *
 * that vertex on the hypercube of dimension "codelength"                  *
 **************************************************************************/

vert_alloc(codelength)
int codelength;

{
char module;
int  i, j, k;

if ((
vertices = (HYPERVERTEX *) calloc(
        (unsigned) codes_num, sizeof(HYPERVERTEX))
) == (HYPERVERTEX *)0) {
fprintf(stderr,
"Insufficient memory for vertices");
exit(-1);
}

for (
i = 0;
i<codes_num;
i++) {
if ((vertices[i].
code = (char *) calloc((unsigned) codelength + 1,
                       sizeof(char))
) == (char *)0) {
fprintf(stderr,
"Insufficient memory for vertices[].code");
exit(-1);
}
}

/* clears vertices[].code[] before filling them with the right codes */
for (
i = 0;
i<codes_num;
i++) {
for (
j = 0;
j<codelength;
j++) {
vertices[i].code[j] = '0';
}
vertices[i].code[j] = '\0';
}

/* vertices[i].code[] contains the binary form of i */
for (
i = 0;
i<codes_num;
i++) {

j = i;
k = codelength - 1;

while (j > 0) {

if (j % 2 == 0) {
module = '0';
} else {
module = '1';
}

vertices[i].code[k] =
module;

j = j / 2;
k--;
}

vertices[i].
label = -1;
}

/*printf("Codes provided by codes_box\n");
for (i=0; i<codes_num; i++) {
       printf("i = %d ", i);
       printf("code = %s", vertices[i].code);
       printf("\n");
}*/
}

int code_index(code)char *code;

{
    int i;

    for (i = 0; i < codes_num; i++) {
        if (strcmp(vertices[i].code, code) == 0) {
            return (i);
        }
    }
    return 0;
}
