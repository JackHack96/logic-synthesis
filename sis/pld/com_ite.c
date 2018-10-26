#include "sis.h"
#include "pld_int.h"
#include "ite_int.h"


/* adding these global variables in, so as to 
   experiment with the variable selection weight
   assignment */

int ACT_ITE_ALPHA, ACT_ITE_GAMMA;
int UNATE_SELECT;

int ACT_ITE_DEBUG;
int ACT_ITE_STATISTICS;
int USE_FAC_WHEN_UNATE;

/* int act_is_or_used; */ /* defined in com_pld.c */

com_act_ite_map(network, argc, argv)
  network_t **network;
  int argc;
  char **argv;
{
  int c;
  act_init_param_t *init_param;
  
  init_param = ALLOC(act_init_param_t, 1);

  /* default */
  /*---------*/
  ACT_ITE_DEBUG = 0;
  ACT_ITE_STATISTICS = 0;
  act_is_or_used = 1;
  init_param->HEURISTIC_NUM = 0;
  init_param->FANIN_COLLAPSE = 3;
  init_param->COLLAPSE_FANINS_OF_FANOUT = 15;
  init_param->DECOMP_FANIN = 4;
  init_param->NUM_ITER = 0;
  init_param->COST_LIMIT = 3;
  init_param->LAST_GASP = 0;
  init_param->map_alg = 1;
  init_param->lit_bound = 200;
  init_param->ITE_FANIN_LIMIT_FOR_BDD = 40;
  init_param->COLLAPSE_UPDATE = INEXPENSIVE;
  init_param->COLLAPSE_METHOD = OLD;
  init_param->DECOMP_METHOD = USE_GOOD_DECOMP;
  init_param->ALTERNATE_REP = 0; /* do not use it */
  init_param->MAP_METHOD = NEW;
  init_param->VAR_SELECTION_LIT = 15;
  init_param->BREAK = 0;
  
  /* the default values are the ones which were used 
     before these variables were introduced         */
  /*------------------------------------------------*/
  ACT_ITE_ALPHA = 2;
  ACT_ITE_GAMMA = 1;
  USE_FAC_WHEN_UNATE = 1; /* to be used with V > 0 */
  MAXOPTIMAL = 6;
  UNATE_SELECT = 0;

  util_getopt_reset();
  while(( c = util_getopt(argc, argv, "A:C:F:G:M:V:b:d:f:h:l:m:n:v:DLNUacorsuw")) != EOF){
      switch(c){ 
        case 'A':
          ACT_ITE_ALPHA = atoi(util_optarg);
          break;
        case 'C':
          init_param->COST_LIMIT = atoi(util_optarg);
          break;
        case 'D':
          init_param->DECOMP_METHOD = USE_FACTOR;
          break;
        case 'F':
          init_param->COLLAPSE_FANINS_OF_FANOUT = atoi(util_optarg);
          break;
        case 'G':
          ACT_ITE_GAMMA = atoi(util_optarg);
          break;
        case 'L':
          init_param->LAST_GASP = 1;
          break;
        case 'M':
          MAXOPTIMAL = atoi(util_optarg);
          break;
        case 'N':
          init_param->COLLAPSE_METHOD = NEW;
          break;
        case 'U':
          init_param->COLLAPSE_UPDATE = EXPENSIVE;
          break;
        case 'V':
          init_param->VAR_SELECTION_LIT = atoi(util_optarg);
          break;
        case 'a':
          init_param->ALTERNATE_REP = 1;
          break;
        case 'b':
          init_param->ITE_FANIN_LIMIT_FOR_BDD = atoi(util_optarg);
          break;
        case 'c':
          init_param->map_alg = 0;
          break;
        case 'd':
          init_param->DECOMP_FANIN = atoi(util_optarg);
          break;
        case 'f':
          init_param->FANIN_COLLAPSE = atoi(util_optarg);
          break;
        case 'h':
          init_param->HEURISTIC_NUM = atoi(util_optarg);
          if (init_param->HEURISTIC_NUM != 0) {
              (void) printf("only -h 0 supported\n");
              return 1;
          }
          break;
        case 'l':
          init_param->lit_bound = atoi(util_optarg);
          break;
        case 'm':
          init_param->MAP_METHOD = atoi(util_optarg);
          break;
        case 'n':
          init_param->NUM_ITER = atoi(util_optarg);
          break;
        case 'o':
          act_is_or_used = 0;
          break;
        case 'r':
          init_param->BREAK = 1;
          break;
        case 's':
          ACT_ITE_STATISTICS = 1;
          break;
        case 'u':
          USE_FAC_WHEN_UNATE = 0;
          break;
        case 'v':
          ACT_ITE_DEBUG = atoi(util_optarg);
          break;
        case 'w':
          UNATE_SELECT = 1;
          break;
        default:
          FREE(init_param);
          (void) fprintf(sisout, "usage: ite_map [-o (or_gate_not_used)] [-h HEURISTIC_NUM] [-d DECOMP_FANIN] [-c (complete_matchin)] \n");
          (void) fprintf(sisout, "\t[-f FANIN_COLLAPSE] [F COLLAPSE_FANINS_OF_FANOUT] [-n NUM_ITER] [-s (STATISTICS)] [-L (last_gasp)] [-N (collapse_method NEW)]\n");
          (void) fprintf(sisout, "\t[-a (use bdd as alternate representation)] [-U (make collapse_update expensive)] [-v verbosity level]\n");          
          (void) fprintf(sisout, "[-m MAP_METHOD 0:old,1:new,2:with_iter3:with_just_decomp] [-V new var selection method - lit count] HEURISTIC_NUM\t0 => ite, 1 => canonical ite (default 0)\n");
          (void) fprintf(sisout, "\t[-D (DECOMP_METHOD: use factored form)] [-w (unate_select = cover)]\n");
          return 1;
      }
  }
  /* some default values */
  /*---------------------*/
  init_param->mode = 0.0;
  init_param->GAIN_FACTOR = 0.01;

  (void) act_ite_map_network_with_iter(*network, init_param);

  FREE(init_param);
  return 0;
}

