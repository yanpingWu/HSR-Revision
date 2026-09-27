#include <iostream>
#include <vector>
#include <queue>
#include <unordered_map>
#include <set>
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <cctype>
#include <ctime>

using namespace std;

struct Path {
    set<int> nodes;
    int last_node;
    int sign;
    int len;
    
    Path() : last_node(-1), sign(1), len(0) {}
    Path(const set<int>& n, int last, int s, int l) : nodes(n), last_node(last), sign(s), len(l) {}
};

class SignedGraph {
public:
    int n;
    vector<vector<pair<int, int>>> adj_out;  //adj_out[u] = {(v, sign), ...}
    vector<vector<pair<int, int>>> adj_in;   //adj_in[v] = {(u, sign), ...}
    
    SignedGraph() : n(0) {}
    
    void loadGraph(const string& path) {
        string filepath = path;
        if (filepath.back() != '/') filepath += "/";
        filepath += "graph.txt";
        
        FILE* fin = fopen(filepath.c_str(), "r");
        if (!fin) {
            cerr << "Error: Cannot open file " << filepath << endl;
            return;
        }
        
        char line[256];
        vector<pair<pair<int, int>, int>> edges;
        int max_node = 0;
        
        // while (fgets(line, sizeof(line), fin)) {
        //     int a, b, sign;
        //     if (sscanf(line, "%d %d %d", &a, &b, &sign) != 3) continue;
        //     if (a == b) continue;
            
        //     // sign >= 1，sign <= 0 
        //     int edge_sign = (sign > 0) ? 1 : 0;
        //     edges.push_back({{a, b}, edge_sign});
        //     max_node = max(max_node, max(a, b));
        // }

        while (fgets(line, sizeof(line), fin)) {
            int a, b, sign;
            // if (sscanf(line, "%d %d %d", &a, &b, &sign) != 3) continue;

            std::string sline(line);
            for (char &c : sline) {
                if (c == '\t' || c == ',') c = ' ';
            }
            if (sscanf(sline.c_str(), "%d %d %d", &a, &b, &sign) != 3) continue;

            if (a == b) continue; 
            
            int edge_sign = (sign > 0) ? 1 : 0;
            edges.push_back({{a, b}, edge_sign});
            max_node = max(max_node, max(a, b));
        }
        
        fclose(fin);
        
        n = max_node + 1;
        adj_out.resize(n);
        adj_in.resize(n);
        
        for (const auto& e : edges) {
            int a = e.first.first;
            int b = e.first.second;
            int sign = e.second;
            adj_out[a].push_back({b, sign});
            adj_in[b].push_back({a, sign});
        }
        
        for (int u = 0; u < n; u++) {
            sort(adj_out[u].begin(), adj_out[u].end());
            adj_out[u].erase(unique(adj_out[u].begin(), adj_out[u].end()), adj_out[u].end());
            
            sort(adj_in[u].begin(), adj_in[u].end());
            adj_in[u].erase(unique(adj_in[u].begin(), adj_in[u].end()), adj_in[u].end());
        }
        
        printf("Graph loaded: n = %d, edges = %lu\n", n, edges.size());
    }
    

    //+*+=+, +*-=-, -*+=-, -*-=+
    int multiplySign(int sign1, int sign2) {
        return (sign1 == sign2) ? 1 : 0;
    }
    
    bool pathsDisjoint(const set<int>& p1, const set<int>& p2, int meeting_point) {
        for (int node : p1) {
            if (node == meeting_point) continue;
            if (p2.count(node)) {
                return false;
            }
        }
        return true;
    }
    
