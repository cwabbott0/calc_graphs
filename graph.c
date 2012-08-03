#include "graph.h"
#include <stdlib.h>
#include <math.h>

void nauty_to_problem(graph *g, problem_graph graph) {
	for (int i = 0; i < graph.n; i++) {
		for (int j = 0; j < graph.n; j++) {
			if (i == j) {
				graph.distances[graph.n*i + j] = 0;
			}
			else {
				if(ISELEMENT(g + i, j)) {
					graph.distances[graph.n*i + j] = 1;
				}
				else {
					graph.distances[graph.n*i + j] = GRAPH_INFINITY;
				}
			}
		}
	}
}

// gets the size of a graph, in setwords, in nauty format given n
int get_nauty_graph_size(int n)
{
	return n;
}

// gets the degree and number of links simultaneously
void get_nauty_degree_num_links(graph *g, int n, int *degree, int *num_links)
{
	*degree = 0;
	*num_links = 0;
	for (int i = 0; i < n; i++)
	{
		int node_degree = POPCOUNT(g[i]);
		if (node_degree > *degree)
			*degree = node_degree;
		*num_links += node_degree;
	}
	
	//We counted each link twice, so divide by 2
	*num_links /= 2;
}

int sum_total_distances(problem_graph g) {
	int ret = 0;
	for (int i = 0; i < g.n; i++)
		for (int j = i + 1; j < g.n; j++)
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