com_act_ite_create_and_map_mroot_network(network, argc, argv)
  network_t **network;
  int argc;
  char **argv;
{
  int c;
  act_init_param_t *init_param;
  
  init_param = ALLOC(act_init_param_t, 1);

  /* default */
  /*---------*/
  ACT_ITE_DEBUG = 0;
  ACT_ITE_STATISTICS = 0;
  act_is_or_used = 1;
  init_param->HEURISTIC_NUM = 0;
  init_param->FANIN_COLLAPSE = 3;
  init_param->COLLAPSE_FANINS_OF_FANOUT = 15;
  init_param->DECOMP_FANIN = 4;
  init_param->NUM_ITER = 0;
  init_param->COST_LIMIT = 3;
  init_param->LAST_GASP = 0;
  init_param->map_alg = 1;
  init_param->lit_bound = 200;
  init_param->ITE_FANIN_LIMIT_FOR_BDD = 40;
  init_param->COLLAPSE_UPDATE = INEXPENSIVE;
  init_param->COLLAPSE_METHOD = OLD;
  init_param->DECOMP_METHOD = USE_GOOD_DECOMP;
  init_param->ALTERNATE_REP = 0; /* do not use it */
  init_param->MAP_METHOD = NEW;
  init_param->VAR_SELECTION_LIT = 15;

  /* the default values are the ones which were used 
     before these variables were introduced         */
  /*------------------------------------------------*/
  ACT_ITE_ALPHA = 2;
  ACT_ITE_GAMMA = 1;
  USE_FAC_WHEN_UNATE = 1; /* to be used with V > 0 */
  MAXOPTIMAL = 6;

  util_getopt_reset();
  while(( c = util_getopt(argc, argv, "A:C:F:G:M:V:b:d:f:h:l:m:n:v:DLNUacorsu")) != EOF){
      switch(c){ 
        case 'A':
          ACT_ITE_ALPHA = atoi(util_optarg);
          break;
        case 'C':
          init_param->COST_LIMIT = atoi(util_optarg);
          break;
        case 'D':
          init_param->DECOMP_METHOD = USE_FACTOR;
          break;
        case 'F':
          init_param->COLLAPSE_FANINS_OF_FANOUT = atoi(util_optarg);
          break;
        case 'G':
          ACT_ITE_GAMMA = atoi(util_optarg);
          break;
        case 'L':
          init_param->LAST_GASP = 1;
          break;
        case 'M':
          MAXOPTIMAL = atoi(util_optarg);
          break;
        case 'N':
          init_param->COLLAPSE_METHOD = NEW;
          break;
        case 'U':
          init_param->COLLAPSE_UPDATE = EXPENSIVE;
          break;
        case 'V':
          init_param->VAR_SELECTION_LIT = atoi(util_optarg);
          break;
        case 'a':
          init_param->ALTERNATE_REP = 1;
          break;
        case 'b':
          init_param->ITE_FANIN_LIMIT_FOR_BDD = atoi(util_optarg);
          break;
        case 'c':
          init_param->map_alg = 0;
          break;
        case 'd':
          init_param->DECOMP_FANIN = atoi(util_optarg);
          break;
        case 'f':
          init_param->FANIN_COLLAPSE = atoi(util_optarg);
          break;
        case 'h':
          init_param->HEURISTIC_NUM = atoi(util_optarg);
          if (init_param->HEURISTIC_NUM != 0) {
              (void) printf("only -h 0 supported\n");
              return 1;
          }
          break;
        case 'l':
          init_param->lit_bound = atoi(util_optarg);
          break;
        case 'm':
          init_param->MAP_METHOD = atoi(util_optarg);
          break;
        case 'n':
          init_param->NUM_ITER = atoi(util_optarg);
          break;
        case 'o':
          act_is_or_used = 0;
          break;
        case 'r':
          init_param->BREAK = 1;
          break;
        case 's':
          ACT_ITE_STATISTICS = 1;
          break;
        case 'u':
          USE_FAC_WHEN_UNATE = 0;
          break;
        case 'v':
          ACT_ITE_DEBUG = atoi(util_optarg);
          break;
        default:
          FREE(init_param);
          (void) fprintf(sisout, "usage: ite_map_mroot [-o (or_gate_not_used)] [-h HEURISTIC_NUM] [-d DECOMP_FANIN] [-c (complete_matchin)] \n");
          (void) fprintf(sisout, "\t[-f FANIN_COLLAPSE] [F COLLAPSE_FANINS_OF_FANOUT] [-n NUM_ITER] [-s (STATISTICS)] [-L (last_gasp)] [-N (collapse_method NEW)]\n");
          (void) fprintf(sisout, "\t[-a (use bdd as alternate representation)] [-U (make collapse_update expensive)] [-v verbosity level]\n");          
          (void) fprintf(sisout, "[-m MAP_METHOD 0:old,1:new,2:with_iter3:with_just_decomp] [-V new var selection method - lit count] HEURISTIC_NUM\t0 => ite, 1 => canonical ite (default 0)\n");
          (void) fprintf(sisout, "\t[-D (DECOMP_METHOD: use factored form)] \n");
          return 1;
      }
  }
  /* some default values */
  /*---------------------*/
  init_param->mode = 0.0;
  init_param->GAIN_FACTOR = 0.01;

  (void) act_ite_create_and_map_mroot_network(*network, init_param);
  FREE(init_param);
  return 0;
}

