#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cassert>
#include <queue>
#include <string>
#include <vector>
#include <limits>
#include <algorithm>
#include <iostream>

#include "sparsepp/spp.h"

#define BUCKET_ID(i, j, l) ((i)*(l) + (j))

using namespace std;

class SignedDigraph {
public:
    uint32_t num_vertices_;
    uint64_t num_edges_;

    uint32_t *out_offset_, *out_adj_, *out_degree_;
    uint8_t  *out_sign_;
    uint32_t *in_offset_, *in_adj_, *in_degree_;

    SignedDigraph() : num_vertices_(0), num_edges_(0),
                      out_offset_(nullptr), out_adj_(nullptr), out_degree_(nullptr),
                      out_sign_(nullptr),
                      in_offset_(nullptr), in_adj_(nullptr), in_degree_(nullptr) {}

    ~SignedDigraph() {
        free(out_offset_); free(out_adj_); free(out_degree_); free(out_sign_);
        free(in_offset_);  free(in_adj_);  free(in_degree_);
    }

    inline uint32_t num_vertices() const { return num_vertices_; }


    inline void out_neighbors(uint32_t u, const uint32_t *&adj, const uint8_t *&sgn,
                              uint32_t &deg) const {
        adj = out_adj_ + out_offset_[u];
        sgn = out_sign_ + out_offset_[u];
        deg = out_degree_[u];
    }
    inline void in_neighbors(uint32_t u, const uint32_t *&adj, uint32_t &deg) const {
        adj = in_adj_ + in_offset_[u];
        deg = in_degree_[u];
    }

    void load(const string &path) {
        string filepath = path;
        if (filepath.empty() || filepath[filepath.size() - 1] != '/') filepath += "/";
        filepath += "graph.txt";
        FILE *fin = fopen(filepath.c_str(), "r");
        if (!fin) { cerr << "Error: Cannot open file " << filepath << endl; exit(1); }

        char line[256];
        vector<pair<pair<uint32_t, uint32_t>, uint8_t> > edges;
        long long max_node = 0;
        while (fgets(line, sizeof(line), fin)) {
            int a, b, sgn;
            string sline(line);
            for (size_t i = 0; i < sline.size(); ++i)
                if (sline[i] == '\t' || sline[i] == ',') sline[i] = ' ';
            if (sscanf(sline.c_str(), "%d %d %d", &a, &b, &sgn) != 3) continue;
            if (a == b) continue;
            if (a < 0 || b < 0) continue;
            uint8_t ep = (sgn > 0) ? 0 : 1;
            edges.push_back(make_pair(make_pair((uint32_t) a, (uint32_t) b), ep));
            max_node = max(max_node, (long long) max(a, b));
        }
        fclose(fin);

        sort(edges.begin(), edges.end());
        edges.erase(unique(edges.begin(), edges.end()), edges.end());

        num_vertices_ = (uint32_t) (max_node + 1);
        num_edges_ = edges.size();

        out_degree_ = (uint32_t*) calloc(num_vertices_, sizeof(uint32_t));
        in_degree_  = (uint32_t*) calloc(num_vertices_, sizeof(uint32_t));
        for (size_t i = 0; i < edges.size(); ++i) {
            out_degree_[edges[i].first.first]++;
            in_degree_[edges[i].first.second]++;
        }
        out_offset_ = (uint32_t*) malloc(sizeof(uint32_t) * (num_vertices_ + 1));
        in_offset_  = (uint32_t*) malloc(sizeof(uint32_t) * (num_vertices_ + 1));
        out_offset_[0] = in_offset_[0] = 0;
        for (uint32_t v = 0; v < num_vertices_; ++v) {
            out_offset_[v + 1] = out_offset_[v] + out_degree_[v];
            in_offset_[v + 1]  = in_offset_[v]  + in_degree_[v];
        }
        out_adj_  = (uint32_t*) malloc(sizeof(uint32_t) * (edges.size() ? edges.size() : 1));
        out_sign_ = (uint8_t*)  malloc(sizeof(uint8_t)  * (edges.size() ? edges.size() : 1));
        in_adj_   = (uint32_t*) malloc(sizeof(uint32_t) * (edges.size() ? edges.size() : 1));
        vector<uint32_t> ocur(num_vertices_, 0), icur(num_vertices_, 0);
        for (size_t i = 0; i < edges.size(); ++i) {
            uint32_t a = edges[i].first.first, b = edges[i].first.second;
            uint32_t p = out_offset_[a] + ocur[a]++;
            out_adj_[p] = b; out_sign_[p] = edges[i].second;
            in_adj_[in_offset_[b] + icur[b]++] = a;
        }
        printf("Graph loaded: n = %u, m = %llu\n", num_vertices_,
               (unsigned long long) num_edges_);
    }
};


