#include <iostream>
#include <vector>
#include <stdlib.h>
#include "graph.h"
#include "diff_func.h"
using namespace std;

double diffusion(vector<vector<struct X>> Strategy, int T, int sample_size, Graph& g){
    long double f = 0;
    int param_c;

    for(int i=0;i<sample_size;i++){
        vector<struct node*>susceptible, infected, ailing, threatened, recovered, dead;
        vector<vector<struct node*>*> total_group{&infected, &ailing, &threatened, &dead, &recovered}; 
        vector<vector<struct node*>*> all_group{&susceptible, &infected, &ailing, &threatened, &dead, &recovered}; 
        susceptible = g.N;
        
        srand(time(0));

        for(int j=0;j<g.V;j++){ // Init stage
            g.N[j]->stage = Stage::susceptible;
            double r = (rand() % 100)/100.0; //here: 0;
            double s_i = g.N[j]->params.relative * g.N[j]->params.contagion;
            cout<<"node_p: "<<s_i<<" < "<<r<<endl;
            if(r < s_i){
                g.N[j]->stage = Stage::infected;
                puts("sus -> inf");
                migrate(&susceptible, &infected, g.N[j]);
            }
        }
        cout<<"\nafter init\n";
        print_group(all_group);
        vector<vector<struct node*>*> positive_group{&infected, &ailing, &threatened};
        vector<vector<struct node*>*> health_group{&susceptible, &infected, &recovered};
        
        
        vector<struct node*> tmp_susceptible, tmp_infected, tmp_ailing, tmp_threatened, tmp_recovered, tmp_dead;
        vector<vector<struct node*>*> tmp_group{&tmp_infected, &tmp_ailing, &tmp_threatened, &tmp_dead, &tmp_recovered};// Shall align the order of total_group 
        for(int t=0;t<T;t++){// Quarantine
            printf("\n day: %d\n", t);
            for(size_t i=0;i<positive_group.size();i++){ // infected, ailing, threatened
                for(size_t j=0;j<positive_group[i]->size();j++){ // node of each group
                    struct node* positive_v = positive_group[i]->at(j);
                    infection_process(g, susceptible, tmp_infected, positive_v, Strategy[t]);
                    self_transmission_process(positive_group, tmp_group, positive_v);
                }
            }
            tmp_push_back(tmp_group, total_group);
            cout<<"\nafter infection & self\n";
            print_group(all_group);
            f += objective_at_t(health_group, Strategy[t], g.V, g.N);
        }
    }
    return f/(double)sample_size;
}