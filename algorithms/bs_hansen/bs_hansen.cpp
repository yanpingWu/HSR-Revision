#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cctype>
#include <ctime>
#include <string>
#include <vector>
#include <algorithm>
#include <iostream>

using namespace std;

static const int INF = 1 << 29;

static inline int signIdx(int s) { return s > 0 ? 0 : 1; }
static inline int signVal(int i) { return i == 0 ? 1 : -1; }

class BSHansenBCT {
public:
    int n;
    vector<vector<pair<int, int> > > adj_out;
    vector<vector<pair<int, int> > > adj_in;
    int epoch;

    long long stat_bnb_targets;
    long long stat_bnb_nodes;
    long long stat_bnb_nodes_max;
    long long stat_pos_circuits;
    int stat_bnb_queries;

    long long stat_total_blocks;
    long long stat_blocks_max_query;
    long long stat_decomposed_queries;
    long long stat_multiblock_queries;
    long long stat_block_solves;
    long long stat_nontrivial_solves;
    long long stat_worklist_recomp;
    int stat_largest_scc;
    int stat_largest_block;

    BSHansenBCT() : n(0),
                    epoch(0),
                    stat_bnb_targets(0), stat_bnb_nodes(0), stat_bnb_nodes_max(0),
                    stat_pos_circuits(0), stat_bnb_queries(0),
                    stat_total_blocks(0), stat_blocks_max_query(0),
                    stat_decomposed_queries(0), stat_multiblock_queries(0),
                    stat_block_solves(0), stat_nontrivial_solves(0),
                    stat_worklist_recomp(0), stat_largest_scc(0), stat_largest_block(0),
                    q_blocks(0), q_decomposed(false), q_multiblock(false),
                    qk(0), qsigma(1), qt_local(-1), qCompOfT(-1), bnb_found(false),
                    query_used_bnb(false), sccToken(0),
                    sccSz(0), sccId(-1), sccMem(NULL), nblocks(0), tMi(-1),
                    cur_sz(0), cur_blk(-1), dlaRun(0) {}

    void loadGraph(const string &path) {
        string filepath = path;
        if (filepath.empty() || filepath[filepath.size() - 1] != '/') filepath += "/";
        filepath += "graph.txt";

        FILE *fin = fopen(filepath.c_str(), "r");
        if (!fin) { cerr << "Error: Cannot open file " << filepath << endl; exit(1); }

        char line[256];
        vector<pair<pair<int, int>, int> > edges;
        int max_node = 0;
        while (fgets(line, sizeof(line), fin)) {
            int a, b, sgn;
            string sline(line);
            for (size_t i = 0; i < sline.size(); ++i)
                if (sline[i] == '\t' || sline[i] == ',') sline[i] = ' ';
            if (sscanf(sline.c_str(), "%d %d %d", &a, &b, &sgn) != 3) continue;
            if (a == b) continue;                 
            int es = (sgn > 0) ? 1 : -1;         
            edges.push_back(make_pair(make_pair(a, b), es));
            max_node = max(max_node, max(a, b));
        }
        fclose(fin);

        n = max_node + 1;
        adj_out.assign(n, vector<pair<int, int> >());
        adj_in.assign(n, vector<pair<int, int> >());
        for (size_t i = 0; i < edges.size(); ++i) {
            int a = edges[i].first.first, b = edges[i].first.second, es = edges[i].second;
            adj_out[a].push_back(make_pair(b, es));
            adj_in[b].push_back(make_pair(a, es));
        }
        long long m = 0;
        for (int u = 0; u < n; ++u) {
            sort(adj_out[u].begin(), adj_out[u].end());
            adj_out[u].erase(unique(adj_out[u].begin(), adj_out[u].end()), adj_out[u].end());
            sort(adj_in[u].begin(), adj_in[u].end());
            adj_in[u].erase(unique(adj_in[u].begin(), adj_in[u].end()), adj_in[u].end());
            m += (long long) adj_out[u].size();
        }
        printf("Graph loaded: n = %d, m = %lld\n", n, m);
    }

    long long q_blocks;
    bool q_decomposed, q_multiblock;
    void commitQueryStats() {
        stat_total_blocks += q_blocks;
        if (q_blocks > stat_blocks_max_query) stat_blocks_max_query = q_blocks;
        if (q_decomposed) ++stat_decomposed_queries;
        if (q_multiblock) ++stat_multiblock_queries;
        q_blocks = 0; q_decomposed = false; q_multiblock = false;
    }

