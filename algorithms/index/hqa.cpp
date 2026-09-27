#include "hqa.h"

void test_query_by_index(const char *dataset_path, int k_value, int query_count) {
	
	string path = dataset_path;
	HQA r(path);

	clock_t ct = clock();
	
	r.reorder();
	r.build_dis_label();

	double indexing_time = (double)(r.index_done_tick - ct) / CLOCKS_PER_SEC;
	printf( "Indexing time = %0.3lf secs\n", indexing_time);

	ct = clock();

	int n0 = 0, n_1 = 0, n1 = 0, n_2 = 0, n2 = 0, n_3 = 0, n3 = 0;
	int N = query_count;

	FILE *finn = fopen( (path + "/query_exp1.txt").c_str(), "r" );


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


		int rst = r.query_reach(a,b,sign,k_value);

		if(rst == -1) ++n_1;
		else if(rst == 1) ++n1;
		else if(rst == -2) ++n_2;
		else if(rst == 2) ++n2;
		else if(rst == 3) ++n3;
		else if(rst == -3) ++n_3;
		else ++n0;
	}
	fclose(finn);
	finn = NULL;

	double total_query_time = (double)(clock()-ct) / CLOCKS_PER_SEC;
	printf( "Querying time = %0.5lf secs\n", total_query_time);

	// output in Index_I_Exp1.txt 
	FILE *index_out = fopen((path + "/Index_I_Exp1.txt").c_str(), "a");
	if (index_out != NULL) {
		fprintf(index_out, "Algorithm: Index-I\n");
		fprintf(index_out, "Query file: %s/query_exp1.txt\n", path.c_str());
		fprintf(index_out, "k=%d;total=%d\n", k_value, N);
		// fprintf(index_out, "Total No=%d,Total Yes=%d,Path No=%d,Path Yes=%d,DP No=%d,DP Yes=%d,Online No=%d,Online Yes=%d,Unknown=%d\n",
		// 		n_1+n_2+n_3, n1+n2+n3, n_1, n1, n_2, n2, n_3, n3, n0);
		fprintf(index_out, "Total No=%d,Total Yes=%d,Invoke online=%d\n",
				n_1+n_2+n_3, n1+n2+n3, n_2+n2+n_3+n3);
		fprintf(index_out, "Indexing time: %.5lf seconds\n", indexing_time);
		fprintf(index_out, "Total query time: %.5lf seconds\n", total_query_time);
		fprintf(index_out, "Index memory usage: %.2lf MB\n\n", r.index_memory_MB);
		fclose(index_out);
	} else {
		perror("Failed to open Index_I_Exp1.txt");
	}
}

void test_query_online(const char *dataset_path, int k_value, int query_count) {
	clock_t ct = clock();
	string path = dataset_path;
	HQA r(path);

	ct = clock();

	int n = r.n, n0 = 0, n_1 = 0, n1 = 0, n_2 = 0, n2 = 0, n_3 = 0, n3 = 0;
	int N = query_count;
	FILE *finn = fopen( (path + "/query.txt").c_str(), "r" ); 

	
	for(int i = 0; i < N; ++i) {
		// int a = rand() % n, b = rand() % n;
		// bool sign = rand() % 2;

		int a, b;
		int sign;

		if (fscanf(finn, "%d,%d,%d", &a, &b, &sign) != 3) {
			printf("Error reading data from file.\n");
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
	}
	fclose(finn);
	finn = NULL;

	printf( "Total No=%d,Total Yes=%d,Path No=%d,Path Yes=%d,DP No=%d,DP Yes=%d,Online No=%d,Online Yes=%d,Unknown=%d\n",
			n_1+n_2+n_3, n1+n2+n3, n_1, n1, n_2, n2, n_3, n3, n0);


	printf( "Querying time = %0.5lf secs\n", (clock()-ct)*1.0/CLOCKS_PER_SEC);
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
		printf("algorithm_type: index or online\n");
		printf("k_value: integer, default 15\n");
		printf("query_count: integer, default 10000\n");
		return 0;
	}

	if(strcmp(algorithm_type, "index") == 0) {
		test_query_by_index(dataset_path, k_value, query_count);
	} else if(strcmp(algorithm_type, "online") == 0) {
		test_query_online(dataset_path, k_value, query_count);
	}

	// t = clock() - t;
	// printf( "Total time=%0.3lf seconds\n", t*1.0/CLOCKS_PER_SEC);

	return 0;
}
