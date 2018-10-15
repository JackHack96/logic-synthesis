
#include "global.h"
#include "sis/util/util.h"
#include "struct.h"
#include "user.h"

static char *d_str, *intsect;
static int *pcount, *mcount;
NODE *root;

/*
char *input;
*/

NLIST **hash_initial();

NLIST **input_hash;
NLIST *_head, *_hend;

disjoint() {
  register i, j, k, l;
  int num_jt;
  int *c_cover;
  EDGE *nedge;
  NLIST *start, *advance, *previous, *inp, *list;
  NLIST *install_input();
  NLIST *before;

  if (!(d_str = ALLOC(char, num_pi)))
    panic("ALLOC");

  /*
          if (!(input=ALLOC(char,num_pi)))
                  panic("ALLOC");
  */

  if (!(intsect = ALLOC(char, num_pi)))
    panic("ALLOC");

  if (!(c_cover = ALLOC(int, num_st)))
    panic("bCover");

  /* I need hash table */

  if ((input_hash = hash_initial(num_product)) == NIL(NLIST *)) {
    panic("ALLOC hash");
  }

  k = 0;

  _head = _hend = NIL(NLIST);
  user.stat.ostates = 0;
  for (i = 0; i < num_st; i++)
    if (states[i]->assigned) {
      for (nedge = states[i]->edge; nedge; nedge = nedge->next) {
        inp = install_input(nedge->input, input_hash, num_product);
        if (inp) {
          k++;
          if (_head) {
            inp->h_prev = _hend;
            _hend->h_next = inp;
            _hend = inp;
          } else {
            _hend = _head = inp;
          }
        }
      }
    } else
      c_cover[user.stat.ostates++] = i;

  user.stat.xinput = k;

  disjoint_sharp();

  k = 0;
  for (start = _head; start; start = start->h_next) {
    if (user.opt.verbose > 10) {
      (void)fprintf(stderr, "%s\n", start->name);
    }
    k++;
  }
  user.stat.disjoint = k;

  for (i = 0; i < num_st; i++) {
    if (states[i]->assigned)
      for (nedge = states[i]->edge; nedge; nedge = nedge->next) {
        for (start = _head; start; start = start->h_next) {
          if (intersection(nedge->input, start->name, num_pi)) {
            cube_split(nedge, start);
          }
        }
      }
  }
}

cube_split(nedge, xlist) EDGE *nedge;
NLIST *xlist;
{
  S_EDGE *pedge, *edge_hptr, *edge_vptr;

  if (!(pedge = ALLOC(S_EDGE, 1)))
    panic("Alloc in disjoint");
  pedge->p_state = nedge->p_state;
  pedge->n_state = nedge->n_state;
  pedge->n_star = nedge->n_star;
  pedge->output = nedge->output;

  if (xlist->ptr) {
    pedge->v_next = (S_EDGE *)xlist->ptr;
  } else {
    pedge->v_next = NIL(S_EDGE);
  }
  xlist->ptr = (int *)pedge;
}

disjoint_sharp() {
  register i;

  if (!(pcount = ALLOC(int, num_pi * 3)))
    panic("xsharp");
  mcount = pcount + num_pi;
  if (!(root = ALLOC(NODE, 1)))
    panic("root");
  root->on_off = mcount + num_pi;
  for (i = 0; i < num_pi; i++)
    root->on_off[i] = 1;
  root->cubes = _head;
  _head = NIL(NLIST);
  sharp(root, num_pi);
}