    int query(int s, int t, int k, int sigma_bit) {
        if (s < 0 || s >= n || t < 0 || t >= n) return 0;
        if (k < 0) return 0;
        if (k > n) k = n;
        int sigma = sigma_bit ? 1 : -1;
        if (s == t) return sigma == 1 ? 1 : 0;   
        ++epoch;
        qk = k; qsigma = sigma;
        query_used_bnb = false;
        circPool.clear();
        q_blocks = 0; q_decomposed = false; q_multiblock = false;

        int R = n;
        Rverts.resize(n);
        for (int i = 0; i < n; ++i) Rverts[i] = i;
        int sLocal = s, tLocal = t;
        locOut.assign(n, vector<pair<int, int> >());
        locIn.assign(n, vector<pair<int, int> >());
        for (int u = 0; u < n; ++u) {
            const vector<pair<int, int> > &nb = adj_out[u];
            for (size_t i = 0; i < nb.size(); ++i) {
                int w = nb[i].first; if (w == s) continue;
                locOut[u].push_back(make_pair(w, nb[i].second));
                locIn[w].push_back(make_pair(u, nb[i].second));
            }
        }
        qt_local = tLocal;

        tarjanSCC(R);
        qCompOfT = comp[tLocal];

        vector<int> compOrder;
        topoOrderComps(R, compOrder);

        int ncomp = 0;
        for (int i = 0; i < R; ++i) ncomp = max(ncomp, comp[i] + 1);
        vector<vector<int> > members(ncomp);
        for (int i = 0; i < R; ++i) members[comp[i]].push_back(i);

        Lpos.assign(R, INF); Lneg.assign(R, INF);
        Lpos[sLocal] = 0; Lneg[sLocal] = INF;

        bnb_found = false;
        sccMiVal.assign(R, -1); sccMiStamp.assign(R, -1); sccToken = 0;
        if ((int) buckets.size() < qk + 1) buckets.resize(qk + 1);

        for (size_t oi = 0; oi < compOrder.size(); ++oi) {
            int c = compOrder[oi];
            processComp(c, members[c], sLocal);
            settleTarget();
            if (bnb_found) return 1;
            if (c == qCompOfT) break;
        }
        int Lt = (qsigma > 0) ? Lpos[tLocal] : Lneg[tLocal];
        return (Lt <= k) ? 1 : 0;
    }

private:
    int qk, qsigma;
    int qt_local, qCompOfT;
    bool bnb_found;
    bool query_used_bnb;

    vector<int> Rverts;
    vector<vector<pair<int, int> > > locOut;
    vector<vector<pair<int, int> > > locIn;
    vector<int> comp;
    vector<int> Lpos, Lneg;


    vector<int> sccMiVal, sccMiStamp;
    int sccToken;
    inline int sccMiOf(int l) const {
        return sccMiStamp[l] == sccToken ? sccMiVal[l] : -1;
    }

    int sccSz, sccId;
    const vector<int> *sccMem;
    vector<int> extSeedP, extSeedN;
    vector<int> sArcU, sArcW, sArcS;
    vector<int> arcEdge;
    vector<int> edgeA, edgeB;
    vector<vector<int> > uadj;
    vector<int> blockOfEdge;
    int nblocks;
    vector<vector<int> > blkVerts;
    vector<vector<int> > blkArcs;
    vector<char> blkNontriv;
    vector<vector<int> > vBlocks;
    vector<vector<int> > vBlockPos;
    vector<vector<int> > thrLab;

    vector<int> wlQ;
    vector<char> wlInQ, wlSolved;
    int tMi;


    int cur_sz, cur_blk;
    vector<int> ctxLid;
    vector<int> miBi;
    vector<int> seedP, seedN;
    vector<int> arcFrom, arcTo, arcSg;
    vector<vector<int> > adjArcs;
    vector<char> exArc;

    vector<int> uDist, uStamp, uPred, uParc;
    int dlaRun;
    vector<vector<int> > buckets;
    vector<int> wkStates, wkArcs;
    vector<int> circPool;

    inline int uGet(int st) const { return uStamp[st] == dlaRun ? uDist[st] : INF; }


