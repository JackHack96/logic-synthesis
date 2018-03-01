/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/delay/com_delay.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:19 $
 *
 */
#include "sis.h"
#include "delay_int.h"

static int delay_sort_flag;

static int
delay_sort(node1_p, node2_p)
char **node1_p, **node2_p;
{
    node_t *node1 = *(node_t **) node1_p;
    node_t *node2 = *(node_t **) node2_p;
    double diff;
    delay_time_t delay1, delay2;

    switch(delay_sort_flag) {
    case 0:
	delay1 = delay_arrival_time(node1);
	delay2 = delay_arrival_time(node2);
	diff = MAX(delay1.rise, delay1.fall) - MAX(delay2.rise, delay2.fall);
	break;
    case 1:
	delay1 = delay_required_time(node1);
	delay2 = delay_required_time(node2);
	diff = MAX(delay2.rise, delay2.fall) - MAX(delay1.rise, delay1.fall);
	break;
    case 2:
	delay1 = delay_slack_time(node1);
	delay2 = delay_slack_time(node2);
	diff = MAX(delay2.rise, delay2.fall) - MAX(delay1.rise, delay1.fall);
	break;
    case 3:
	diff = DELAY(node1)->load - DELAY(node2)->load;
	break;
    default:
	fail("bad delay sort flag");
	/* NOTREACHED */
    }

    if (diff > 0) {
	return -1;
    } else if (diff == 0) {
	return 0;
    } else {
	return 1;
    }
}



#define PRINT_VALUE(n) \
	if (n == DELAY_NOT_SET) \
	    (void)fprintf(sisout, "%s", DELAY_NOT_SET_STRING); \
	else \
	    (void)fprintf(sisout, "%5.2f ", n);

#define PRINT_DEFAULT(parm) \
	value = delay_get_default_parameter(net, parm); PRINT_VALUE(value)

