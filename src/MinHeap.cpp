// This file has the function definitions declared in MinHeap.h.
// The assertions check that the caller follows the rules in the header.

#include "MinHeap.h"

#include <cassert>
#include <utility>

void MinHeap::setup(const int n)
{
    assert(n >= 0);

    heap.clear();
    heap.reserve(n);
    pos.assign(n, -1);
    key.assign(n, 0);
}

bool MinHeap::empty() const
{
    return heap.empty();
}

bool MinHeap::contains(const int v) const
{
    assert(v >= 0 && v < static_cast<int>(pos.size()));
    return pos[v] != -1;
}

void MinHeap::swapAt(const int a, const int b)
{
    std::swap(heap[a], heap[b]);
    pos[heap[a]] = a;
    pos[heap[b]] = b;

    // Keys stay indexed by vertex ID, so they do not need to be swapped.
}

void MinHeap::siftUp(int index)
{
    while (index > 0)
    {
        const int parent = (index - 1) / 2;

        if (key[heap[parent]] <= key[heap[index]])
        {
            break;
        }

        swapAt(parent, index);
        index = parent;
    }
}

void MinHeap::siftDown(int index)
{
    const int size = static_cast<int>(heap.size());

    // Only indices before size / 2 have children.
    while (index < size / 2)
    {
        const int left = 2 * index + 1;
        const int right = left + 1;
        int smallest = index;

        if (key[heap[left]] < key[heap[smallest]])
        {
            smallest = left;
        }
        if (right < size && key[heap[right]] < key[heap[smallest]])
        {
            smallest = right;
        }
        if (smallest == index)
        {
            break;
        }

        swapAt(index, smallest);
        index = smallest;
    }
}

void MinHeap::insert(const int v, const long long k)
{
    assert(!contains(v));

    key[v] = k;
    heap.push_back(v);
    pos[v] = static_cast<int>(heap.size()) - 1;
    siftUp(pos[v]);
}

int MinHeap::extractMin()
{
    assert(!empty());

    const int top = heap[0];
    const int last = static_cast<int>(heap.size()) - 1;

    // Move the last vertex to the root and remove the old root from the end.
    swapAt(0, last);
    heap.pop_back();
    pos[top] = -1;

    if (!heap.empty())
    {
        siftDown(0);
    }

    return top;
}

void MinHeap::decreaseKey(const int v, const long long k)
{
    assert(contains(v));
    assert(k <= key[v]);

    key[v] = k;
    siftUp(pos[v]);
}

void MinHeap::clear()
{
    // Only visit vertices still in the heap, not all n vertices.
    for (const int v : heap)
    {
        pos[v] = -1;
    }

    heap.clear();
    // Old keys can stay: insert() writes a new key when a vertex is reused.
}