class BSPathEnum {
public:
    BSPathEnum() : digraph_(nullptr), length_constraint_(0), src_(0), dst_(0),
                   target_parity_(0), found_(false), stack_(nullptr),
                   visited_(nullptr), distance_(nullptr), updated_values_(nullptr),
                   buckets_offset_(nullptr), buckets_(nullptr),
                   single_bigraph_offset_(nullptr), single_bigraph_adj_(nullptr),
                   single_bigraph_sign_(nullptr) {}

    ~BSPathEnum() { clear(); }

    void initialize(SignedDigraph *digraph, uint32_t length_constraint) {
        assert(length_constraint > 0 && digraph != nullptr);
        digraph_ = digraph;
        length_constraint_ = length_constraint;

        uint64_t size = sizeof(uint32_t) * (length_constraint + 2);
        stack_ = (uint32_t*) malloc(size);
        memset(stack_, 0, size);

        uint32_t n = digraph->num_vertices();
        visited_ = (bool*) calloc(n, sizeof(bool));
        updated_values_ = (uint32_t*) calloc(n, sizeof(uint32_t));
        distance_ = (pair<uint8_t, uint8_t>*) malloc(sizeof(pair<uint8_t, uint8_t>) * n);
        memset((uint8_t*) distance_, static_cast<uint8_t>(length_constraint) + 1,
               sizeof(pair<uint8_t, uint8_t>) * n);

        buckets_offset_ = (uint32_t*) calloc((length_constraint_ + 1) * (length_constraint_ + 1) + 1,
                                             sizeof(uint32_t));
    }

    void clear() {
        free(stack_);          stack_ = nullptr;
        free(visited_);        visited_ = nullptr;
        free(updated_values_); updated_values_ = nullptr;
        free(distance_);       distance_ = nullptr;
        free(buckets_offset_); buckets_offset_ = nullptr;
        free(buckets_);        buckets_ = nullptr;
    }

    int query(uint32_t s, uint32_t t, uint32_t k, int sigma_bit) {
        if (s >= digraph_->num_vertices() || t >= digraph_->num_vertices()) return 0;
        if (s == t) return sigma_bit == 1 ? 1 : 0;
        assert(k == length_constraint_);

        src_ = s; dst_ = t;
        target_parity_ = (sigma_bit == 1) ? 0 : 1;
        found_ = false;

        fast_build_bigraph();
        dfs_on_bigraph(src_, 0, 0);
        clear_bigraph();
        return found_ ? 1 : 0;
    }

private:
    SignedDigraph *digraph_;
    uint32_t length_constraint_;
    uint32_t src_, dst_;
    uint8_t  target_parity_;
    bool     found_;

    uint32_t *stack_;
    bool *visited_;
    pair<uint8_t, uint8_t> *distance_;
    uint32_t *updated_values_;
    uint32_t *buckets_offset_;
    uint32_t *buckets_;

    spp::sparse_hash_map<uint32_t, uint32_t> single_bigraph_;
    uint32_t *single_bigraph_offset_;
    uint32_t *single_bigraph_adj_;
    uint8_t  *single_bigraph_sign_;


