#ifndef SIS_INCLUDE
#define SIS_INCLUDE

#include "espresso.h"
#include "avl.h"
#include "st.h"
#include "array.h"
#include "list.h"
//#include "bdd.h"

/* NO SIS NETWORKs (Jiang) */
typedef void network_t;

#define misout stdout
#define miserr stderr
#define sisout stdout
#define siserr stderr

#ifndef INFINITY
#define INFINITY	(1 << 30)
#endif

#endif /*SIS_INCLUDE*/
