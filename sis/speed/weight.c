
#include "speed_int.h"
#include "sis.h"
#include <math.h>

static void weight_compute();

static int speed_weight_bfs();

static void speed_mark_input_cone();

static void linfit();

static void speed_sort2();

static void speed_rank();

static delay_time_t sp_compute_input_req();

delay_time_t *sp_compute_side_req_time();

enum st_retval speed_weight_count_lit();

/*
 * If the return value is 1 only then the components should
 * be combined. If the return is 0 then a large weight needs
 * put on the node
 */

int speed_weight(node, speed_param, t_cost, a_cost) node_t *node;
speed_global_t *speed_param;
double *t_cost;
int *a_cost;
{
  double max_a, min_a, min_d, mean_a, mean_d; /* For scaling */
  double m, o_time, a_time, d_time;           /* For correlation */
  double s_dev, tf, crit_frac, area_cost;
  double rad, sum_a, sum_d, co, angle, p_angle, slope;
  int i, n, crit;
  delay_time_t t;
  st_table *table;
  array_t *a_array, *d_array;
  /* Some day do special casing of a Boolean nature */
  /*
    int to_cofactor;
    */

  a_array = array_alloc(double, 0);
  d_array = array_alloc(double, 0);
  min_a = min_d = POS_LARGE;
  max_a = NEG_LARGE;
  sum_a = sum_d = rad = 0.0;

  table = st_init_table(st_ptrcmp, st_ptrhash);

  t = delay_arrival_time(node);
  o_time = D_MAX(t.rise, t.fall);

  *a_cost = 0;
  crit = 0;
  n = 0;

  (void)speed_weight_bfs(node, speed_param, table, a_array, d_array, o_time,
                         &crit, &n, &min_a, &max_a, &min_d, &sum_a, &sum_d,
                         &area_cost);
  *a_cost = (int)ceil(area_cost);

  /*
   * Compute the t_frac as based on the Distribution
   * of the delays and a_times in a 2-D plane
   */

  if (n > 2) {
    m = (double)n;
    /*
      mean_a = (sum_a / m ) - min_a;
      mean_d = (sum_d / m ) - min_d;
      */
    mean_a = (sum_a / m);
    mean_d = (sum_d / m);

    for (i = 0; i < n; i++) {
      a_time = array_fetch(double, a_array, i) - mean_a;
      d_time = array_fetch(double, d_array, i) - mean_d;

      rad += (pow(d_time, 2.0) + pow(a_time, 2.0));
    }
    /*
     * Get the standard deviation (preferred over the absolute
     * deviation) since we want a large separation and the
     * angle from the linear regression.
     */
    s_dev = sqrt(rad) / m;

    if (min_a == max_a) {
      angle = SP_PI_2;
    } else {
      (void)linfit(a_array, d_array, min_a, min_d, &slope);
      angle = atan(slope);
    }

    if (angle > SP_PI_4) {
      co = 4 - (4 * angle * SP_1_PI);
    } else {
      co = 2 + (4 * angle * SP_1_PI);
    }
    co = MAX(co, 0.3);

    tf = co * pow((s_dev / (double)speed_param->dist), 2.0);
    /*
      tf = pow(co, 2.0) * (s_dev / (double)speed_param->dist);
      */
    /*
      tf = co  *  (s_dev / (double)speed_param->dist);
      */
  } else {
    tf = -1.0;
    s_dev = 0.0; /* Default */
    angle = -SP_PI;
  }

  if (speed_param->debug) {
    p_angle = angle * 180 * SP_1_PI;
    (void)fprintf(
        sisout, "%-10s T_comp =(%4.0f:%4.2f) Area = %d   %3d d_fanin inputs \n",
        node_name(node), p_angle, s_dev, *a_cost, array_n(a_array));
  }

  array_free(d_array);
  array_free(a_array);
  st_free_table(table);

  crit_frac = (n > 0 ? ((double)(crit) / (double)(n)) : 1);

  /*
   * tf < 0 => this node should never be considered
   */
  if (tf <= 0) {
    *t_cost = POS_LARGE;
    return 0;
  } else {
    *t_cost = (MAXWEIGHT * crit_frac) / tf;
    return 1;
  }
}

static int speed_weight_bfs(node, speed_param, table, a_array, d_array, o_time,
                            crit, n, min_a, max_a, min_d, sum_a, sum_d,
                            area) node_t *node;