static int
com_print_delay_constraints(network, argc, argv)
network_t **network;
int argc;
char **argv;
{
    array_t *node_vec;
    int i;
    node_t *node;
    double value;		/* used in the PRINT_DEFAULT macro */
    delay_time_t t;
    network_t *net = *network;
    double f;

    (void) fprintf(sisout, "\t\tA setting of %s means value not specified\n", DELAY_NOT_SET_STRING);
    
    (void) fprintf(sisout, "Default settings:\n\t\tinput arrival=( ");
	    PRINT_DEFAULT(DELAY_DEFAULT_ARRIVAL_RISE);
	    PRINT_DEFAULT(DELAY_DEFAULT_ARRIVAL_FALL);
    (void) fprintf(sisout, ")\n\t\tinput drive=( ");
	    PRINT_DEFAULT(DELAY_DEFAULT_DRIVE_RISE);
	    PRINT_DEFAULT(DELAY_DEFAULT_DRIVE_FALL);
    (void) fprintf(sisout, ")\n\t\tmax input load=");
            PRINT_DEFAULT(DELAY_DEFAULT_MAX_INPUT_LOAD);
    (void) fprintf(sisout, "\n\t\toutput load=");
	    PRINT_DEFAULT(DELAY_DEFAULT_OUTPUT_LOAD);
    (void) fprintf(sisout, "\n\t\toutput required=( ");
	    PRINT_DEFAULT(DELAY_DEFAULT_REQUIRED_RISE);
	    PRINT_DEFAULT(DELAY_DEFAULT_REQUIRED_FALL);
    (void) fprintf(sisout, ")\n");
    delay_print_wire_loads(net, sisout);
    
    node_vec = com_get_nodes(net, argc, argv);

    for(i = 0; i < array_n(node_vec); ++i) {
	node = array_fetch(node_t *, node_vec, i);
#ifdef SIS
	if(network_is_real_pi(*network, node)) {
#else
	if(node->type == PRIMARY_INPUT) {
#endif /* SIS */
	    (void) fprintf(sisout, "Settings for input %s:\tarrival=( ",
		    node_name(node));
	    t.rise = t.fall = DELAY_NOT_SET;
	    (void)delay_get_pi_arrival_time(node, &t);
	    PRINT_VALUE(t.rise); PRINT_VALUE(t.fall);
	    (void) fprintf(sisout, ")\tdrive=( ");
	    t.rise = t.fall = DELAY_NOT_SET;
	    (void)delay_get_pi_drive(node, &t);
	    PRINT_VALUE(t.rise); PRINT_VALUE(t.fall);
	    (void) fprintf(sisout, ")\tmax input load=");
	    f = DELAY_NOT_SET;
	    (void)delay_get_pi_load_limit(node, &f);
	    PRINT_VALUE(f);
	    (void) fprintf(sisout, "\n");
#ifdef SIS
	} else if(network_is_real_po(*network, node)) {
#else
	} else if(node->type == PRIMARY_OUTPUT) {
#endif /* SIS */
	    (void) fprintf(sisout, "Settings for output %s:\tload=",
		    node_name(node));
	    value = DELAY_NOT_SET;
	    (void)delay_get_po_load(node, &value);
	    PRINT_VALUE(value);
	    (void) fprintf(sisout, "\trequired=( ");
	    t.rise = t.fall = DELAY_NOT_SET;
	    (void)delay_get_po_required_time(node, &t);
	    PRINT_VALUE(t.rise); PRINT_VALUE(t.fall);
	    (void) fprintf(sisout, ")\n");
	}
    }
    array_free(node_vec);
    return 0;
}
	   
   

static int
com_print_delay(network, argc, argv)
network_t **network;
int argc;
char **argv;
{
    int i, c, j, aflag, rflag, sflag, lflag, some_flag, print_max;
    array_t *node_vec;
    node_t *node;
    char *model_filename;
    delay_time_t arrival, required, slack;
    delay_model_t model;

    aflag = 0;
    rflag = 0;
    sflag = 0;
    lflag = 0;
    some_flag = 0;
    print_max = INFINITY;
    delay_sort_flag = -1;
    model = DELAY_MODEL_LIBRARY;
    model_filename = NIL(char);
    util_getopt_reset();
    while ((c = util_getopt(argc, argv, "alm:p:f:rs")) != EOF) {
	switch(c) {
	case 'f':
	    model_filename = util_strsav(util_optarg);
	    break;
	case 'a':
	    some_flag = 1;
	    aflag = 1;
	    if (delay_sort_flag == -1) delay_sort_flag = 0;
	    break;
	case 'm':
	    model = delay_get_model_from_name(util_optarg);
	    if (model == DELAY_MODEL_UNKNOWN) goto usage;
	    break;
	case 'l':
	    some_flag = 1;
	    lflag = 1;
	    if (delay_sort_flag == -1) delay_sort_flag = 3;
	    break;
	case 'p':
	    print_max = atoi(util_optarg);
	    break;
	case 'r':
	    some_flag = 1;
	    rflag = 1;
	    if (delay_sort_flag == -1) delay_sort_flag = 1;
	    break;
	case 's':
	    some_flag = 1;
	    sflag = 1;
	    if (delay_sort_flag == -1) delay_sort_flag = 2;
	    break;
	default:
	    goto usage;
	}
    }

    node_vec = com_get_true_nodes(*network, argc-util_optind+1, argv+util_optind-1);
    if (array_n(node_vec) < 1) goto usage;

    switch (model) {
    case DELAY_MODEL_LIBRARY:
	if (lib_network_is_mapped(*network)) {
	    (void) fprintf(sisout, " ... using library delay model\n");
	    if (! delay_trace(*network, DELAY_MODEL_LIBRARY)) {
		(void) fprintf(siserr, "%s", error_string());
		array_free(node_vec);
		return 1;
	    }
	} else {
	    (void) fprintf(siserr, 
		"network not mapped, cannot use library delay model\n");
	    array_free(node_vec);
	    return 1;
	}
	break;
     case DELAY_MODEL_MAPPED:
	if (lib_network_is_mapped(*network)) {
	    (void) fprintf(sisout,
			   "...network mapped, using library delay model\n");
	    if (! delay_trace(*network, DELAY_MODEL_LIBRARY)) {
		(void) fprintf(siserr, "%s", error_string());
		array_free(node_vec);
		return 1;
	    }
	} else {
	    (void) fprintf(siserr, 
		"...network not mapped, using mapped delay model\n");
	    if (! delay_trace(*network, DELAY_MODEL_MAPPED)) {
		(void) fprintf(siserr, "%s", error_string());
		array_free(node_vec);
		return 1;
	    }
	}
	break;


    case DELAY_MODEL_UNIT_FANOUT:
	(void) fprintf(sisout, " ... using unit delay model (with fanout)\n");
	if (! delay_trace(*network, DELAY_MODEL_UNIT_FANOUT)) {
	    (void) fprintf(siserr, "%s", error_string());
	    array_free(node_vec);
	    return 1;
	}
	break;

    case DELAY_MODEL_UNIT:
	(void) fprintf(sisout, " ... using unit delay model (levels)\n");
	if (! delay_trace(*network, DELAY_MODEL_UNIT)) {
	    (void) fprintf(siserr, "%s", error_string());
	    array_free(node_vec);
	    return 1;
	}
	break;

    case DELAY_MODEL_TDC:
	(void) fprintf(sisout, " ... using tdc delay model with fanout\n");
	delay_set_tdc_params(model_filename);
	if (! delay_trace(*network, DELAY_MODEL_TDC)) {
	    (void) fprintf(siserr, "%s", error_string());
	    array_free(node_vec);
	    return 1;
	}
	break;

    default:
	array_free(node_vec);
	goto usage;		/* shouldn't happen ? */
    }
	

    if (delay_sort_flag != -1) {
	array_sort(node_vec, delay_sort);
    }

    for(i = 0, j = 0; (j < print_max) && (i < array_n(node_vec)); i++) {
	node = array_fetch(node_t *, node_vec, i);

	/*
	 * Print data on the primary output only once. When multiple
	 * primary outputs share the same internal node, we will print
	 * the primary outputs as well as the internal node
	 */
        if (argc == util_optind && node_function(node) == NODE_PO &&
                node_num_fanout(node_get_fanin(node,0)) == 1 &&
                node_type(node_get_fanin(node,0)) == INTERNAL) continue;
	else j++;

	arrival = delay_arrival_time(node);
	required = delay_required_time(node);
	slack = delay_slack_time(node);

	(void) fprintf(sisout, "%-10s: ", node_name(node));
	if (! some_flag || aflag) {
	    if (arrival.rise < -(INFINITY/2) || arrival.fall < -(INFINITY/2)){
		(void) fprintf(sisout, "arrival=( -INF  -INF) ");
	    } else {
		(void) fprintf(sisout, "arrival=(%5.2f %5.2f) ", 
		    arrival.rise, arrival.fall);
	    }
	}
	if (! some_flag || rflag) {
	    if (required.rise < -(INFINITY/2) || required.fall < -(INFINITY/2)){
		(void) fprintf(sisout, "required=(0 0) ");
	    } else {
		(void) fprintf(sisout, "required=(%5.2f %5.2f) ", 
		    required.rise, required.fall);
	    }
	}
	if (! some_flag || sflag) {
	    if (slack.rise  > (INFINITY/2) || slack.fall  > (INFINITY/2)){
		(void) fprintf(sisout, "slack=(  INF   INF) ");
	    } else {
		(void) fprintf(sisout, "slack=(%5.2f %5.2f) ", 
		    slack.rise, slack.fall);
	    }
	}
	if (lflag) {
	    (void) fprintf(sisout, "load=%6.3f", DELAY(node)->load);
	}
	(void) fprintf(sisout, "\n");
    }
    if (model_filename != NIL(char)){
	FREE(model_filename);
    }
    array_free(node_vec);
    return 0;

usage:
    (void) fprintf(siserr, 
	"print_delay [-alrs] [-m model] [-f file] [-p n] n1 n2 ...\n");
    (void) fprintf(siserr, "    -a\t\tprint arrival times\n");
    (void) fprintf(siserr, "    -l\t\tprint output loads\n");
    (void) fprintf(siserr, "    -r\t\tprint required times\n");
    (void) fprintf(siserr, "    -s\t\tprint slack times\n");
    (void) fprintf(siserr,
    	"    -m model\tchoose delay model (unit, unit-fanout, library, mapped, tdc)\n");
    (void) fprintf(siserr,
    	"    -f file\t Force parameters to be read from file (for tdc model)\n");
    (void) fprintf(siserr, "    -p n\tonly print top 'n' values\n");
    return 1;
}

enum parm_enum {ATIME, RTIME, LOAD, DRIVE, DEFAULT_RTIME, DEFAULT_ATIME,
		    DEFAULT_LOAD, DEFAULT_DRIVE, DEFAULT_WIRE, MAX_INPUT_LOAD};
#define INPUTS 1
#define OUTPUTS 2

static int
com_set_delay(network, argc, argv)
network_t **network;
int argc;
char **argv;
{
  int i, c, set = 0, nodetype = 0, found = 0;
  array_t *node_vec;
  node_t *node;
  delay_param_t rise_parm, fall_parm;
  double parm_val;
  enum parm_enum set_parameter;

    
  util_getopt_reset();
  while ((c = util_getopt(argc, argv, "?a:d:i:l:r:A:D:I:L:R:S:W:")) != EOF) {
    switch(c) {
    case 'a':
      set_parameter = ATIME;
      ++set;
      found = 1;
      nodetype = INPUTS;
      break;
    case 'd':
      set_parameter = DRIVE;
      ++set;
      found = 1;
      nodetype = INPUTS;
      break;
    case 'i':
      set_parameter = MAX_INPUT_LOAD;
      ++set;
      found = 1;
      nodetype = INPUTS;
      break;
    case 'l':
      set_parameter = LOAD;
      ++set;
      found = 1;
      nodetype = OUTPUTS;
      break;
    case 'r':
      set_parameter = RTIME;
      ++set;
      found = 1;
      nodetype = OUTPUTS;
      break;
    case 'A':
    case 'D':
    case 'I':
    case 'L':
    case 'R':
    case 'S':
    case 'W':
      if((parm_val = atof(util_optarg)) == HUGE) goto usage;
      if(parm_val < 0.0) parm_val = DELAY_VALUE_NOT_GIVEN;
      switch(c) {
      case 'A':
	delay_set_default_parameter(*network, DELAY_DEFAULT_ARRIVAL_RISE, parm_val);
	delay_set_default_parameter(*network, DELAY_DEFAULT_ARRIVAL_FALL, parm_val);
	break;
      case 'D':
	delay_set_default_parameter(*network, DELAY_DEFAULT_DRIVE_RISE, parm_val);
	delay_set_default_parameter(*network, DELAY_DEFAULT_DRIVE_FALL, parm_val);
	break;
      case 'I':
	delay_set_default_parameter(*network, DELAY_DEFAULT_MAX_INPUT_LOAD, parm_val);
	break;
      case 'L':
	delay_set_default_parameter(*network, DELAY_DEFAULT_OUTPUT_LOAD, parm_val);
	break;
      case 'R':
	delay_set_default_parameter(*network, DELAY_DEFAULT_REQUIRED_RISE, parm_val);
	delay_set_default_parameter(*network, DELAY_DEFAULT_REQUIRED_FALL, parm_val);
	break;
      case 'S':
	delay_set_default_parameter(*network, DELAY_WIRE_LOAD_SLOPE, parm_val);
	break;
      case 'W':
	delay_set_default_parameter(*network, DELAY_ADD_WIRE_LOAD, parm_val);
	break;
      }
      break;
    default:
      goto usage;
    }

    if(set > 1) goto usage;
	
    if(found) {
      if((parm_val = atof(util_optarg)) == HUGE) goto usage;
      if(parm_val < 0.0) parm_val = DELAY_VALUE_NOT_GIVEN;
    }
	
    found = 0;
  }

  node_vec = com_get_true_nodes(*network, argc-util_optind+1, argv+util_optind-1);

  if(set) {
    if(array_n(node_vec) == 0) goto usage;

    for(i = 0; i < array_n(node_vec); ++i) {
      node = array_fetch(node_t *, node_vec, i);
      if(nodetype == INPUTS) {
	if(node_function(node) != NODE_PI) goto usage;
      } else if(nodetype == OUTPUTS) {
	if(node_function(node) != NODE_PO) goto usage;
      } else {
	goto usage;
      }
    }
	
    switch(set_parameter) {
    case ATIME:
      if(nodetype == OUTPUTS) goto usage;
      rise_parm = DELAY_ARRIVAL_RISE;
      fall_parm = DELAY_ARRIVAL_FALL;
      break;
    case DRIVE:
      if(nodetype == OUTPUTS) goto usage;
      rise_parm = DELAY_DRIVE_RISE;
      fall_parm = DELAY_DRIVE_FALL;
      break;
    case MAX_INPUT_LOAD:
      rise_parm = fall_parm = DELAY_MAX_INPUT_LOAD;
      break;
    case LOAD:
      if(nodetype == INPUTS) goto usage;
      rise_parm = fall_parm = DELAY_OUTPUT_LOAD;
      break;
    case RTIME:
      if(nodetype == INPUTS) goto usage;
      rise_parm = DELAY_REQUIRED_RISE;
      fall_parm = DELAY_REQUIRED_FALL;
      break;
    default:
      abort(); /* not reached */
    }
	

    for(i = 0; i < array_n(node_vec); ++i) {
      node = array_fetch(node_t *, node_vec, i);
      delay_set_parameter(node, rise_parm, parm_val);
      delay_set_parameter(node, fall_parm, parm_val);
    }
  }
	
  array_free(node_vec);
  return 0;

 usage:
  (void) fprintf(siserr, 
		 "set_delay [-a|d|i|l|r f]  [-A f] [-D f] [-I f] [-L f] [-R f] [-S f] [-W f] [o1 o2 ... | i1 i2 ...]\n");
  (void) fprintf(siserr, "    -a f\t\tset arrival times to f\n");
  (void) fprintf(siserr, "    -d f\t\tset drives from primary inputs to f\n");
  (void) fprintf(siserr, "    -i f\t\tset max load limit on primary inputs to f\n");
  (void) fprintf(siserr, "    -l f\t\tset loads on primary outputs to f\n");
  (void) fprintf(siserr, "    -r f\t\tset required times to f\n");
  (void) fprintf(siserr, "    -A f\t\tset default arrival time to f\n");
  (void) fprintf(siserr, "    -D f\t\tset default input drive to f\n");
  (void) fprintf(siserr, "    -I f\t\tset default max input load to f\n");
  (void) fprintf(siserr, "    -L f\t\tset default output to f\n");
  (void) fprintf(siserr, "    -R f\t\tset default required time to f\n");
  (void) fprintf(siserr, "    -S f\t\tSet the wire load slope to f\n");
  (void) fprintf(siserr, "    -W f\t\tSet the next wire load to f\n");
  (void) fprintf(siserr, "specify at most one of a,r,l,i,d\n");
  (void) fprintf(siserr, "i1...in a vector of primary inputs\n");
  (void) fprintf(siserr, "o1...on a vector of primary outputs\n");
  (void) fprintf(siserr, "NOTE: a negative value means that the parameter is unspecified\n");
  return 1;
}


init_delay()
{
    com_add_command("print_delay", com_print_delay, /* changes-network */ 0);
    com_add_command("set_delay", com_set_delay, /* changes-network */ 0);
    com_add_command("constraints", com_print_delay_constraints,
		    /* changes-network */ 0);

    node_register_daemon(DAEMON_ALLOC, delay_alloc);
    node_register_daemon(DAEMON_FREE, delay_free);
    node_register_daemon(DAEMON_DUP, delay_dup);
    node_register_daemon(DAEMON_INVALID, delay_invalid);

}


end_delay()
{
}
