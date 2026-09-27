#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <set>
#include <unordered_set>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <ctime>

#include "Struct_type.h"

using namespace std;


int numNode = 0;
long long numEdge = 0;
int labelNum = 2;                       // '+' and '-'
int bfs_count = 0;
int search_count = 0;
int max_depth = 0;
int conflict_size = 0;
int resultNumber = 0;
bool found = false;                     // CHANGE 1: stop at the first result
vector<vector<edge> > graph;
vector<vector<edge> > Back_graph;
vector<unordered_map<int, unordered_set<int> > > conflictSet;       // [node][state] -> set
vector<unordered_map<int, unordered_set<int> > > conflictSetUnion;  // [node][state] -> set
vector<int> resultsNode, resultsLabel, resultState;
unordered_set<int> passNode;
FinalDFA dfa;
vector<unordered_map<int, bool> > BFS_Arrive;
queue<pair<int, int> > Q;

// run-wide instrumentation (not part of [38])
long long stat_bfs_states = 0;
long long stat_conflict_entries = 0;
int       stat_max_depth = 0;

bool pathConflict(int node, int state){
    for(auto it = conflictSet[node][state].begin();it != conflictSet[node][state].end();it++){
        if(passNode.count(*it)){
            return true;
        }
    }
    return false;
}

void update_conflictSet(int node,int state){
    int number = 0;
    vector<int> setTemp;
    unordered_set<int> temp;
    if(conflictSetUnion[node][state].size() > 1){
        conflictSetUnion[node][state].clear();
        return;
    }
    for(auto it = graph[node].begin();it != graph[node].end();it++){
        int nextnode = it->toNode;
        if(dfa.labelToState[state].count(it->label)){
            int nextstate = dfa.labelToState[state][it->label];
            if(!BFS_Arrive[nextnode][nextstate]){
                if(conflictSetUnion[node][state].count(nextnode)){
                    setTemp.push_back(nextnode);
                    number++;
                }else{
                    if(conflictSet[nextnode][nextstate].size() == 0){
                        conflictSetUnion[node][state].clear();
                        return;
                    }
                    number++;
                    for(auto it1 = conflictSet[nextnode][nextstate].begin();it1 != conflictSet[nextnode][nextstate].end();it1++){
                        setTemp.push_back(*it1);
                    }
                }
            }
        }
    }
    if(number == 1){
        for(auto it = setTemp.begin();it != setTemp.end();it++){
            conflict_size++;
            conflictSet[node][state].insert(*it);
        }
    }else{
        for(auto it = setTemp.begin();it != setTemp.end();it++){
            if(temp.count(*it)){
                conflict_size++;
                conflictSet[node][state].insert(*it);
            }else{
                temp.insert(*it);
            }
        }
    }
    conflictSetUnion[node][state].clear();
    setTemp.clear();
    temp.clear();
}

void BFS(int endID,int end_state){
    pair<int,int> temp;
    temp.first = endID;
    temp.second = end_state;
    Q.push(temp);
    BFS_Arrive[endID][end_state] = true;
    while(!Q.empty()){
        int node = Q.front().first;
        int state = Q.front().second;
        Q.pop();
        for(auto it = Back_graph[node].begin();it != Back_graph[node].end();it++){
            int label = it->label;
            if(dfa.Back_labelToState[state].count(label)){
                int nextnode = it->toNode;
                for(auto iter = dfa.Back_labelToState[state][label].begin();iter != dfa.Back_labelToState[state][label].end();iter++){
                    int nextstate = *iter;
                    if(!BFS_Arrive[nextnode].count(nextstate)){
                        bfs_count++;
                        temp.first = nextnode;
                        temp.second = nextstate;
                        Q.push(temp);
                        BFS_Arrive[nextnode][nextstate] = true;
                    }
                }
            }
        }
    }
}

