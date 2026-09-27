#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>
#include <vector>
#include <algorithm>
#include <iostream>

using namespace std;

#define rep(i, n) for (int i = 0; i < (int)(n); i++)
#define NEGATIVE -1
#define POSITIVE 1

struct v_sign
{
    int v;
    int sign;
};

struct PathList
{
    int k;
    vector< vector<int> > pathList;
};

struct VPath
{
    vector<PathList> pLList;
};

struct NTable
{
    bool built;          // sentinel fix: the release used "v == 0"
    int v;
    VPath entries;
};

typedef vector< vector< v_sign > > g_t;

g_t G, G_reverse;
int k = 6;
vector<int> BN_S;
long long table_time = 0;
long long entry = 0;
long long hit = 0, miss = 0;
bool found = false;      // CHANGE 1: decision semantics (first balanced path)

VPath build_new_table(int v, vector<int> &F, int k)
{
    VPath vpath;
    vector<PathList> B_List;
    vector<PathList> U_List;
    PathList bPath;
    PathList uPath;
    bPath.k = 1;
    uPath.k = 1;

    rep(i, G[v].size())
    {
        vector<int> path;
        path.push_back(v);
        v_sign p;
        if (G[v][i].sign != NEGATIVE && F[G[v][i].v] < k)
        {
            path.push_back(G[v][i].v);
            bPath.pathList.push_back(path);
        }
        else if (G[v][i].sign == NEGATIVE && F[G[v][i].v] < k)
        {
            path.push_back(G[v][i].v);
            uPath.pathList.push_back(path);
            entry+= 2;
        }
    }
    B_List.push_back(bPath);
    U_List.push_back(uPath);

    for (int i = 1; i < k; i++)
    {

        bPath.pathList.clear();
        uPath.pathList.clear();
        bPath.k = i + 1;
        uPath.k = i + 1;
        if (B_List[i - 1].pathList.size() == 0)
        {
            break;
        }
        for (vector<int> path : B_List[i - 1].pathList)
        {
            rep(j, G[path[path.size() - 1]].size())
            {
                vector<int> path_eample = path;
                int u = G[path[path.size() - 1]][j].v;
                if (!count(path.begin(), path.end(), u) && G[path[path.size() - 1]][j].sign != NEGATIVE && F[u] + path_eample.size() <= k)
                {
                    path_eample.push_back(u);
                    bPath.pathList.push_back(path_eample);
                }
                else if (!count(path.begin(), path.end(), u) && G[path[path.size() - 1]][j].sign == NEGATIVE && F[u] + path_eample.size() <= k)
                {
                    path_eample.push_back(u);
                    uPath.pathList.push_back(path_eample);
                    entry+= path_eample.size();
                }
            }
        }
        B_List.push_back(bPath);
        U_List.push_back(uPath);
    }

    vpath.pLList = U_List;
    return vpath;
}

