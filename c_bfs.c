#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <omp.h>


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
int *bfs(int *topo, int *num_ex, int *ex_names, int *pre_priv, int *post_priv, int *pacc, int cont_cnt, int outside_name, int docker_host_name, int max_num_ex, unsigned int *node_name, unsigned int *node_priv, unsigned int *edge_start, unsigned int *edge_end, unsigned int *node_cnt, unsigned int *edge_cnt, int numThreads, int queueSize){
    
    /*int min=0;
    int max=0;
    for(int i=0; i<cont_cnt; i++){
        for(int j=0; j<num_ex[i]; j++){
            if(post_priv[i*max_num_ex+j]>max) max=post_priv[i*max_num_ex+j];
            if(post_priv[i*max_num_ex+j]<min) min=post_priv[i*max_num_ex+j];            
        }
    }
    printf("\n\n>>>>>> From C: max %d, min %d\n", max, min);*/


    struct timeval c_start, c_end;
    gettimeofday(&c_start, NULL);

    printf("\n\n>>>>>> From C: serial execution starts ...\n");
    
    for(int i=0; i<cont_cnt; i++){
        if(i!=docker_host_name) topo[i*cont_cnt+i]=1;        
    }    

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
    //while(fifo_curr_size(masterQueue)){//serial expansion while-loop
    while(fifo_curr_size(masterQueue)<queueSize){//initial serial expansion
        unsigned int curr_node_id;
        fifo_read(masterQueue,&curr_node_id);
        unsigned int curr_node_name = node_name[curr_node_id];
        unsigned int curr_node_priv = node_priv[curr_node_id];
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
    free(rootAddr);
    free(addr);
    free(addr2);
    
    struct timeval c_middle;
    gettimeofday(&c_middle, NULL);
    double tSerial=(c_middle.tv_sec-c_start.tv_sec)+(c_middle.tv_usec-c_start.tv_usec)/1000000.0;
    

    printf(">>>>>> From C: serial execution done!!!\n");
    printf(">>>>>> Serial execution took %lf seconds\n", tSerial);
    printf(">>>>>> Serial excution expanded %d nodes\n", (*node_cnt)-fifo_curr_size(masterQueue));
    printf("\n");
    printf(">>>>>> From C: parallel execution starts ...\n");
    printf(">>>>>> master thread queue size: preset %d, actual %d\n", queueSize, fifo_curr_size(masterQueue));
    printf(">>>>>> Number of OpenMP threads is %d\n", numThreads);


    /*Parallel expansion starts here*/

    omp_lock_t *nLock = (omp_lock_t *)malloc(sizeof(omp_lock_t));
    omp_init_lock(nLock); //define a lock for OpenMP threads to write to shared data structures    

    omp_lock_t *eLock = (omp_lock_t *)malloc(sizeof(omp_lock_t));
    omp_init_lock(eLock); //define a lock for OpenMP threads to write to shared data structures    


    omp_lock_t *nodeLock = (omp_lock_t *)malloc(sizeof(omp_lock_t));
    omp_init_lock(nodeLock); //define a lock for OpenMP threads to write to shared data structures    

    omp_lock_t *edgeLock = (omp_lock_t *)malloc(sizeof(omp_lock_t));
    omp_init_lock(edgeLock); //define a lock for OpenMP threads to write to shared data structures    

    omp_lock_t *edgeDirLock = (omp_lock_t *)malloc(sizeof(omp_lock_t));
    omp_init_lock(edgeDirLock); //define a lock for OpenMP threads to write to shared data structures    

    omp_lock_t *labelLock = (omp_lock_t *)malloc(sizeof(omp_lock_t));
    omp_init_lock(labelLock); //define a lock for OpenMP threads to write to shared data structures


    omp_lock_t *threadLocks = (omp_lock_t *)malloc(numThreads*sizeof(omp_lock_t));
    for(int i=0; i<numThreads; i++) omp_init_lock(&threadLocks[i]);

    fifo *threadQueues = (fifo *)malloc(numThreads*sizeof(fifo));
    for(int i=0; i<numThreads; i++) fifo_init(&threadQueues[i]);
 
    int *numExpansions = (int *)malloc(sizeof(int)*numThreads);
    for(int i=0; i<numThreads; i++) numExpansions[i]=0;

    #pragma omp parallel num_threads(numThreads) default(none) shared(nLock, eLock, nodeLock, edgeLock, edgeDirLock, labelLock, threadLocks, edge_label, masterQueue, nodeTable, edgeTable, edgeDirTable, topo, num_ex, ex_names, pre_priv, post_priv, pacc, cont_cnt, outside_name, docker_host_name, max_num_ex, node_name, node_priv, edge_start, edge_end, node_cnt, edge_cnt, numThreads, queueSize, numExpansions, threadQueues)
    {//OpenMP starts
        
        //set local environment and initialize local workload
        int threadID = omp_get_thread_num();
        omp_lock_t *myLock = &threadLocks[threadID];
        int *myCnt = &numExpansions[threadID]; 
        unsigned int *addr = (unsigned int *)malloc(sizeof(unsigned int));
        unsigned int *addr2 = (unsigned int *)malloc(sizeof(unsigned int)); 
        fifo *localQueue = &threadQueues[threadID];
        for(int fcnt=threadID; fcnt<fifo_curr_size(masterQueue); fcnt+=numThreads){
            unsigned int initID;
            fifo_idx_read(masterQueue, fcnt, &initID);
            //omp_set_lock(myLock);
            fifo_write(localQueue, initID);
            //omp_unset_lock(myLock);
        }
        
        unsigned int curr_node_id;
        unsigned int curr_node_name;
        unsigned int curr_node_priv;

        hashUnit *localEdgeTable = (hashUnit *)malloc(sizeof(hashUnit)*20000033);
        for(int i=0; i<20000033; i++) localEdgeTable[i].hashNum=0;

        int *nodePriv = (int *)malloc(5*sizeof(int));
        int *edgePriv = (int *)malloc(5*sizeof(int));
        int *nodeID = (int *)malloc(5*sizeof(int));


        printf(">>>>>> Thread %d starts\n", threadID);

        expanding_loop:;
        fifo_read(localQueue,&curr_node_id);
        (*myCnt)++;
        //if(curr_node_id%20==0) printf(">>>>>> Thread %d expands node %d\n", threadID, curr_node_id);
        curr_node_name = node_name[curr_node_id];
        curr_node_priv = node_priv[curr_node_id];
        for(int ncnt=0; ncnt<cont_cnt; ncnt++){//check each neighbor, ncnt is neighbor name value
            if(topo[curr_node_name*cont_cnt+ncnt]==1){//must be a connected neighbor
                omp_set_lock(nodeLock);
                if(curr_node_name == docker_host_name && nodeHashing(nodeEncoding(ncnt,4),nodeTable,addr)==1){//case 1
                    omp_unset_lock(nodeLock);
                    omp_set_lock(edgeDirLock);
                    int reversed = nodeHashing(edgeDirEncoding(ncnt,curr_node_name,4),edgeDirTable,addr2);
                    if(reversed){
                        omp_unset_lock(edgeDirLock);                       
                        continue;
                    }
                    else{//update edge direction table
                        unsigned int curr_edge_dir_val = edgeDirEncoding(curr_node_name,ncnt,4);
                        nodeHashing(curr_edge_dir_val,edgeDirTable,addr2);
                        edgeDirTable[*addr2].hashNum=curr_edge_dir_val;//works for both hit and miss
                        omp_unset_lock(edgeDirLock);
                    }
                    
                    unsigned int nvalue = nodeEncoding(ncnt,4);
                    unsigned int newNodeID;
                    omp_set_lock(nodeLock);
                    if(nodeHashing(nvalue,nodeTable,addr)==0){
                        newNodeID = ++(*node_cnt);
                        nodeTable[*addr].hashNum=nvalue;// update node hashtable
                        nodeTable[*addr].ID=newNodeID;
                        omp_unset_lock(nodeLock);
                        node_name[newNodeID]=ncnt;
                        node_priv[newNodeID]=4;
                        fifo_write(localQueue, newNodeID);//?
                    }
                    else{
                        omp_unset_lock(nodeLock);
                        newNodeID = nodeTable[*addr].ID;
                    }

                    unsigned int curr_edge_val = edgeEncoding(curr_node_id, newNodeID);
                    omp_set_lock(edgeLock);
                    if(nodeHashing(curr_edge_val, edgeTable, addr2)==0){
                        omp_unset_lock(edgeLock);
                        omp_set_lock(eLock);
                        unsigned int newEdgeID = ++(*edge_cnt);
                        omp_unset_lock(eLock);
                        omp_set_lock(edgeLock);
                        edgeTable[*addr2].hashNum = curr_edge_val;
                        edgeTable[*addr2].ID = newEdgeID;
                        omp_unset_lock(edgeLock);
                        edge_start[newEdgeID]=curr_node_id;
                        edge_end[newEdgeID]=newNodeID;
                        int *edge_label_cnt = &(edge_label[newEdgeID*100]);
                        int *edge_label_row = &(edge_label[newEdgeID*100+1]);
                        omp_set_lock(labelLock); 
                        if((*edge_label_cnt)<99){
                            edge_label_row[*edge_label_cnt]=-1; //-1 represents root access due to docker host being compromised
                            (*edge_label_cnt)++;
                        }
                        omp_unset_lock(labelLock);
                    }
                    else{
                        omp_unset_lock(edgeLock);
                        unsigned int existing_edge_id = edgeTable[*addr2].ID;
                        int *edge_label_cnt = &(edge_label[existing_edge_id*100]);
                        int *edge_label_row = &(edge_label[existing_edge_id*100+1]);
                        omp_set_lock(labelLock);
                        if((*edge_label_cnt)<99){
                            edge_label_row[*edge_label_cnt]=-1; //-1 represents root access due to docker host being compromised
                            (*edge_label_cnt)++;
                        }
                        omp_unset_lock(labelLock);
                    }
                }//case 1
                else if(ncnt == docker_host_name && pacc[curr_node_name]==1){//case 2
                    omp_unset_lock(nodeLock);
                    omp_set_lock(edgeDirLock);
                    int reversed = nodeHashing(edgeDirEncoding(ncnt,curr_node_name,curr_node_priv),edgeDirTable,addr2);
                    if(reversed){
                        omp_unset_lock(edgeDirLock);
                        continue;
                    }
                    else{//update edge direction table
                        unsigned int curr_edge_dir_val = edgeDirEncoding(curr_node_name,ncnt,4);
                        nodeHashing(curr_edge_dir_val,edgeDirTable,addr2);
                        edgeDirTable[*addr2].hashNum=curr_edge_dir_val;
                        omp_unset_lock(edgeDirLock);
                    }
                   
                    unsigned int nvalue = nodeEncoding(ncnt,4);
                    unsigned int newNodeID;
                    omp_set_lock(nodeLock);
                    if(nodeHashing(nvalue, nodeTable, addr)==0){
                        newNodeID = ++(*node_cnt); 
                        nodeTable[*addr].hashNum=nvalue;// update node hashtable
                        nodeTable[*addr].ID=newNodeID;
                        omp_unset_lock(nodeLock);
                        node_name[newNodeID]=ncnt;
                        node_priv[newNodeID]=4;
                        fifo_write(localQueue, newNodeID);
                    }
                    else{
                        omp_unset_lock(nodeLock);
                        newNodeID = nodeTable[*addr].ID;
                    }
                      
                    unsigned int curr_edge_val = edgeEncoding(curr_node_id, newNodeID);
                    omp_set_lock(edgeLock);
                    if(nodeHashing(curr_edge_val, edgeTable, addr2)==0){
                        omp_unset_lock(edgeLock);
                        omp_set_lock(eLock);
                        unsigned int newEdgeID = ++(*edge_cnt);
                        omp_unset_lock(eLock);
                        omp_set_lock(edgeLock);
                        edgeTable[*addr2].hashNum = curr_edge_val;
                        edgeTable[*addr2].ID = newEdgeID;
                        omp_unset_lock(edgeLock);
                        edge_start[newEdgeID]=curr_node_id;
                        edge_end[newEdgeID]=newNodeID;
                        int *edge_label_cnt = &(edge_label[newEdgeID*100]);
                        int *edge_label_row = &(edge_label[newEdgeID*100+1]);
                        omp_set_lock(labelLock);
                        if((*edge_label_cnt)<99){
                            edge_label_row[*edge_label_cnt]=-2; //-2 represents priviledged access to compromise docker host
                            (*edge_label_cnt)++;
                        }
                        omp_unset_lock(labelLock);
                    }
                    else{
                        omp_unset_lock(edgeLock);
                        unsigned int existing_edge_id = edgeTable[*addr2].ID;
                        int *edge_label_cnt = &(edge_label[existing_edge_id*100]);
                        int *edge_label_row = &(edge_label[existing_edge_id*100+1]);
                        omp_set_lock(labelLock);
                        if((*edge_label_cnt)<99){
                            edge_label_row[*edge_label_cnt]=-2; //-2 represents priviledged access to compromise docker host
                            (*edge_label_cnt)++;
                        }
                        omp_unset_lock(labelLock); 
                    }                                       
                }//case 2
                else if(ncnt != outside_name && ncnt != docker_host_name){//case 3
                    omp_unset_lock(nodeLock);

                    omp_set_lock(edgeDirLock);
                    int reversed = nodeHashing(edgeDirEncoding(ncnt,curr_node_name,curr_node_priv), edgeDirTable,addr2);
                    omp_unset_lock(edgeDirLock);
                    if(reversed) continue;
                    nodePriv[1] = 0;
                    nodePriv[2] = 0;
                    nodePriv[3] = 0;
                    nodePriv[4] = 0;
                    edgePriv[1] = 0;
                    edgePriv[2] = 0;
                    edgePriv[3] = 0;
                    edgePriv[4] = 0;

                    for(int ecnt=0; ecnt<num_ex[ncnt]; ecnt++){//check each exploit on the neighbor
                        int ex_idx = ncnt*max_num_ex + ecnt;
                        if(curr_node_priv >= pre_priv[ex_idx] && ((ncnt!=curr_node_name && post_priv[ex_idx]!=0)||(ncnt==curr_node_name && curr_node_priv<post_priv[ex_idx]))){//priv escalation

                            unsigned int nvalue = nodeEncoding(ncnt,post_priv[ex_idx]);
                            unsigned int newNodeID;
                             
                            if(edgePriv[post_priv[ex_idx]]==0){
                                unsigned int curr_edge_dir_val = edgeDirEncoding(curr_node_name,ncnt,post_priv[ex_idx]);
                                omp_set_lock(edgeDirLock);                            
                                if(nodeHashing(curr_edge_dir_val,edgeDirTable,addr2)==0){
                                    edgeDirTable[*addr2].hashNum=curr_edge_dir_val;
                                }
                                omp_unset_lock(edgeDirLock);                                
                                edgePriv[post_priv[ex_idx]]=1;
                            }
                             
                            /*omp_set_lock(nodeLock);
                            if(nodeHashing(nvalue, nodeTable, addr)==0){
                                newNodeID = ++(*node_cnt);
                                nodeTable[*addr].hashNum=nvalue;// update node hashtable
                                nodeTable[*addr].ID=newNodeID;
                                omp_unset_lock(nodeLock);
                                node_name[newNodeID]=ncnt;
                                node_priv[newNodeID]=post_priv[ex_idx];
                                fifo_write(localQueue, newNodeID);
                            }
                            else{
                                omp_unset_lock(nodeLock);
                                newNodeID = nodeTable[*addr].ID;
                            }*/

                            if(nodePriv[post_priv[ex_idx]]==0){
                                omp_set_lock(nodeLock);
                                if(nodeHashing(nvalue, nodeTable, addr)==0){
                                    newNodeID = ++(*node_cnt);
                                    nodeTable[*addr].hashNum=nvalue;// update node hashtable
                                    nodeTable[*addr].ID=newNodeID;
                                    omp_unset_lock(nodeLock);
                                    node_name[newNodeID]=ncnt;
                                    node_priv[newNodeID]=post_priv[ex_idx];
                                    //omp_set_lock(myLock);
                                    fifo_write(localQueue, newNodeID);
                                    //omp_unset_lock(myLock);
                                }
                                else{
                                    omp_unset_lock(nodeLock);
                                    newNodeID = nodeTable[*addr].ID;
                                }
                                nodeID[post_priv[ex_idx]]=newNodeID;
                                nodePriv[post_priv[ex_idx]]=1;
                            }
                            else{
                                newNodeID = nodeID[post_priv[ex_idx]];
                            }
                            

                            /*unsigned int curr_edge_dir_val = edgeDirEncoding(curr_node_name,ncnt,post_priv[ex_idx]);
                            omp_set_lock(edgeDirLock);                            
                            nodeHashing(curr_edge_dir_val,edgeDirTable,addr2);
                            edgeDirTable[*addr2].hashNum=curr_edge_dir_val;
                            omp_unset_lock(edgeDirLock);*/


                            unsigned int curr_edge_val = edgeEncoding(curr_node_id, newNodeID);
                            if(nodeHashing(curr_edge_val, localEdgeTable, addr2)==0){
                                omp_set_lock(eLock);
                                unsigned int newEdgeID = ++(*edge_cnt);
                                omp_unset_lock(eLock);
                                edge_start[newEdgeID]=curr_node_id;
                                edge_end[newEdgeID]=newNodeID;
                                localEdgeTable[*addr2].hashNum = curr_edge_val;
                                localEdgeTable[*addr2].ID = newEdgeID;
                                int *edge_label_cnt = &(edge_label[newEdgeID*100]);
                                int *edge_label_row = &(edge_label[newEdgeID*100+1]);
                                if((*edge_label_cnt)<99){
                                    edge_label_row[*edge_label_cnt]=ex_names[ex_idx]; //store the exploit name value
                                    (*edge_label_cnt)++;
                                }
                            }
                            else{
                                unsigned int existing_edge_id = localEdgeTable[*addr2].ID;
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
                else{//case 4, trivial, only to release nodeLock
                    omp_unset_lock(nodeLock);
                }//case 4
            }//must be a connected neighbor
        }//check each neighbor
        
        //int half=1;
        //int minS=3;
        //omp_set_lock(myLock);
        if(fifo_curr_size(localQueue)){
            //omp_unset_lock(myLock);
            goto expanding_loop;
        }
        /*else{
            omp_unset_lock(myLock);
            int numStolen = 0;
            unsigned int idBuf[half];
            for(int m=1; m<numThreads*5; m++){
                omp_set_lock(&threadLocks[(threadID+m)%numThreads]);
                if(fifo_curr_size(&threadQueues[(threadID+m)%numThreads])>=minS){
                    for(int n=0; n<half; n++){
                        fifo_read(&threadQueues[(threadID+m)%numThreads], &idBuf[n]);
                    }
                    numStolen=half;
                }
                omp_unset_lock(&threadLocks[(threadID+m)%numThreads]);
                if(numStolen>0){
                    omp_set_lock(myLock);
                    for(int n=0; n<numStolen; n++) fifo_write(localQueue, idBuf[n]);
                    omp_unset_lock(myLock);
                    break;
                }
            }
            if(numStolen==0) goto finish;
            else goto expanding_loop;
        }*/
        finish:;

        free(addr);
        free(addr2);
        free(localEdgeTable);        
    }//OpenMP ends 
   
    printf("\n>>>>>> From C: parallel execution done!!!\n");
    for(int i=0; i<numThreads; i++){
        printf(">>>>>> Thread %d expaned %d nodes\n", i, numExpansions[i]);
    }

    free(masterQueue);
    free(nodeTable);
    free(edgeTable);
    free(edgeDirTable);
    free(nodeLock);
    free(edgeLock);
    free(edgeDirLock);
    free(nLock);
    free(eLock);
    free(labelLock);
    free(threadLocks);
    free(numExpansions);
    free(threadQueues);   

    gettimeofday(&c_end, NULL);
    double tParallel=(c_end.tv_sec-c_middle.tv_sec)+(c_end.tv_usec-c_middle.tv_usec)/1000000.0;
    double tdiff=(c_end.tv_sec-c_start.tv_sec)+(c_end.tv_usec-c_start.tv_usec)/1000000.0;
    printf(">>>>>> Parallel execution took %lf seconds\n", tParallel);

    /*struct timeval s1,e1;
    gettimeofday(&s1, NULL);
    #pragma omp parallel for num_threads(numThreads) schedule(static,1)
    for(long i=0; i<2000000000; i++){
        long b=i*i*i*i*i;
    }
    gettimeofday(&e1, NULL);
    double td= e1.tv_sec-s1.tv_sec + (e1.tv_usec-s1.tv_usec)/1000000.0;
    printf(">>>>>> td is %lf seconds\n", td);*/

    printf("\n");
    printf(">>>>>> From C: Number of nodes in the AG is %d\n", (*node_cnt));
    printf(">>>>>> From C: Number of edges in the AG is %d\n", (*edge_cnt));
    printf(">>>>>> From C: Total Expansion Time: %lf seconds\n", tdiff);
    printf("\n\n");
    return edge_label;
}
