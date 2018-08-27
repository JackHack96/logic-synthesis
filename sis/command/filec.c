#include <dirent.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define NAMLEN(dirent) strlen((dirent)->d_name)

#if defined(_POSIX_VERSION) || defined(__CYGWIN__)
#define USE_TERMIO
#else
#undef USE_TERMIO
#endif

#include <sys/types.h> /* Part of util.h if PORT_H is not defined */
/* Still needs to be included for sun4 compile */
#include "sis.h"

#if defined(USE_TERMIO)

#include <termio.h>

#else
#include <sgtty.h>
#endif

#include "com_int.h"

#define ESC '\033'

#define BEEP '\007'
#define HIST '%'
#define SUBST '^'

char sis_hist_char = HIST; /* can be changed by "set hist_char" */

static char *seperator = " \t\n;";

/* DAK: these are used outside file completion code */
#define STDIN 0
#define STDOUT 1

/* Do not have access to file-completion in the HPUX systems ... */

#if defined(hpux) || defined(SYSTYPE_BSD43) || defined(SYSTYPE_SYSV)
char *fgets_filec(buf, size, stream, prompt) char *buf;
int size;
FILE *stream;
char *prompt;
{
  if (prompt != NIL(char)) {
    (void)print_prompt(prompt);
    (void)fflush(stdout);
  }

  return (fgets(buf, size, stream));
}
#else

/*
 * Words are seperated by any of the characters in `seperator'.  The seperator
 * is used to distinguish words from each other in file completion and history
 * substitution. The recommeded seperator string is " \t\n;".
 */
static int cmp();

static int match();

static int getnum();

/*
 * Duplicates the function of fgets, but also provides file completion in the
 * same style as csh.
 *
 * Input is read from `stream' and returned in `buf'.  Up to `size' bytes
 * will be placed into `buf'.  If `stream' is not stdin, is equivalent to
 * calling fgets(buf, size, stream).
 *
 * `prompt' is the prompt you want to appear at the beginning of the line.
 * The caller does not have to print the prompt string before calling this
 * routine.  The prompt has to be reprinted if the user hits ^D.
 *
 * The file completion routines are derived from the source code for csh,
 * which is copyrighted by the Regents of the University of California.
 */