int BN_dfs(vector<int> &F, vector<NTable> &i_table, vector< vector<int> > &result_path, int s, int t, int k) {
    if (found) return 1;                                     // CHANGE 1
    BN_S.push_back(s);
    int f;

    if (s == t) {
        result_path.push_back(BN_S);
        found = true;                                        // CHANGE 1
        BN_S.pop_back();
        return 0;
    }

    else {
        for (int i = 0; i < G[s].size(); ++i) {
            if (found) break;                                // CHANGE 1
            v_sign v = G[s][i];
            if (!count(BN_S.begin(), BN_S.end(), v.v)) {
                if (v.sign == POSITIVE) {
                    if (BN_S.size() + F[v.v] <= k)
                        f = BN_dfs(F, i_table, result_path, v.v, t, k);
                }

                else {
                    if (BN_S.size() + F[v.v] <= k && v.v != t) {

                        if (!i_table[v.v].built) {
                            miss++;
                            clock_t start = clock();
                            VPath entries = build_new_table(v.v, F, k - 1);
                            clock_t end = clock();
                            table_time += (long long)(end - start);
                            i_table[v.v].built = true;
                            i_table[v.v].v = v.v;
                            i_table[v.v].entries = entries;
                        }

                        else hit++;

                        int min_k = min(i_table[v.v].entries.pLList.size(), k - BN_S.size());

                        for (int o = 0; o < min_k; ++o) {
                            if (found) break;                // CHANGE 1
                            for (int g = 0; g < i_table[v.v].entries.pLList[o].pathList.size(); ++g) {
                                if (found) break;            // CHANGE 1
                                int r;
                                for (r = 1; r < i_table[v.v].entries.pLList[o].pathList[g].size(); ++r) {
                                    if (BN_S.size() + F[i_table[v.v].entries.pLList[o].pathList[g][r]] + r > k || count(BN_S.begin(), BN_S.end(), i_table[v.v].entries.pLList[o].pathList[g][r]))
                                        break;
                                }
                                if (r == i_table[v.v].entries.pLList[o].pathList[g].size()) {
                                    int e;
                                    for (e = 0; e < i_table[v.v].entries.pLList[o].pathList[g].size() - 1; ++e)
                                        BN_S.push_back(i_table[v.v].entries.pLList[o].pathList[g][e]);
                                    BN_dfs(F, i_table, result_path, i_table[v.v].entries.pLList[o].pathList[g][e], t, k);
                                    for (e = 0; e < i_table[v.v].entries.pLList[o].pathList[g].size() - 1; ++e)
                                        BN_S.pop_back();
                                }
                            }
                        }

                    }
                }
            }
        }
    }

    BN_S.pop_back();
    return 1;
}

void compute_Dw(vector<int> &F, int t, int k)
{
    fill(F.begin(), F.end(), k + 1);
    F[t] = 0;
    vector<int> q;
    q.push_back(t);
    for (size_t h = 0; h < q.size(); ++h) {
        int v = q[h];
        if (F[v] >= k) continue;
        rep(i, G_reverse[v].size()) {
            int u = G_reverse[v][i].v;
            if (F[u] == k + 1) {
                F[u] = F[v] + 1;
                q.push_back(u);
            }
        }
    }
}

static long long loadGraph(const string &dir)
{
    string filepath = dir + "graph.txt";
    FILE *fin = fopen(filepath.c_str(), "r");
    if (!fin) { cerr << "Error: Cannot open file " << filepath << endl; exit(1); }

    char line[256];
    vector<pair<pair<int,int>,int> > edges;
    int max_node = 0;
    while (fgets(line, sizeof(line), fin)) {
        int a, b, sgn;
        string sline(line);
        for (size_t i = 0; i < sline.size(); ++i)
            if (sline[i] == '\t' || sline[i] == ',') sline[i] = ' ';
        if (sscanf(sline.c_str(), "%d %d %d", &a, &b, &sgn) != 3) continue;
        if (a == b) continue;
        if (a < 0 || b < 0) continue;
        edges.push_back(make_pair(make_pair(a, b), sgn > 0 ? POSITIVE : NEGATIVE));
        max_node = max(max_node, max(a, b));
    }
    fclose(fin);

    sort(edges.begin(), edges.end());
    edges.erase(unique(edges.begin(), edges.end()), edges.end());

    int n = max_node + 1;
    G.assign(n, vector<v_sign>());
    G_reverse.assign(n, vector<v_sign>());
    for (size_t i = 0; i < edges.size(); ++i) {
        v_sign v; v.v = edges[i].first.second; v.sign = edges[i].second;
        v_sign u; u.v = edges[i].first.first;  u.sign = edges[i].second;
        G[edges[i].first.first].push_back(v);
        G_reverse[edges[i].first.second].push_back(u);
    }
    printf("Graph loaded: n = %d, m = %lld\n", n, (long long) edges.size());
    return (long long) edges.size();
}

