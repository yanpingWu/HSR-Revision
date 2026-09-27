#pragma once

#include<iostream>
#include<cstring>
#include<string>
#include<stack>
#include<vector>
#include<set>
#include<queue>
#include<unordered_map> 
#include<unordered_set> 

#define MAX 128

using namespace std;

typedef set<int> IntSet;
typedef set<char> CharSet;

struct edge{
    int toNode;
    int label;
};

struct NfaState
{
	int index;
	char input;
	int chTrans;
	IntSet epTrans;
};

struct NFA
{
	NfaState *head;
	NfaState *tail;
};

struct Edge
{
	char input;
	int Trans;
};

struct DfaState
{
	bool isEnd;
	int index;
	IntSet closure;
	int edgeNum;
	Edge Edges[10];
};

struct DFA
{
	int startState;
	set<int> endStates;
	set<char> terminator;
	int trans[MAX][26];	
};

struct stateSet	
{
	int index;
	IntSet s;
};

struct FinalDFA
{
    int startState;
    set<int> endStates;
	int stateNum;
    set<int> labelSet;
    vector<unordered_map<int,int>> labelToState;
	vector<unordered_map<int,unordered_set<int> > > Back_labelToState;
	unordered_map<int,unordered_set<int>> LabelArriveState;
};

class TreeNode{
public:
	bool start;
	int value;//node id
	int label;//edge label
	bool finish;//end or not
	unordered_map<int,unordered_map<int,TreeNode*> > nextnode;
	TreeNode(int _value){
		value = _value;
		finish = false;
		start = false;
	}
	TreeNode(int _value,int _label){
		value = _value;
		label = _label;
		finish = false;
		start = false;
	}
};