char *fgets_filec(buf, size, stream, prompt) char *buf;
int size;
FILE *stream;
char *prompt;
{
  int n_read, i, len, maxlen, col, sno, modname;
#if defined(USE_TERMIO)
  struct termios tchars, oldtchars;
#else
  struct tchars tchars, oldtchars;
#endif
  DIR *dir;
  struct dirent *dp;
#if !defined(_IBMR2)
  int omask;
#ifndef __STDC__
  struct sgttyb tty, oldtty; /* To mask interuupts */
#endif
#endif
  char *last_word, *file, *path, *name, *line;
  char last_char, found[MAXNAMLEN];
  array_t *names;
#if !defined(__STDC__)
  int pending = LPENDIN;
#endif
  if (prompt != NIL(char)) {
    (void)print_prompt(prompt);
    (void)fflush(stdout);
  }

  sno = fileno(stream);
  if (sno != STDIN || !isatty(sno)) {
    return (fgets(buf, size, stream));
  } else if (com_get_flag("filec") == NIL(char)) {
    /* Get rid of the trailing newline */
    line = fgets(buf, size, stream);
    if (line != NULL) {
      len = strlen(line);
      if (len > 0 && line[len - 1] == '\n') {
        line[len - 1] = '\0';
      }
    }
    return line;
  }
/* Allow hitting ESCAPE to break a read() */
#if defined(USE_TERMIO)
  tcgetattr(sno, &tchars);
  oldtchars = tchars;
  tchars.c_cc[VEOL] = ESC;
  tcsetattr(sno, TCSANOW, &tchars);
#else
  (void)ioctl(sno, TIOCGETC, (char *)&tchars);
  oldtchars = tchars;
  tchars.t_brkc = ESC;
  (void)ioctl(sno, TIOCSETC, (char *)&tchars);
#endif

  while ((n_read = read(sno, buf, size)) > 0) {
    buf[n_read] = '\0';
    last_word = &buf[n_read - 1];
    last_char = *last_word;
    if (last_char == '\n' || n_read == size) {
#if defined(USE_TERMIO)
      tcsetattr(sno, TCSANOW, &oldtchars);
#else
      (void)ioctl(sno, TIOCSETC, (char *)&oldtchars);
#endif
      *last_word = '\0';
      return (buf);
    }
    if (last_char == ESC) {
      *last_word-- = '\0';
      (void)fprintf(stdout, "\b\b  \b\b");
    } else {
      names = array_alloc(char *, 10);
      (void)fputc('\n', stdout);
    }
    for (; last_word >= buf; --last_word) {
      if (strchr(seperator, *last_word) != NIL(char)) {
        break;
      }
    }
    last_word++;
    file = strrchr(buf, '/');
    if (file == NIL(char)) {
      file = last_word;
      modname = 0;
      path = ".";
    } else {
      *file++ = '\0';
      modname = 1;
      path = (*last_word == '~') ? util_tilde_expand(last_word) : last_word;
    }
    len = strlen(file);
    dir = opendir(path);
    if (dir == NIL(DIR) || len > MAXNAMLEN) {
      (void)fputc(BEEP, stdout);
    } else {
      *found = '\0';
      maxlen = 0;
      while ((dp = readdir(dir)) != NIL(struct dirent)) {
        if (strncmp(file, dp->d_name, len) == 0) {
          if (last_char == ESC) {
            if (match(dp->d_name, found, file) == 0) {
              break;
            }
          } else if (len != 0 || *(dp->d_name) != '.') {
            if (maxlen < NAMLEN(dp)) {
              maxlen = NAMLEN(dp);
            }
            array_insert_last(char *, names, util_strsav(dp->d_name));
          }
        }
      }
      (void)closedir(dir);
      if (last_char == ESC) {
        if (*found == '\0' || strcmp(found, file) == 0) {
          (void)fputc(BEEP, stdout);
        } else {
          (void)strcpy(file, found);
          (void)fprintf(stdout, "%s", &buf[n_read - 1]);
        }
      } else {
        maxlen += 2;
        col = maxlen;
        array_sort(names, cmp);
        for (i = 0; i < array_n(names); i++) {
          name = array_fetch(char *, names, i);
          (void)fprintf(stdout, "%-*s", maxlen, name);
          FREE(name);
          col += maxlen;
          if (col >= 80) {
            col = maxlen;
            (void)fputc('\n', stdout);
          }
        }
        array_free(names);
        if (col != maxlen) {
          (void)fputc('\n', stdout);
        }
      }
    }
    (void)fflush(stdout);
    if (modname != 0) {
      if (path != last_word) {
        FREE(path);
      }
      *--file = '/';
    }

#if !defined(_IBMR2) && !defined(__STDC__)
    /* mask interrupts temporarily */
    omask = sigblock(sigmask(SIGINT));
    (void)ioctl(STDOUT, TIOCGETP, (char *)&tty);
    oldtty = tty;
    tty.sg_flags &= ~(ECHO | CRMOD);
    (void)ioctl(STDOUT, TIOCSETN, (char *)&tty);
#endif

    /* reprint prompt */
    (void)write(STDOUT, "\r", 1);
    print_prompt(prompt);

/* shove chars from buf back into the input queue */
#if !defined(__CYGWIN__)
    for (i = 0; buf[i]; i++) {
      (void)ioctl(STDOUT, TIOCSTI, &buf[i]);
    }
#endif
#if !defined(_IBMR2) && !defined(__STDC__)
    /* restore interrupts */
    (void)ioctl(STDOUT, TIOCSETN, (char *)&oldtty);
    (void)sigsetmask(omask);
    (void)ioctl(STDOUT, TIOCLBIS, (char *)&pending);
#endif
  }
/* restore read() behavior */
#if defined(USE_TERMIO)
  tcsetattr(sno, TCSANOW, &oldtchars);
#else
  (void)ioctl(sno, TIOCSETC, (char *)&oldtchars);
#endif
  return (NIL(char));
}