speed_global_t *speed_param;
int *crit, *n;
st_table *table;
array_t *a_array, *d_array;
double o_time;
double *sum_a, *sum_d, *min_a, *min_d, *max_a;
double *area;
{
  int i, j, k, first, last;
  int to_cofactor = FALSE;
  int more_to_come;
  int dist = speed_param->dist;
  array_t *temp_array;
  st_table *area_table, *duplicated_nodes;
  node_t *fi, *no;

  temp_array = array_alloc(node_t *, 0);
  area_table = st_init_table(st_ptrcmp, st_ptrhash);
  array_insert_last(node_t *, temp_array, node);
  (void)st_insert(table, (char *)node, (char *)(dist + 1));
  first = 0;
  more_to_come = TRUE;
  for (i = dist; (i > 0) && more_to_come; i--) {
    more_to_come = FALSE;
    last = array_n(temp_array);
    for (j = first; j < last; j++) {
      no = array_fetch(node_t *, temp_array, j);
      foreach_fanin(no, k, fi) {
        if (node_function(fi) == NODE_PI) {
          if (!st_insert(table, (char *)fi, (char *)0)) {
            if (speed_critical(fi, speed_param))
              (*crit)++;
            (*n)++;
            weight_compute(fi, speed_param->model, a_array, d_array, o_time,
                           min_a, max_a, min_d, sum_a, sum_d);
          }
        } else if (!st_is_member(table, (char *)fi)) {
          /* Not yet visited */
          if (speed_critical(fi, speed_param)) {
            /*
             * Add to elements that will be processed
             */
            (void)st_insert(table, (char *)fi, (char *)i);
            array_insert_last(node_t *, temp_array, fi);
            more_to_come = TRUE;
          } else {
            /*
             * Will be a fanin to collapsed node
             */
            (void)st_insert(table, (char *)fi, (char *)0);
            (*n)++;
            weight_compute(fi, speed_param->model, a_array, d_array, o_time,
                           min_a, max_a, min_d, sum_a, sum_d);
          }
        }
      }
    }
    first = last;
  }

  /*
   * For the nodes in the array  see if they
   * fanout elsewhere. If so then mark and count the
   * the nodes in the fanin cone
   */
  duplicated_nodes = st_init_table(st_ptrcmp, st_ptrhash);
  *area = sp_compute_duplicated_area(temp_array, table, duplicated_nodes,
                                     DELAY_MODEL_UNKNOWN);
  st_free_table(duplicated_nodes);

  /*
   * Now the elements added in the last loop are
   * the inputs to the collapsed node.
   */

  for (i = first; i < array_n(temp_array); i++) {
    no = array_fetch(node_t *, temp_array, i);
    foreach_fanin(no, k, fi) {
      if (!st_insert(table, (char *)fi, (char *)0)) {
        if (speed_critical(fi, speed_param))
          (*crit)++;
        (*n)++;
        weight_compute(fi, speed_param->model, a_array, d_array, o_time, min_a,
                       max_a, min_d, sum_a, sum_d);
      }
    }
  }

  array_free(temp_array);
  st_free_table(area_table);
  return to_cofactor;
}

/*
 * Given a node that fans out of the collapsed logic... mark the part of
 * the collapsed logic that needs to be duplicate. In addition, if the
 * model != DELAY_MODEL_UNKNOWN, then also compute the required times of side
 * output paths and store it alongwith the entry for the duplicated nodes.
 */
static void speed_mark_input_cone(node, area_table, table, model) node_t *node;
st_table *area_table, *table;
delay_model_t model; /* If not UNKNOWN, store required times of side outputs */
{
  array_t *array;
  int i, j, first = 0, last;
  int more_to_come = TRUE;
  delay_time_t *req;
  node_t *temp, *fi;

  array = array_alloc(node_t *, 0);
  array_insert_last(node_t *, array, node);
  while (more_to_come) {
    more_to_come = FALSE;
    last = array_n(array);
    for (i = first; i < last; i++) {
      temp = array_fetch(node_t *, array, i);
      foreach_fanin(temp, j, fi) {
        if (st_is_member(table, (char *)fi)) {
          more_to_come = TRUE;
          array_insert_last(node_t *, array, fi);
          (void)st_insert(area_table, (char *)fi, NIL(char));
        }
      }
      first = last;
    }
  }
  if (model != DELAY_MODEL_UNKNOWN) {
    req = sp_compute_side_req_time(node, table, area_table, model);
  } else {
    req = NIL(delay_time_t);
  }
  (void)st_insert(area_table, (char *)node, (char *)req);
  array_free(array);
}