    void tarjanSCC(int R) {
        comp.assign(R, -1);
        vector<int> idx(R, -1), low(R, 0);
        vector<char> onstk(R, 0);
        vector<int> stk;
        vector<pair<int, int> > call;
        int idxc = 0, ncomp = 0;
        for (int s0 = 0; s0 < R; ++s0) {
            if (idx[s0] != -1) continue;
            call.push_back(make_pair(s0, 0));
            idx[s0] = low[s0] = idxc++; stk.push_back(s0); onstk[s0] = 1;
            while (!call.empty()) {
                int u = call.back().first;
                int &ci = call.back().second;
                if (ci < (int) locOut[u].size()) {
                    int w = locOut[u][ci++].first;
                    if (idx[w] == -1) {
                        idx[w] = low[w] = idxc++; stk.push_back(w); onstk[w] = 1;
                        call.push_back(make_pair(w, 0));
                    } else if (onstk[w]) {
                        if (idx[w] < low[u]) low[u] = idx[w];
                    }
                } else {
                    if (low[u] == idx[u]) {
                        while (true) {
                            int x = stk.back(); stk.pop_back(); onstk[x] = 0;
                            comp[x] = ncomp;
                            if (x == u) break;
                        }
                        ++ncomp;
                    }
                    call.pop_back();
                    if (!call.empty()) {
                        int p = call.back().first;
                        if (low[u] < low[p]) low[p] = low[u];
                    }
                }
            }
        }
    }


    void topoOrderComps(int R, vector<int> &order) {
        int ncomp = 0;
        for (int i = 0; i < R; ++i) ncomp = max(ncomp, comp[i] + 1);
        vector<int> indeg(ncomp, 0);
        vector<vector<int> > cadj(ncomp);
        for (int lu = 0; lu < R; ++lu) {
            int cu = comp[lu];
            for (size_t i = 0; i < locOut[lu].size(); ++i) {
                int cv = comp[locOut[lu][i].first];
                if (cu == cv) continue;
                cadj[cu].push_back(cv); indeg[cv]++;
            }
        }
        vector<int> q;
        for (int c = 0; c < ncomp; ++c) if (indeg[c] == 0) q.push_back(c);
        for (size_t h = 0; h < q.size(); ++h) {
            int c = q[h]; order.push_back(c);
            for (size_t i = 0; i < cadj[c].size(); ++i)
                if (--indeg[cadj[c][i]] == 0) q.push_back(cadj[c][i]);
        }
    }


    void settleTarget() {
        if (qt_local < 0) return;
        int Lt = (qsigma > 0) ? Lpos[qt_local] : Lneg[qt_local];
        if (Lt <= qk) bnb_found = true;
    }


    void processComp(int c, const vector<int> &mem, int sLocal) {
        int sz = (int) mem.size();
        extSeedP.assign(sz, INF); extSeedN.assign(sz, INF);
        for (int mi = 0; mi < sz; ++mi) {
            int b = mem[mi];
            for (size_t i = 0; i < locIn[b].size(); ++i) {
                int w = locIn[b][i].first; int e = locIn[b][i].second;
                if (comp[w] == c) continue;
                if (Lpos[w] < INF) {   
                    int val = Lpos[w] + 1;
                    if (val <= qk) {
                        if (e > 0) { if (val < extSeedP[mi]) extSeedP[mi] = val; }
                        else       { if (val < extSeedN[mi]) extSeedN[mi] = val; }
                    }
                }
                if (Lneg[w] < INF) {
                    int val = Lneg[w] + 1;
                    if (val <= qk) {
                        if (e > 0) { if (val < extSeedN[mi]) extSeedN[mi] = val; }
                        else       { if (val < extSeedP[mi]) extSeedP[mi] = val; }
                    }
                }
            }
        }

        if (sz == 1) {  
            int b = mem[0];
            if (b != sLocal) {
                if (extSeedP[0] < Lpos[b]) Lpos[b] = extSeedP[0];
                if (extSeedN[0] < Lneg[b]) Lneg[b] = extSeedN[0];
            }
            return;
        }

        blockStage(c, mem);
    }

