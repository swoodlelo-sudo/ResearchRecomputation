#ifndef DDFLPD_WL_MINHEAP_H
#define DDFLPD_WL_MINHEAP_H

#include <vector>

// This is an indexed binary min-heap with insert, extract-min, and decrease-key.
// It is the queue Q used by Dijkstra and PROCESSRED in our pseudo.
//
// Our pseudo assumes decrease-key takes O(log n). To avoid searching the heap
// for a vertex, this heap keeps a position list:
//     pos[v] = where vertex v sits in the heap, or -1 if it is not in the heap.
//
// Each heap owns its three vectors. heap stores vertex IDs in heap order,
// pos stores their positions, and key[v] stores vertex v's current priority.
// pos and key have n entries. heap starts empty with room reserved for n.
//
// There is no work counter here. Dijkstra and the repair code count their
// priority-queue operations themselves.
class MinHeap {
private:
    std::vector<int> heap;
    std::vector<int> pos;
    std::vector<long long> key;

    void swapAt(int a, int b);
    void siftUp(int index);
    void siftDown(int index);

public:
    // Set up an empty heap for vertex IDs 0 through n - 1. n must be >= 0.
    // This is O(n); use clear() when reusing the same setup.
    void setup(int n);

    [[nodiscard]] bool empty() const;

    // v must be a valid vertex ID from setup().
    [[nodiscard]] bool contains(int v) const;

    // v must be valid and not already in the heap.
    void insert(int v, long long k);

    // The heap must not be empty.
    [[nodiscard]] int extractMin();

    // v must be in the heap, and k must not be larger than its current key.
    void decreaseKey(int v, long long k);

    // Reset positions only for vertices still in the heap.
    // This is O(h), where h is the number of vertices currently in the heap.
    void clear();
};

#endif