com_act_ite_mux_network(network, argc, argv)
  network_t **network;
  int argc;
  char **argv;
{
  int c;
  float FAC_TO_SOP_RATIO;
  act_init_param_t *init_param;
  
  init_param = ALLOC(act_init_param_t, 1);

  /* default */
  /*---------*/
  ACT_ITE_DEBUG = 0;
  ACT_ITE_STATISTICS = 0;
  act_is_or_used = 1;
  init_param->HEURISTIC_NUM = 0;
  init_param->FANIN_COLLAPSE = 3;
  init_param->COLLAPSE_FANINS_OF_FANOUT = 15;
  init_param->DECOMP_FANIN = 4;
  init_param->NUM_ITER = 0;
  init_param->COST_LIMIT = 3;
  init_param->LAST_GASP = 0;
  init_param->map_alg = 1;
  init_param->lit_bound = 200;
  init_param->ITE_FANIN_LIMIT_FOR_BDD = 40;
  init_param->COLLAPSE_UPDATE = INEXPENSIVE;
  init_param->COLLAPSE_METHOD = OLD;
  init_param->DECOMP_METHOD = USE_GOOD_DECOMP;
  init_param->ALTERNATE_REP = 0; /* do not use it */
  init_param->MAP_METHOD = NEW;
  init_param->VAR_SELECTION_LIT = 15;
  init_param->BREAK = 0;

  /* the default values are the ones which were used 
     before these variables were introduced         */
  /*------------------------------------------------*/
  ACT_ITE_ALPHA = 2;
  ACT_ITE_GAMMA = 1;
  USE_FAC_WHEN_UNATE = 1; /* to be used with V > 0 */
  MAXOPTIMAL = 6;
  FAC_TO_SOP_RATIO = 0.7;

  util_getopt_reset();
  while(( c = util_getopt(argc, argv, "A:C:F:G:M:R:V:b:d:f:h:l:m:n:v:DLNUacorsu")) != EOF){
      switch(c){ 
        case 'A':
          ACT_ITE_ALPHA = atoi(util_optarg);
          break;
        case 'C':
          init_param->COST_LIMIT = atoi(util_optarg);
          break;
        case 'D':
          init_param->DECOMP_METHOD = USE_FACTOR;
          break;
        case 'F':
          init_param->COLLAPSE_FANINS_OF_FANOUT = atoi(util_optarg);
          break;
        case 'G':
          ACT_ITE_GAMMA = atoi(util_optarg);
          break;
        case 'L':
          init_param->LAST_GASP = 1;
          break;
        case 'M':
          MAXOPTIMAL = atoi(util_optarg);
          break;
        case 'N':
          init_param->COLLAPSE_METHOD = NEW;
          break;
        case 'R':
          FAC_TO_SOP_RATIO = atof(util_optarg);
          break;
        case 'U':
          init_param->COLLAPSE_UPDATE = EXPENSIVE;
          break;
        case 'V':
          init_param->VAR_SELECTION_LIT = atoi(util_optarg);
          break;
        case 'a':
          init_param->ALTERNATE_REP = 1;
          break;
        case 'b':
          init_param->ITE_FANIN_LIMIT_FOR_BDD = atoi(util_optarg);
          break;
        case 'c':
          init_param->map_alg = 0;
          break;
        case 'd':
          init_param->DECOMP_FANIN = atoi(util_optarg);
          break;
        case 'f':
          init_param->FANIN_COLLAPSE = atoi(util_optarg);
          break;
        case 'h':
          init_param->HEURISTIC_NUM = atoi(util_optarg);
          if (init_param->HEURISTIC_NUM != 0) {
              (void) printf("only -h 0 supported\n");
              return 1;
          }
          break;
        case 'l':
          init_param->lit_bound = atoi(util_optarg);
          break;
        case 'm':
          init_param->MAP_METHOD = atoi(util_optarg);
          break;
        case 'n':
          init_param->NUM_ITER = atoi(util_optarg);
          break;
        case 'o':
          act_is_or_used = 0;
          break;
        case 'r':
          init_param->BREAK = 1;
          break;
        case 's':
          ACT_ITE_STATISTICS = 1;
          break;
        case 'u':
          USE_FAC_WHEN_UNATE = 0;
          break;
        case 'v':
          ACT_ITE_DEBUG = atoi(util_optarg);
          break;
        default:
          FREE(init_param);
          (void) fprintf(sisout, "usage: ite_mux [-o (or_gate_not_used)] [-h HEURISTIC_NUM] [-d DECOMP_FANIN] [-c (complete_matchin)] \n");
          (void) fprintf(sisout, "\t[-f FANIN_COLLAPSE] [F COLLAPSE_FANINS_OF_FANOUT] [-n NUM_ITER] [-s (STATISTICS)] [-L (last_gasp)] [-N (collapse_method NEW)]\n");
          (void) fprintf(sisout, "\t[-a (use bdd as alternate representation)] [-U (make collapse_update expensive)] [-v verbosity level]\n");          
          (void) fprintf(sisout, "[-m MAP_METHOD 0:old,1:new,2:with_iter3:with_just_decomp] [-V new var selection method - lit count] HEURISTIC_NUM\t0 => ite, 1 => canonical ite (default 0)\n");
          (void) fprintf(sisout, "\t[-D (DECOMP_METHOD: use factored form)] [-R fac_to_sop_ratio]\n");
          return 1;
      }
  }
  /* some default values */
  /*---------------------*/
  init_param->mode = 0.0;
  init_param->GAIN_FACTOR = 0.01;

  (void) act_ite_mux_network(*network, init_param, FAC_TO_SOP_RATIO);
  FREE(init_param);
  return 0;
}

