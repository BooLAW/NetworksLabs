#include <Windows.h>
#include <iostream>
#include <climits>
#include <queue>
#include <algorithm>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>

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


/** Messages sent among nodes. */
struct Message {
	int sender; // Sender identifier (from 0 to V-1)
	int dist;   // Distance of the sender
};


// State vectors (each position is a different node)
std::vector<std::thread> threads(V); // Threads
std::vector<std::mutex> mutexes(V); // Mutexes for accessing thread data
std::vector<std::condition_variable> event(V); // Condition variables to wait for events
std::vector< std::queue<Message> > messages(V); // Each node has a queue of messages


// Whether or not the algorithm should finish
bool shouldFinish = false;


/**
* Given a graph G, this function computes the minimum distance
* from the passed source node to all the other nodes of the graph.
* @param id [IN] Id of the node to compute the distances
* @param dist [OUT] Distance from src to each node
* @param prev [OUT] For each node, the previous node in the shortest path
*/
void parallel_dijkstra(int id, int dist[V], int prev[V])
{
	int u = id; // To keep similarity with standard dijkstra

	while (true)
	{
		// When a message is received, this will be set to true if
		// the current path distance is better than the previous one
		bool distance_improved = false;

		// Wait for incoming events (from other nodes) and receive them
		{
			
			std::unique_lock<std::mutex> lck(mutexes[u]); // - DONE 6-a: lock this thread mutex to protect access to global data

			if (messages[u].empty())
			{
				event[u].wait(lck);// DONE 6-b: wait for a message from another node
			}
			// DONE 6-c: check if 'shouldFinish' is true to finish this thread
			if (shouldFinish) 
				break;

			// Get the incoming message
			Message msg(messages[u].front());
			messages[u].pop();

			// Update the cost to reach here
			if (msg.sender == NaN)
			{
				dist[u] = 0;
				prev[u] = NaN;
				distance_improved = true;
			}
			else
			{
				int edge_cost = G[u][msg.sender];
				if (msg.dist + edge_cost < dist[u]) {
					dist[u] = msg.dist + edge_cost;
					prev[u] = msg.sender;
					distance_improved = true;
				}
			}
		}

		// If the distance didn't change, skip the rest of the
		// loop and continue to the next iteration
		if (!distance_improved)
		{
			continue;
		}

		
		// DONE 7: Create a message for the neigbouring nodes
		Message m;
		// - the message needs to contain the information about this thread's node
		m.sender = u;
		m.dist = dist[u];

		// Send messages to neighbour nodes to update their state
		for (int v = 0; v < V; ++v)
		{
			int edge_dist = G[u][v];
			if (edge_dist != Inf)
			{
				// DONE 8: Send the message to the neighbor
				std::unique_lock<std::mutex> lck(mutexes[v]);				// - lock the neighbor's mutex
				messages[v].push(m);										// - enqueue the message into its queue
				event[v].notify_one();										// - notify the neighbor about this event (use notify_one)
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


	// DONE 1: Using a loop...
	for (int i = 0; i < V; ++i)
	{
		dist[i] = Inf; // - initialize 'dist' array elements to Inf and NaN respectively
		prev[i] = NaN; // - initialize 'prev' array elements to Inf and NaN respectively
	}


	// DONE 2: Using a loop...
	for (int i = 0; i < V; ++i)
	{
		//The Same line does everything of the TODO
		threads[i] = std::thread(parallel_dijkstra, i, dist, prev); // - create a thread for each node store them in the global array 'threads'
																	// - make them execute the 'parallel_dijkstra' function
																	// - pass each thread their id, and the two arrays 'dist' and 'prev'
	}


	// DONE 3: Look at this code (you don't have to write anything)
	// - it starts the algorithm (notifies the source node to start)
	// - creation of the initial message
	Message msg;
	msg.dist = Inf;
	msg.sender = NaN;
	{
		std::unique_lock<std::mutex> lck(mutexes[0]); // - protects the message queue of the thread 0
		messages[0].push(msg);                        // - pushes the message into the queue of messages for the thread 0
		event[0].notify_one();                        // - notifies the thread 0 that it has a message
	}


	// wait a tenth of a second
	const unsigned int hundred_millis = 100;
	Sleep(hundred_millis);


	// DONE 4: Using a loop... notify all threads to finish
	for (int i = 0; i < V; ++i)
	{
		std::unique_lock<std::mutex> lck(mutexes[i]);		// - protect each node critical section (locking their mutex)

		shouldFinish = true;		// - set the global 'shouldFinish' to true

		event[i].notify_one();		// - notify the thread that an event occurred

	}


	// DONE 5: Using a loop...
	for (int i = 0; i < V; ++i)
	{
		threads[i].join();		// - wait for all threads to finish using thread::join()
	}


	// print results
	printPaths(prev);

	system("pause");
	return 0;
}