    void buildBlocks(int sz) {
        int ne = (int) edgeA.size();
        blockOfEdge.assign(ne, -1);
        nblocks = 0;
        vector<int> disc(sz, -1), low(sz, 0);
        vector<int> estk;
        vector<int> fV, fPe, fIt;
        int timer = 0;
        for (int r = 0; r < sz; ++r) {
            if (disc[r] != -1) continue;
            disc[r] = low[r] = timer++;
            fV.push_back(r); fPe.push_back(-1); fIt.push_back(0);
            while (!fV.empty()) {
                int v = fV.back();
                int fi = (int) fV.size() - 1;
                if (fIt[fi] < (int) uadj[v].size()) {
                    int eid = uadj[v][fIt[fi]++];
                    if (eid == fPe[fi]) continue;
                    int w = (edgeA[eid] == v) ? edgeB[eid] : edgeA[eid];
                    if (disc[w] == -1) { 
                        estk.push_back(eid);
                        disc[w] = low[w] = timer++;
                        fV.push_back(w); fPe.push_back(eid); fIt.push_back(0);
                    } else if (disc[w] < disc[v]) { 
                        estk.push_back(eid);
                        if (disc[w] < low[v]) low[v] = disc[w];
                    }
                } else {
                    int pe = fPe[fi];
                    fV.pop_back(); fPe.pop_back(); fIt.pop_back();
                    if (!fV.empty()) {
                        int u = fV.back();
                        if (low[v] < low[u]) low[u] = low[v];
                        if (low[v] >= disc[u]) {
                            // u is an articulation point (or the root): one block
                            int b = nblocks++;
                            while (!estk.empty()) {
                                int e = estk.back(); estk.pop_back();
                                blockOfEdge[e] = b;
                                if (e == pe) break;
                            }
                        }
                    }
                }
            }
        }
    }


