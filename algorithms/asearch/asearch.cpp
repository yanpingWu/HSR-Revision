#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <queue>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

using namespace std;

struct Graph {
    int n;
    vector<vector<pair<int,int>>> adj; // u -> (v, sign) sign: 1 positive, 0 negative
    vector<vector<pair<int,int>>> rev; // reverse adjacency: v -> (u, sign)
};

static inline int sign_mul(int a, int b) {
    return (a == b) ? 1 : 0; // same sign -> positive, different -> negative
}

Graph load_graph(const string& dataset_path) {
    string path = dataset_path;
    if (!path.empty() && path.back() != '/') path += "/";
    string file = path + "graph.txt";

    FILE* fin = fopen(file.c_str(), "r");
    if (!fin) {
        cerr << "Error: Cannot open file " << file << endl;
        exit(1);
    }

    char line[256];
    int a, b, s;
    int max_node = -1;
    vector<tuple<int,int,int>> edges;
    while (fgets(line, sizeof(line), fin)) {
        // if (sscanf(line, "%d %d %d", &a, &b, &s) != 3) continue;

        string sline(line);
        for (char &c : sline) {
            if (c == '\t' || c == ',') c = ' ';
        }
        if (sscanf(sline.c_str(), "%d %d %d", &a, &b, &s) != 3) continue;

        if (a == b) continue;
        int sign = (s > 0) ? 1 : 0;
        edges.emplace_back(a, b, sign);
        max_node = max(max_node, max(a, b));
    }
    fclose(fin);

    set<tuple<int,int,int>> edge_set;
    for (size_t i = 0; i < edges.size(); ++i) {
        edge_set.insert(edges[i]);
    }
    
    Graph g;
    g.n = max_node + 1;
    g.adj.assign(g.n, {});
    g.rev.assign(g.n, {});
    for (const auto& e : edge_set) {
        int u, v, sign;
        tie(u, v, sign) = e;
        g.adj[u].push_back(make_pair(v, sign));
        g.rev[v].push_back(make_pair(u, sign));
    }

    for (int u = 0; u < g.n; ++u) {
        sort(g.adj[u].begin(), g.adj[u].end());
        sort(g.rev[u].begin(), g.rev[u].end());
    }
    return g;
}

// Reverse Signed BFS from t to compute Dpos/Dneg using reverse adjacency
void reverse_signed_bfs(const vector<vector<pair<int,int>>>& rev, int t, vector<int>& Dpos, vector<int>& Dneg, int k) {
    const int INF = 1e9;
    int n = static_cast<int>(rev.size());
    Dpos.assign(n, INF);
    Dneg.assign(n, INF);

    queue<tuple<int,int,int>> q; // node, dist, sigma_end
    Dpos[t] = 0;
    q.push(make_tuple(t, 0, 1)); // start with positive sign

    while (!q.empty()) {
        int u, dist, sigma_end;
        tie(u, dist, sigma_end) = q.front();
        q.pop();

        if (dist >= k) {  
            continue;
        }

        const vector<pair<int,int>>& inc = rev[u]; // incoming edges v->u
        for (size_t i = 0; i < inc.size(); ++i) {
            int v = inc[i].first;
            int sigma_vu = inc[i].second;
            int sigma_req = sign_mul(sigma_end, sigma_vu); // sigma_req * sigma_vu = sigma_end
            if (sigma_req == 1 && Dpos[v] == INF) {
                Dpos[v] = dist + 1;
                q.push(make_tuple(v, dist + 1, 1));
            } else if (sigma_req == 0 && Dneg[v] == INF) {
                Dneg[v] = dist + 1;
                q.push(make_tuple(v, dist + 1, 0));
            }
        }
    }
}

struct State {
    int f, g, u, sigma;
    vector<int> path; // simple path nodes
    bool operator<(const State& other) const {
        // for priority_queue max-heap, invert
        return f > other.f;
    }
};