static int cmp(s1, s2) char **s1, **s2;
{ return (strcmp(*s1, *s2)); }

static int match(newmatch, lastmatch, actual) char *newmatch, *lastmatch,
    *actual;
{
  int i = 0;

  if (*actual == '\0' && *newmatch == '.') {
    return (1);
  }
  if (*lastmatch == '\0') {
    (void)strcpy(lastmatch, newmatch);
    return (1);
  }
  while (*newmatch++ == *lastmatch) {
    lastmatch++;
    i++;
  }
  *lastmatch = '\0';
  return (i);
}
#endif /* defined(hpux) */

static char *do_subst();

static char *bad_event();

static char *getarg();

#if defined(SYSTYPE_BSD43)
#include <memory.h>
/*  BUG: this should be in a library somewhere.
 */
char *strstr(s, pat) const char *s, *pat;
{
  int len;

  len = strlen(pat);
  for (; *s != '\0'; ++s)
    if (*s == *pat && memcmp(s, pat, len) == 0)
      return (char *)s; /* UGH */
  return NULL;
}
#endif

/*
 * Simple history substitution routine.
 *
 * Not, repeat NOT, the complete csh history substitution mechanism.
 *
 * In the following ^ is the SUBST character and ! is the HIST character.
 * Deals with:
 *	!!			last command
 *	!stuff			last command that began with "stuff"
 *	!*			all but 0'th argument of last command
 *	!$			last argument of last command
 *	!:n			n'th argument of last command
 *	!n			repeat the n'th command
 *	!-n			repeat n'th previous command
 *	^old^new		replace "old" w/ "new" in previous command
 *
 * Trailing spaces are significant.
 * Removes all initial spaces.
 *
 * Returns `line' if no changes were made.
 * Returns pointer to a static buffer if any changes were made.
 * Sets `changed' to 1 if a history substitution took place, o/w set to 0.
 * Returns NULL if error occured;
 */
