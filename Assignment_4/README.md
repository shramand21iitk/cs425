# Routing Algorithm Simulator

This project implements two fundamental routing algorithms used in computer networks:

- **Distance Vector Routing (DVR)**
- **Link State Routing (LSR)**

It reads a network topology from an input file (in the form of an adjacency matrix) and simulates both DVR and LSR, printing each node’s routing table.

## File Structure

- `main.cpp` – Contains the implementation for DVR and LSR simulations and outputs the routing table for each node/router by running both the algorithms.
- `<input_file>` – Input file containing the adjacency matrix for the graph of the network, where each node represents a router.
- We will be testing on the following files containing adjacency matrices of the network graph --> `input1.txt`, `input2.txt`, `input3.txt`, `input4.txt`

## Compilation and running the script

To compile the program, use any C++ compiler (like `g++`):

```bash
g++ routing_sim.cpp -o main.o
```

To run the executable output file with suppose input1.txt, run:

```bash
./main.o input1.txt
```

## Link State Routing (Djikstra's Akgorithm) - Pseudocode
```
For each node in graph:
   Initialize:
   for all nodes:
      dist = INF; dist[src] = 0
      prev = -1
      visited = false

   While there are unvisited nodes:
      Find the unvisited node nbr with the smallest dist[nbr]
      If no such node exists, break
      Mark nbr as visited

      For each neighbor j of nbr:
         If j is not visited and edge(nbr, j) exists:
               newDist = dist[nbr] + weight(nbr, j)
               If newDist < dist[j]:
                  dist[j] = newDist
                  prev[j] = nbr

Output routing table using dist[] and prev[]
```

## Distance Vector Routing (Bellman-Ford equation) - Pseudocode
```
Initialize: 
   For each node src: 
      For each node dest: 
         If src == dest: 
            dist[src][dest] = 0 
            nextHop[src][dest] = src 
         Else if graph[src][dest] != INF: 
            dist[src][dest] = graph[src][dest] 
            nextHop[src][dest] = dest 
         Else: 
            dist[src][dest] = INF 
            nextHop[src][dest] = -1

Repeat until no updates: 
   updated = false 
   For each node src: 
      For each node dest where src ≠ dest: 
         For each node nbr (intermediate node) where nbr ≠ src and nbr ≠ dest: 
            If dist[src][nbr] ≠ INF and dist[nbr][dest] ≠ INF: 
               alternateCost = dist[src][nbr] + dist[nbr][dest] 
            If alternateCost < dist[src][dest]: 
               dist[src][dest] = alternateCost 
               nextHop[src][dest] = nextHop[src][nbr] updated = true
   If updated is false, break the loop

Print routing table using dist[][] and nextHop[][]
```

## Distance Vector Routing (DVR) vs Link State Routing (LSR)

| Feature                    | Distance Vector Routing (DVR)     | Link State Routing (LSR)           |
|---------------------------|-----------------------------------|-------------------------------------|
| **Algorithm Used**        | Bellman-Ford                      | Dijkstra                           |
| **Information Sharing**    | Routing tables (cost to destinations) | Link-state packets (neighbor info) |
| **Topology Knowledge** | Knows only neighbors              | Knows entire network               |
| **Update Method**         | Periodic or triggered to neighbors | Triggered, floods to all nodes     |
| **Convergence Speed**     | Slower                            | Faster                             |
| **Scalability**           | Limited to smaller networks       | Suitable for large networks        |
| **Loop Method**       | Uses split horizon, poison reverse | Loop-free due to global view       |
| **Memory requirement**        | Low CPU & memory                  | Higher CPU & memory                |

## Contribution of Each Member

| Contributor [Roll Number]  | Code Contribution (%)| Documentation Contribution (%)| Testing & Debugging (%)|
|----------------------------|----------------------|-------------------------------|------------------------|
| Ravija Chandel [210835]    | 33%                  | 33%                           | 34%                    |
| Shaurya Singh  [218070969] | 33%                  | 34%                           | 33%                    |
| Shraman Das    [218070996] | 34%                  | 33%                           | 33%                    |

## Sources Referred
1. **Books:**
   - Computer Networking: A Top-Down Approach, 8th Edition
2. **Blogs/Articles/Videos:**
   - [Belman Ford Distance Vector Routing](https://www.youtube.com/watch?v=jJU2AVX6gpU)
   - [Dijkstra Algorithm | Single Source Shortest Path | Pseudocode](https://www.youtube.com/watch?v=Y4cfFPKceik&t=1008s)