bool pruning(int node,int state,int dst){
    bool f = false;
    if(found){
        return false;
    }
    if(!BFS_Arrive[node][state]){
        return false;
    }
    passNode.insert(node);

    if(pathConflict(node,state)){
        passNode.erase(node);
        return f;
    }

    if(!dfa.labelToState[state].empty()){
        for(auto it = graph[node].begin();it != graph[node].end();it++){
            resultsNode.push_back(node);
            resultState.push_back(state);
            search_count++;
            if(search_count > max_depth){
                max_depth = search_count;
            }
            if(dfa.labelToState[state].count(it->label)){
                int nextnode = it->toNode;
                int nextstate = dfa.labelToState[state][it->label];
                resultsLabel.push_back(it->label);
                if(nextnode == dst && dfa.endStates.count(nextstate)){
                    f = true;
                    resultNumber++;
                    found = true;                // CHANGE 1: decision query
                }
                if(nextnode != dst && !passNode.count(nextnode)){
                    pruning(nextnode,nextstate,dst);
                }
                if(nextnode != dst && passNode.count(nextnode)){
                    conflictSetUnion[node][state].insert(nextnode);
                }
                resultsLabel.pop_back();
            }
            resultsNode.pop_back();
            resultState.pop_back();
            search_count--;
        }
        update_conflictSet(node,state);
    }
    passNode.erase(node);
    return f;
}

void DFS_pruning(int src,int dst){
    for(auto it = dfa.endStates.begin();it != dfa.endStates.end();it++){
        BFS(dst,*it);
    }
    if(BFS_Arrive[src][dfa.startState]){
        pruning(src,dfa.startState,dst);
    }
}

void clear_vector(){
    resultsNode.clear();
    resultsLabel.clear();
    passNode.clear();
    conflictSet.clear();
    conflictSetUnion.clear();
    resultState.clear();
    BFS_Arrive.clear();
    while(!Q.empty()) Q.pop();
}

void initialize_vector(){
    resultNumber = 0;
    bfs_count = 0;
    search_count = 0;
    max_depth = 0;
    conflict_size = 0;
    found = false;
    conflictSet.resize(numNode);
    conflictSetUnion.resize(numNode);
    BFS_Arrive.resize(numNode);
}

void buildParityHopDFA(int k, int target_parity){
    int stateNum = 2 * (k + 1);
    dfa = FinalDFA();
    dfa.stateNum = stateNum;
    dfa.startState = 0;                       // (d = 0, p = 0)
    dfa.labelSet.insert(0);
    dfa.labelSet.insert(1);
    dfa.labelToState.assign(stateNum, unordered_map<int,int>());
    dfa.Back_labelToState.assign(stateNum, unordered_map<int,unordered_set<int> >());
    for(int d = 0; d < k; ++d){
        for(int p = 0; p < 2; ++p){
            int s = d * 2 + p;
            for(int l = 0; l < 2; ++l){
                int ns = (d + 1) * 2 + (p ^ l);
                dfa.labelToState[s][l] = ns;
                dfa.Back_labelToState[ns][l].insert(s);
                dfa.LabelArriveState[l].insert(ns);
            }
        }
    }
    for(int d = 1; d <= k; ++d){
        dfa.endStates.insert(d * 2 + target_parity);
    }
}

void ReadData(const string &dir){
    string filepath = dir;
    if(filepath.empty() || filepath[filepath.size()-1] != '/') filepath += "/";
    filepath += "graph.txt";
    FILE *fin = fopen(filepath.c_str(), "r");
    if(!fin){ cerr << "Error: Cannot open file " << filepath << endl; exit(1); }

    char line[256];
    vector<pair<pair<int,int>,int> > edges;
    long long max_node = 0;
    while(fgets(line, sizeof(line), fin)){
        int a, b, sgn;
        string sline(line);
        for(size_t i = 0; i < sline.size(); ++i)
            if(sline[i] == '\t' || sline[i] == ',') sline[i] = ' ';
        if(sscanf(sline.c_str(), "%d %d %d", &a, &b, &sgn) != 3) continue;
        if(a == b) continue;
        if(a < 0 || b < 0) continue;
        int lab = (sgn > 0) ? 0 : 1;
        edges.push_back(make_pair(make_pair(a,b), lab));
        max_node = max(max_node, (long long) max(a,b));
    }
    fclose(fin);

    sort(edges.begin(), edges.end());
    edges.erase(unique(edges.begin(), edges.end()), edges.end());

    numNode = (int)(max_node + 1);
    numEdge = (long long) edges.size();
    graph.resize(numNode);
    Back_graph.resize(numNode);
    for(size_t i = 0; i < edges.size(); ++i){
        int src = edges[i].first.first, dst = edges[i].first.second, lab = edges[i].second;
        edge temp;  temp.toNode = dst;  temp.label = lab;   graph[src].push_back(temp);
        edge temp1; temp1.toNode = src; temp1.label = lab;  Back_graph[dst].push_back(temp1);
    }
    printf("Graph loaded: n = %d, m = %lld\n", numNode, numEdge);
}