    void blockStage(int c, const vector<int> &mem) {
        int sz = (int) mem.size();
        sccSz = sz; sccId = c; sccMem = &mem;
        if (sz > stat_largest_scc) stat_largest_scc = sz;
        ++sccToken;
        for (int mi = 0; mi < sz; ++mi) {
            sccMiVal[mem[mi]] = mi; sccMiStamp[mem[mi]] = sccToken;
        }
        sArcU.clear(); sArcW.clear(); sArcS.clear();
        for (int mi = 0; mi < sz; ++mi) {
            int u = mem[mi];
            for (size_t i = 0; i < locOut[u].size(); ++i) {
                int w = locOut[u][i].first;
                if (comp[w] != c) continue;              // internal arcs only
                sArcU.push_back(mi);
                sArcW.push_back(sccMiOf(w));
                sArcS.push_back(locOut[u][i].second);
            }
        }
        int nA = (int) sArcU.size();

        vector<long long> keys(nA);
        for (int i = 0; i < nA; ++i) {
            int a = sArcU[i], b = sArcW[i];
            if (a > b) { int tmp = a; a = b; b = tmp; }
            keys[i] = (long long) a * sz + b;
        }
        vector<long long> ekeys(keys);
        sort(ekeys.begin(), ekeys.end());
        ekeys.erase(unique(ekeys.begin(), ekeys.end()), ekeys.end());
        int ne = (int) ekeys.size();
        edgeA.assign(ne, 0); edgeB.assign(ne, 0);
        for (int e = 0; e < ne; ++e) {
            edgeA[e] = (int) (ekeys[e] / sz);
            edgeB[e] = (int) (ekeys[e] % sz);
        }
        arcEdge.assign(nA, -1);
        for (int i = 0; i < nA; ++i)
            arcEdge[i] = (int) (lower_bound(ekeys.begin(), ekeys.end(), keys[i]) - ekeys.begin());
        uadj.assign(sz, vector<int>());
        for (int e = 0; e < ne; ++e) {
            uadj[edgeA[e]].push_back(e);
            uadj[edgeB[e]].push_back(e);
        }
        buildBlocks(sz);


        vector<vector<int> > edgesOfBlock(nblocks);
        for (int e = 0; e < ne; ++e)
            if (blockOfEdge[e] >= 0) edgesOfBlock[blockOfEdge[e]].push_back(e);
        blkVerts.assign(nblocks, vector<int>());
        vBlocks.assign(sz, vector<int>());
        vBlockPos.assign(sz, vector<int>());
        vector<int> seenB(sz, -1);
        for (int b = 0; b < nblocks; ++b) {
            for (size_t j = 0; j < edgesOfBlock[b].size(); ++j) {
                int e = edgesOfBlock[b][j];
                int ends[2] = { edgeA[e], edgeB[e] };
                for (int x = 0; x < 2; ++x) {
                    int mi = ends[x];
                    if (seenB[mi] == b) continue;
                    seenB[mi] = b;
                    vBlocks[mi].push_back(b);
                    vBlockPos[mi].push_back((int) blkVerts[b].size());
                    blkVerts[b].push_back(mi);
                }
            }
            if ((int) blkVerts[b].size() > stat_largest_block)
                stat_largest_block = (int) blkVerts[b].size();
        }

        blkArcs.assign(nblocks, vector<int>());
        for (int i = 0; i < nA; ++i) {
            int b = blockOfEdge[arcEdge[i]];
            if (b >= 0) blkArcs[b].push_back(i);
        }


        blkNontriv.assign(nblocks, 0);
        for (int b = 0; b < nblocks; ++b) {
            if ((int) blkVerts[b].size() >= 3) { blkNontriv[b] = 1; continue; }
            if ((int) blkVerts[b].size() == 2) {
                int m0 = blkVerts[b][0];
                bool d0 = false, d1 = false;
                for (size_t j = 0; j < blkArcs[b].size(); ++j) {
                    int a = blkArcs[b][j];
                    if (sArcU[a] == m0) d0 = true; else d1 = true;
                }
                if (d0 && d1) blkNontriv[b] = 1;
            }
        }

        q_decomposed = true;
        q_blocks += nblocks;
        if (nblocks > 1) q_multiblock = true;

        thrLab.assign(nblocks, vector<int>());
        for (int b = 0; b < nblocks; ++b)
            thrLab[b].assign(2 * blkVerts[b].size(), INF);

        bool isT = (c == qCompOfT);
        tMi = isT ? sccMiOf(qt_local) : -1;

 
        wlInQ.assign(nblocks, 0); wlSolved.assign(nblocks, 0); wlQ.clear();
        for (int b = 0; b < nblocks; ++b) {
            for (size_t j = 0; j < blkVerts[b].size(); ++j) {
                int mi = blkVerts[b][j];
                if (extSeedP[mi] < INF || extSeedN[mi] < INF) {
                    wlInQ[b] = 1; wlQ.push_back(b);
                    break;
                }
            }
        }
        for (size_t head = 0; head < wlQ.size(); ++head) {
            int b = wlQ[head]; wlInQ[b] = 0;
            if (wlSolved[b]) ++stat_worklist_recomp;
            wlSolved[b] = 1;
            solveBlock(b, isT);
        }

        if (isT) {
            int si = signIdx(qsigma);
            int best = (si == 0) ? extSeedP[tMi] : extSeedN[tMi];
            for (size_t j = 0; j < vBlocks[tMi].size(); ++j) {
                int b2 = vBlocks[tMi][j], pos = vBlockPos[tMi][j];
                int v2 = thrLab[b2][2 * pos + si];
                if (v2 < best) best = v2;
            }

            if (vBlocks[tMi].size() == 1) {
                int b = vBlocks[tMi][0];
                buildBlockContext(b);
                buildSeeds(b);
                ++stat_block_solves;
                if (blkNontriv[b]) ++stat_nontrivial_solves;
                int tbi = vBlockPos[tMi][0];
                int tState = 2 * tbi + si;
                ++dlaRun;
                unitDLA(tState);
                    int rootVal = uGet(tState);
                if (rootVal < INF && rootVal <= qk) {
                    bool rootElem = false;
                    if (reconstructWalk(tState)) { int i0; rootElem = (firstRepeat(i0) < 0); }
                    int val = rootElem ? rootVal
                                       : elementaryValue(tState, true);
                    if (val < best) best = val;
                }
                }
            if (best <= qk) {
                int &lab = (si == 0) ? Lpos[qt_local] : Lneg[qt_local];
                if (best < lab) lab = best;
            }
        }
    }


    void buildBlockContext(int b) {
        const vector<int> &bv = blkVerts[b];
        cur_sz = (int) bv.size(); cur_blk = b;
        miBi.assign(sccSz, -1);
        ctxLid.assign(cur_sz, 0);
        for (int bi = 0; bi < cur_sz; ++bi) {
            miBi[bv[bi]] = bi;
            ctxLid[bi] = (*sccMem)[bv[bi]];
        }
        arcFrom.clear(); arcTo.clear(); arcSg.clear();
        adjArcs.assign(cur_sz, vector<int>());
        const vector<int> &ba = blkArcs[b];
        for (size_t j = 0; j < ba.size(); ++j) {
            int a = ba[j];
            int from = miBi[sArcU[a]], to = miBi[sArcW[a]];
            int id = (int) arcFrom.size();
            arcFrom.push_back(from);
            arcTo.push_back(to);
            arcSg.push_back(sArcS[a]);
            adjArcs[from].push_back(id);
        }
        exArc.assign(arcFrom.size(), 0);                 // X = {} at the root
        uDist.assign(2 * cur_sz, 0); uStamp.assign(2 * cur_sz, -1);
        uPred.assign(2 * cur_sz, -1); uParc.assign(2 * cur_sz, -1);
        dlaRun = 0;
    }

