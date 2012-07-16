#include <stdio.h>
#include <mpi.h>
#include "graph.h"

#define GENERATOR_OUTPUT_TAG 1
#define GENERATOR_DONE_TAG 2
#define GENERATOR_KILL_TAG 3
#define GENERATOR_INPUT_TAG 4
#define WORKER_OUTPUT_TAG 5
#define WORKER_INPUT_TAG 6
#define WORKER_KILL_TAG 7
#define WORKER_NUM_VERTICES_TAG 8

typedef struct _queue_element {
	problem_graph graph;
	struct _queue_element *next;
} queue_element;

typedef struct {
	queue_element *first, *last;
} queue;

void push_element(queue_element *element, queue *my_queue) {
	//printf("pushing %p to queue\n", element);
	if(my_queue->first == NULL) {
		my_queue->first = my_queue->last = element;
	}
	else {
		my_queue->first->next = element;
		my_queue->first = element;
	}
}

queue_element *pop_element(queue *my_queue) {
	if (my_queue->last == NULL) {
		//printf("popping from empty queue\n");
		return NULL;
	}
	queue_element *ret = my_queue->last;
	my_queue->last = my_queue->last->next;
	if(my_queue->last == NULL)
		my_queue->first = NULL;
	//printf("popping %p from queue\n", ret);
	return ret;
}

void empty_queue(queue *my_queue) {
	queue_element *element = my_queue->last;
	while (element != NULL) {
		queue_element *temp = element;
		element = element->next;
		free(temp->graph.distances);
		free(temp);
	}
}

int geng(int argc, char *argv[]); //entry point for geng

//Callback when geng finds a graph
//Name defined in the Makefile
//Sends the data to the master using the GENERATOR_OUTPUT_TAG
void geng_callback(FILE *file, graph *g, int n)
{
	//printf("Generator: got graph\n");
	problem_graph graph = nauty_to_problem(g, n);
	MPI_Send(graph.distances,
		 n*n,
		 MPI_INT,
		 0,
		 GENERATOR_OUTPUT_TAG,
		 MPI_COMM_WORLD);
	free(graph.distances);
}

//Wrapper around the geng entry function
//n is the number of vertices, m is the number of edges
int call_geng(unsigned int n, unsigned int m)
{
	char n_buf[10], m_buf[10];
	char *geng_args[] = {
		"geng",
		"-ucq",
		"",
		""
	};
	sprintf(n_buf, "%d", n);
	sprintf(m_buf, "%d", m);
	geng_args[2] = n_buf;
	geng_args[3] = m_buf;
	return geng(4, geng_args);
}

void generator()
{
	int num_verts_and_edges[2], geng_result;
	MPI_Status status;
	while(1) {
		MPI_Probe(
			 MPI_ANY_SOURCE,
			 MPI_ANY_TAG,
			 MPI_COMM_WORLD,
			 &status);
		
		if(status.MPI_TAG == GENERATOR_KILL_TAG) {
			//printf("Generator: got kill message, returning...\n");
			return;
		}
		
		MPI_Recv(
				 num_verts_and_edges,
				 2,
				 MPI_INT,
				 MPI_ANY_SOURCE,
				 MPI_ANY_TAG,
				 MPI_COMM_WORLD,
				 &status);
		//printf("Generator: got %d verts and %d edges\n",
			   //num_verts_and_edges[0], num_verts_and_edges[1]);
		
		if(geng_result = call_geng(num_verts_and_edges[0],
			num_verts_and_edges[1])) {
			fprintf(stderr, "geng failed: %d\n", geng_result);
			return;
		}
		//printf("Generator: done\n");
		MPI_Send(0, 0, MPI_INT, 0, GENERATOR_DONE_TAG,
			MPI_COMM_WORLD);
	}
}