sharp(xnode, bit) NODE *xnode;
{
  int index;
  NODE *fnode;

  if ((xnode->literal = single_cube_contain(xnode)) < 2) {
    xnode->right = xnode->left = NIL(NODE);
    if (xnode->literal == 1) {
      if (_head) {
        _hend->h_next = xnode->cubes;
        _hend = xnode->cubes;
      } else {
        _head = _hend = xnode->cubes;
      }
      _hend->h_next = NIL(NLIST);
    }
    /*
                    return xnode;
    */
    return;
  }
  index = binate_select(xnode);

  if (!(xnode->left = ALLOC(NODE, 2)))
    panic("sharp");
  if (!(xnode->left->on_off = ALLOC(int, num_pi * 2)))
    panic("on_Off");

  xnode->right = xnode->left + 1;
  xnode->right->on_off = xnode->left->on_off + num_pi;
  xnode->right->cubes = xnode->left->cubes = NIL(NLIST);
  MEMCPY(xnode->right->on_off, xnode->on_off, sizeof(int) * num_pi);
  MEMCPY(xnode->left->on_off, xnode->on_off, sizeof(int) * num_pi);
  xnode->left->on_off[index] = 0;
  xnode->right->on_off[index] = 0;

  divide(index, xnode);

  sharp(xnode->left, bit - 1);
  sharp(xnode->right, bit - 1);
  if (FREE(xnode->left->on_off) || FREE(xnode->left))
    panic("lint");
}

binate_select(xnode) NODE *xnode;
{
  register i;
  NLIST *start;
  NLIST *list;
  int index, maxi, num_list;

  for (i = 0; i < num_pi; i++) {
    pcount[i] = mcount[i] = 0;
  }

  start = xnode->cubes;
  num_list = 0;
  for (list = start; list; list = list->h_next)
    num_list++;
  for (i = 0; i < num_pi; i++) {
    if (xnode->on_off[i]) {
      for (list = start; list; list = list->h_next) {
        switch (list->name[i]) {
        case '0':
          mcount[i]++;
          break;
        case '1':
          pcount[i]++;
          break;
        default:
          break;
        }
      }
    }
  }
  maxi = 0;
  for (i = 0; i < num_pi; i++) {
    int abs_count;

    if (xnode->on_off[i]) {
      abs_count = pcount[i] + mcount[i];
      if ((!abs_count) || (mcount[i] == num_list) || (pcount[i] == num_list)) {
        xnode->on_off[i] = 0;
        continue;
      }
      if (abs_count > maxi) {
        maxi = abs_count;
        index = i;
      }
    }
  }
  return index;
}

divide(index, xnode) NODE *xnode;
{
  NLIST *list;
  NLIST *append_right;
  NLIST *append_left;

  /* Bissect and expand linked list into two part */
  list = xnode->cubes;
  while (list) {
    NLIST *xlist;
    NLIST *ylist;

    ylist = list->h_next;
    switch (list->name[index]) {
    case '1':
      append(list, xnode->right);
      break;
    case '0':
      append(list, xnode->left);
      break;
    case '-':
      if (!(xlist = ALLOC(NLIST, 1)))
        panic("xlist");
      if (!(xlist->name = ALLOC(char, num_pi + 1)))
        panic("xlist name");
      list->name[index] = '1';
      MEMCPY(xlist->name, list->name, num_pi + 1);
      xlist->name[index] = '0';
      xlist->ptr = NIL(int);
      /*
                              xlist->ptr= NIL(S_EDGE);
      */
      xlist->h_prev = xlist->h_next = NIL(NLIST);
      append(list, xnode->right);
      append(xlist, xnode->left);
      break;
    default:
      panic("Non binary input specification");
    }
    list = ylist;
  }
}

single_cube_contain(xnode) NODE *xnode;
{
  register cube_count;
  NLIST *list;
  NLIST *child;

  list = xnode->cubes;
  cube_count = 0;
  while (list) {
    child = list->h_next;
    while (child) {
      if (!xintersection(list->name, child->name, num_pi, xnode)) {
        /* remove child entry */
        cdelete(child);
      }
      child = child->h_next;
    }
    cube_count++;
    list = list->h_next;
  }
  return cube_count;
}

append(list, xnode) NLIST *list;
NODE *xnode;
{
  NLIST *cube;

  cube = xnode->cubes;
  if (cube) {
    while (cube->h_next)
      cube = cube->h_next;
    cube->h_next = list;
    list->h_prev = cube;
  } else {
    xnode->cubes = list;
  }
  list->h_next = NIL(NLIST);
}

cdelete(link) NLIST *link;
{
  link->h_prev->h_next = link->h_next;
  if (link->h_next) {
    link->h_next->h_prev = link->h_prev;
  }
}