    void buildSeeds(int b) {
        seedP.assign(cur_sz, INF); seedN.assign(cur_sz, INF);
        for (int bi = 0; bi < cur_sz; ++bi) {
            int mi = blkVerts[b][bi];
            int sp = extSeedP[mi], sn = extSeedN[mi];
            if (vBlocks[mi].size() >= 2) {               // cut vertex
                for (size_t j = 0; j < vBlocks[mi].size(); ++j) {
                    int b2 = vBlocks[mi][j];
                    if (b2 == b) continue;               // direction-exclusion
                    int pos = vBlockPos[mi][j];
                    int vp = thrLab[b2][2 * pos + 0];
                    int vn = thrLab[b2][2 * pos + 1];
                    if (vp < sp) sp = vp;
                    if (vn < sn) sn = vn;
                }
            }
            seedP[bi] = sp; seedN[bi] = sn;
        }
    }


    void solveBlock(int b, bool isTComp) {
        buildBlockContext(b);
        buildSeeds(b);
        bool any = false;
        for (int bi = 0; bi < cur_sz && !any; ++bi)
            if (seedP[bi] <= qk || seedN[bi] <= qk) any = true;
        if (!any) return;
        ++stat_block_solves;
        if (blkNontriv[b]) ++stat_nontrivial_solves;

        if (!isTComp) {
            ++dlaRun;
            unitDLA(-1);
            vector<int> cVal(2 * cur_sz, INF); vector<char> cElem(2 * cur_sz, 0);
            for (int st = 0; st < 2 * cur_sz; ++st) {
                    cVal[st] = uGet(st);
                if (cVal[st] < INF && reconstructWalk(st)) {
                    int i0; cElem[st] = (firstRepeat(i0) < 0);
                }
            }
            for (int bi = 0; bi < cur_sz; ++bi) {
                int v = ctxLid[bi];
                    for (int si = 0; si < 2; ++si) {
                    int st = 2 * bi + si;
                    if (cVal[st] >= INF) continue;
                    if (cVal[st] > qk) continue;
                    int val = cElem[st] ? cVal[st]
                                        : elementaryValue(st, false);
                            if (val >= INF || val > qk) continue;
                    int &lab = (si == 0) ? Lpos[v] : Lneg[v];
                    if (val < lab) lab = val;
                }
            }
        }

        for (int bi = 0; bi < cur_sz; ++bi) {
            int mi = blkVerts[b][bi];
            if (vBlocks[mi].size() < 2) continue;
            int svP = seedP[bi], svN = seedN[bi];
            seedP[bi] = INF; seedN[bi] = INF;
            ++dlaRun;
            unitDLA(-1);
            int vals[2]; char elems[2];
            for (int si = 0; si < 2; ++si) {
                int st = 2 * bi + si;
                vals[si] = uGet(st); elems[si] = 0;
                if (vals[si] < INF && reconstructWalk(st)) {
                    int i0; elems[si] = (firstRepeat(i0) < 0);
                }
            }
            for (int si = 0; si < 2; ++si) {
                int st = 2 * bi + si;
                if (vals[si] >= INF) continue;
                if (vals[si] > qk) continue;
                int val = elems[si] ? vals[si]
                                    : elementaryValue(st, false);
                if (val >= INF || val > qk) continue;
                if (val < thrLab[b][2 * bi + si]) {
                    thrLab[b][2 * bi + si] = val;
                    for (size_t j = 0; j < vBlocks[mi].size(); ++j) {
                        int b2 = vBlocks[mi][j];
                        if (b2 == b || wlInQ[b2]) continue;
                        wlInQ[b2] = 1; wlQ.push_back(b2);
                    }
                }
            }
            seedP[bi] = svP; seedN[bi] = svN;            // unmask
        }
    }


    int elementaryValue(int st, bool isQ) {
        if (!query_used_bnb) { query_used_bnb = true; ++stat_bnb_queries; }
        ++stat_bnb_targets;
        long long nodes = 0;
        int incumbent = INF;
        bnbNode(st, incumbent, nodes, isQ);
        stat_bnb_nodes += nodes;
        if (nodes > stat_bnb_nodes_max) stat_bnb_nodes_max = nodes;
        return incumbent;
    }