//First, send the number of vertices and edges to the generator.
//When a graph arrives from the generator, push it on the queue
//And if workers are ready, empty the queue.
//When a worker finishes its task, try to get a task from the queue.
//If the queue is empty, mark it as ready and wait for the generator
//To provide it with input.
void master()
{
	MPI_Status status;
	int ntasks;
	int num_vertices = 11, num_edges = 14;
	int generator_done = 0, master_done = 0;
	queue_element *element;
	queue my_queue = {
		first: NULL,
		last: NULL
	};
	
	MPI_Comm_size(MPI_COMM_WORLD, &ntasks);
	problem_graph *workers = malloc((ntasks - 2) *
											 sizeof(problem_graph));
	
	{
		int min_result = (int) GRAPH_INFINITY;
		queue min_graphs = {
			first: NULL,
			last: NULL
		};
		
		for (int i = 0; i < ntasks - 2; i++) {
			workers[i].distances = NULL;
			workers[i].n = -1;
			MPI_Send(&num_vertices,
					 1,
					 MPI_INT,
					 2 + i,
					 WORKER_NUM_VERTICES_TAG,
					 MPI_COMM_WORLD);
		}
	
		int num_verts_and_edges[2] = {num_vertices, num_edges};
		//printf("Sending start message to generator...\n"); 
		MPI_Send(num_verts_and_edges,
				 2,
				 MPI_INT,
				 1,
				 GENERATOR_INPUT_TAG,
				 MPI_COMM_WORLD);
		
		master_done = 0;
		int result;
		while(!master_done) {
			//Wait for a message from the generator or one of the workers
			MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
			
			switch(status.MPI_TAG){
				case GENERATOR_OUTPUT_TAG:
					printf("Master: got graph from generator\n");
					//Get the graph and add it to the queue
					element = malloc(sizeof(queue_element));
					element->graph.distances = malloc(num_vertices * num_vertices * sizeof(int));
					element->graph.n = num_vertices;
					element->next = NULL;
					MPI_Recv(element->graph.distances,
							 num_vertices * num_vertices,
							 MPI_INT,
							 1,
							 GENERATOR_OUTPUT_TAG,
							 MPI_COMM_WORLD,
							 &status);
					
					/*for (int i = 0; i < num_vertices; i++) {
						for (int j = 0; j < num_vertices; j++) {
							printf("%d,", 
								element->graph.distances[num_vertices*i + j]);
						}
						printf("\n");
					}*/
				
					//printf("Master: recieved graph from generator\n");
					push_element(element, &my_queue);
					
					//Empty the queue as much as possible
					for (int i = 0; i < ntasks - 2; i++) {
						if (workers[i].n != -1)
							continue;
						element = pop_element(&my_queue);
						if(element == NULL)
							break; //queue is empty
						printf("Master: sending graph to task %d\n", i + 2);
						MPI_Send(element->graph.distances,
								 num_vertices*num_vertices,
								 MPI_INT,
								 i + 2,
								 WORKER_INPUT_TAG,
								 MPI_COMM_WORLD);
						workers[i] = element->graph;
						free(element);
					}
					break;
					
				case GENERATOR_DONE_TAG:
					MPI_Recv(0, 0, MPI_INT, 1, GENERATOR_DONE_TAG, MPI_COMM_WORLD,
							 &status);
					generator_done = 1;
					break;
					
				case WORKER_OUTPUT_TAG:
					MPI_Recv(&result,
							 1,
							 MPI_INT,
							 MPI_ANY_SOURCE,
							 WORKER_OUTPUT_TAG,
							 MPI_COMM_WORLD,
							 &status);
					
					printf("Master: got result from %d: %d\n", 
						   status.MPI_SOURCE, result);
					
					if (result <= min_result) {
						if (result < min_result) {
							min_result = result;
							empty_queue(&min_graphs);
						}
						queue_element *element = malloc(sizeof(queue_element));
						element->next = NULL;
						element->graph = workers[status.MPI_SOURCE - 2];
						push_element(element, &min_graphs);
					}
					else {
						free(workers[status.MPI_SOURCE - 2].distances);
					}
					workers[status.MPI_SOURCE - 2].n = -1;
					
					element = pop_element(&my_queue);
					if(element == NULL) {
						if (generator_done) {
							printf("Master: checking to see if done...\n");
							//Check to see if we're all done
							master_done = 1;
							for (int i = 0; i < ntasks - 2; i++) {
								if (workers[i].n != -1) {
									master_done = 0;
								}
							}
						}
						break;
					}
					
					printf("Master: sending new graph to %d\n", status.MPI_SOURCE);
					
					MPI_Send(element->graph.distances,
							 num_vertices*num_vertices,
							 MPI_INT,
							 status.MPI_SOURCE,
							 WORKER_INPUT_TAG,
							 MPI_COMM_WORLD);
					workers[status.MPI_SOURCE - 2] = element->graph;
					free(element);
			}
		}
	
		for (int i = 0; i < ntasks - 2; i++) {
			if(workers[i].n != -1)
				free(workers[i].distances);
		}
		
		int num_opt_solutions = 0;
		while ((element = pop_element(&min_graphs)) != NULL) {
			num_opt_solutions++;
			for (int i = 0; i < num_vertices; i++) {
				for (int j = 0; j < num_vertices; j++) {
					if (element->graph.distances[num_vertices*i + j] == GRAPH_INFINITY) {
						printf("n,");
					}
					else {
						printf("y,");
					}
				}
				printf("\n");
			}
			printf("\n");
			free(element->graph.distances);
			free(element);
		}
		printf("minimum distance: %d\n", min_result);
		printf("number of optimal solutions: %d\n", num_opt_solutions);
	}
	
	MPI_Send(0, 0, MPI_INT, 1,
			 GENERATOR_KILL_TAG, MPI_COMM_WORLD);
	
	for (int i = 2; i < ntasks; i++) {
		MPI_Send(0, 0, MPI_INT, i, WORKER_KILL_TAG, MPI_COMM_WORLD);
	}

	free(workers);
}

void slave() {
	MPI_Status status;
	problem_graph graph;
	
	while (1) {
		MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		switch (status.MPI_TAG) {
			case WORKER_NUM_VERTICES_TAG:
				MPI_Recv(&graph.n,
						 1,
						 MPI_INT,
						 MPI_ANY_SOURCE,
						 WORKER_NUM_VERTICES_TAG,
						 MPI_COMM_WORLD,
						 &status);
				if(graph.distances)
					free(graph.distances);
				graph.distances = malloc(graph.n*graph.n*sizeof(int));
				//printf("Worker: got %d vertices\n", graph.n);
				break;
				
			case WORKER_KILL_TAG:
				free(graph.distances);
				return;
				
			case WORKER_INPUT_TAG:
				MPI_Recv(graph.distances,
						 graph.n*graph.n,
						 MPI_INT,
						 MPI_ANY_SOURCE,
						 WORKER_INPUT_TAG,
						 MPI_COMM_WORLD,
						 &status);
				//printf("Worker: got input\n");
				
				int result = minimum_total_distance(graph);
				//printf("Worker: sending result %d\n", result);
				MPI_Send(&result,
						 1,
						 MPI_INT,
						 0,
						 WORKER_OUTPUT_TAG,
						 MPI_COMM_WORLD);
				break;
				
			default:
				break;
		}
	}
}

int main(int argc, char *argv[])
{
	int rank;
	
	MPI_Init(&argc, &argv);
	
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	switch (rank) {
		case 0:
			master();
			break;
		case 1:
			generator();
			break;
		default:
			slave();
			break;
	}
	
	MPI_Finalize();
	return 0;
}