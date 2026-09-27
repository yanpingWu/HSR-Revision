#ifndef SignedKReachability_H_
#define SignedKReachability_H_

#include <string>
#include <vector>
#include <cstring>
#include <climits>
#include <algorithm>
#include <utility>
#include <cstdio>
#include <cmath>
#include <set>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <chrono>

#define MAXST 1024
#define MAXN 1000000000
#define MAXD 128

#define ID(X) ((X)>=0?(X):(X)+MAXN)
#define SIGN(X) ((X)>=0?1:0)
#define CMB(ID,SIGN) ((SIGN)==1?(ID):(ID-MAXN))

using namespace std;

void set_insert(vector<int> &x, int v) {
	int p = x.size()-1;
	x.push_back(v);
	for(; p >= 0 && x[p] > v; --p) x[p+1] = x[p];
	x[p+1] = v;
}

template <class InputIterator1, class InputIterator2, class OutputIterator>
OutputIterator set_intersect(InputIterator1 first1, InputIterator1 last1,  InputIterator2 first2,
		InputIterator2 last2, int val, OutputIterator result) {
	while (first1 != last1 && first2 != last2) {
		if(*first1 == val) {
			*result = val; ++result; ++first1;
		} else if (*first1 < *first2) ++first1;
		else if (*first2 < *first1) ++first2;
		else {
			*result = *first1;
			++result; ++first1; ++first2;
		}
	}
	return result;
}

class LabelNode {
public:
	int id;
	int pre;
	short val;
	short pos;
public:
	LabelNode();
	LabelNode(int id, int val, int pre);
};

LabelNode::LabelNode() {
	id = -1; val = 0;
	pre = -1; pos = -1;
}

LabelNode::LabelNode(int id, int val, int pre) {
	this->id = id; this->val = val;
	this->pre = pre; this->pos = -1;
}

class HQA {
public:
	vector<vector<int> > nbr_in, nbr_ot;
	int n, m;
	vector<int> id_org, id_new;
	clock_t index_done_tick;
	double index_memory_MB;

private:
	int get_num_cnt(string path);
	bool get_signed_edge(char *line, int &a, int &b, int &sign, int num_cnt);

private:
	// vector<vector<int> > cut[2];
	// vector<vector<int> > c[2][MAXD];

	vector<int> reach[2];
	vector<int> q[2];
	vector<int> q_used;

	vector<pair<int,int> > q_usd;
	vector<int> lst[2], usd[2], pre[2];

	vector<char> used;
	vector<int> l_used;

	vector<vector<pair<int,int> > > nb_in[2];

	vector<vector<LabelNode> > dis_in[2], dis_ot[2];

	int now_maxd;

private:
	void build_dis_label(int o, vector<vector<int> > &nbr, vector<vector<LabelNode> > *dis, vector<vector<LabelNode> > *_dis);
	int query_path(int u, int v, int sign, int maxd);
	int search_reach_online(int a, int w, int si_w, int dep, int maxd);

public:
	HQA(string path);
	void reorder();
	void build_dis_label();
	int query_reach_online(int a, int b, int sign, int maxd);
	int query_reach(int u, int v, int sign, int maxd);

};

