
#include "sis.h"

static void check_error(s, node) char *s;
node_t *node;
{
  error_append("network_check: inconsistency detected");
  if (node != 0) {
    error_append(" at ");
    error_append(node_name(node));
  }
  error_append(" -- ");
  error_append(s);
  error_append("\n");
}

int network_check(network) network_t *network;
{
  int i, pin, found_pin, warn;
  lsGen gen, gen1;
  node_t *p, *p1, *fanin, *fanout, *found_fanout;
  st_table *table;
  char *key, *value, *data;

  warn = 0;

  /* Make sure each node is correct wrt its flags */
  foreach_node(network, gen, p) {
    i = node_check(p);
    if (i == 0)
      return 0;
    if (i == 2)
      warn = 1;
  }

  /* check that every node is in the network, and its fanin is as well */
  foreach_node(network, gen, p) {
    if (p->network != network) {
      check_error("node on network list not in network", p);
      return 0;
    }
    gen1 = lsGenHandle(p->net_handle, &data, LS_BEFORE);
    if ((node_t *)data != p) {
      check_error("net handle messed up", p);
      return 0;
    }
    LS_ASSERT(lsFinish(gen1));
    foreach_fanin(p, i, fanin) {
      if (fanin->network != network) {
        check_error("a fanin of a node is not in network", p);
        return 0;
      }
    }
    foreach_fanout(p, gen1, fanout) {
      if (fanout->network != network) {
        check_error("a fanout of a node is not in network", p);
        return 0;
      }
    }
  }

  foreach_primary_output(network, gen, p) {
    if (p->type != PRIMARY_OUTPUT) {
      check_error("node on PO list is not type PO", p);
      return 0;
    }
    if (node_num_fanin(p) != 1) {
      check_error("node on PO list has fanin not 1", p);
      return 0;
    }
    if (node_num_fanout(p) != 0) {
      check_error("node on PO list has fanout not 0", p);
      return 0;
    }
  }

  foreach_primary_input(network, gen, p) {
    if (p->type != PRIMARY_INPUT) {
      check_error("node on PI list is not type PI", p);
      return 0;
    }
    if (node_num_fanin(p) != 0) {
      check_error("node on PI list has fanin not 0", p);
      return 0;
    }
  }

  foreach_node(network, gen, p) {
    switch (p->type) {
    case PRIMARY_OUTPUT:
      foreach_primary_output(network, gen1, p1) {
        if (p == p1) {
          LS_ASSERT(lsFinish(gen1));
          break;
        }
      }
      if (p != p1) {
        check_error("node has type PO, but is not on PO list", p);
        return 0;
      }
      break;

    case PRIMARY_INPUT:
      foreach_primary_input(network, gen1, p1) {
        if (p == p1) {
          LS_ASSERT(lsFinish(gen1));
          break;
        }
      }
      if (p != p1) {
        check_error("node has type PI, but is not on PI list", p);
        return 0;
      }
      break;

    case INTERNAL:
      foreach_fanin(p, i, fanin) {
        found_fanout = 0;
        found_pin = -1;
        foreach_fanout_pin(fanin, gen1, p1, pin) {
          if (p == p1 && i == pin) {
            if (found_fanout != 0) {
              check_error("fanout duplicated for some node", p);
              return 0;
            }
            found_fanout = p1;
            found_pin = pin;
          }
        }
        if (p != found_fanout) {
          check_error("fanout list of some fanin is corrupted", p);
          return 0;
        }
        if (i != found_pin) {
          check_error("fanout pin of some fanin is corrupted", p);
          return 0;
        }
      }

      foreach_fanout_pin(p, gen1, fanout, pin) {
        found_fanout = 0;
        found_pin = -1;
        foreach_fanin(fanout, i, p1) {
          if (p == p1 && i == pin) {
            if (found_fanout != 0) {
              check_error("fanout duplicated for some node", p);
              return 0;
            }
            found_fanout = p1;
            found_pin = i;
          }
        }
        if (p != found_fanout) {
          check_error("fanin list of some fanout is corrupted", p);
          return 0;
        }
        if (pin != found_pin) {
          check_error("fanin pin of some fanout is corrupted", p);
          return 0;
        }
      }
      break;

    default:
      check_error("node type is not PI, PO, or INTERNAL", p);
      return 0;
    }
  }

  table = st_copy(network->name_table);
  foreach_node(network, gen, p) {
    key = p->name;
    if (!st_delete(table, &key, &value)) {
      check_error("node name is not in name_table", p);
      return 0;
    }
    if ((node_t *)value != p) {
      check_error("node doesn't match in name_table", p);
      return 0;
    }
  }
  if (st_count(table) != 0) {
    check_error("name_table contains superfluous entries", NIL(node_t));
    return 0;
  }
  st_free_table(table);

  table = st_copy(network->short_name_table);
  foreach_node(network, gen, p) {
    key = p->short_name;
    if (!st_delete(table, &key, &value)) {
      check_error("node short_name is not in short_name_table", p);
      return 0;
    }
    if ((node_t *)value != p) {
      check_error("node doesn't match in short_name_table", p);
      return 0;
    }
  }
  if (st_count(table) != 0) {
    check_error("short_name_table contains superfluous entries", NIL(node_t));
    return 0;
  }
  st_free_table(table);

  return warn ? 0 : 1;
}