int
com_act_bool_map_network(network, argc, argv)
network_t **network;
int argc;
char **argv;
{
  int c;
  int map_alg;  
  int print_flag;
  int act_is_or_used; /* though a global variable too - Added Jan 29 '93 */

  /* default */
  /*---------*/
  map_alg = 1; /* do just algebraic matching */
  print_flag = 1; /* print the matches */
  act_is_or_used = 1; /* or gate is used */

  util_getopt_reset();
  while(( c = util_getopt(argc, argv, "cop")) != EOF){
      switch(c){ 
        case 'c':
          map_alg = 0;
          break;
        case 'o':
          act_is_or_used = 0;
          break;
        case 'p':
          print_flag = 0;
          break;
        default: 
          (void) fprintf(sisout, "usage: act_bool [-c (complete_matching)] [-p (do not print matches)]\n");
          return 1;
      }
  }
  ACT_ITE_DEBUG = print_flag; /* put Jan 11 '93 */
  act_bool_map_network(*network, map_alg, act_is_or_used, print_flag);
  return 0;
}

init_ite()
{
  com_add_command("ite_map", com_act_ite_map, 1);
  com_add_command("_ite_mux", com_act_ite_mux_network, 1);
  com_add_command("_act_bool", com_act_bool_map_network, 0);

  node_register_daemon(DAEMON_ALLOC, act_ite_alloc);
  node_register_daemon(DAEMON_FREE, act_ite_free);

}

