/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/clock/com_clock.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:18 $
 *
 */
#ifdef SIS
#include "sis.h"
#include "clock.h"

/* ARGSUSED */
com_print_clock(network, argc, argv)
network_t **network;
int argc;
char **argv;
{
    lsGen gen, gen2;
    sis_clock_t *clock;
    int i, num_dep;
    clock_edge_t edge, *new_edge;
    clock_setting_t flag;

    error_init();
    util_getopt_reset();

    if (network_num_clock(*network) == 0) return 0;
    flag = clock_get_current_setting(*network);

    (void)fprintf(misout,"\t(A value of %.2f means unspecified)\n", CLOCK_NOT_SET);
    (void)fprintf(misout,"%s cycle_time = %6.2f\n",
	    (flag == SPECIFICATION ? "SPECIFIED" : "WORKING"),
	    clock_get_cycletime(*network));

    foreach_clock(*network, gen, clock){
	edge.clock = clock;
	for (i = RISE_TRANSITION; i <= FALL_TRANSITION; i++){
	    edge.transition = i;
	    (void)fprintf(misout,"%s%s, Nominal=%4.2f, Range=(%5.2f,%-5.2f)\n",
		    (i == RISE_TRANSITION ? "r'":"f'"), clock_name(clock),
		    clock_get_parameter(edge, CLOCK_NOMINAL_POSITION),
		    clock_get_parameter(edge, CLOCK_LOWER_RANGE),
		    clock_get_parameter(edge, CLOCK_UPPER_RANGE));

	    num_dep = clock_num_dependent_edges(edge);
	    if ( num_dep  > 0){
		(void)fprintf(misout,"\tDependent edges --");
		gen2 = clock_gen_dependency(edge);
		while (lsNext(gen2, (char **)&new_edge, LS_NH) == LS_OK){
		    (void)fprintf(misout,"   %s%s",
			(new_edge->transition == RISE_TRANSITION ? "r'":"f'"),
			clock_name(new_edge->clock));
		}
		LS_ASSERT(lsFinish(gen2));
		(void)fprintf(misout,"\n");
	    }
	}
    }

    return 0;
}

/* ARGSUSED */
com_chng_clock(network, argc, argv)
network_t **network;
int argc;
char **argv;
{
    clock_setting_t setting;

    error_init();
    util_getopt_reset();

    setting = CLOCK(*network)->flag;
    if (setting == SPECIFICATION){
	CLOCK(*network)->flag = WORKING;
    } else if (setting == WORKING){
	CLOCK(*network)->flag = SPECIFICATION;
    } else {
	(void)fprintf(miserr,"Unknown value for clock setting\n");
	return 0;
    }
    setting = CLOCK(*network)->flag;

    (void)fprintf(misout,"Switching to setting %s\n",
	    (setting == SPECIFICATION ? "SPECIFICATION" : "WORKING"));
    return 0;

}

init_clock()
{
    (void)com_add_command("print_clock", com_print_clock, 0);
    (void)com_add_command("chng_clock", com_chng_clock, 0);
}
end_clock()
{
}
#endif /* SIS */