char *hist_subst(line, changed) char *line;
int *changed;
{
  static char buf[1024], c;
  char *value;
  char *last, *old, *new, *start, *b, *l;
  int n, len, i, num, internal_change;

  *changed = 0;
  internal_change = 0;
  while (isspace(*line)) {
    line++;
  }
  if (*line == '\0') {
    return (line);
  }
  n = array_n(command_hist);
  last = (n > 0) ? array_fetch(char *, command_hist, n - 1) : "";

  b = buf;
  if (*line == SUBST) {
    old = line + 1;
    new = strchr(old, SUBST);
    if (new == NIL(char)) {
      goto bad_modify;
    }
    *new ++ = '\0'; /* makes change in contents of line */
    start = strstr(last, old);
    if (start == NIL(char)) {
      *--new = SUBST;
    bad_modify:
      (void)fprintf(stderr, "Modifier failed\n");
      return (NIL(char));
    }
    while (last != start) {
      *b++ = *last++;
    }
    b = do_subst(b, new);
    last += strlen(old);
    while (*b++ = *last++) {
    }
    *changed = 1;
    return (buf);
  }

  if ((value = com_get_flag("history_char")) != NIL(char)) {
    sis_hist_char = *value;
  }

  for (l = line; *b = *l; l++) {
    if (*l == sis_hist_char) {
      /*
       * If a \ immediately preceeds a HIST char, pass just HIST char
       * Otherwise pass both \ and the character.
       */
      if (l > line && l[-1] == '\\') {
        b[-1] = sis_hist_char;
        internal_change = 1;
        continue;
      }
      if (n == 0) {
        return (bad_event(0));
      }
      l++;
      /* Cannot use a switch since the history char is a variable !!! */
      if (*l == sis_hist_char) {
        /* replace !! in line with last */
        b = do_subst(b, last);
      } else if (*l == '$') {
        /* replace !$ in line with last arg of last */
        b = do_subst(b, getarg(last, -1));
      } else if (*l == '*') {
        b = do_subst(b, getarg(last, -2));
      } else if (*l == ':') {
        /* replace !:n in line with n'th arg of last */
        l++;
        num = getnum(&l);
        new = getarg(last, num);
        if (new == NIL(char)) {
          (void)fprintf(stderr, "Bad %c arg selector\n", sis_hist_char);
          return (NIL(char));
        }
        b = do_subst(b, new);
      } else if (*l == '-') {
        /* replace !-n in line with n'th prev cmd */
        l++;
        num = getnum(&l);
        if (num > n || num == 0) {
          return (bad_event(n - num + 1));
        }
        b = do_subst(b, array_fetch(char *, command_hist, n - num));
      } else {
        /* replace !n in line with n'th command */
        if (isdigit(*l)) {
          num = getnum(&l);
          if (num > n || num == 0) {
            return (bad_event(num));
          }
          b = do_subst(b, array_fetch(char *, command_hist, num - 1));
        } else { /* replace !boo w/ last cmd beginning w/ boo */
          start = l;
          while (*l && strchr(seperator, *l) == NIL(char)) {
            l++;
          }
          c = *l;
          *l = '\0';
          len = strlen(start);
          for (i = n - 1; i >= 0; i--) {
            old = array_fetch(char *, command_hist, i);
            if (strncmp(old, start, len) == 0) {
              b = do_subst(b, old);
              break;
            }
          }
          if (i < 0) {
            (void)fprintf(stderr, "Event not found: %s\n", start);
            *l = c;
            return (NIL(char));
          }
          *l-- = c;
        }
      }
      *changed = 1;
    } else {
      b++;
    }
  }
  if (*changed != 0 || internal_change != 0) {
    return (buf);
  }
  return (line);
}

static int getnum(linep) char **linep;
{
  int num = 0;
  char *line = *linep;

  for (; isdigit(*line); line++) {
    num *= 10;
    num += *line - '0';
  }
  *linep = line - 1;
  return (num);
}

static char *getarg(line, num) char *line;
int num;
{
  static char buf[128];
  char *b, *c;
  int i;

  if (num == -1) {
    i = 123456;
  } else if (num == -2) {
    i = 1;
  } else {
    i = num;
  }

  c = line;
  do {
    b = line = c;
    while (*line && strchr(seperator, *line) == NIL(char)) {
      line++;
    }
    c = line;
    while (*c && strchr(seperator, *c) != NIL(char)) {
      c++;
    }
    if (*c == '\0') {
      break;
    }
  } while (--i >= 0);

  if (i > 0) {
    if (num == -1) {
      return (b);
    }
    return (NIL(char));
  }
  if (num < 0) {
    return (b);
  }
  c = buf;
  do {
    *c++ = *b++;
  } while (b < line && c < &buf[127]);
  *c = '\0';
  return (buf);
}

static char *bad_event(n) int n;
{
  (void)fprintf(stderr, "Event %d not found\n", n);
  return (NIL(char));
}

static char *do_subst(dest, new) char *dest, *new;
{
  while (*dest = *new ++) {
    dest++;
  }
  return (dest);
}

void print_prompt(prompt) char *prompt;
{
  char buf[256];

  if (prompt == NIL(char))
    return;

  while (*prompt != '\0') {
    if (*prompt == sis_hist_char) {
      (void)sprintf(buf, "%d", array_n(command_hist) + 1);
      (void)write(STDOUT, buf, (int)strlen(buf));
    } else {
      (void)write(STDOUT, prompt, 1);
    }
    prompt++;
  }
}