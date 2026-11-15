#include "Graph.h"
#include <iostream>

int main() {
    // To start make the graph and load our test file.
    Graph graph;

    if (!graph.load("data/test/fig1.txt")) {
        std::cerr << "Could not load data/test/fig1.txt\n";
        return 1;
    }

    // Print the number of vertices, edges, and the fixed source.
    const int vertexCount = graph.getVertexCount();

    std::cout << "Vertices: " << vertexCount << "\n";
    std::cout << "Edges: " << graph.getEdgeCount() << "\n";
    std::cout << "Source: " << graph.getSource() << "\n";

    // Go through each vertex.
    for (int vertex = 0; vertex < vertexCount; vertex++) {
        std::cout << vertex << ": ";

        // Now go through its neighbors and get each weight by edge ID.
        for (const Adjacent& entry : graph.neighborsOf(vertex)) {
            std::cout << entry.neighbor
                      << " (weight " << graph.getWeight(entry.edgeId)
                      << ") ";
        }

        std::cout << "\n";
    }

    return 0;
}