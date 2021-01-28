#include <stdio.h>
#include <stdlib.h>
//#include "bfs.h"
/*
typedef struct{
    int numOfNodes;
    int nodeName[100000];
    int nodePriv[100000];
    int node_hash[100000];
    int numOfEdges;
    int edge_start_name[8000000];
    int edge_start_
}*/



int bfs1(int *a, int *b, int *c, int *d, int *e, int *f, int g, int m, int n, int p){
    printf("+++++ print from C +++++ %d %d %d %d\n", g, m, n, p);
    return 2;
}

int *bfs(int *topo, int *num_ex, int *ex_names, int *pre_priv, int *post_priv, int *pacc, int cont_cnt, int outside_id, int docker_host_id, int max_num_ex, int *node_name, int *node_priv, int *edge_start, int *edge_end, int *node_cnt, int *edge_cnt){
    printf("+++++ print from C +++++ %d %d %d %d %d %d\n", cont_cnt, outside_id, docker_host_id, max_num_ex, *node_cnt, *edge_cnt);
    int *edge_label = (int *)malloc(sizeof(int)*2500000*100);
    edge_label[2500000*100-1]=35;
    edge_label[0]=53;  
    return edge_label;
}