static void linfit(xin, yin, xmin, ymin, slope) array_t *yin, *xin;
double *slope, xmin, ymin;
{
  double a = 0, d = 0, aa = 0, ad = 0, a_dev;
  double *x, *y, m;
  int num, j;

  num = array_n(xin);

  if (num != array_n(yin)) {
    (void)fprintf(siserr, " Arrays not of same size \n");
    exit(-1);
  }
  x = ALLOC(double, num + 1);
  y = ALLOC(double, num + 1);

  for (j = 1; j <= num; j++) {
    x[j] = (array_fetch(double, xin, j - 1) - xmin);
    y[j] = (array_fetch(double, yin, j - 1) - ymin);
  }

  /* Work with ranks rather than the delay values */
  speed_sort2(num, y, x);
  speed_rank(num, y);
  speed_sort2(num, x, y);
  speed_rank(num, x);

  /* Compute the least square fit */
  for (j = 1; j <= num; j++) {
    a += x[j];
    d += y[j];
    aa += x[j] * x[j];
    ad += x[j] * y[j];
  }
  m = (double)num;

  a_dev = (aa / m) - ((a * a) / (m * m));
  *slope = ((ad / m) - ((a * d) / (m * m))) / (sqrt(a_dev));

  FREE(x);
  FREE(y);
}

static void speed_sort2(n, ra, rb) int n;
double ra[], rb[];
{
  int i, j, ir, l;
  double rra, rrb;

  l = (n >> 1) + 1;
  ir = n;
  for (;;) {
    if (l > 1) {
      rra = ra[--l];
      rrb = rb[l];
    } else {
      rra = ra[ir];
      rrb = rb[ir];
      ra[ir] = ra[1];
      rb[ir] = rb[1];
      if (--ir == 1) {
        ra[1] = rra;
        rb[1] = rrb;
        return;
      }
    }
    i = l;
    j = l << 1;
    while (j <= ir) {
      if (j < ir && ra[j] < ra[j + 1])
        ++j;
      if (rra < ra[j]) {
        ra[i] = ra[j];
        rb[i] = rb[j];
        j += (i = j);
      } else
        j = ir + 1;
    }
    ra[i] = rra;
    rb[i] = rrb;
  }
}

static void speed_rank(n, w) double w[];
int n;
{
  int j = 1, ji, jt;
  double rank;

  while (j < n) {
    if (w[j + 1] != w[j]) {
      w[j] = (double)j;
      ++j;
    } else {
      for (jt = j + 1; jt < n; jt++) {
        if (w[jt] != w[j])
          break;
      }
      rank = 0.5 * (j + jt - 1);
      for (ji = j; ji <= (jt - 1); ji++) {
        w[ji] = rank;
      }
      j = jt;
    }
  }
  if (j == n)
    w[n] = n;
}

/*
 * For the node, gets the slacks and arrival times.
 * Updates the min, max and sums and inserts the
 * delay and arrival times in the corresponding arrays
 */
static void weight_compute(node, model, a_array, d_array, o_time, min_a, max_a,
                           min_d, sum_a, sum_d) node_t *node;
delay_model_t model;
array_t *a_array, *d_array;
double o_time;
double *min_a, *max_a, *min_d, *sum_a, *sum_d;
{
  delay_time_t at, st;
  double d_time, a_time, s_time;

  st = delay_slack_time(node);
  at = delay_arrival_time(node);

  a_time = D_MIN(at.rise, at.fall);
  s_time = D_MIN(st.rise, st.fall);

  d_time = o_time - s_time - a_time;
  *min_a = D_MIN(*min_a, a_time);
  *max_a = D_MAX(*max_a, a_time);
  *min_d = D_MIN(*min_d, d_time);

  (void)array_insert_last(double, a_array, a_time);
  (void)array_insert_last(double, d_array, d_time);

  *sum_a += a_time;
  *sum_d += d_time;
}

/* ARGSUSED */
enum st_retval speed_weight_count_lit(key, value, arg) char *key, *value, *arg;
{
  lib_gate_t *gate;
  node_t *node = (node_t *)key;
  double *in = (double *)arg;