    void fast_build_bigraph() {
        uint32_t num_vertices = digraph_->num_vertices();
        uint32_t updated_values_count = 0;

        if (length_constraint_ > numeric_limits<uint8_t>::max()) {
            cerr << "The length constraint is greater than uint8_t." << endl;
            exit(-1);
        }
        auto k = static_cast<uint8_t>(length_constraint_);
        queue<uint32_t> q;

        visited_[src_] = true;
        visited_[dst_] = true;
        updated_values_[updated_values_count++] = src_;
        updated_values_[updated_values_count++] = dst_;
        q.push(src_);
        distance_[src_].first = 0;

        while (!q.empty()) {
            uint32_t v = q.front(); q.pop();
            if (distance_[v].first < k - 1) {
                uint8_t next_distance = distance_[v].first + 1;
                const uint32_t *adj; const uint8_t *sgn; uint32_t deg;
                digraph_->out_neighbors(v, adj, sgn, deg);
                for (uint32_t i = 0; i < deg; ++i) {
                    uint32_t vv = adj[i];
                    if (!visited_[vv]) {
                        visited_[vv] = true;
                        distance_[vv].first = next_distance;
                        updated_values_[updated_values_count++] = vv;
                        q.push(vv);
                    }
                }
            }
        }

        vector<vector<uint32_t> > temp_buckets((length_constraint_ + 1) * (length_constraint_ + 1));

        if (num_vertices / updated_values_count > 64 * 32) {
            for (uint32_t i = 0; i < updated_values_count; ++i) visited_[updated_values_[i]] = false;
        } else {
            memset(visited_, 0, sizeof(bool) * num_vertices);
        }

        q.push(dst_);
        visited_[dst_] = true;
        visited_[src_] = true;
        distance_[dst_].second = 0;

        uint32_t active_vertices_count = 0;
        while (!q.empty()) {
            uint32_t v = q.front(); q.pop();
            if (distance_[v].second < k - 1) {
                uint32_t next_distance = distance_[v].second + 1;
                const uint32_t *adj; uint32_t deg;
                digraph_->in_neighbors(v, adj, deg);
                for (uint32_t i = 0; i < deg; ++i) {
                    uint32_t vv = adj[i];
                    if (!visited_[vv] && distance_[vv].first + next_distance <= k) {
                        visited_[vv] = true;
                        distance_[vv].second = next_distance;
                        q.push(vv);
                        active_vertices_count += 1;
                        uint32_t bucket_id = BUCKET_ID(distance_[vv].first, distance_[vv].second, k + 1);
                        temp_buckets[bucket_id].push_back(vv);
                    }
                }
            }
        }

        active_vertices_count += 1;
        buckets_ = (uint32_t*) malloc(sizeof(uint32_t) * active_vertices_count);
        buckets_[0] = src_;
        uint32_t offset = 1;
        for (uint32_t i = 0; i < length_constraint_ + 1; ++i) {
            for (uint32_t j = 0; j < length_constraint_ + 1; ++j) {
                uint32_t bucket_id = BUCKET_ID(i, j, length_constraint_ + 1);
                buckets_offset_[bucket_id] = offset;
                memcpy(buckets_ + offset, temp_buckets[bucket_id].data(),
                       sizeof(uint32_t) * temp_buckets[bucket_id].size());
                offset += temp_buckets[bucket_id].size();
                temp_buckets[bucket_id].clear();
            }
        }
        buckets_offset_[(length_constraint_ + 1) * (length_constraint_ + 1)] = offset;

        vector<uint32_t> temp_bigraph_adj;   temp_bigraph_adj.reserve(1024);
        vector<uint8_t>  temp_bigraph_sign;  temp_bigraph_sign.reserve(1024);
        vector<vector<uint32_t> > temp_adj(length_constraint_);
        vector<vector<uint8_t> >  temp_sgn(length_constraint_);

        single_bigraph_offset_ = (uint32_t*) calloc(length_constraint_ * (active_vertices_count + 1),
                                                    sizeof(uint32_t));

        for (uint32_t i = 0; i < active_vertices_count; ++i) {
            uint32_t v = buckets_[i];
            const uint32_t *adj; const uint8_t *sgn; uint32_t deg;
            digraph_->out_neighbors(v, adj, sgn, deg);

            for (uint32_t j = 0; j < deg; ++j) {
                uint32_t vv = adj[j];
                if (vv == dst_) {
                    temp_adj[0].push_back(vv);
                    temp_sgn[0].push_back(sgn[j]);
                } else if (visited_[vv] && distance_[vv].second < k) {
                    temp_adj[distance_[vv].second].push_back(vv);
                    temp_sgn[distance_[vv].second].push_back(sgn[j]);
                }
            }

            uint32_t temp_offset = i * length_constraint_;
            for (uint32_t j = 0; j < length_constraint_; ++j) {
                single_bigraph_offset_[temp_offset + j] = temp_bigraph_adj.size();
                temp_bigraph_adj.insert(temp_bigraph_adj.end(), temp_adj[j].begin(), temp_adj[j].end());
                temp_bigraph_sign.insert(temp_bigraph_sign.end(), temp_sgn[j].begin(), temp_sgn[j].end());
                temp_adj[j].clear();
                temp_sgn[j].clear();
            }
            single_bigraph_offset_[temp_offset + length_constraint_] = temp_bigraph_adj.size();
            single_bigraph_[v] = temp_offset;
        }

        single_bigraph_adj_  = (uint32_t*) malloc(sizeof(uint32_t) * (temp_bigraph_adj.size() + 1));
        single_bigraph_sign_ = (uint8_t*)  malloc(sizeof(uint8_t)  * (temp_bigraph_adj.size() + 1));
        memcpy(single_bigraph_adj_,  temp_bigraph_adj.data(),  sizeof(uint32_t) * temp_bigraph_adj.size());
        memcpy(single_bigraph_sign_, temp_bigraph_sign.data(), sizeof(uint8_t)  * temp_bigraph_sign.size());

        if (num_vertices / updated_values_count > 16 * 8) {
            for (uint32_t i = 0; i < updated_values_count; ++i) {
                visited_[updated_values_[i]] = false;
                distance_[updated_values_[i]] = make_pair((uint8_t)(k + 1), (uint8_t)(k + 1));
            }
        } else {
            memset(visited_, 0, sizeof(bool) * num_vertices);
            memset((uint8_t*) distance_, k + 1, sizeof(pair<uint8_t, uint8_t>) * num_vertices);
        }
    }

