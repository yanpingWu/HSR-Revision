#include "lvo_i.h"

void test_query_online(const char *dataset_path, int k_value, int query_count) {
	string path = dataset_path;
	LVO_I r(path);

	int n0 = 0, n_1 = 0, n1 = 0, n_2 = 0, n2 = 0, n_3 = 0, n3 = 0;
	int N = query_count;
	FILE *finn = fopen( (path + "/query_exp1.txt").c_str(), "r" );

	// exact 
	vector<vector<int> > exact_queries;

	clock_t query_start_time = clock();

	for(int i = 0; i < N; ++i) {
		// int a = rand() % n, b = rand() % n;
		// bool sign = rand() % 2;

		int a, b;
		int sign;

		if (fscanf(finn, "%d,%d,%d", &a, &b, &sign) != 3) {
			printf("Error reading data from file.\n");
			fclose(finn);
			return;
		}

		int rst = r.query_reach_online(a,b,sign,k_value);

		if(rst == -1) ++n_1;
		else if(rst == 1) ++n1;
		else if(rst == -2) ++n_2;
		else if(rst == 2) ++n2;
		else if(rst == 3) ++n3;
		else if(rst == -3) ++n_3;
		else ++n0;

		if(rst == 3 || rst == -3) {
			//exact
			vector<int> qabs;
			qabs.push_back(a); qabs.push_back(b); qabs.push_back(sign);
			exact_queries.push_back(qabs);
		}

	}
	fclose(finn);
	finn = NULL;

	clock_t query_end_time = clock();
	double total_query_time = (double)(query_end_time - query_start_time) / CLOCKS_PER_SEC;
	double total_exact_time = (double) r.exact_total_ticks / CLOCKS_PER_SEC;

	for(int i = 0; i < (int) exact_queries.size(); ++i)
		printf("a = %d, b = %d, sign = %d\n", exact_queries[i][0], exact_queries[i][1], exact_queries[i][2]);

	FILE *online_out = fopen((path + "/LVO_I_Exp1.txt").c_str(), "a");
	if (online_out != NULL) {
		fprintf(online_out, "Algorithm: LVO-I\n");
		fprintf(online_out, "Query file: %s/query_exp1.txt\n", path.c_str());
		fprintf(online_out, "k=%d;total=%d\n", k_value, N);
		// fprintf(online_out, "Total No=%d,Total Yes=%d,Path No=%d,Path Yes=%d,DP No=%d,DP Yes=%d,Online No=%d,Online Yes=%d,Unknown=%d\n",
		// 		n_1+n_2+n_3, n1+n2+n3, n_1, n1, n_2, n2, n_3, n3, n0);
		fprintf(online_out, "Total No=%d,Total Yes=%d,Invoke Exact=%d\n",
				n_1+n_2+n_3, n1+n2+n3, n_3+n3);
		fprintf(online_out, "Total query time: %.5lf seconds\n", total_query_time);
		fprintf(online_out, "Total exact algorithm time: %.5lf seconds (%d / %d queries used exact)\n\n",
				total_exact_time, r.exact_query_count, N);
		fclose(online_out);
	} else {
		perror("Failed to open LVO_I_Exp1.txt");
	}
}

int main(int argc, char *argv[]) {
	// printf( "argc=%d\n", argc );
	// for( int i = 0; i < argc; ++i )
	// 	printf( "argv[%d]=%s\n", i, argv[i] );

	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
	clock_t t = clock();

	const char *algorithm_type;
	const char *dataset_path;
	int k_value;
	int query_count;

	if(argc >= 5) {
		dataset_path = argv[1];
		algorithm_type = argv[2];
		k_value = atoi(argv[3]);
		query_count = atoi(argv[4]);
	} else if(argc > 1) {
		printf("Usage: %s <dataset_path> <algorithm_type> <k_value> <query_count>\n", argv[0]);
		printf("dataset_path: path to dataset folder\n");
		printf("algorithm_type: online\n");
		printf("k_value: integer, default 15\n");
		printf("query_count: integer, default 10000\n");
		return 0;
	}

	if(strcmp(algorithm_type, "online") == 0) {
		test_query_online(dataset_path, k_value, query_count);
	}

	// t = clock() - t;
	// printf( "Total time=%0.3lf seconds\n", t*1.0/CLOCKS_PER_SEC);

	return 0;
}
