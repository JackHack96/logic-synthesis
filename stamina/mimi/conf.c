
/* SCCSID%W% */

int merge();
int maximal_compatibles();
int prime_compatible();
int sm_setup();
int bound();

int disjoint();
int map();
int say_solution();
int iso_find();

null()
{
}

int (*method1[])()= {merge, disjoint, iso_find, maximal_compatibles,
	bound, 
	prime_compatible, sm_setup, map, say_solution, (int(*)()) 0};

make_null(id)
{
	method1[id] = null;
}