end_ite()
{
}

void
act_ite_alloc(node)
  node_t *node;
{
  ACT_ITE_COST_STRUCT *cost_node;

  cost_node = ALLOC(ACT_ITE_COST_STRUCT, 1);
  cost_node->node = node;
  cost_node->cost = 0;
  cost_node->arrival_time = (double) 0.0;
  cost_node->required_time = (double) 0.0;
  cost_node->slack = (double) 0.0;
  cost_node->is_critical = 0;
  cost_node->area_weight = (double) 0.0;
  cost_node->cost_and_arrival_time = (double) 0.0;
  cost_node->ite = NIL(ite_vertex);
  /* cost_node->will_ite = (char *) ite_alloc(); */
  cost_node->will_ite = NIL (char);
  cost_node->act = NIL(ACT_VERTEX); /* added Feb 8, 1992 */
  cost_node->match = NIL (ACT_MATCH);
  cost_node->network = NIL (network_t);
  ACT_DEF_ITE_SLOT(node) = (char *) cost_node;
}

void
act_ite_free(node)
  node_t *node;
{
  ACT_ITE_COST_STRUCT *cost_node;
  
  cost_node = ACT_ITE_SLOT(node);
  /* ite_free(cost_node->will_ite); */
  FREE(cost_node);
}