static int query_positive(int s, int t, int kk)
{
    int n = (int) G.size();
    if (s < 0 || s >= n || t < 0 || t >= n) return 0;
    if (s == t) return 1;

    found = false;
    BN_S.clear();
    vector<int> F(n);
    compute_Dw(F, t, kk);
    vector<NTable> i_table(n);
    vector< vector<int> > result_path;
    BN_dfs(F, i_table, result_path, s, t, kk);
    return found ? 1 : 0;
}

int main(int argc, char *argv[])
{
    if (argc < 3) {
        printf("Usage: %s <dataset_dir> <k> [query_file]\n", argv[0]);
        printf("Example: %s ../../datasets/bitcoin 6\n", argv[0]);
        printf("BMAS of [53]; answers\n");
        printf("sigma='+' queries only (sigma='-' is outside [53]'s problem: result -1).\n");
        return 1;
    }
    string dataset_path = argv[1];
    int kin = atoi(argv[2]);
    if (kin < 1) { cerr << "k must be >= 1" << endl; return 1; }

    string dir = dataset_path;
    if (dir.empty() || dir[dir.size() - 1] != '/') dir += "/";
    string query_path = (argc >= 4) ? string(argv[3]) : dir + "query_exp1.txt";

    long long m = loadGraph(dir);
    (void) m;
    k = kin;
    if ((int) G.size() > 0 && k > (int) G.size()) k = (int) G.size();   // a simple path has <= n-1 edges

    FILE *fin = fopen(query_path.c_str(), "r");
    if (!fin) { cerr << "Error: Cannot open query file " << query_path << endl; return 1; }

    char resname[64];
    snprintf(resname, sizeof(resname), "BS_BMAS_results_k%d.txt", kin);
    FILE *fres = fopen((dir + resname).c_str(), "w");

    char line[256];
    int query_count = 0, true_count = 0, false_count = 0, na_count = 0;
    long long tot_hit = 0, tot_miss = 0, tot_entry = 0;

    clock_t start_time = clock();
    while (fgets(line, sizeof(line), fin)) {
        int s, t, sigma;
        if (sscanf(line, "%d,%d,%d", &s, &t, &sigma) != 3) continue;
        ++query_count;
        int r;
        if (sigma != 1) {
            r = -1;
            ++na_count;
        } else {
            entry = 0; hit = 0; miss = 0;
            r = query_positive(s, t, k);
            tot_hit += hit; tot_miss += miss; tot_entry += entry;
            if (r == 1) ++true_count; else ++false_count;
        }
        if (fres) fprintf(fres, "%d,%d,%d,%d\n", s, t, sigma, r);
    }
    clock_t end_time = clock();
    double query_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;

    fclose(fin);
    if (fres) fclose(fres);

    FILE *fout = fopen((dir + "BS_BMAS_Exp1.txt").c_str(), "a");
    if (!fout) { cerr << "Error: Cannot create output file" << endl; return 1; }
    fprintf(fout, "\nBS-BMAS (HSR baseline, sigma='+' only; BMAS of [53])\n");
    fprintf(fout, "Dataset: %s\n", dataset_path.c_str());
    fprintf(fout, "Query file: %s\n", query_path.c_str());
    fprintf(fout, "Hop constraint k = %d (no time limit)\n", kin);
    fprintf(fout, "\nSummary:\n");
    fprintf(fout, "Total queries: %d  (sigma='+' answered: %d, sigma='-' not applicable: %d)\n",
            query_count, query_count - na_count, na_count);
    fprintf(fout, "TRUE: %d\n", true_count);
    fprintf(fout, "FALSE: %d\n", false_count);
    fprintf(fout, "Total query time: %.5lf seconds (table construction included)\n", query_time);
    fprintf(fout, "AS-Table: hits = %lld, misses = %lld, entries = %lld\n", tot_hit, tot_miss, tot_entry);
    fprintf(fout, "========================================\n\n");
    fclose(fout);

    printf("BS-BMAS done: %d queries (%d sigma='+', %d n/a), TRUE=%d FALSE=%d, time=%.5lf s\n",
           query_count, query_count - na_count, na_count, true_count, false_count, query_time);
    return 0;
}
