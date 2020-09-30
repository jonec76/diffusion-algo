#include <stdio.h>
#include <string.h>
#include <time.h>
#include "init.h"
#include "mipc.h"

const char* NAME = "mipc.txt";
extern char GRAPH_PATH[50];
extern char OUTPUT_FILE[30];

extern size_t period_T;
extern double A_END ;
extern double THETA ;
extern int TARGET_V ;
extern vector<struct el> level_table;

int period_t=3; // 0 means himself, 1 means edge len=1 (1st extand)
vector<vector<X> > S;

char get_s(Stage s);
void output_path(vector<vector<Path>>&  infection_path);
void do_extend(vector<Path>& extend_path, vector<Path>& original_path, Path p);
double h_prob(vector<vector<Path>> infection_path, size_t period_t, vector<vector<X> > S, Graph& g);

int main(int argc, char** argv){
    Graph g;
    set_mipc_config(argv[1], NAME);
    create_graph(g, GRAPH_PATH);
    vector<vector<Path>> infection_path;
    for(size_t j=0;j<g.V;j++){ // Init stage
        g.N[j]->stage = Stage::susceptible;
        double r = (rand() % 100)/100.0; //here: 0;
        double s_i = g.N[j]->params.relative * g.N[j]->params.contagion;
        if(r < s_i){
            g.N[j]->stage = Stage::infected;
        }
    }
    g.N[TARGET_V]->stage = Stage::infected; // To be removed;
    algo_mipc(g, TARGET_V, THETA, infection_path);

    h_prob(infection_path, period_t, g.U, g);
}

// Because parameters get a random value between 0~1
// so the probability of edge would become negative after calculating the following formula
//   prob * (1 - end_node->params.symptom - end_node->params.healing_fromI);


void algo_mipc(Graph& g, int target_v, double theta, vector<vector<Path>>& infection_path){

    printf("Start Multi-hop infection Path Construction of node [%d].. \n\n", target_v);

    vector<Path> single_path;
    vector<bool> visited(g.V, false);

    Path p;
    p.neighbor = target_v;
    p.path_prob = 1;
    visited[target_v] = true;
    p.visited = visited;
    p.neighbor_stage = g.N[target_v]->stage;

    single_path.push_back(p);
    infection_path.push_back(single_path);

    for(size_t i=0;i<infection_path.size();i++){
        if(infection_path[i].size() > period_T)
            break;
        
        vector<Path> original_path = infection_path[i];
        int end_id = original_path[original_path.size()-1].neighbor;
        struct node* end_node = g.N[end_id];
        Stage end_stage = original_path[original_path.size()-1].neighbor_stage;
        double end_prob = original_path[original_path.size()-1].path_prob;
        Path p;
        
        if(end_prob * A_END < theta)
            continue;
        switch (end_stage){
            case Stage::infected:
                if(end_id != target_v){
                    vector<Path> extend_path = original_path;
                    p.neighbor = end_id;
                    p.neighbor_stage = Stage::infected;
                    p.path_prob = end_prob * (1 - end_node->params.symptom - end_node->params.healing_fromI);
                    do_extend(extend_path, original_path, p);
                    infection_path.push_back(extend_path);
                }

                for(size_t e=0;e < g.adj[end_id].size();e++){
                    struct node* u = g.N[g.adj[end_id][e].neighbor];
                    double edge_prob = g.get_edge_prob(g.N[target_v], u);
                    if(original_path[original_path.size()-1].visited[u->id])
                        continue;
                    for(int i=0;i<3;i++){
                        vector<Path> extend_path = original_path;
                        Path p ;
                        p.neighbor = u->id;
                        switch (i){
                            case 0:
                                p.path_prob = end_prob * edge_prob * end_node->params.relative * end_node->params.contagion;
                                p.neighbor_stage = Stage::infected;
                                break;
                            case 1:
                                p.path_prob = end_prob * edge_prob * end_node->params.contagion;
                                p.neighbor_stage = Stage::ailing;
                                break;
                            case 2:
                                p.path_prob = end_prob * edge_prob * end_node->params.contagion;
                                p.neighbor_stage = Stage::threatened;
                                break;
                            default:
                                break;
                        }
                        do_extend(extend_path, original_path, p);
                        extend_path[extend_path.size()-1].visited[u->id] = true;
                        infection_path.push_back(extend_path);

                        // For debuggin the visited boolean table.
                        // cout<<"end: "<<end_id<<": ";
                        // for(int i=0;i < original_path[original_path.size()-1].visited.size();i++)
                        //     cout<<original_path[original_path.size()-1].visited[i]<<" ";
                        // cout<<endl; 
                    }
                }
                break;
            case Stage::ailing:
                for(int i=0;i<2;i++){
                    vector<Path> extend_path = original_path;
                    Path p;
                    p.neighbor = end_id;
                    switch (i){
                        case 0:
                            p.path_prob = end_prob * end_node->params.symptom;
                            p.neighbor_stage = Stage::infected;
                            break;
                        case 1:
                            p.path_prob = end_prob * (1 - end_node->params.critical - end_node->params.healing_fromA);
                            p.neighbor_stage = Stage::ailing;
                            break;
                        default:
                            break;
                    }
                    do_extend(extend_path, original_path, p);
                    infection_path.push_back(extend_path);
                }
                break;
            case Stage::threatened:
                for(int i=0;i<2;i++){
                    vector<Path> extend_path = original_path;
                    Path p;
                    p.neighbor = end_id;
                    switch (i){
                        case 0:
                            p.path_prob = end_prob * end_node->params.critical;
                            p.neighbor_stage = Stage::ailing;
                            break;
                        case 1:
                            p.path_prob = end_prob * (1 - end_node->params.death - end_node->params.healing_fromT);
                            p.neighbor_stage = Stage::threatened;
                            break;
                        default:
                            break;
                    }
                    do_extend(extend_path, original_path, p);
                    infection_path.push_back(extend_path);
                }
                break;
            default:
                continue;
        }
        if(end_stage != Stage::infected){
            infection_path.erase(i + infection_path.begin());
            i--;
        }else{
            original_path[original_path.size()-1].path_prob *= A_END;
        }
    }

    // check the len=T-1 path and remove it if the end.Stage is Infected;
    for(int k=infection_path.size()-1;k>=0;k--){ 
        vector<Path> check_path = infection_path[k];
        size_t len = check_path.size()-1;
        if(len != period_T-1)
            break;
        if(check_path[len].neighbor_stage != Stage::infected){
            infection_path.erase(k + infection_path.begin());
        }  
    }
    output_path(infection_path);
}