void HQA::build_dis_label(int o, vector<vector<int> > &nbr, vector<vector<LabelNode> > *dis,
		vector<vector<LabelNode> > *_dis) {
	for(int si = 0; si < 2; ++si)
		for(int i = 0; i < (int) _dis[si][o].size(); ++i)
			lst[si][_dis[si][o][i].id] = _dis[si][o][i].val;

	q_usd.clear(); q_usd.push_back(make_pair(1,o)); usd[1][o] = 0; pre[1][o] = o;

	for(int s = 0, d = 1; s < (int) q_usd.size(); ++d) {
		int t = (int) q_usd.size();
		for(;s<t;++s) {
			int si_u = q_usd[s].first, u = q_usd[s].second;
			for(int i = 0; i < nbr[u].size(); ++i) {
				int v = ID(nbr[u][i]), si_uv = SIGN(nbr[u][i]);
				int si_v = 1-(si_u^si_uv);
				if(v<=o) continue;
				if(usd[si_v][v] == -1) {
					bool r = false;
					for(int si = 0; si < 2; ++si) {
						int _si_v = (1-si_v)^si;
						for(int i = 0; i < (int) dis[_si_v][v].size(); ++i) {
							int id = dis[_si_v][v][i].id, val = dis[_si_v][v][i].val;
							if(lst[si][id] != -1 && lst[si][id]+val <= d) {r = true; break;}
						}
					}
					if(r) continue;
					usd[si_v][v] = d;
					pre[si_v][v] = CMB(u,si_u);
					q_usd.push_back(make_pair(si_v, v));
				}
			}
		}
	}

	for(int i = 0; i < (int) q_usd.size(); ++i) {
		int si = q_usd[i].first, id = q_usd[i].second;
		dis[si][id].push_back(LabelNode(o,usd[si][id],pre[si][id]));
		usd[si][id] = -1;
	}
	for(int i = 0; i < (int) q_usd.size(); ++i) {
		int si = q_usd[i].first, id = q_usd[i].second;
		int p = dis[si][id].size()-1, si_p = SIGN(pre[si][id]), id_p = ID(pre[si][id]);
		dis[si][id][p].pos = dis[si_p][id_p].size()-1;
	}

	for(int si = 0; si < 2; ++si)
		for(int i = 0; i < (int) _dis[si][o].size(); ++i)
			lst[si][_dis[si][o][i].id] = -1;
}

void HQA::build_dis_label() {
	for(int i = 0; i < 2; ++i) {
		dis_in[i].resize(n);
		dis_ot[i].resize(n);
	}

	for(int o = 0; o < n; ++o) {
		build_dis_label(o, nbr_in, dis_ot, dis_in);
		build_dis_label(o, nbr_ot, dis_in, dis_ot);
	}

	index_done_tick = clock();   // 

	double t_in = 0, t_ot = 0;
	size_t max_label = 0;
	for(int si = 0; si < 2; ++si)
		for(int i = 0; i < n; ++i) {
			t_in += dis_in[si][i].size();
			t_ot += dis_ot[si][i].size();
			if(dis_in[si][i].size() > max_label) max_label = dis_in[si][i].size();
			if(dis_ot[si][i].size() > max_label) max_label = dis_ot[si][i].size();
		}

	// printf( "Average dis_in_label size = %0.2lf, average dis_ot_label size = %0.2lf\n", t_in/n, t_ot/n );
	size_t t_in_bytes = 0, t_ot_bytes = 0; 
	t_in_bytes = t_in * sizeof(LabelNode);
	t_ot_bytes = t_ot * sizeof(LabelNode);
	size_t total_bytes = t_in_bytes + t_ot_bytes;

	double t_in_MB = t_in_bytes / (1024.0 * 1024.0);
	double t_ot_MB = t_ot_bytes / (1024.0 * 1024.0);
	double total_MB = total_bytes / (1024.0 * 1024.0);

	index_memory_MB = total_MB;

	cout << "Total memory usage = " << total_MB << " MB" << endl; // (dis_in + dis_ot)
	cout << "Max label list size = " << max_label << (max_label > 32767 ? "  *** WARNING: > 32767, short(pos) will overflow! ***" : "  (OK, within short range)") << endl; 
}