    bool bs_bbfs(int s, int t, int k, int sigma) {
        queue<pair<pair<set<int>, int>, int>> Q1, Q2;
        unordered_map<int, vector<Path>> M1, M2;
        
        int d1 = 0, d2 = 0;
        
        set<int> init_p1, init_p2;
        init_p1.insert(s);
        init_p2.insert(t);
        Q1.push({{init_p1, s}, 1});
        Q2.push({{init_p2, t}, 1});
        
        M1[s].push_back(Path(init_p1, s, 1, 0));
        M2[t].push_back(Path(init_p2, t, 1, 0));
        
        while ((!Q1.empty() || !Q2.empty()) && (d1 + d2) < k) {
            if (!Q1.empty() && d1 + d2 < k) {
                queue<pair<pair<set<int>, int>, int>> Next_Q1;
                
                while (!Q1.empty()) {
                    pair<pair<set<int>, int>, int> front = Q1.front();
                    set<int> p1 = front.first.first;
                    int u = front.first.second;
                    int sigma_p1 = front.second;
                    Q1.pop();
                    
                    for (const auto& edge : adj_out[u]) {
                        int v = edge.first;
                        int sigma_uv = edge.second;
                        if (p1.count(v)) continue;
                        
                        int new_sigma = multiplySign(sigma_p1, sigma_uv);
                        
                        if (v == t && new_sigma == sigma && d1 + 1 <= k) {
                            return true;
                        }
                        
                        if (M2.find(v) != M2.end()) {
                            for (const auto& path2 : M2[v]) {
                                int total_len = d1 + 1 + path2.len;
                                int combined_sigma = multiplySign(new_sigma, path2.sign);
                                
                                set<int> new_p1 = p1;
                                new_p1.insert(v);
                                if (total_len <= k && combined_sigma == sigma && 
                                    pathsDisjoint(new_p1, path2.nodes, v)) {
                                    return true;
                                }
                            }
                        }
                        
                        set<int> new_p1 = p1;
                        new_p1.insert(v);
                        Next_Q1.push({{new_p1, v}, new_sigma});
                        
                        M1[v].push_back(Path(new_p1, v, new_sigma, d1 + 1));
                    }
                }
                
                Q1 = Next_Q1;
                d1++;
            }
            
            if (!Q2.empty() && d1 + d2 < k) {
                queue<pair<pair<set<int>, int>, int>> Next_Q2;
                
                while (!Q2.empty()) {
                    pair<pair<set<int>, int>, int> front = Q2.front();
                    set<int> p2 = front.first.first;
                    int w = front.first.second; 
                    int sigma_p2 = front.second;
                    Q2.pop();
                    
                    for (const auto& edge : adj_in[w]) {
                        int v = edge.first;
                        int sigma_vw = edge.second;
                        if (p2.count(v)) continue;
                        
                        // sigma_vw * sigma_p2
                        int new_sigma = multiplySign(sigma_vw, sigma_p2);
                        
                        if (v == s && new_sigma == sigma && d2 + 1 <= k) {
                            return true;
                        }
                        
                        if (M1.find(v) != M1.end()) {
                            for (const auto& path1 : M1[v]) {
                                int total_len = path1.len + d2 + 1;
                                int combined_sigma = multiplySign(path1.sign, new_sigma);
                                
                                set<int> new_p2 = p2;
                                new_p2.insert(v);
                                if (total_len <= k && combined_sigma == sigma && 
                                    pathsDisjoint(path1.nodes, new_p2, v)) {
                                    return true;
                                }
                            }
                        }
                        
                        set<int> new_p2 = p2;
                        new_p2.insert(v);
                        Next_Q2.push({{new_p2, v}, new_sigma});
                        
                        M2[v].push_back(Path(new_p2, v, new_sigma, d2 + 1));
                    }
                }
                
                Q2 = Next_Q2;
                d2++;
            }
        }
        
        return false;
    }
};

int main(int argc, char* argv[]) {
    if (argc < 3) {
        printf("Usage: %s <dataset_path> <k_value>\n", argv[0]);
        printf("Example: %s ../dataset/bitcoin 8\n", argv[0]);
        return 1;
    }
    
    string dataset_path = argv[1];
    int k = atoi(argv[2]);
    
    string output_path = dataset_path;
    if (output_path.back() != '/') output_path += "/";
    output_path += "BBFS_Exp1.txt";
    
    FILE* fout = fopen(output_path.c_str(), "a");
    if (!fout) {
        cerr << "Error: Cannot create output file " << output_path << endl;
        return 1;
    }
    
    // fprintf(fout, "\n========================================\n");
    fprintf(fout, "\nBBFS Algorithm\n");
    // fprintf(fout, "\nThis is the result of BBFS 1000 wiki queries\n");
    fprintf(fout, "Dataset: %s\n", dataset_path.c_str());
    fprintf(fout, "Hop constraint k = %d\n\n", k);
    // fprintf(fout, "========================================\n");
    
    SignedGraph graph;
    graph.loadGraph(dataset_path);
    
    string query_path = dataset_path;
    if (query_path.back() != '/') query_path += "/";
    query_path += "query_exp1.txt";
    
    FILE* fin = fopen(query_path.c_str(), "r");
    if (!fin) {
        cerr << "Error: Cannot open query file " << query_path << endl;
        fclose(fout);
        return 1;
    }
    
    char line[256];
    int query_count = 0;
    int true_count = 0;
    int false_count = 0;
    
    clock_t start_time = clock();
    
    while (fgets(line, sizeof(line), fin)) {
        int s, t, sigma;
        if (sscanf(line, "%d,%d,%d", &s, &t, &sigma) != 3) continue;
        
        query_count++;
        bool result = graph.bs_bbfs(s, t, k, sigma);
        
        if (result) {
            true_count++;
        } else {
            false_count++;
        }
    }
    
    clock_t end_time = clock();
    double query_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    
    fclose(fin);
    
    fprintf(fout, "\nSummary (Query_path: %s, k = %d):\n", query_path.c_str(), k);
    fprintf(fout, "Total queries: %d\n", query_count);
    fprintf(fout, "TRUE: %d\n", true_count);
    fprintf(fout, "FALSE: %d\n", false_count);
    fprintf(fout, "Total query time: %.5lf seconds\n", query_time);
    fprintf(fout, "========================================\n\n");
    
    fclose(fout);
    
    return 0;
}

