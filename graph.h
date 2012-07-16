#ifndef GRAPH_H

#include "nauty.h"

//Represents when there is no connection
//graph.c adds stuff to infinity (crazy, I know),
//So make sure it won't overflow
#define GRAPH_INFINITY ((int)1000000)

typedef struct {
	int *distances; //n-by-n matrix of distances, INFINITY for no connection
	int n; //number of vertices
} problem_graph;

problem_graph nauty_to_problem(graph *g, int n);
int minimum_total_distance(problem_graph g);

#define GRAPH_H
#endif //GRAPH_H