int HQA::query_path(int u, int v, int sign, int maxd) {
	bool reach = false;

	for(int sign_v = 0; sign_v < 2; ++sign_v ) {
		int sign_u = (1-sign)^sign_v;
		for(int pu = 0, pv = 0; pu < (int)dis_ot[sign_u][u].size() && pv < (int)dis_in[sign_v][v].size(); )
			if(dis_ot[sign_u][u][pu].id < dis_in[sign_v][v][pv].id) ++pu;
			else if(dis_ot[sign_u][u][pu].id > dis_in[sign_v][v][pv].id) ++pv;
			else {
				if(dis_ot[sign_u][u][pu].val + dis_in[sign_v][v][pv].val <= maxd) {
					reach = true;
					l_used.clear();
					bool now_dup = false;
					int id = dis_ot[sign_u][u][pu].id;
					used[id] = true;
					l_used.push_back(id);

					for(int si = sign_u, w = u, p = pu; w != id;){
						if(used[w]) {now_dup = true; break;}
						used[w] = true;
						l_used.push_back(w);
						int p_tmp = dis_ot[si][w][p].pos;
						int si_tmp = SIGN(dis_ot[si][w][p].pre);
						w = ID(dis_ot[si][w][p].pre);
						si = si_tmp;
						p = p_tmp;
					}

					if(!now_dup) {
						for(int si = sign_v, w = v, p = pv; w != id;){
							if(used[w]) {now_dup = true; break;}
							used[w] = true;
							l_used.push_back(w);
							int p_tmp = dis_in[si][w][p].pos;
							int si_tmp = SIGN(dis_in[si][w][p].pre);
							w = ID(dis_in[si][w][p].pre);
							si = si_tmp;
							p = p_tmp;
						}
					}

					for(int i = 0; i < (int) l_used.size(); ++i) used[l_used[i]] = false;
					if(!now_dup) return 1;
				}
				++pu; ++pv;
			}
	}

	return reach?0:-1;
}



int HQA::query_reach(int a, int b, int sign, int maxd) {
	int u = id_new[a], v = id_new[b];
	int rst = query_path(u,v,sign,maxd);
	if(rst == 0) rst = query_reach_online(a,b,sign,maxd);
	return rst;
}

void HQA::reorder() {
	vector<pair<int,int> > ord(n);
	for(int u = 0; u < n; ++u) ord[u] = make_pair(-(nbr_in[u].size()*nbr_ot[u].size()),u);
	sort(ord.begin(),ord.end());
	for(int u = 0; u < n; ++u) {
		id_org[u] = ord[u].second;
		id_new[ord[u].second] = u;
	}

	vector<vector<int> > nbr_in_new(n), nbr_ot_new(n);

	for(int u = 0; u < n; ++u) {
		for(int i = 0; i < (int) nbr_in[u].size(); ++i) {
			int v = ID(nbr_in[u][i]), sign_v = SIGN(nbr_in[u][i]);
			nbr_in[u][i] = CMB(id_new[v],sign_v);
		}
		for(int i = 0; i < (int) nbr_ot[u].size(); ++i) {
			int v = ID(nbr_ot[u][i]), sign_v = SIGN(nbr_ot[u][i]);
			nbr_ot[u][i] = CMB(id_new[v],sign_v);
		}
		sort(nbr_in[u].begin(),nbr_in[u].end());
		sort(nbr_ot[u].begin(),nbr_ot[u].end());
		nbr_in_new[id_new[u]] = nbr_in[u];
		nbr_ot_new[id_new[u]] = nbr_ot[u];
	}

	nbr_in = nbr_in_new;
	nbr_ot = nbr_ot_new;
}