    void bnbNode(int target, int &incumbent, long long &nodes, bool isQ) {
        ++nodes;

        ++dlaRun;
        unitDLA(target);
        int LB = uGet(target);
        if (LB >= INF) return;
        if (LB > qk) return;
        if (LB >= incumbent) return;
        if (!reconstructWalk(target)) return;
        int i = -1;
        int j = firstRepeat(i);
        if (j < 0) {
            incumbent = LB;
            if (isQ) bnb_found = true;
            return;
        }
        if (((wkStates[i] ^ wkStates[j]) & 1) == 0)
            ++stat_pos_circuits; 

        size_t base = circPool.size();
        for (int p = i; p < j; ++p) circPool.push_back(wkArcs[p]);
        size_t end = circPool.size();
        for (size_t p = base; p < end; ++p) {
            int e = circPool[p];
            exArc[e] = 1;                    // child: X' = X + {e}
            bnbNode(target, incumbent, nodes, isQ);
            exArc[e] = 0;
            if (isQ && bnb_found) break;
        }
        circPool.resize(base);
    }

    void unitDLA(int earlyTarget) {
        for (int d = 0; d <= qk; ++d) buckets[d].clear();
        for (int mi = 0; mi < cur_sz; ++mi) {
            if (seedP[mi] < INF && seedP[mi] <= qk) {
                int st = 2 * mi;
                uStamp[st] = dlaRun; uDist[st] = seedP[mi];
                uPred[st] = -1; uParc[st] = -1;
                buckets[seedP[mi]].push_back(st);
            }
            if (seedN[mi] < INF && seedN[mi] <= qk) {
                int st = 2 * mi + 1;
                uStamp[st] = dlaRun; uDist[st] = seedN[mi];
                uPred[st] = -1; uParc[st] = -1;
                buckets[seedN[mi]].push_back(st);
            }
        }
        for (int d = 0; d <= qk; ++d) {
            vector<int> &B = buckets[d];
            for (size_t qi = 0; qi < B.size(); ++qi) {
                int st = B[qi];
                if (uStamp[st] != dlaRun || uDist[st] != d) continue;
                if (d >= qk) continue;
                int mi = st >> 1; int su = signVal(st & 1);
                const vector<int> &as = adjArcs[mi];
                for (size_t ai = 0; ai < as.size(); ++ai) {
                    int e = as[ai];
                    if (exArc[e]) continue;
                    int nst = 2 * arcTo[e] + signIdx(su * arcSg[e]);

                    int nd = d + 1;
                    if (uStamp[nst] != dlaRun || uDist[nst] > nd) {
                        uStamp[nst] = dlaRun; uDist[nst] = nd;
                        uPred[nst] = st; uParc[nst] = e;
                        buckets[nd].push_back(nst);
                    }
                }
            }
            if (earlyTarget >= 0 && uStamp[earlyTarget] == dlaRun &&
                uDist[earlyTarget] <= d + 1) return;
        }
    }

    bool reconstructWalk(int st) {
        wkStates.clear(); wkArcs.clear();
        int cur = st, guard = 0;
        while (true) {
            wkStates.push_back(cur);
            int pr = uPred[cur];
            if (pr == -1) break;
            wkArcs.push_back(uParc[cur]);
            cur = pr;
            if (++guard > qk + 2) return false;
        }
        reverse(wkStates.begin(), wkStates.end());
        reverse(wkArcs.begin(), wkArcs.end());
        return true;
    }

    int firstRepeat(int &iOut) {
        int L = (int) wkStates.size();
        for (int j = 1; j < L; ++j) {
            int vj = wkStates[j] >> 1;
            for (int i = 0; i < j; ++i)
                if ((wkStates[i] >> 1) == vj) { iOut = i; return j; }
        }
        iOut = -1;
        return -1;
    }
};

