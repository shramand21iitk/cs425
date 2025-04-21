#include <iostream>
#include <vector>
#include <limits>
#include <queue>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>

using namespace std;

const int INF = 9999;

void printDVRTable(int node, const vector<vector<int>>& table, const vector<vector<int>>& nextHop) {
    cout << "Node " << node << " Routing Table:\n";
    cout << "Dest\tCost\tNext Hop\n";
    for (int i = 0; i < table.size(); ++i) {
        cout << i << "\t" << table[node][i] << "\t";
        if (nextHop[node][i] == -1) cout << "-";
        else cout << nextHop[node][i];
        cout << endl;
    }
    cout << endl;
}

void simulateDVR(const vector<vector<int>>& graph) {
    int n = graph.size();
    vector<vector<int>> dist = graph;
    vector<vector<int>> nextHop(n, vector<int>(n));

    //TODO: Complete this
    // Initialize next hop and distance tables
    // Loop through every source node
    for (int src=0; src<n; ++src){
        // Loop thorugh every destination node to find neighbours of the source node
        for (int dest=0; dest<n; ++dest){
            // If source and destination are the same, set distance to 0 and next hop to source
            if (src == dest) {
                dist[src][dest] = 0;
                nextHop[src][dest] = -1;
            // If there is a direct link between source and destination, set distance from the weight of direct graph link and next hop
            } else if (graph[src][dest] != INF) {
                dist[src][dest] = graph[src][dest];
                nextHop[src][dest] = dest;
            // If there is no direct link between source and destination, set distance to INF and next hop to -1
            } else {
                dist[src][dest] = INF;
                nextHop[src][dest] = -1;
            }
        }
    }
    // Run the Bellman-Ford algorithm as long as updates are being made
    while (true){
        // Initialize updated flag to false
        bool updated = false;
        // Loop through every source node
        for (int src = 0; src<n; ++src){
            // Loop through every destination node 
            for (int dest=0; dest<n; ++dest){
                //Skip self-loops
                if (src == dest) continue; 
                // Loop through every neighbour of the source node
                for (int nbr=0; nbr<n; ++nbr){
                    //Skip self-loops
                    if (nbr == src || nbr == dest) continue; 
                    // If the neighbour is not the source or destination node, and there is a path from source to neighbour and from neighbour to destination
                    if (dist[src][nbr] != INF && dist[nbr][dest] != INF){
                        // Then calculate the alternate cost 
                        int alternateCost = dist[src][nbr] + dist[nbr][dest];
                        //Check if it is less than the current cost
                        if (alternateCost < dist[src][dest]){
                            // If it is, update the distance and next hop tables
                            dist[src][dest] = alternateCost;
                            nextHop[src][dest] = nextHop[src][nbr];
                            // Set the updated flag to true
                            updated = true;
                        }

                    }
                }
            }
        }
        // No updates means the algorithm has converged and can terminate
        if (!updated) break; 
    }
    // Print the final routing tables for each node
    cout << "--- DVR Final Tables ---\n";
    for (int i = 0; i < n; ++i) printDVRTable(i, dist, nextHop);
}

void printLSRTable(int src, const vector<int>& dist, const vector<int>& prev) {
    cout << "Node " << src << " Routing Table:\n";
    cout << "Dest\tCost\tNext Hop\n";
    for (int i = 0; i < dist.size(); ++i) {
        if (i == src) continue;
        cout << i << "\t" << dist[i] << "\t";
        int hop = i;
        while (prev[hop] != src && prev[hop] != -1)
            hop = prev[hop];
        cout << (prev[hop] == -1 ? -1 : hop) << endl;
    }
    cout << endl;
}

void simulateLSR(const vector<vector<int>>& graph) {
    int n = graph.size();
    for (int src = 0; src < n; ++src) {
        vector<int> dist(n, INF);
        vector<int> prev(n, -1);
        vector<bool> visited(n, false);
        dist[src] = 0;
        //TODO: Complete this
        // Dijkstra's algorithm for node src
        //Run the algorithm as long as there are unvisited nodes
        while (any_of(visited.begin(), visited.end(), [](bool v){ return !v; })) {
            // Find the unvisited node with the smallest distance
            int mindist = INF, nbr = -1;
            // Loop through all nodes to find the minimum distance
            for (int i = 0; i < n; ++i) {
                // If the node is unvisited and its distance is less than the current minimum distance
                if (!visited[i] && dist[i] < mindist) {
                    // Then update the minimum distance and the node
                    mindist = dist[i];
                    nbr = i;
                }
            }
            // If no unvisited nodes are reachable, break. This means, all reachable nodes are visited
            if (nbr == -1) break; 
            // Mark the node as visited
            visited[nbr] = true; 
            // Loop through all neighbors of the current node which are unvisited
            for (int j = 0; j < n; ++j) {
                if (!visited[j] && graph[nbr][j] != INF) {
                    // Calculate the alternate distance from the current node to the destination node via the neighbour
                    int newDist = dist[nbr] + graph[nbr][j];
                    // If the new distance is less than the current distance, update the distance and previous node
                    if (newDist < dist[j]) {
                        dist[j] = newDist;
                        prev[j] = nbr;
                    }
                }
            }
        }
        // Print the routing table for the current source node
        printLSRTable(src, dist, prev);
    }
}

vector<vector<int>> readGraphFromFile(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Error: Could not open file " << filename << endl;
        exit(1);
    }
    
    int n;
    file >> n;
    vector<vector<int>> graph(n, vector<int>(n));

    for (int i = 0; i < n; ++i)
        for (int j = 0; j < n; ++j)
            file >> graph[i][j];

    file.close();
    return graph;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <input_file>\n";
        return 1;
    }

    string filename = argv[1];
    vector<vector<int>> graph = readGraphFromFile(filename);

    cout << "\n--- Distance Vector Routing Simulation ---\n";
    simulateDVR(graph);

    cout << "\n--- Link State Routing Simulation ---\n";
    simulateLSR(graph);

    return 0;
}