    void dfs_on_bigraph(uint32_t u, uint32_t k, uint8_t parity) {
        stack_[k] = u;
        visited_[u] = true;

        uint32_t budget = length_constraint_ - k - 1;
        uint32_t neighbor_offset = single_bigraph_[u];
        uint32_t start = single_bigraph_offset_[neighbor_offset];
        uint32_t end   = single_bigraph_offset_[neighbor_offset + budget + 1];

        for (uint32_t i = start; i < end; ++i) {
            if (found_) goto EXIT;

            uint32_t v = single_bigraph_adj_[i];
            uint8_t  p = parity ^ single_bigraph_sign_[i];

            if (v == dst_) {
                if (p == target_parity_) {
                    stack_[k + 1] = dst_;
                    found_ = true;
                    goto EXIT;
                }
            }
            else if (k == length_constraint_ - 2 && !visited_[v]) {
                auto it = single_bigraph_.find(v);
                if (it != single_bigraph_.end()) {
                    uint32_t voff = it->second;
                    uint32_t vs = single_bigraph_offset_[voff];
                    uint32_t ve = single_bigraph_offset_[voff + 1];
                    for (uint32_t j = vs; j < ve; ++j) {
                        if (single_bigraph_adj_[j] != dst_) continue;
                        if ((uint8_t)(p ^ single_bigraph_sign_[j]) == target_parity_) {
                            stack_[k + 1] = v;
                            stack_[k + 2] = dst_;
                            found_ = true;
                            goto EXIT;
                        }
                    }
                }
            }
            else if (!visited_[v]) {
                dfs_on_bigraph(v, k + 1, p);
                if (found_) goto EXIT;
            }
        }

        EXIT:
        visited_[u] = false;
    }

