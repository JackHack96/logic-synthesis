#ifndef ST_H
#define ST_H

typedef struct st_table_entry st_table_entry;
struct st_table_entry {
  char *key;
  char *record;
  st_table_entry *next;
};

typedef struct st_table st_table;

struct st_table {
  int (*compare)();

  int (*hash)();

  int num_bins;
  int num_entries;
  int max_density;
  int reorder_flag;
  double grow_factor;
  st_table_entry **bins;
};

typedef struct st_generator st_generator;
struct st_generator {
  st_table *table;
  st_table_entry *entry;
  int index;
};

#define st_is_member(table, key) st_lookup(table, key, (char **)0)
#define st_count(table) ((table)->num_entries)

enum st_retval { ST_CONTINUE, ST_STOP, ST_DELETE };

typedef enum st_retval (*ST_PFSR)();

typedef int (*ST_PFI)();

typedef int (*ST_PFICPI)();

int st_delete(st_table *, char **, char **);

int st_delete_int(st_table *, int *, char **);

int st_insert(st_table *, char *, char *);

int st_foreach(st_table *, ST_PFSR, char *);

int st_gen(st_generator *, char **, char **);

int st_gen_int(st_generator *, char **, int *);

int st_lookup(st_table *, char *, char **);

int st_lookup_int(st_table *, char *, int *);

int st_find_or_add(st_table *, char *, char ***);

int st_find(st_table *, char *, char ***);

void st_add_direct(st_table *table, char *key, char *value);

int st_strhash(char *, int);

int st_numhash(char *, int);

int st_ptrhash(char *, int);

int st_numcmp(char *, char *);

int st_ptrcmp(char *, char *);

st_table *st_init_table(ST_PFI, ST_PFI);

st_table *st_init_table_with_params(ST_PFI, ST_PFI, int, int, double,
                                           int);

st_table *st_copy(st_table *);

st_generator *st_init_gen(st_table *);

void st_free_table(st_table *);

void st_free_gen(st_generator *);

#define ST_DEFAULT_MAX_DENSITY 5
#define ST_DEFAULT_INIT_TABLE_SIZE 11
#define ST_DEFAULT_GROW_FACTOR 2.0
#define ST_DEFAULT_REORDER_FLAG 0

#define st_foreach_item(table, gen, key, value)                                \
  for (gen = st_init_gen(table);                                               \
       st_gen(gen, key, value) || (st_free_gen(gen), 0);)

#define st_foreach_item_int(table, gen, key, value)                            \
  for (gen = st_init_gen(table);                                               \
       st_gen_int(gen, key, value) || (st_free_gen(gen), 0);)

#endif /* ST_H */
