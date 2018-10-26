
#include "sis.h"
#include "delay_int.h"


#define ARRIVAL(time, new_time)	\
    if (new_time > time) time = new_time;


delay_time_t
delay_map_simulate(nin, pin_arrivals, model, load)
int nin;
delay_time_t **pin_arrivals;
char **model;
double load;
{
    register int i;
    register delay_pin_t *pin_delay, **lib_delay_model;
    delay_time_t delay, arrival, *pin_arrival;

    lib_delay_model = (delay_pin_t **) model;
    if (lib_delay_model == 0) {
	fail("no delay model -- should have been caught earlier");
    }

    arrival.rise = - INFINITY;
    arrival.fall = - INFINITY;

    for(i = nin-1; i >= 0; i--) {
	pin_delay = lib_delay_model[i];
	if (pin_delay == 0) {
	    fail("no pin delay -- should have been caught earlier");
	}

	pin_arrival = pin_arrivals[i];
	if (pin_arrival == 0) {
	    fail("no pin_arrival time ?");
	}

	delay.rise = pin_delay->block.rise + pin_delay->drive.rise * load;
	delay.fall = pin_delay->block.fall + pin_delay->drive.fall * load;

	switch(pin_delay->phase) {
	case PHASE_INVERTING:
	    ARRIVAL(arrival.rise, pin_arrival->fall + delay.rise);
	    ARRIVAL(arrival.fall, pin_arrival->rise + delay.fall);
	    break;

	case PHASE_NONINVERTING:
	    ARRIVAL(arrival.rise, pin_arrival->rise + delay.rise);
	    ARRIVAL(arrival.fall, pin_arrival->fall + delay.fall);
	    break;

	case PHASE_NEITHER:
	    ARRIVAL(arrival.rise, pin_arrival->fall + delay.rise);
	    ARRIVAL(arrival.fall, pin_arrival->rise + delay.fall);
	    ARRIVAL(arrival.rise, pin_arrival->rise + delay.rise);
	    ARRIVAL(arrival.fall, pin_arrival->fall + delay.fall);
	    break;
	}
    }

    return arrival;
}