    void clear_bigraph() {
        memset(buckets_offset_, 0,
               sizeof(uint32_t) * ((length_constraint_ + 1) * (length_constraint_ + 1) + 1));
        free(single_bigraph_adj_);    single_bigraph_adj_ = nullptr;
        free(single_bigraph_sign_);   single_bigraph_sign_ = nullptr;
        free(single_bigraph_offset_); single_bigraph_offset_ = nullptr;
        free(buckets_);               buckets_ = nullptr;
        single_bigraph_.clear();
    }
};

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s <dataset_dir> <k>\n", argv[0]);
        printf("Example: %s ../../datasets/bitcoin 6\n", argv[0]);
        printf("PathEnum's IDX-DFS adapted to HSR: the per-query light-weight index\n");
        printf("is built as published, the DFS carries the sign parity and stops at\n");
        printf("the first path whose sign matches the query.\n");
        return 1;
    }
    string dataset_path = argv[1];
    int k = atoi(argv[2]);
    if (k < 1) { cerr << "k must be >= 1" << endl; return 1; }

    string dir = dataset_path;
    if (dir.empty() || dir[dir.size() - 1] != '/') dir += "/";

    SignedDigraph graph;
    graph.load(dataset_path);

    uint32_t kk = (uint32_t) k;
    if (kk < 2) kk = 2;
    if (graph.num_vertices() > 0 && kk > graph.num_vertices()) kk = graph.num_vertices();

    BSPathEnum solver;
    solver.initialize(&graph, kk);

    FILE *fin = fopen((dir + "query_exp1.txt").c_str(), "r");
    if (!fin) { cerr << "Error: Cannot open query file " << dir + "query_exp1.txt" << endl; return 1; }

    char resname[64];
    snprintf(resname, sizeof(resname), "BS_PATHENUM_results_k%d.txt", k);
    FILE *fres = fopen((dir + resname).c_str(), "w");

    char line[256];
    int query_count = 0, true_count = 0, false_count = 0;

    clock_t start_time = clock();
    while (fgets(line, sizeof(line), fin)) {
        int s, t, sigma;
        if (sscanf(line, "%d,%d,%d", &s, &t, &sigma) != 3) continue;
        ++query_count;
        int r = 0;
        if (s >= 0 && t >= 0) r = solver.query((uint32_t) s, (uint32_t) t, kk, sigma);
        if (r == 1) ++true_count; else ++false_count;
        if (fres) fprintf(fres, "%d,%d,%d,%d\n", s, t, sigma, r);
    }
    clock_t end_time = clock();
    double query_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;

    fclose(fin);
    if (fres) fclose(fres);

    FILE *fout = fopen((dir + "BS_PATHENUM_Exp1.txt").c_str(), "a");
    if (!fout) { cerr << "Error: Cannot create output file" << endl; return 1; }
    fprintf(fout, "\nBS-PathEnum (HSR baseline adapted from PathEnum [50])\n");
    fprintf(fout, "Dataset: %s\n", dataset_path.c_str());
    fprintf(fout, "Hop constraint k = %d (no time limit)\n", k);
    fprintf(fout, "\nSummary (Query_path: %squery_exp1.txt, k = %d):\n", dir.c_str(), k);
    fprintf(fout, "Total queries: %d\n", query_count);
    fprintf(fout, "TRUE: %d\n", true_count);
    fprintf(fout, "FALSE: %d\n", false_count);
    fprintf(fout, "Total query time: %.5lf seconds\n", query_time);
    fprintf(fout, "========================================\n\n");
    fclose(fout);

    printf("BS-PathEnum done: %d queries, TRUE=%d FALSE=%d, time=%.5lf s\n",
           query_count, true_count, false_count, query_time);
    return 0;
}
