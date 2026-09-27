#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>
#include <vector>
#include <algorithm>
#include <iostream>

using namespace std;

static int n = 0;
static vector<vector<pair<int,int> > > adj;

static int  target, wantParity, hopBound;
static bool found;
static vector<char> onPath;
static long long steps;

static void dfs(int v, int depth, int parity)
{
    ++steps;
    if (v == target) {
        if (parity == wantParity) found = true;
        return;
    }
    if (depth == hopBound) return;
    const vector<pair<int,int> > &nb = adj[v];
    for (size_t i = 0; i < nb.size() && !found; ++i) {
        int w = nb[i].first;
        if (onPath[w]) continue;              // simple: never revisit a vertex
        onPath[w] = 1;
        dfs(w, depth + 1, parity ^ nb[i].second);
        onPath[w] = 0;
    }
}

// TRUE iff a simple s->t path with <= k edges and sign product sigma exists
static int brute(int s, int t, int k, int sigma)
{
    if (s < 0 || s >= n || t < 0 || t >= n) return 0;
    target = t;
    wantParity = (sigma == 1) ? 0 : 1;
    hopBound = k;
    found = false;
    fill(onPath.begin(), onPath.end(), 0);
    onPath[s] = 1;
    dfs(s, 0, 0);
    return found ? 1 : 0;
}

static long long loadGraph(const string &dir)
{
    string fp = dir + "graph.txt";
    FILE *fin = fopen(fp.c_str(), "r");
    if (!fin) { cerr << "Error: Cannot open file " << fp << endl; exit(1); }
    char line[256];
    vector<pair<pair<int,int>,int> > edges;
    int mx = -1;
    while (fgets(line, sizeof(line), fin)) {
        int a, b, s;
        string sl(line);
        for (size_t i = 0; i < sl.size(); ++i) if (sl[i] == '\t' || sl[i] == ',') sl[i] = ' ';
        if (sscanf(sl.c_str(), "%d %d %d", &a, &b, &s) != 3) continue;
        if (a == b || a < 0 || b < 0) continue;
        edges.push_back(make_pair(make_pair(a, b), s > 0 ? 0 : 1));
        mx = max(mx, max(a, b));
    }
    fclose(fin);
    sort(edges.begin(), edges.end());
    edges.erase(unique(edges.begin(), edges.end()), edges.end());
    n = mx + 1;
    adj.assign(n, vector<pair<int,int> >());
    for (size_t i = 0; i < edges.size(); ++i)
        adj[edges[i].first.first].push_back(make_pair(edges[i].first.second, edges[i].second));
    onPath.assign(n, 0);
    printf("Graph loaded: n = %d, m = %lld\n", n, (long long) edges.size());
    return (long long) edges.size();
}

int main(int argc, char *argv[])
{
    if (argc < 3) {
        printf("Usage: %s <dataset_dir> <k> [query_file] [--offset N] [--limit M] [--out NAME]\n", argv[0]);
        return 1;
    }
    string dir = argv[1];
    if (dir.empty() || dir[dir.size() - 1] != '/') dir += "/";
    int k = atoi(argv[2]);
    if (k < 0) k = 0;

    string query_path;
    long long offset = 0, limit = -1;             // -1 = to the end of the file
    string out_name;
    for (int i = 3; i < argc; ++i) {
        string a = argv[i];
        if (a == "--offset" && i + 1 < argc)      offset   = atoll(argv[++i]);
        else if (a == "--limit"  && i + 1 < argc) limit    = atoll(argv[++i]);
        else if (a == "--out"    && i + 1 < argc) out_name = argv[++i];
        else if (query_path.empty())              query_path = a;
        else { cerr << "Unexpected argument: " << a << endl; return 1; }
    }
    if (query_path.empty()) query_path = dir + "query_exp1.txt";
    if (offset < 0) offset = 0;

    loadGraph(dir);

    FILE *fin = fopen(query_path.c_str(), "r");
    if (!fin) { cerr << "Error: Cannot open query file " << query_path << endl; return 1; }
    char resname[64];
    snprintf(resname, sizeof(resname), "BRUTE_FORCE_results_k%d.txt", k);
    string res_path = dir + (out_name.empty() ? string(resname) : out_name);
    FILE *fres = fopen(res_path.c_str(), "w");
    if (!fres) { cerr << "Error: Cannot create " << res_path << endl; return 1; }

    char line[256];
    long long seen = 0;                       // queries read, for --offset/--limit
    int query_count = 0, true_count = 0, false_count = 0;
    steps = 0;
    clock_t t0 = clock();                     // informational; the checker is never a timed method
    while (fgets(line, sizeof(line), fin)) {
        int s, t, sigma;
        if (sscanf(line, "%d,%d,%d", &s, &t, &sigma) != 3) continue;
        long long idx = seen++;
        if (idx < offset) continue;
        if (limit >= 0 && idx >= offset + limit) break;
        ++query_count;
        int r = brute(s, t, k, sigma);
        if (r) ++true_count; else ++false_count;
        fprintf(fres, "%d,%d,%d,%d\n", s, t, sigma, r);
    }
    double secs = (double)(clock() - t0) / CLOCKS_PER_SEC;
    fclose(fin);
    fclose(fres);

    FILE *fout = fopen((dir + "BRUTE_FORCE_Exp1.txt").c_str(), "a");
    if (fout) {
        fprintf(fout, "\nBRUTE-FORCE CHECKER (exhaustive simple-path enumeration; not a baseline)\n");
        fprintf(fout, "Dataset: %s\nQuery file: %s\nHop constraint k = %d\n", argv[1], query_path.c_str(), k);
        if (offset || limit >= 0)
            fprintf(fout, "Query range: [%lld, %lld)\n", offset, limit >= 0 ? offset + limit : (long long) -1);
        fprintf(fout, "Total queries: %d   TRUE: %d   FALSE: %d\n", query_count, true_count, false_count);
        fprintf(fout, "Search steps: %lld   Time: %.3lf s (informational)\n", steps, secs);
        fprintf(fout, "========================================\n\n");
        fclose(fout);
    }
    printf("BRUTE-FORCE done: %d queries, TRUE=%d FALSE=%d, %lld search steps, %.3lf s\n",
           query_count, true_count, false_count, steps, secs);
    return 0;
}
