
typedef struct graph_struct_int {
	gGeneric user_data;
	lsList v_list;
	lsList e_list;
} graph_t_int;

typedef struct vertex_struct_int {
	gGeneric user_data;
	graph_t_int *g;
	lsList in_list;
	lsList out_list;
	int id;
	lsHandle handle;	/* for quick deletion in the graph v_list */
} vertex_t_int;

typedef struct edge_struct_int {
	gGeneric user_data;
	vertex_t_int *from;
	vertex_t_int *to;
	int id;
	lsHandle handle;	/* for quick deletion in the graph e_list */
} edge_t_int;

extern void del_from_list();
extern int g_unique_id; 