HQA::HQA(string path) {
	n = 0;
	if(path[path.size()-1] != '/') path = path + "/";
	FILE *fin = fopen((path + "graph.txt").c_str(), "r" );
	char line[MAXST];
	int a, b, sign, num_cnt = get_num_cnt(path), cnt = 0;
	vector<pair<pair<int,int>,int> > el;

	printf("Loading text, path = %s, num_cnt = %d...\n", path.c_str(), num_cnt);
	while(fgets(line, MAXST, fin)) {
		if(!get_signed_edge(line, a, b, sign, num_cnt)) continue;
		if(a == b) continue;
		el.push_back(make_pair(make_pair(a, b),sign));
		n = max(n, a+1);
		n = max(n, b+1);
		if((++cnt) % 10000000 == 0) printf("%d lines finished\n", cnt);
	}

	nbr_in.resize(n);
	nbr_ot.resize(n);

	for(int i = 0; i < (int) el.size(); ++i) {
		int a = el[i].first.first, b = el[i].first.second, sign = el[i].second;
		sign = (sign>0?1:0);
		nbr_ot[a].push_back(CMB(b,sign));
		nbr_in[b].push_back(CMB(a,sign));
	}

	m = 0;
	for(int u = 0; u < n; ++u) {
		sort(nbr_ot[u].begin(), nbr_ot[u].end());
		sort(nbr_in[u].begin(), nbr_in[u].end());

		if(nbr_ot[u].size() > 0) {
			int p = 1;
			for(int j = 1; j < (int) nbr_ot[u].size(); ++j)
				if(nbr_ot[u][j-1] != nbr_ot[u][j]) nbr_ot[u][p++] = nbr_ot[u][j];
			nbr_ot[u].resize(p);
			m += p;
		}

		if(nbr_in[u].size() > 0) {
			int p = 1;
			for(int j = 1; j < (int) nbr_in[u].size(); ++j)
				if(nbr_in[u][j-1] != nbr_in[u][j]) nbr_in[u][p++] = nbr_in[u][j];
			nbr_in[u].resize(p);
		}
	}

	for(int i = 0; i < 2; ++i) {
		// cut[i].resize(n);
		reach[i].resize(n,-1);
	}

	id_org.resize(n);
	id_new.resize(n);

	for(int i = 0; i < 2; ++i) {
		lst[i].resize(n,-1);
		usd[i].resize(n,-1);
		pre[i].resize(n);
		nb_in[i].resize(n);
	}

	for(int i = 0; i < n; ++i) {
		id_org[i] = i;
		id_new[i] = i;
	}

	used.resize(n,false);

	index_done_tick = 0;
	index_memory_MB = 0;

	// printf( "Graph loaded, n = %d, m = %d, cnt = %d\n", n, m, cnt );

	fclose(fin);
}

int HQA::search_reach_online(int a, int w, int si_w, int dep, int maxd) {
	if(a == w) return 3;
	int rst = -3, nowd = min(maxd - dep, now_maxd);
	used[w] = true;
	for(int i = 0; i < (int) nb_in[si_w][w].size(); ++i) {
		int v = ID(nb_in[si_w][w][i].second), si_wv = SIGN(nb_in[si_w][w][i].second);
		if(dep == maxd - 1 && v != a) continue;
		if(nb_in[si_w][w][i].first + dep >= maxd) continue;
		if(used[v]) continue;
		int si_v = 1-(si_w^si_wv);
		// bool find_dup = false;
		// for(int j = 0; j < (int) c[si_v][nowd][v].size() && !find_dup; ++j)
		// 	if(used[c[si_v][nowd][v][j]]) find_dup = true;
		// if(find_dup) continue;
		if(search_reach_online(a,v,si_v,dep+1,maxd) == 3) {rst = 3; break;}
	}
	used[w] = false;
	return rst;
}

