
typedef struct espterm { /* product terms in minimized file */

  char *input;
  char *output;
  char *nstate;
  char *poutput;
  struct espterm *next;

} ESPTERM;

typedef struct termlink { /* list of product terms */

  char *input; /* input and output parts of the product term */
  char *output;
  char *nstate;
  char *poutput;
  struct termlink *next;

} TERMLINK;

typedef struct translink {

  char *input; /* fields of the transition */
  char *pstate;
  char *nstate;
  char *output;
  int ilab;
  int plab;
  int nlab;
  struct translink *next;

} TRANSLINK;

typedef struct coverlink { /* list of product terms/covered transitions */

  char *input; /* input and output parts of the product term */
  char *output;
  TRANSLINK *ccovered; /* list of transitions completely covered */
  TRANSLINK *pcovered; /* list of transitions partially covered */
  struct coverlink *next;

} COVERLINK;

typedef struct orlink { /* list of transitions/ored next states */

  char *input; /* fields of the transitions */
  char *pstate;
  char *nstate;
  char *output;
  TERMLINK *orstate; /* list of ored product terms */
  struct orlink *next;

} ORLINK;

typedef struct codex {

  char *code;           /* code to analyze */
  char *state;          /* name of the state */
  TRANSLINK *transnull; /* list transitions not implemented */
  COVERLINK *cover;     /* list of product terms/covered transitions */
  ORLINK *oring;        /* list ored transitions */

} CODEX;

/* GLOBAL VARIABLES */

ESPTERM *esptable;  /* pointer to esp file product terms */
CODEX *codici;      /* array of states/codes descriptors */
int codexnum;       /* number of next state binary patterns */
int espterm_card;   /* number of product terms of minimized implem.   */
int espinp_card;    /* number of binary inputs of minimized implem.   */
int espout_card;    /* number of binary outputs of minimized implem.  */
int nulltrans_card; /* number of product terms saved for zero effect */
int mvtrans_card;   /* number of product terms saved for join effect */
int ortrans_card;   /* number of product terms saved for oring effect */
