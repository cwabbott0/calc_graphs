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

void nauty_to_problem(graph *nauty_graph, problem_graph graph);
int get_nauty_graph_size(int n);
int minimum_total_distance(problem_graph g);

#define GRAPH_H
#endif //GRAPH_H
