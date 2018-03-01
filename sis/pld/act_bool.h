typedef struct act_match_defn {
    node_t *A0;
    node_t *A1;
    node_t *SA;
    node_t *B0;
    node_t *B1;
    node_t *SB;
    node_t *S0;
    node_t *S1;
} ACT_MATCH;
  
typedef struct cofc_struct_defn {
    node_t *pos;
    node_t *neg;
    int pos_mux;
    node_t *B0;
    node_t *B1;
    node_t *SB;
    node_t *node;
    node_t *fanin;
} COFC_STRUCT;

extern ACT_MATCH *act_is_act_function();
