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


typedef struct{
    unsigned int hashNum;
    unsigned int ID;
} hashUnit;


unsigned int nodeEncoding(unsigned int nodeName, unsigned int nodePriv){
    return (nodeName<<3)+nodePriv;
}

unsigned int edgeDirEncoding(unsigned int startNodeName, unsigned int endNodeName, unsigned int endNodePriv){
    return (startNodeName<<18)+(endNodeName<<3)+endNodePriv;
}

unsigned int edgeEncoding(unsigned int startNodeID, unsigned int endNodeID){
    return (startNodeID<<16)+endNodeID;
}


unsigned int hash1(unsigned int key) 
{
    unsigned int M=20000033; 
    return (key%M); 
} 

unsigned int hash2(unsigned int key) 
{
    unsigned int R=19991003;
    return (R-(key%R)); 
} 

/*search the encoded value of a given node/edge in a hash table: use double hashing*/
unsigned int nodeHashing(unsigned int hv, hashUnit* hTable, unsigned int* ha){
    unsigned int M=20000033;
    unsigned int index=hash1(hv);
    if (hTable[index].hashNum==0)
    {//search miss
        *ha=index;
        return 0;     
    }
    else if (hTable[index].hashNum==hv)
    {//search hit
        *ha=index;
        return 1;
    }
    else
    {  
        unsigned int index2 = hash2(hv);
        int i = 1;
        while (1) 
        { 
            unsigned int newIndex = (index+i*index2)%M; 
            if (hTable[newIndex].hashNum==0) {
                *ha = newIndex;
                return 0;
            }
            else if (hTable[newIndex].hashNum==hv) {
                *ha = newIndex;
                return 1;
            } 
            i++;
        } 
    }   
}



typedef struct {
    unsigned int array[50000];
    int read_idx;
    int write_idx;
    int empty;
    int full;
} fifo;


int fifo_read(fifo* ff, unsigned int * buf){
    if((*ff).empty==1) return 0;
    (*ff).full=0;
    (*buf)=(*ff).array[(*ff).read_idx];
    (*ff).read_idx+=1;
    if((*ff).read_idx==sizeof((*ff).array)/sizeof(int)) (*ff).read_idx=0;
    if((*ff).read_idx==(*ff).write_idx) (*ff).empty=1;
    return 1;
}

int fifo_write(fifo* ff, unsigned int val){
    if((*ff).full==1) return 0;
    (*ff).empty=0;
    (*ff).array[(*ff).write_idx]=val;
    (*ff).write_idx+=1;
    if((*ff).write_idx==sizeof((*ff).array)/sizeof(int)) (*ff).write_idx=0;
    if((*ff).write_idx==(*ff).read_idx) (*ff).full=1;
    return 1;
}

void fifo_init(fifo* ff){
    (*ff).read_idx=0;
    (*ff).write_idx=0;
    (*ff).empty=1;
    (*ff).full=0;
}

int fifo_idx_read(fifo* ff, int idx, unsigned int * buf){
    if((*ff).empty==1) return 0;
    int max_idx;
    int size_ff=sizeof((*ff).array)/sizeof(int);
    if((*ff).read_idx==(*ff).write_idx){
        max_idx=size_ff;
    }
    else if((*ff).read_idx<(*ff).write_idx){
        max_idx=(*ff).write_idx-(*ff).read_idx;
    }
    else{
        max_idx=(*ff).write_idx+size_ff-(*ff).read_idx;
    }
    if(idx>=max_idx || idx<0) return 0;
    int temp=(*ff).read_idx+idx;
    if(temp>=size_ff) temp=temp-size_ff;
    (*buf) = (*ff).array[temp];
    return 1;
}

int fifo_idx_write(fifo* ff, int idx, unsigned int *val){
    if((*ff).empty==1) return 0;
    int max_idx;
    int size_ff=sizeof((*ff).array)/sizeof(int);
    if((*ff).read_idx==(*ff).write_idx){
        max_idx=size_ff;
    }
    else if((*ff).read_idx<(*ff).write_idx){
        max_idx=(*ff).write_idx-(*ff).read_idx;
    }
    else{
        max_idx=(*ff).write_idx+size_ff-(*ff).read_idx;
    }
    if(idx>=max_idx || idx<0) return 0;
    int temp=(*ff).read_idx+idx;
    if(temp>=size_ff) temp=temp-size_ff;
    (*ff).array[temp]=(*val);
    return 1;
}

