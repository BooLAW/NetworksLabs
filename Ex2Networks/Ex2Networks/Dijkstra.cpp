#include <iostream>
#include <climits>
#include <queue>

// Number of nodes in the graph G
const int V = 8;
const int Inf = INT_MAX;
const int NaN = INT_MAX;

// Adjacency matrix of the graph
int G[V][V] = {
	//A,   B,   C,   D,   E,   F,   G,   H
	{ Inf, 1,   4,   Inf, Inf, Inf, Inf, Inf }, // A
	{ 1,   Inf, 2,   Inf, Inf, Inf, 4,   2   }, // B
	{ 4,   2,   Inf, 1,   3,   Inf, Inf, Inf }, // C
	{ Inf, Inf, 1,   Inf, 1,   3,   1,   Inf }, // D
	{ Inf, Inf, 3,   1,   Inf, 1,   Inf, Inf }, // E
	{ Inf, Inf, Inf, 3,   1,   Inf, 6,   Inf }, // F
	{ Inf, 4,   Inf, 1,   Inf, 6,   Inf, 14  }, // G
	{ Inf, 2,   Inf, Inf, Inf, Inf, 14,  Inf }  // H
};

/**
* Given a graph G, this function computes the minimum distance
* from the passed source node to all the other nodes of the graph.
* @param src [IN] Source node to compute distance to other nodes
* @param dist [OUT] Distance from src to each node
* @param prev [OUT] For each node, the previous node in the shortest path
*/
void dijkstra(int src, int dist[V], int prev[V])
{
	// Initialize arrays
	for (int i = 0; i < V; ++i) {
		dist[i] = Inf;
		prev[i] = NaN;
	}

	// Ascending order priority queue
	// We can insert node indices, and sorts them by 'dist'
	auto comp = [dist](int a, int b) { return dist[a] > dist[b]; };
	std::priority_queue<
		int,
		std::vector<int>,
		decltype(comp)> Q(comp);

	// Start with the source node
	dist[src] = 0;
	Q.push(src);

	// While the queue is not empty
	while (!Q.empty())
	{
		// Pop the node with smaller cost
		int u = Q.top();
		Q.pop();

		// For each neighbour of u
		for (int v = 0; v < V; ++v) {
			int edge_dist = G[u][v];
			if (edge_dist != Inf) {
				if (dist[u] + edge_dist < dist[v]) {
					dist[v] = dist[u] + edge_dist;
					prev[v] = u;
					Q.push(v);
				}
			}
		}
	}
}

/**
* For each node in prev, it prints the computed shortest path from src.
* @param prev Array of previous nodes in the shortest path.
*/
void printPaths(int prev[V])
{
	for (auto i = 0; i < V; ++i)
	{
		int n = i;
		while (prev[n] != NaN) {
			std::cout << (char)('A' + n) << " <- " << std::flush;
			n = prev[n];
		}
		std::cout << (char)('A' + n) << std::endl;
	}
}

int main(int argc, char **argv)
{
	int dist[V];
	int prev[V];

	dijkstra(0, dist, prev);

	printPaths(prev);

	system("pause");
	return 0;
}
