/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/jedi/rp.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:08 $
 *
 */
/*
 * Bill Lin
 * University of California, Berkeley
 * Comments to billlin@ic.Berkeley.EDU
 *
 * Copyright (c) 1989 Bill Lin, UC Berkeley CAD Group
 *     Permission is granted to do anything with this
 *     code except sell it or remove this message.
 */

#include "inc/jedi.h"
#include "inc/rp.h"
#include "inc/rp_int.h"

double determine_cost();

void
combinatorial_optimize (
  cost_function, generate_function,
  accept_state, reject_state,
  fin, fout, ferr,
  beginning_states, ending_states, 
  starting_temperature,
  maximum_temperature,
  problem_size, verbose
)
int (*cost_function)();
int (*generate_function)();
int (*accept_state)();
int (*reject_state)();
FILE *fin;
FILE *fout;
FILE *ferr;
double beginning_states;
double ending_states;
double starting_temperature;
double maximum_temperature;
int problem_size;
int verbose;
{
    int max_gen, cur_gen, temp_points;
    double c1, c2, c3, c4, start_cost;
    double temp, start_temp, stop_temp;
    double update_temp();
    double find_starting_temp();
    long time;

    /*
     * initialization
     */
    Fin = fin;
    Fout = fout;
    Ferr = ferr;

    if (beginning_states == SA_DEFAULT_PARAMETER) {
	StatesBegin = 1; 
    } else {
	StatesBegin = beginning_states;
    }

    if (ending_states == SA_DEFAULT_PARAMETER) {
	StatesEnd = 1; 
    } else {
	StatesEnd = ending_states;
    }

    Alpha = 0.90;
    stop_temp = 1;
    NewStatesPerUnit = StatesBegin;
    RangeSmall = SA_FULL_RANGE;
    Verbose = verbose;

    OldCost = determine_cost(cost_function);
    start_temp = find_starting_temp(starting_temperature, maximum_temperature);
    temp = start_temp;

    /* start time */
    time = util_cpu_time();

    c1 = OldCost + 10;
    c2 = c1 + 10;
    c3 = OldCost;
    c4 = c2 + 10;
    start_cost = OldCost;

    cur_gen = 0;
    temp_points = 0;
    if (Verbose) {
	(void) fprintf(Fout, "starting condition: ");
	(void) fprintf(Fout, "starting temperature is %.4g; ", start_temp);
	(void) fprintf(Fout, "starting cost is %.4g\n", start_cost);
	(void) fprintf(Fout, "\n");
	(void) fflush(Fout);
    }

    while (!stopping_criterion(c1, c2, c3, c4, stop_temp, temp)) {
	max_gen = NewStatesPerUnit * problem_size;
	cur_gen = 0;
	temp_points++;

	while (!inner_loop_criterion(max_gen, cur_gen)) {
	    generate_new_state(RangeSmall,generate_function);
	    cur_gen++;

	    if (should_accept(temp,cost_function)) {
		accept_new_state(accept_state);
	    } else {
		reject_new_state(reject_state);
	    }
	}

	c1 = c2;
	c2 = c3;
	c3 = c4;
	c4 = OldCost;
	temp = update_temp(temp, start_temp);

	if (Verbose) {
	    (void) fprintf(Fout, "new temperature is %.2g; ", temp);
	    (void) fprintf(Fout, "new cost is %.4g; ", OldCost);
	    (void) fprintf(Fout, "changed after %d states\n", cur_gen);
	    (void) fflush(Fout);
	}
    }

    /*
     * print statistics
     */
    if (Verbose) {
	(void) fprintf(Fout, "\n");
	(void) fprintf(Fout, "starting condition: ");
	(void) fprintf(Fout, "starting temperature was %.4g; ", start_temp);
	(void) fprintf(Fout, "starting cost was %.4g\n", start_cost);
	(void) fprintf(Fout, "final condition: ");
	(void) fprintf(Fout, "final temperature was %.4g; ", temp);
	(void) fprintf(Fout, "final cost was %.4g\n", OldCost);
	(void) fprintf(Fout, "stopped after %d temperature points\n", 
	  temp_points);
	(void) fprintf(Fout, "\n");

	time = util_cpu_time() - time;
	(void) fprintf(Ferr, "total cpu time is %s.\n", util_print_time(time));
    }
} /* end of simulated annealing */