int main(int argc, char *argv[]) {
    vector<string> pos;
    for (int i = 1; i < argc; ++i) pos.push_back(string(argv[i]));
    if (pos.size() < 2) {
        printf("Usage: %s <dataset_dir> <k>\n", argv[0]);
        printf("Example: %s ../../datasets/bitcoin 6\n", argv[0]);
        printf("Every query runs Hansen's full decomposition pipeline (steps a-e).\n");
        return 1;
    }
    string dataset_path = pos[0];
    int k = atoi(pos[1].c_str());

    string dir = dataset_path;
    if (dir.empty() || dir[dir.size() - 1] != '/') dir += "/";

    BSHansenBCT solver;
    solver.loadGraph(dataset_path);

    FILE *fin = fopen((dir + "query_exp1.txt").c_str(), "r");
    if (!fin) { cerr << "Error: Cannot open query file " << dir + "query_exp1.txt" << endl; return 1; }

    char resname[64];
    snprintf(resname, sizeof(resname), "BS_HANSEN_BCT_results_k%d.txt", k);
    FILE *fres = fopen((dir + resname).c_str(), "w");

    char line[256];
    int query_count = 0, true_count = 0, false_count = 0;

    clock_t start_time = clock();
    while (fgets(line, sizeof(line), fin)) {
        int s, t, sigma;
        if (sscanf(line, "%d,%d,%d", &s, &t, &sigma) != 3) continue;
        ++query_count;
        int r = solver.query(s, t, k, sigma);
        if (r == 1) ++true_count; else ++false_count;
        if (fres) fprintf(fres, "%d,%d,%d,%d\n", s, t, sigma, r);
        solver.commitQueryStats();
    }
    clock_t end_time = clock();
    double query_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;

    fclose(fin);
    if (fres) fclose(fres);

    double blocks_avg = query_count > 0
        ? (double) solver.stat_total_blocks / (double) query_count : 0.0;

    FILE *fout = fopen((dir + "BS_HANSEN_BCT_Exp1.txt").c_str(), "a");
    if (!fout) { cerr << "Error: Cannot create output file" << endl; return 1; }
    fprintf(fout, "\nBS-Hansen\n");
    fprintf(fout, "Dataset: %s\n", dataset_path.c_str());
    fprintf(fout, "Hop constraint k = %d (no time limit)\n", k);
    fprintf(fout, "\nSummary (Query_path: %squery_exp1.txt, k = %d):\n", dir.c_str(), k);
    fprintf(fout, "Total queries: %d\n", query_count);
    fprintf(fout, "TRUE: %d\n", true_count);
    fprintf(fout, "FALSE: %d\n", false_count);
    fprintf(fout, "Total query time: %.5lf seconds\n", query_time);
    fprintf(fout, "B&B instrumentation: queries reaching B&B = %d, (v,sign) B&B runs = %lld, total B&B nodes expanded = %lld, max nodes for a single (v,sign) = %lld, positive-circuit anomalies = %lld, blocks per query (avg/max) = %.4f/%lld, nontrivial blocks solved = %lld, worklist recomputations = %lld\n",
            solver.stat_bnb_queries, solver.stat_bnb_targets, solver.stat_bnb_nodes,
            solver.stat_bnb_nodes_max, solver.stat_pos_circuits,
            blocks_avg, solver.stat_blocks_max_query,
            solver.stat_nontrivial_solves, solver.stat_worklist_recomp);
    fprintf(fout, "Block structure: queries reaching decomposition = %lld, multi-block-SCC queries = %lld, block solves = %lld, largest SCC processed = %d, largest block processed = %d\n",
            solver.stat_decomposed_queries, solver.stat_multiblock_queries,
            solver.stat_block_solves, solver.stat_largest_scc, solver.stat_largest_block);
    fprintf(fout, "========================================\n\n");
    fclose(fout);

    printf("BS-Hansen-BCT done: %d queries, TRUE=%d FALSE=%d, time=%.5lf s\n",
           query_count, true_count, false_count, query_time);
    printf("B&B stats: queries reaching B&B = %d, (v,sign) B&B runs = %lld, total nodes = %lld, max nodes single (v,sign) = %lld, positive-circuit anomalies = %lld\n",
           solver.stat_bnb_queries, solver.stat_bnb_targets, solver.stat_bnb_nodes,
           solver.stat_bnb_nodes_max, solver.stat_pos_circuits);
    printf("Block stats: decomposed queries = %lld, multi-block-SCC queries = %lld, total blocks = %lld, max blocks single query = %lld, block solves = %lld, nontrivial block solves = %lld, worklist recomputations = %lld, largest SCC = %d, largest block = %d\n",
           solver.stat_decomposed_queries, solver.stat_multiblock_queries,
           solver.stat_total_blocks, solver.stat_blocks_max_query,
           solver.stat_block_solves, solver.stat_nontrivial_solves,
           solver.stat_worklist_recomp, solver.stat_largest_scc, solver.stat_largest_block);
    return 0;
}