int fifo_curr_size(fifo* ff){
    if((*ff).empty==1) return 0;
    int size_ff=sizeof((*ff).array)/sizeof(int);
    if((*ff).read_idx==(*ff).write_idx){
        return size_ff;
    }
    else if((*ff).read_idx<(*ff).write_idx){
        return (*ff).write_idx-(*ff).read_idx;
    }
    else{
        return (*ff).write_idx+size_ff-(*ff).read_idx;
    }    
}


/*Breadth First Search to expand for an AG*/
int *bfs(int *topo, int *num_ex, int *ex_names, int *pre_priv, int *post_priv, int *pacc, int cont_cnt, int outside_name, int docker_host_name, int max_num_ex, unsigned int *node_name, unsigned int *node_priv, unsigned int *edge_start, unsigned int *edge_end, unsigned int *node_cnt, unsigned int *edge_cnt){
    //printf("+++++ print from C +++++ %d %d %d %d %d %d\n", cont_cnt, outside_id, docker_host_id, max_num_ex, *node_cnt, *edge_cnt);
    /* data structure initialization starts here*/
    int *edge_label = (int *)malloc(sizeof(int)*2500000*100); //return this array to reduce memory op time
    for(int i=0; i<2500000; i++) edge_label[i*100]=0; //the first element for each edge's label row indicates how many labels that edge has, and is set 0 initially
    fifo *masterQueue = (fifo *)malloc(sizeof(fifo)); //define a FIFO for unexpanded nodes
    fifo_init(masterQueue);
    hashUnit *nodeTable = (hashUnit *)malloc(sizeof(hashUnit)*20000033);
    for(int i=0; i<20000033; i++) nodeTable[i].hashNum=0;
    hashUnit *edgeDirTable = (hashUnit *)malloc(sizeof(hashUnit)*20000033);
    for(int i=0; i<20000033; i++) edgeDirTable[i].hashNum=0; 
    hashUnit *edgeTable = (hashUnit *)malloc(sizeof(hashUnit)*20000033);
    for(int i=0; i<20000033; i++) edgeTable[i].hashNum=0;
    

    (*node_cnt)=1;
    node_name[0]=outside_name; //store the root node to node list
    node_priv[0]=4;
    fifo_write(masterQueue, 0); //enqueue the root node to frontier
    unsigned int rootVal = nodeEncoding(node_name[0], node_priv[0]);
    unsigned int *rootAddr = (unsigned int *)malloc(sizeof(unsigned int));
    nodeHashing(rootVal, nodeTable, rootAddr); //hash the root node to node hashtable
    nodeTable[*rootAddr].hashNum=rootVal;
    nodeTable[*rootAddr].ID=0; 
    (*edge_cnt)=0;
    unsigned int *addr = (unsigned int *)malloc(sizeof(unsigned int));
    unsigned int *addr2 = (unsigned int *)malloc(sizeof(unsigned int));

    /* data structure initialization ends here*/

    /*serial expansion starts here*/
    while(fifo_curr_size(masterQueue)){//serial expansion while-loop
        unsigned int curr_node_id;
        fifo_read(masterQueue,&curr_node_id);
        unsigned int curr_node_name = node_name[curr_node_id];
        unsigned int curr_node_priv = node_priv[curr_node_id];
        //allows local exploit
        if(curr_node_name != docker_host_name) topo[curr_node_name*cont_cnt+curr_node_name]=1;
        //unsigned int *addr;
        for(int ncnt=0; ncnt<cont_cnt; ncnt++){//check each neighbor, ncnt is neighbor name value
            if(topo[curr_node_name*cont_cnt+ncnt]==1){//must be a connected neighbor
                if(curr_node_name == docker_host_name && nodeHashing(nodeEncoding(ncnt,4),nodeTable,addr)==1){//case 1
                    //unsigned int *addr2;
                    int reversed = nodeHashing(edgeDirEncoding(ncnt,curr_node_name,4),edgeDirTable,addr2);
                    if(reversed) continue;
                    else{//update edge direction table
                        unsigned int curr_edge_dir_val = edgeDirEncoding(curr_node_name,ncnt,4);
                        nodeHashing(curr_edge_dir_val,edgeDirTable,addr2);
                        edgeDirTable[*addr2].hashNum=curr_edge_dir_val;//works for both hit and miss
                    }
                    
                    unsigned int nvalue = nodeEncoding(ncnt,4);
                    unsigned int newNodeID;
                    if(nodeHashing(nvalue,nodeTable,addr)==0){
                        newNodeID = ++(*node_cnt);
                        nodeTable[*addr].hashNum=nvalue;// update node hashtable
                        nodeTable[*addr].ID=newNodeID;
                        node_name[newNodeID]=ncnt;
                        node_priv[newNodeID]=4;
                        fifo_write(masterQueue, newNodeID);//?
                    }
                    else{
                        newNodeID = nodeTable[*addr].ID;
                    }
                    unsigned int curr_edge_val = edgeEncoding(curr_node_id, newNodeID);
                    if(nodeHashing(curr_edge_val, edgeTable, addr2)==0){
                        unsigned int newEdgeID = ++(*edge_cnt);
                        edgeTable[*addr2].hashNum = curr_edge_val;
                        edgeTable[*addr2].ID = newEdgeID;
                        edge_start[newEdgeID]=curr_node_id;
                        edge_end[newEdgeID]=newNodeID;
                        int *edge_label_cnt = &(edge_label[newEdgeID*100]);
                        int *edge_label_row = &(edge_label[newEdgeID*100+1]);
                        if((*edge_label_cnt)<99){
                            edge_label_row[*edge_label_cnt]=-1; //-1 represents root access due to docker host being compromised
                            (*edge_label_cnt)++;
                        }
                    }
                    else{
                        unsigned int existing_edge_id = edgeTable[*addr2].ID;
                        int *edge_label_cnt = &(edge_label[existing_edge_id*100]);
                        int *edge_label_row = &(edge_label[existing_edge_id*100+1]);
                        if((*edge_label_cnt)<99){
                            edge_label_row[*edge_label_cnt]=-1; //-1 represents root access due to docker host being compromised
                            (*edge_label_cnt)++;
                        }
                    }
                }//case 1
                else if(ncnt == docker_host_name && pacc[curr_node_name]==1){//case 2
                    //unsigned int *addr2;
                    int reversed = nodeHashing(edgeDirEncoding(ncnt,curr_node_name,curr_node_priv),edgeDirTable,addr2);
                    if(reversed) continue;
                    else{//update edge direction table
                        unsigned int curr_edge_dir_val = edgeDirEncoding(curr_node_name,ncnt,4);
                        nodeHashing(curr_edge_dir_val,edgeDirTable,addr2);
                        edgeDirTable[*addr2].hashNum=curr_edge_dir_val;   
                    }
                   
                    unsigned int nvalue = nodeEncoding(ncnt,4);
                    unsigned int newNodeID;
                    if(nodeHashing(nvalue, nodeTable, addr)==0){
                        newNodeID = ++(*node_cnt); 
                        nodeTable[*addr].hashNum=nvalue;// update node hashtable
                        nodeTable[*addr].ID=newNodeID;
                        node_name[newNodeID]=ncnt;
                        node_priv[newNodeID]=4;
                        fifo_write(masterQueue, newNodeID);
                    }
                    else{
                        newNodeID = nodeTable[*addr].ID;
                    }                      
                    unsigned int curr_edge_val = edgeEncoding(curr_node_id, newNodeID);
                    if(nodeHashing(curr_edge_val, edgeTable, addr2)==0){
                        unsigned int newEdgeID = ++(*edge_cnt);
                        edgeTable[*addr2].hashNum = curr_edge_val;
                        edgeTable[*addr2].ID = newEdgeID;
                        edge_start[newEdgeID]=curr_node_id;
                        edge_end[newEdgeID]=newNodeID;
                        int *edge_label_cnt = &(edge_label[newEdgeID*100]);
                        int *edge_label_row = &(edge_label[newEdgeID*100+1]);
                        if((*edge_label_cnt)<99){
                            edge_label_row[*edge_label_cnt]=-2; //-2 represents priviledged access to compromise docker host
                            (*edge_label_cnt)++;
                        }
                    }
                    else{
                        unsigned int existing_edge_id = edgeTable[*addr2].ID;
                        int *edge_label_cnt = &(edge_label[existing_edge_id*100]);
                        int *edge_label_row = &(edge_label[existing_edge_id*100+1]);
                        if((*edge_label_cnt)<99){
                            edge_label_row[*edge_label_cnt]=-2; //-2 represents priviledged access to compromise docker host
                            (*edge_label_cnt)++;
                        }
                       
                    }                                       
                }//case 2
                else if(ncnt != outside_name && ncnt != docker_host_name){//case 3
                    for(int ecnt=0; ecnt<num_ex[ncnt]; ecnt++){//check each exploit on the neighbor
                        int ex_idx = ncnt*max_num_ex + ecnt;
                        if(curr_node_priv >= pre_priv[ex_idx] && ((ncnt!=curr_node_name && post_priv[ex_idx]!=0)||(ncnt==curr_node_name && curr_node_priv<post_priv[ex_idx]))){//priv escalation

                            //unsigned int *addr2;
                            int reversed = nodeHashing(edgeDirEncoding(ncnt,curr_node_name,curr_node_priv),edgeDirTable,addr2);
                            if(reversed) continue;
                            else{//update edge direction table
                                unsigned int curr_edge_dir_val = edgeDirEncoding(curr_node_name,ncnt,post_priv[ex_idx]);
                                nodeHashing(curr_edge_dir_val,edgeDirTable,addr2);
                                edgeDirTable[*addr2].hashNum=curr_edge_dir_val;   
                            }
                   
                            unsigned int nvalue = nodeEncoding(ncnt,post_priv[ex_idx]);
                            unsigned int newNodeID;
                            if(nodeHashing(nvalue, nodeTable, addr)==0){
                                newNodeID = ++(*node_cnt); 
                                nodeTable[*addr].hashNum=nvalue;// update node hashtable
                                nodeTable[*addr].ID=newNodeID;
                                node_name[newNodeID]=ncnt;
                                node_priv[newNodeID]=post_priv[ex_idx];
                                fifo_write(masterQueue, newNodeID);
                            }
                            else{
                                newNodeID = nodeTable[*addr].ID;
                            }                      
                            unsigned int curr_edge_val = edgeEncoding(curr_node_id, newNodeID);
                            if(nodeHashing(curr_edge_val, edgeTable, addr2)==0){
                                unsigned int newEdgeID = ++(*edge_cnt);
                                edgeTable[*addr2].hashNum = curr_edge_val;
                                edgeTable[*addr2].ID = newEdgeID;
                                edge_start[newEdgeID]=curr_node_id;
                                edge_end[newEdgeID]=newNodeID;
                                int *edge_label_cnt = &(edge_label[newEdgeID*100]);
                                int *edge_label_row = &(edge_label[newEdgeID*100+1]);
                                if((*edge_label_cnt)<99){
                                    edge_label_row[*edge_label_cnt]=ex_names[ex_idx]; //store the exploit name value
                                    (*edge_label_cnt)++;
                                }
                            }
                            else{
                                unsigned int existing_edge_id = edgeTable[*addr2].ID;
                                int *edge_label_cnt = &(edge_label[existing_edge_id*100]);
                                int *edge_label_row = &(edge_label[existing_edge_id*100+1]);
                                if((*edge_label_cnt)<99){
                                    edge_label_row[*edge_label_cnt]=ex_names[ex_idx]; //store the exploit name value
                                    (*edge_label_cnt)++;
                                }
                            }
                        }//priv escalation
                    }//check each exploit on the neighbor                      
                }//case 3
            }//must be a connected neighbor
        }//check each neighbor
    }//serial expansion while-loop   
    /*Serial expansion ends here*/
    free(masterQueue);
    free(nodeTable);
    free(edgeTable);
    free(edgeDirTable);
    free(rootAddr);
    free(addr);
    free(addr2);
    printf(">>>>>> From C: Number of nodes in the AG is %d\n", (*node_cnt));
    printf(">>>>>> From C: Number of edges in the AG is %d\n", (*edge_cnt));

    return edge_label;
}
