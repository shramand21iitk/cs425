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
    for (int src=0; src<n; ++src){
        for (int dest=0; dest<n; ++dest){
            if (src == dest) {
                dist[src][dest] = 0;
                nextHop[src][dest] = src;
            } else if (graph[src][dest] != INF) {
                dist[src][dest] = graph[src][dest];
                nextHop[src][dest] = dest;
            } else {
                dist[src][dest] = INF;
                nextHop[src][dest] = -1;
            }
        }
    }
    // Run the Bellman-Ford algorithm as long as updates are being made
    while (true){
        bool updated = false;
        for (int src = 0; src<n; ++src){
            for (int dest=0; dest<n; ++dest){
                if (src == dest) continue; //Skip self-loops
                for (int nbr=0; nbr<n; ++nbr){
                    if (nbr == src || nbr == dest) continue; //Skip self-loops
                    if (dist[src][nbr] != INF && dist[nbr][dest] != INF){
                        int alternateCost = dist[src][nbr] + dist[nbr][dest];
                        if (alternateCost < dist[src][dest]){
                            dist[src][dest] = alternateCost;
                            nextHop[src][dest] = nextHop[src][nbr];
                            updated = true; // An update was made
                        }

                    }
                }
            }
        }
        if (!updated) break; // No updates means we are done
    }

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
                if (!visited[i] && dist[i] < mindist) {
                    mindist = dist[i];
                    nbr = i;
                }
            }
            if (nbr == -1) break; // All reachable nodes are visited
            visited[nbr] = true; // Mark the node as visited
            // Update distances to neighbors
            // Loop through all neighbors of the current node
            for (int j = 0; j < n; ++j) {
                if (!visited[j] && graph[nbr][j] != INF) {
                    int newDist = dist[nbr] + graph[nbr][j];
                    if (newDist < dist[j]) {
                        dist[j] = newDist;
                        prev[j] = nbr;
                    }
                }
            }
        }
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