bool bs_astar(const Graph& g, int s, int t, int k, int sigma_query, double& reverse_bfs_time, double& dfs_time) {
    const int INF = 1e9;

    clock_t reverse_bfs_start = clock();
    vector<int> Dpos, Dneg;
    reverse_signed_bfs(g.rev, t, Dpos, Dneg, k);
    clock_t reverse_bfs_end = clock();
    reverse_bfs_time += double(reverse_bfs_end - reverse_bfs_start) / CLOCKS_PER_SEC;

    int h = (sigma_query == 1) ? Dpos[s] : Dneg[s];
    if (h > k) return false;

    clock_t dfs_start = clock();

    priority_queue<State> pq;
    State init;
    init.g = 0;
    init.u = s;
    init.sigma = 1; // start sign +
    init.f = h;
    init.path = {s};
    pq.push(init);

    while (!pq.empty()) {
        State cur = pq.top(); pq.pop();
        int u = cur.u;
        int g_now = cur.g;
        int sigma_path = cur.sigma;

        const vector<pair<int,int>>& outs = g.adj[u];
        for (size_t i = 0; i < outs.size(); ++i) {
            int v = outs[i].first;
            int sigma_uv = outs[i].second;
            
            // simple path check
            bool visited = find(cur.path.begin(), cur.path.end(), v) != cur.path.end();
            // vector<int> sorted_path = cur.path;
            // sort(sorted_path.begin(), sorted_path.end());
            // bool visited = binary_search(sorted_path.begin(), sorted_path.end(), v);
            if (visited) continue;

            int new_sigma = sign_mul(sigma_path, sigma_uv);
            int new_g = g_now + 1;

            if (v == t && new_sigma == sigma_query && new_g <= k) {
                clock_t dfs_end = clock();
                dfs_time += double(dfs_end - dfs_start) / CLOCKS_PER_SEC;
                return true;
            }

            if (new_g > k) continue;

            int sigma_rem = (new_sigma == sigma_query) ? 1 : 0;
            int h_val = (sigma_rem == 1) ? Dpos[v] : Dneg[v];
            if (h_val == INF) continue;
            if (new_g + h_val > k) continue;

            State nxt;
            nxt.g = new_g;
            nxt.u = v;
            nxt.sigma = new_sigma;
            nxt.f = new_g + h_val;
            nxt.path = cur.path;
            nxt.path.push_back(v);
            pq.push(std::move(nxt));
        }
    }
    clock_t dfs_end = clock();
    dfs_time += double(dfs_end - dfs_start) / CLOCKS_PER_SEC;
    return false;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        printf("Usage: %s <dataset_path> <k_value>\n", argv[0]);
        printf("Example: %s ../dataset/bitcoin 4\n", argv[0]);
        return 1;
    }

    string dataset_path = argv[1];
    int k = atoi(argv[2]);

    // output file (append)
    string output_path = dataset_path;
    if (!output_path.empty() && output_path.back() != '/') output_path += "/";
    output_path += "Asearch_Exp1.txt";
    FILE* fout = fopen(output_path.c_str(), "a");
    if (!fout) {
        cerr << "Error: Cannot create output file " << output_path << endl;
        return 1;
    }

    Graph g = load_graph(dataset_path);

    // read queries
    string query_path = dataset_path;
    if (!query_path.empty() && query_path.back() != '/') query_path += "/";
    query_path += "query_exp1.txt";
    FILE* fin = fopen(query_path.c_str(), "r");
    if (!fin) {
        cerr << "Error: Cannot open query file " << query_path << endl;
        fclose(fout);
        return 1;
    }

    char line[256];
    int query_count = 0, true_count = 0, false_count = 0;
    double total_reverse_bfs_time = 0.0;
    double total_dfs_time = 0.0;

    fprintf(fout, "\nA* Algorithm\n");
    fprintf(fout, "query_path is: %s\n", query_path.c_str());
    fprintf(fout, "Dataset: %s\n", dataset_path.c_str());
    fprintf(fout, "Hop constraint k = %d\n", k);

    clock_t start_time = clock();
    while (fgets(line, sizeof(line), fin)) {
        int s, t, sigma;
        if (sscanf(line, "%d,%d,%d", &s, &t, &sigma) != 3) continue;
        if (sigma != 0 && sigma != 1) continue;
        ++query_count;
        double reverse_bfs_time_this = 0.0;
        double dfs_time_this = 0.0;
        bool result = bs_astar(g, s, t, k, sigma, reverse_bfs_time_this, dfs_time_this);
        total_reverse_bfs_time += reverse_bfs_time_this;
        total_dfs_time += dfs_time_this;
        if (result) ++true_count; else ++false_count;
    }
    clock_t end_time = clock();
    double query_time = double(end_time - start_time) / CLOCKS_PER_SEC;

    fclose(fin);

    fprintf(fout, "\nSummary (Dataset: %s, k = %d):\n", dataset_path.c_str(), k);
    fprintf(fout, "Total queries: %d\n", query_count);
    fprintf(fout, "TRUE: %d\n", true_count);
    fprintf(fout, "FALSE: %d\n", false_count);
    fprintf(fout, "Total query time (all queries): %.5lf seconds\n", query_time);
    fprintf(fout, "========================================\n\n");

    fclose(fout);
    return 0;
}
