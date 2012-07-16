#include "graph.h"
#include <stdlib.h>
#include <math.h>

problem_graph nauty_to_problem(graph *g, int n) {
	problem_graph out;
	int m = ceil((float)n/WORDSIZE); //# of setword's per set
	out.n = n;
	out.distances = malloc(n*n*sizeof(*(out.distances)));
	for (int i = 0; i < n; i++) {
		for (int j = 0; j < n; j++) {
			if (i == j) {
				out.distances[n*i + j] = 0;
			}
			else {
				if(ISELEMENT(g + m*i, j)) {
					out.distances[n*i + j] = 1;
				}
				else {
					out.distances[n*i + j] = GRAPH_INFINITY;
				}
			}
		}
	}
	return out;
}

int sum_total_distances(problem_graph g) {
	int ret = 0;
	for (int i = 0; i < g.n; i++)
		for (int j = 0; j < g.n; j++)
			ret += g.distances[g.n*i + j];
	return ret;
}

void solve_floyd_warshall(problem_graph g) {
	for (int k = 0; k < g.n; k++) {
		for (int i = 0; i < g.n; i++) {
			for (int j = 0; j < g.n; j++) {
				int dist = g.distances[g.n*i + k] + g.distances[g.n*k + j];
				if(dist < g.distances[g.n*i + j]) {
					g.distances[g.n*i + j] = dist;
				}
			}
		}
	}
}

int minimum_total_distance(problem_graph g) {
	solve_floyd_warshall(g);
	return sum_total_distances(g);
}