// ---------------------------------------------------------------------------
int main(int argc, char* argv[]){
    if(argc < 3){
        printf("Usage: %s <dataset_dir> <k>\n", argv[0]);
        printf("Example: %s ../../datasets/bitcoin 6\n", argv[0]);
        printf("The exact engine of [38] (Candidate Detection + Conflict DFS) run on\n");
        printf("the parity x hop automaton; stops at the first sigma-matching path.\n");
        return 1;
    }
    string dataset_path = argv[1];
    int k = atoi(argv[2]);
    if(k < 1){ cerr << "k must be >= 1" << endl; return 1; }

    string dir = dataset_path;
    if(dir.empty() || dir[dir.size()-1] != '/') dir += "/";

    ReadData(dataset_path);

    int kk = k;
    if(numNode > 0 && kk > numNode) kk = numNode;

    FILE *fin = fopen((dir + "query_exp1.txt").c_str(), "r");
    if(!fin){ cerr << "Error: Cannot open query file " << dir + "query_exp1.txt" << endl; return 1; }

    char resname[64];
    snprintf(resname, sizeof(resname), "BS_RSPQ_EXACT_results_k%d.txt", k);
    FILE *fres = fopen((dir + resname).c_str(), "w");

    char line[256];
    int query_count = 0, true_count = 0, false_count = 0;

    clock_t start_time = clock();
    while(fgets(line, sizeof(line), fin)){
        int s, t, sigma;
        if(sscanf(line, "%d,%d,%d", &s, &t, &sigma) != 3) continue;
        ++query_count;

        int r = 0;
        if(s < 0 || t < 0 || s >= numNode || t >= numNode){
            r = 0;
        } else if(s == t){
            r = (sigma == 1) ? 1 : 0;
        } else {
            buildParityHopDFA(kk, (sigma == 1) ? 0 : 1);
            initialize_vector();
            DFS_pruning(s, t);
            r = found ? 1 : 0;
            stat_bfs_states += bfs_count;
            stat_conflict_entries += conflict_size;
            if(max_depth > stat_max_depth) stat_max_depth = max_depth;
            clear_vector();
        }

        if(r == 1) ++true_count; else ++false_count;
        if(fres) fprintf(fres, "%d,%d,%d,%d\n", s, t, sigma, r);
    }
    clock_t end_time = clock();
    double query_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;

    fclose(fin);
    if(fres) fclose(fres);

    FILE *fout = fopen((dir + "BS_RSPQ_EXACT_Exp1.txt").c_str(), "a");
    if(!fout){ cerr << "Error: Cannot create output file" << endl; return 1; }
    fprintf(fout, "\nBS-RSPQ-EXACT (exact HSR baseline; Candidate Detection + Conflict DFS of [38] on the parity x hop automaton)\n");
    fprintf(fout, "Dataset: %s\n", dataset_path.c_str());
    fprintf(fout, "Hop constraint k = %d (no time limit)\n", k);
    fprintf(fout, "\nSummary (Query_path: %squery_exp1.txt, k = %d):\n", dir.c_str(), k);
    fprintf(fout, "Total queries: %d\n", query_count);
    fprintf(fout, "TRUE: %d\n", true_count);
    fprintf(fout, "FALSE: %d\n", false_count);
    fprintf(fout, "Total query time: %.5lf seconds\n", query_time);
    fprintf(fout, "Instrumentation: candidate-detection states = %lld, conflict-set entries = %lld, max search depth = %d\n",
            stat_bfs_states, stat_conflict_entries, stat_max_depth);
    fprintf(fout, "========================================\n\n");
    fclose(fout);

    printf("BS-RSPQ-EXACT done: %d queries, TRUE=%d FALSE=%d, time=%.5lf s\n",
           query_count, true_count, false_count, query_time);
    printf("Instrumentation: candidate-detection states = %lld, conflict-set entries = %lld, max search depth = %d\n",
           stat_bfs_states, stat_conflict_entries, stat_max_depth);
    return 0;
}
