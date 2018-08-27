#ifdef notdef
#define _POSIX_SOURCE /* Argh!  IBM strikes again... */
#include "../port/copyright.h"
#include "../port/port.h"
#include "../utility/utility.h"
#include "st.h"

#define PATHLEN 1024

#include <pwd.h>

#define SCPY(s) strcpy(ALLOC(char, strlen(s) + 1), s)

static st_table *table = NIL(st_table);

#define DEF_SIZE 100
#define DEF_INCR 50

typedef struct rev_user_defn {
  char *home_dir;
  char *user_name;
} rev_user;

typedef struct rev_tbl_defn {
  int num_entries;
  int alloc_size;
  int needs_to_be_sorted;
  rev_user *items;
} rev_tbl;

static rev_tbl *other_table = (rev_tbl *)0;

static void init_table() {
  char *dir;

  table = st_init_table(strcmp, st_strhash);
  /* Get CADROOT directory from environment */
  dir = getenv("OCTTOOLS");
  if (dir) {
    (void)st_add_direct(table, "octtools", dir);
  }

  /* Get user set of translations from environment */
  dir = getenv("OCTTOOLS-TRANSLATIONS");
  if (dir) {
    char *ptr = SCPY(dir);
    char *token, *user, *home;

    while ((token = strtok(ptr, ", \t")) != NIL(char)) {
      ptr = NIL(char);
      if ((home = strchr(token, ':')) == NIL(char)) {
        /* malformed entry */
        continue;
      }
      user = token;
      *home = '\0';
      home++;
      (void)st_add_direct(table, user, home);
    }
  }

  return;
}

static void insert_other_table(home_dir, user_name) char *home_dir;
char *user_name;
{
  if (other_table->num_entries >= other_table->alloc_size) {
    other_table->alloc_size += DEF_INCR;
    other_table->items =
        REALLOC(rev_user, other_table->items, other_table->alloc_size);
  }
  other_table->items[other_table->num_entries].home_dir = SCPY(home_dir);
  other_table->items[other_table->num_entries].user_name = SCPY(user_name);
  other_table->num_entries += 1;
  other_table->needs_to_be_sorted = 1;
}

static void init_other_table() {
  struct passwd *entry;
  char *dir;

  other_table = ALLOC(rev_tbl, 1);
  other_table->num_entries = 0;
  other_table->alloc_size = DEF_SIZE;
  other_table->needs_to_be_sorted = 0;
  other_table->items = ALLOC(rev_user, other_table->alloc_size);

  dir = getenv("OCTTOOLS");
  if (dir) {
    insert_other_table(dir, "octtools");
  }
  dir = getenv("OCTTOOLS-TRANSLATIONS");
  if (dir) {
    char *ptr = SCPY(dir);
    char *token, *user, *home;

    while ((token = strtok(ptr, ", \t")) != NIL(char)) {
      ptr = NIL(char);
      if ((home = strchr(token, ':')) == NIL(char)) {
        /* malformed entry */
        continue;
      }
      user = token;
      *home = '\0';
      home++;
      insert_other_table(home, user);
    }
  }
  setpwent();
  while (entry = getpwent()) {
    insert_other_table(entry->pw_dir, entry->pw_name);
  }
  endpwent();
}

void util_register_user(user, directory) char *user, *directory;
{
  if (table == NIL(st_table)) {
    init_table();
  }
  (void)st_insert(table, user, directory);
  if (other_table == NIL(rev_tbl)) {
    init_other_table();
  }
  insert_other_table(directory, user);
  return;
}

char *util_tilde_expand(fname) char *fname;
{
  char username[PATHLEN];
  static char result[8][PATHLEN];
  static int count = 0;
  struct passwd *userRecord;
  extern struct passwd *getpwuid(), *getpwnam();
  register int i, j;
  char *dir;

  if (++count > 7) {
    count = 0;
  }

  /* Clear the return string */
  i = 0;
  result[count][0] = '\0';

  /* Tilde? */
  if (fname[0] == '~') {
    j = 0;
    i = 1;
    while ((fname[i] != '\0') && (fname[i] != '/')) {
      username[j++] = fname[i++];
    }
    username[j] = '\0';

    if (table == NIL(st_table)) {
      init_table();
    }

    if (!st_lookup(table, username, &dir)) {
      /* ~/ resolves to the home directory of the current user */
      if (username[0] == '\0') {
        if ((userRecord = getpwuid(getuid())) != 0) {
          (void)strcpy(result[count], userRecord->pw_dir);
          (void)st_add_direct(table, "", util_strsav(userRecord->pw_dir));
        } else {
          i = 0;
        }
      } else if ((userRecord = getpwnam(username)) != 0) {
        /* ~user/ resolves to home directory of 'user' */
        (void)strcpy(result[count], userRecord->pw_dir);
        (void)st_add_direct(table, util_strsav(username),
                            util_strsav(userRecord->pw_dir));
      } else {
        i = 0;
      }
    } else {
      (void)strcat(result[count], dir);
    }
  }

  /* Concantenate remaining portion of file name */
  (void)strcat(result[count], fname + i);
  return result[count];
}

static int comp(a, b) char *a, *b;
{
  int diff;

  diff = strlen(((rev_user *)b)->home_dir) - strlen(((rev_user *)a)->home_dir);
  if (diff == 0) {
    diff =
        strlen(((rev_user *)a)->user_name) - strlen(((rev_user *)b)->user_name);
  }
  return diff;
}

char *util_tilde_compress(fname) char *fname;
/*
 * Makes tilde expanded name from the given fully expanded name.
 * If no match is found, it returns the filename back unmodified.
 */
{
  static char result[8][PATHLEN];
  static int count = 0;
  int i, len;

  if (++count > 7)
    count = 0;
  if (!other_table)
    init_other_table();
  if (other_table->needs_to_be_sorted) {
    qsort(other_table->items, other_table->num_entries, sizeof(rev_user), comp);
    other_table->needs_to_be_sorted = 0;
  }
  for (i = 0; i < other_table->num_entries; i++) {
    len = strlen(other_table->items[i].home_dir);
    if (strncmp(other_table->items[i].home_dir, fname, len) == 0) {
      result[count][0] = '\0';
      strcat(result[count], "~");
      strcat(result[count], other_table->items[i].user_name);
      strcat(result[count], fname + len);
      return result[count];
    }
  }
  return fname;
}
#endif
