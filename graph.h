#ifndef GRAPH_H

//Note: we make the same assumptions as geng about graph size

#ifndef MAXN
#define MAXN 32
#endif

#if MAXN > 32
#error "Can't have MAXN greater than 32"
#endif

#define ONE_WORD_SETS

#include "nauty.h"

//Represents when there is no connection
//graph.c adds stuff to infinity (crazy, I know),
//So make sure it won't overflow
#define GRAPH_INFINITY ((int)1000000)

typedef struct {
	int *distances; //n-by-n matrix of distances, INFINITY for no connection
	int n; //number of vertices
} problem_graph;

void nauty_to_problem(graph *nauty_graph, problem_graph graph);
int get_nauty_graph_size(int n);
int minimum_total_distance(problem_graph g);

#define GRAPH_H
#endif //GRAPH_H