int HQA::query_reach_online(int a, int b, int sign, int maxd) {
	if(a==b) return sign == 1 ? 2 : -2;
	if(sign == -1) sign = 0;
	a = id_new[a]; b = id_new[b];
	q[0].clear(); q[1].clear();
	q[0].push_back(a);
	q_used.clear();
	q_used.push_back(a);
	reach[1][a] = 0;
	pre[1][a] = a;
	// c[0][0].resize(n); c[1][0].resize(n);

	bool succ = false, touch = false;
	now_maxd = 0;

	for(int pr = 0, nxt = 1, d = 0; q[pr].size() && !succ && d < maxd; pr = nxt, nxt = 1-nxt, q[nxt].clear(), ++d, ++now_maxd) {
		for(int i = 0; i < (int) q[pr].size() && !succ; ++i) {
			int u = ID(q[pr][i]), sign_u = SIGN(q[pr][i]);
			for(int j = 0; j < (int) nbr_ot[u].size(); ++j) {
				int v = ID(nbr_ot[u][j]), sign_uv = SIGN(nbr_ot[u][j]);
				int sign_v = 1-(sign_u^sign_uv);
				if(v == a) continue;

				// if(binary_search(c[sign_u][d][u].begin(), c[sign_u][d][u].end(), v)) continue;

				if(reach[sign_v][v] == -1) {
					if(v == b && sign_v == sign) {
						l_used.clear();
						used[b] = true;
						l_used.push_back(b);
						bool now_dup = false;
						for(int si = sign_u, w = u; w != a;){
							if(used[w]) {now_dup = true; break;}
							used[w] = true;
							l_used.push_back(w);
							int tmp = pre[si][w];
							si = SIGN(tmp);
							w = ID(tmp);
						}
						for(int k = 0; k < (int) l_used.size(); ++k) used[l_used[k]] = false;
						touch = true;
						if(!now_dup) {succ = true; break;}
					} else if(v == b) continue;
					reach[sign_v][v] = d+1;
					q_used.push_back(CMB(v,sign_v));
					// cut[sign_v][v] = c[sign_u][d][u];
					// set_insert(cut[sign_v][v], v);
					q[nxt].push_back(CMB(v,sign_v));
					pre[sign_v][v] = CMB(u,sign_u);
				} else {
					// int len_pre = (int) cut[sign_v][v].size();
					// int len = set_intersect(cut[sign_v][v].begin(),cut[sign_v][v].end(),
					// 		c[sign_u][d][u].begin(),c[sign_u][d][u].end(), v, cut[sign_v][v].begin()) - cut[sign_v][v].begin();
					// cut[sign_v][v].resize(len);
					// if(len != len_pre) {
					// 	q[nxt].push_back(CMB(v,sign_v));
					// }
				}
			}
		}

		// c[0][d+1] = cut[0]; c[1][d+1] = cut[1];
	}

	int rst = 0;
	if(succ) rst = 2;
	else if(!touch) rst = -2;
	else {
		for(int si = 0; si < 2; ++si)
			for(int u = 0; u < n; ++u) {
				nb_in[si][u].clear();
				for(int i = 0; i < (int) nbr_in[u].size(); ++i) {
					int v = ID(nbr_in[u][i]), si_uv = SIGN(nbr_in[u][i]);
					int si_v = 1-(si^si_uv);
					if(reach[si_v][v] == -1) continue;
					nb_in[si][u].push_back(make_pair(reach[si_v][v], nbr_in[u][i]));
				}
				sort(nb_in[si][u].begin(), nb_in[si][u].end());
			}
		if(maxd <= 10)
			rst = search_reach_online(a,b,sign,0,maxd);
		else for(int d = 10; d <= maxd; ++d)
			if(search_reach_online(a,b,sign,0,d) == 3) {rst = 3; break;}
	}

	for(int i = 0; i < (int) q_used.size(); ++i) {
		int v = ID(q_used[i]), sign_v = SIGN(q_used[i]);
		reach[sign_v][v] = -1;
		// cut[sign_v][v].clear();
	}

	return rst;
}

bool HQA::get_signed_edge(char *line, int &a, int &b, int &sign, int num_cnt) {
	if( !isdigit(line[0]) ) return false;
	vector<char*> v_num;
	int len = (int) strlen(line);
	for( int i = 0; i < len; ++i )
		if( !isdigit(line[i]) && line[i] != '.' && line[i] != '-') line[i] = '\0';
		else if(i == 0 || !line[i-1]) v_num.push_back(line+i);
	if( (int) v_num.size() != num_cnt ) return false;
	sscanf( v_num[0], "%d", &a );
	sscanf( v_num[1], "%d", &b );
	sscanf( v_num[2], "%d", &sign );
	return true;
}

int HQA::get_num_cnt(string path) {
	FILE *fin = fopen( (path + "graph.txt").c_str(), "r" );
	char line[MAXST];
	int cnt = 0, min_cnt = 100;

	while(fgets( line, MAXST, fin ) && cnt < 10) {
		if(!isdigit(line[0])) continue;
		vector<char*> v_num;
		int len = (int) strlen(line);
		for(int i = 0; i < len; ++i)
			if(!isdigit(line[i]) && line[i] != '.' && line[i] != '-') line[i] = '\0';
			else if(i == 0 || !line[i-1]) v_num.push_back(line+i);
		if( (int) v_num.size() < 2 ) continue;
		min_cnt = min(min_cnt, (int) v_num.size());
		++cnt;
	}
	fclose(fin);
	return min_cnt;
}

#endif /* SignedKReachability_H_ */