  if (node->type == INTERNAL) {
    if ((gate = lib_gate_of(node)) != NIL(lib_gate_t)) {
      *in += lib_gate_area(gate);
    } else {
      *in += node_num_literal(node);
    }
  }

  return ST_CONTINUE;
}

/*
 * For the nodes in temp_array (all these nodes should be part of the table)
 * that is also passed into this routine, find the area of duplicated logic
 * The first node in the array is the root node...
 * The duplicated nodes are returned as part of "area_table"...
 * if the model is not "DELAY_MODEL_UNKNOWN", then additional work is done
 * to find the required time generated by the side-paths to the outputs.
 * (Needed to estimate the increase in load tolerated at the inputs)
 */
double sp_compute_duplicated_area(array, table, area_table,
                                  model) array_t *array;
st_table *table;
st_table *area_table; /* Filled with the nodes that are duplicated */
delay_model_t model;  /* If not UNKNOWN, then store required times too */
{
  lsGen gen;
  double area;
  node_t *temp, *fo;
  int i, fanout_flag;

  for (i = 1 /* ignore the root */; i < array_n(array); i++) {
    temp = array_fetch(node_t *, array, i);
    if (!st_is_member(area_table, (char *)temp)) {
      if (node_num_fanout(temp) > 1) {
        fanout_flag = FALSE;
        foreach_fanout(temp, gen, fo) {
          if (!st_is_member(table, (char *)fo)) {
            fanout_flag = TRUE;
            (void)lsFinish(gen);
            break;
          }
        }
        if (fanout_flag) {
          speed_mark_input_cone(temp, area_table, table, model);
        }
      }
    }
  }
  area = 0.0;
  st_foreach(area_table, speed_weight_count_lit, (char *)&area);

  return area;
}

delay_time_t *sp_compute_side_req_time(node, table, dup_table,
                                       model) node_t *node;
st_table *table;     /* table of nodes to collapse */
st_table *dup_table; /* Table of nodes already marked as duplicated */
delay_model_t model; /* delay model to compute delays with */
{
  int pin;
  lsGen gen;
  node_t *fo;
  delay_time_t *po_req, *req_time, cur_req;

  req_time = ALLOC(delay_time_t, 1);
  req_time->rise = req_time->fall = POS_LARGE;

  foreach_fanout_pin(node, gen, fo, pin) {
    if (st_lookup(dup_table, (char *)fo, (char **)&po_req)) {
      /*
       * The required time for that fanout is already computed
       */
      cur_req = sp_compute_input_req(fo, pin, model, *po_req);
    } else if (!st_is_member(table, (char *)fo)) {
      /*
       * There exists a path ...  node-fo ... to some other PO
       */
      cur_req = sp_compute_input_req(fo, pin, model, delay_required_time(fo));
    } else {
      /*
       * The node is not duplicated and will be collapsed... So,
       * the only path to output is through the node beiong collapsed
       */
      continue;
    }
    req_time->rise = MIN(req_time->rise, cur_req.rise);
    req_time->fall = MIN(req_time->fall, cur_req.fall);
  }

  return req_time;
}

/*
 * Given the required time "fo_req" at the output of a node "fo", compute
 * the required time at the "pin" input of that node...
 */
static delay_time_t sp_compute_input_req(fo, pin, model, fo_req) node_t *fo;
int pin;
delay_model_t model;
delay_time_t fo_req;
{
  delay_time_t delay;
  input_phase_t phase;
  delay_time_t input_req;

  input_req = fo_req;
  if (fo->type == PRIMARY_OUTPUT) {
    return input_req; /* Cant call node_input_phase() for PO nodes */
  }
  delay = delay_node_pin(fo, pin, model);
  phase = node_input_phase(fo, node_get_fanin(fo, pin));

  switch (phase) {
  case POS_UNATE:
    input_req.rise = fo_req.rise - delay.rise;
    input_req.fall = fo_req.fall - delay.fall;
    break;
  case NEG_UNATE:
    input_req.fall = fo_req.rise - delay.rise;
    input_req.rise = fo_req.fall - delay.fall;
    break;
  case BINATE:
    input_req.rise = fo_req.rise - MAX(delay.rise, delay.fall);
    input_req.fall = fo_req.fall - MAX(delay.rise, delay.fall);
    break;
  case PHASE_UNKNOWN:
  default:
    break;
  }
  return input_req;
}