void do_extend(vector<Path>& extend_path, vector<Path>& original_path, Path p){
    extend_path.push_back(p);
    extend_path[extend_path.size()-1].visited = original_path[original_path.size()-1].visited;
}

void output_path(vector<vector<Path>>&  infection_path){
    size_t total_path_num = infection_path.size();
    string line = "";
    FILE *fp = fopen(OUTPUT_FILE, "a");
    if (fp == NULL) {
        printf("Failed to open file %s.", OUTPUT_FILE);
        exit(EXIT_FAILURE);
    }
    for(size_t i=0;i<total_path_num;i++){
        fprintf(fp, "%-5ld :", i);
        if(infection_path[i].size() == 0)
            continue;
        fprintf(fp, " %d(%c)", infection_path[i][0].neighbor, get_s(infection_path[i][0].neighbor_stage));
        for(size_t j=1;j<infection_path[i].size();j++){
            fprintf(fp, " <---%.3f--- %d(%c)", infection_path[i][j].path_prob, infection_path[i][j].neighbor, get_s(infection_path[i][j].neighbor_stage));
        }
        fprintf(fp, "\n\n");
    }
    fclose(fp);
}

char get_s(Stage s){
    char res;
    switch (s){
        case Stage::ailing:
            res = 'a';
            return res;
        case Stage::susceptible:
            res = 's';
            return res;
        case Stage::infected :
            res = 'i';
            return res;
        case Stage::recovered :
            res = 'r';
            return res;
        case Stage::dead :
            res = 'd';
            return res;
        case Stage::threatened :
            res = 't';
            return res;
        default:
            return '?';
            break;
    }
}

double h_prob(vector<vector<Path>> infection_path, size_t T, vector<vector<X> > S, Graph& g){
    double A_END;
    double AIP=1;

    if(period_t == 0)
        return A_END;

    vector<vector<Path>> infection_path_t;
    for(size_t i=0;i<infection_path.size();i++){
        // period + 1: If period=3 means we want |path|=3, but this condition will only add the path whose len=2
        if(infection_path[i].size() == period_t+1)
            infection_path_t.push_back(infection_path[i]);
    }

    if(S.size() < period_t){
        printf("Wrong strategies set sizes");
        return 0;
    }

    for(size_t i=0;i<infection_path_t.size();i++){
        vector<Path> path = infection_path_t[i];
        double success = path[period_t].path_prob;

        // t >= 1: the reason for setting this lower bound is we'll take two nodes at one iteration.
        // infection_path_t[T-1] will be affected by quarantine strategy (first day)
        for(int t=period_t;t>=1;t--){
            int u_today = path[t].neighbor, u_next_day = path[t-1].neighbor;

            // // Strategies starts from 0 to period_t-1, but the infection path traverse with the reverse order.
            g.set_node_lv(S[period_t - t]); 

            // For debuging each edge level (set level value with the max level after comparing from two nodes).
            // printf("today:%d(level=%d) , next day: %d(level=%d) |", u_today, g.N[u_today]->q_level, u_next_day, g.N[u_next_day]->q_level);
            double remove_prob = max(level_table[g.N[u_today]->q_level].remove_p, level_table[g.N[u_next_day]->q_level].remove_p);
            success *= (1 - remove_prob);
        }
        AIP *= (1-success);
    }

    return AIP;
}