should_accept(temp, cost_function)
double temp;
int (*cost_function)();
{
    double change_in_cost, R, Y, random_generator();

    NewCost = determine_cost(cost_function);
    change_in_cost = NewCost - OldCost;

    if (change_in_cost < 0) {
	return (TRUE);
    } else {
	Y = exp(-change_in_cost/temp);
	R = random_generator(0, 1);
	if (R < Y)
	    return (TRUE);
	else
	    return (FALSE);
    }
} /* end of should_accept */


double
update_temp(temp, start_temp)
double temp;
double start_temp;
{
    double new_temp, factor;

    if (sqrt(start_temp) < temp)
	new_temp = temp * Alpha * Alpha;
    else 
	new_temp = temp * Alpha;

    if (new_temp >= 1.0) {
	factor = log(new_temp)/log(start_temp);
    } else {
	factor = 0;
	RangeSmall = SA_SMALL_RANGE;
    }

    NewStatesPerUnit = StatesEnd - (StatesEnd - StatesBegin)*factor;

    return (new_temp);
} /* end of update_temp */


accept_new_state(accept_state)
int (*accept_state)();
{
    int ret_val;

    OldCost = NewCost;
    ret_val = (*accept_state)();
    if (ret_val != SA_OK) {
	(void) fprintf(Ferr, "Panic: error in accept new state function\n");
	(void) exit(1);
    }
} /* end of accept_new_state */


reject_new_state(reject_state)
int (*reject_state)();
{
    int ret_val;

    ret_val = (*reject_state)();
    if (ret_val != SA_OK) {
	(void) fprintf(Ferr, "Panic: error in reject new state function\n");
	(void) exit(1);
    }
} /* end of reject_new_state */


inner_loop_criterion(num_states_per_stage, num_states_generated)
int num_states_per_stage, num_states_generated;
{
    if (num_states_per_stage < num_states_generated)
	return (TRUE);
    else
	return (FALSE);
} /* end of inner_loop_criterion */


stopping_criterion(c1, c2, c3, c4, stop_temp, temp)
double c1, c2, c3, c4;
double stop_temp, temp;
{
    if (stop_temp < temp)
	return (FALSE);
    if ((c1 == c2) && (c1 == c3) && (c1 == c4))
	return (TRUE);
    else
	return (FALSE);
} /* end of stopping_criterion */


double
find_starting_temp(starting_temperature, maximum_temperature)
double starting_temperature, maximum_temperature;
{
    double delta_c;
    double start_temp;

    delta_c = 0.04 * OldCost;
    start_temp = delta_c/log(1.25);
    
    if (starting_temperature != SA_DEFAULT_PARAMETER) {
	start_temp = starting_temperature;
    }

    if (maximum_temperature != SA_DEFAULT_PARAMETER) {
	if (start_temp > maximum_temperature) {
	    start_temp = maximum_temperature;
	}
    }

    return (start_temp);
} /* end of find_starting_temp */
    

double determine_cost(cost_function)
int (*cost_function)();
{
    int ret_val;
    double cost = 0.0;

    ret_val = (*cost_function)(&cost);
    if (ret_val != SA_OK) {
	(void) fprintf(Ferr, "Panic: error in determine cost function\n");
	(void) exit(1);
    }

    return (cost);
} /* end of determine_cost */


generate_new_state(range, generate_function)
int range;
int (*generate_function)();
{
    int ret_val;

    ret_val = (*generate_function)(range);
    if (ret_val != SA_OK) {
	(void) fprintf(Ferr, "Panic: error in generate new state function\n");
	(void) exit(1);
    }
} /* end of generate_new_state */


double
random_generator(left,right)
int left,right;
{
    double range, divide, rnum, number;

    rnum = (double) random();
    divide = 2147483647;
    number = rnum/divide;
    range = (double) right - left;
    number = number*range + left;

    return(number);

} /* end of random_generator */


int int_random_generator(x,y)
int x, y;
{
    int i;

    i = (int) floor(random_generator(x,y+1));
    if (i > y) {
	return y;
    } else {
	return i;
    }
} /* end of int_random_generator */
