// Test the heap on its own before using it in Dijkstra.

#include "MinHeap.h"

#include <iostream>

int main() {
    // To start with, set up the heap for four vertices.
    MinHeap heap;
    heap.setup(4);

    heap.insert(0, 5);
    heap.insert(1, 3);
    heap.insert(2, 8);
    heap.insert(3, 1);

    // Vertex 2 should move to the top after its key becomes 0.
    heap.decreaseKey(2, 0);

    const int expected[] = {2, 3, 1, 0};
    int count = 0;

    std::cout << "Extract order: ";

    while (!heap.empty()) {
        const int vertex = heap.extractMin();
        std::cout << vertex << " ";

        if (count >= 4 || vertex != expected[count]) {
            std::cerr << "\nWrong extraction order.\n";
            return 1;
        }

        count++;
    }

    std::cout << "\n";

    if (count != 4) {
        std::cerr << "The heap did not return all four vertices.\n";
        return 1;
    }

    // After extracting everything, no vertex should still be in the heap.
    std::cout << std::boolalpha;

    for (int vertex = 0; vertex < 4; vertex++) {
        const bool present = heap.contains(vertex);
        std::cout << "contains(" << vertex << "): " << present << "\n";

        if (present) {
            std::cerr << "An extracted vertex is still marked present.\n";
            return 1;
        }
    }

    return 0;
}