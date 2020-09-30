#include <string.h>
#include <fstream>
#include <iostream>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include "data.h"
#include "diff_func.h"
#include "graph.h"
#include "init.h"

using namespace std;

struct X X1, X2, X3, X4;
struct el el0 = {0, 0.5}, el1 = {0.3, 0.3}, el2 = {0.7, 0.7}, el3 = {1, 1};
vector<struct el> level_table = {el0, el1, el2, el3};
const char* RESULT_DIR = "./result/";
char OUTPUT_FILE[30];

char GRAPH_PATH [50];
size_t sample_size = 1, period_T=3;
double budget = 10;

double A_END  = 0;
double THETA  = 0;
int TARGET_V  = 0;

void init_node(Graph& g, vector<char*>& input_line){
    struct node* n = (struct node*)malloc(sizeof(struct node));
    n->id = atoi(input_line[0]);
    n->type = atoi(input_line[1]);
    n->params.relative = atof(input_line[2]);
    n->params.contagion = atof(input_line[3]);
    n->params.symptom = atof(input_line[4]);
    n->params.critical = atof(input_line[5]);
    n->params.healing_fromI = atof(input_line[6]);
    n->params.healing_fromA = atof(input_line[7]);
    n->params.healing_fromT = atof(input_line[8]);
    n->params.death = atof(input_line[9]);
    g.N.push_back(n);
}

void init_edge(Graph& g, vector<char*>& input_line){
    g.addEdge(atoi(input_line[0]), atoi(input_line[1]), atof(input_line[2]));
}

void init_strategy(Graph& g, vector<char*>& input_line){
    struct X x;
    x.t = atoi(input_line[0]);
    x.cost = atoi(input_line[1]);
    x.lv = atoi(input_line[2]);
    x.eta = atoi(input_line[3]);
    x.id = g.U[x.t].size();
    char delim[] = ",";
    char *token = strtok(input_line[4], delim);
    while (token) {
        x.D.push_back(atoi(token));
        token = strtok(NULL, delim);
    }
    g.U[x.t].push_back(x);
}

void get_split_data(vector<char*>& input_line, char* data, char const data_delim[]){
    char *token = strtok(data, data_delim);
    while (token) {
        input_line.push_back(token);
        token = strtok(NULL, data_delim);
    }
}


/**
 * " ": split type
 * ",": split data(exclude group data)
 * "_": split group data
 */
void create_graph(Graph &g, const char* GRAPH_FILE) {
    FILE *fp_graph = fopen(GRAPH_FILE, "r");
    if (fp_graph == NULL) {
        printf("Failed to open file %s.", GRAPH_FILE);
        exit(EXIT_FAILURE);
    }
    
    DIR* dir = opendir(RESULT_DIR);
    if (dir) {
        closedir(dir);
    } else{
        if (mkdir("result", S_IRWXU|S_IRWXG|S_IROTH))
            printf("wrong at create dir");
    }
    
    char *line = NULL;
    size_t len = 0;
    
    while ((getline(&line, &len, fp_graph)) != -1) {
        char* type = strtok(line, " ");
        char* data = strtok(NULL, " ");
        vector<char*>input_line;
        if(strcmp(type, "g") == 0){
            get_split_data(input_line, data, ",");
            int V=atoi(input_line[0]);
            int E=atoi(input_line[1]); //input_line[2]: U length 
            int U_LENGTH = atoi(input_line[2]);
            g.init_graph(V, E, U_LENGTH);
        }else if(strcmp(type, "e") == 0){
            get_split_data(input_line, data, ",");
            init_edge(g, input_line);
        }else if(strcmp(type, "n") == 0){
            get_split_data(input_line, data, ",");
            init_node(g, input_line);
        }else if(strcmp(type, "X") == 0){
            get_split_data(input_line, data, "_");
            init_strategy(g, input_line); // TODO
        }else{
            cout<<"wrong input"<<endl;
        }
    }
    fclose(fp_graph);
    if (line) free(line);
}


void set_config(char* argv, const char* file_name){
    FILE *fp_config = fopen(argv, "r");
    if (fp_config == NULL) {
        printf("Failed to open file %s.", argv);
        exit(EXIT_FAILURE);
    }
    
    char *line = NULL;
    size_t len = 0;
    vector<char*>input_line;
    
    while ((getline(&line, &len, fp_config)) != -1) {
        char* type = strtok(line, " ");
        char* data = strtok(NULL, " ");
        if(strcmp(type, "#") == 0){
            continue;
        }else if(strcmp(type, "c") == 0){
            get_split_data(input_line, data, ",");
            strncpy(GRAPH_PATH, input_line[0], 50);
            sample_size = atoi(input_line[1]);
            budget = atof(input_line[2]);
            period_T = atoi(input_line[3]);
        }
    }
    
    strncpy(OUTPUT_FILE, RESULT_DIR, 10);
    strcat(OUTPUT_FILE, file_name);
    FILE* pFile = fopen (OUTPUT_FILE, "a");
    if (pFile == NULL) {
        printf("Failed to open file %s.", argv);
        exit(EXIT_FAILURE);
    }
    fprintf (pFile, "=======================================================\n");
    fprintf (pFile, "%-15s %s\n%-15s %s\n%-15s %s\n%-15s %s\n","Graph path: ", input_line[0], "Sample size: ", input_line[1], "Budget: ", input_line[2], "T:", input_line[3]);
    fprintf (pFile, "=======================================================\n");
    fclose (pFile);
    fclose (fp_config);
}

void set_mipc_config(char* argv, const char* file_name){
    FILE *fp_config = fopen(argv, "r");
    if (fp_config == NULL) {
        printf("Failed to open file %s.", argv);
        exit(EXIT_FAILURE);
    }
    
    char *line = NULL;
    size_t len = 0;
    vector<char*>input_line;
    
    while ((getline(&line, &len, fp_config)) != -1) {
        char* type = strtok(line, " ");
        char* data = strtok(NULL, " ");
        if(strcmp(type, "#") == 0){
            continue;
        }else if(strcmp(type, "m") == 0){
            get_split_data(input_line, data, ",");
            strncpy(GRAPH_PATH, input_line[0], 50);
            A_END  = atof(input_line[1]);
            THETA  = atof(input_line[2]);
            period_T = atoi(input_line[3]);
            TARGET_V  = atoi(input_line[4]);
        }
    }
    
    strncpy(OUTPUT_FILE, RESULT_DIR, 10);
    strcat(OUTPUT_FILE, file_name);
    FILE* pFile = fopen (OUTPUT_FILE, "a");
    if (pFile == NULL) {
        printf("Failed to open file %s.", argv);
        exit(EXIT_FAILURE);
    }
    fprintf (pFile, "=======================================================\n");
    fprintf (pFile, "%-15s %s\n%-15s %s\n%-15s %s\n%-15s %s\n","Graph path: ", input_line[0], "Sample size: ", input_line[1], "Budget: ", input_line[2], "T:", input_line[3]);
    fprintf (pFile, "=======================================================\n");
    fclose (pFile);
    fclose (fp_config);
}
