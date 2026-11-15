// This file has the functions declared in DDFLPD.h.
// The labels below match the repair and support algorithms in our pseudo.

#include "DDFLPD.h"
#include "Dijkstra.h"

#include <algorithm>
#include <cassert>
#include <limits>
#include <vector>

void EdgeStore::clear(int n, int m) {
    owner_.assign(m, -1);
    ownedCount_.assign(n, 0);
    heapOf_.assign(n, {});
    heapPos_.assign(m, -1);
}

int EdgeStore::chooseOwner(int u, int v) {
    const int chosen = ownedCount_[u] < ownedCount_[v] ? u
                     : ownedCount_[v] < ownedCount_[u] ? v
                     : std::min(u, v);

    ++ownedCount_[chosen];
    return chosen;
}

bool EdgeStore::less(int a, int b, const Graph& graph) const {
    if (graph.getWeight(a) != graph.getWeight(b)) {
        return graph.getWeight(a) < graph.getWeight(b);
    }

    return a < b;
}

void EdgeStore::swapAt(int vertex, int a, int b) {
    std::vector<int>& heap = heapOf_[vertex];

    std::swap(heap[a], heap[b]);
    heapPos_[heap[a]] = a;
    heapPos_[heap[b]] = b;
}

void EdgeStore::siftUp(int vertex, int position, const Graph& graph) {
    std::vector<int>& heap = heapOf_[vertex];

    while (position > 0) {
        const int p = (position - 1) / 2;
        if (!less(heap[position], heap[p], graph)) break;

        swapAt(vertex, position, p);
        position = p;
    }
}

void EdgeStore::siftDown(int vertex, int position, const Graph& graph) {
    std::vector<int>& heap = heapOf_[vertex];
    const int size = static_cast<int>(heap.size());

    // Only internal nodes have a left child. This also keeps 2*position+1
    // within the int range, even for a very large heap.
    while (position < size / 2) {
        const int left = 2 * position + 1;
        const int right = left + 1;
        int smallest = position;

        if (left < size && less(heap[left], heap[smallest], graph)) {
            smallest = left;
        }

        if (right < size && less(heap[right], heap[smallest], graph)) {
            smallest = right;
        }

        if (smallest == position) break;

        swapAt(vertex, position, smallest);
        position = smallest;
    }
}

void EdgeStore::add(int e, int owner, const Graph& graph) {
    owner_[e] = owner;

    std::vector<int>& heap = heapOf_[owner];
    heapPos_[e] = static_cast<int>(heap.size());
    heap.push_back(e);

    siftUp(owner, heapPos_[e], graph);
}

void EdgeStore::increaseKey(int e, const Graph& graph) {
    siftDown(owner_[e], heapPos_[e], graph);
}

int EdgeStore::ownerOf(int e) const {
    return owner_[e];
}

bool EdgeStore::check(const Graph& graph) const {
    const int n = graph.getVertexCount();
    const int m = graph.getEdgeCount();

    if (static_cast<int>(owner_.size()) != m ||
        static_cast<int>(heapPos_.size()) != m ||
        static_cast<int>(heapOf_.size()) != n ||
        static_cast<int>(ownedCount_.size()) != n) {
        return false;
    }

    std::vector<int> seen(m, 0);

    for (int v = 0; v < n; ++v) {
        const std::vector<int>& heap = heapOf_[v];

        if (static_cast<int>(heap.size()) != ownedCount_[v]) return false;

        for (int i = 0; i < static_cast<int>(heap.size()); ++i) {
            const int e = heap[i];

            if (e < 0 || e >= m || owner_[e] != v ||
                heapPos_[e] != i || ++seen[e] != 1) {
                return false;
            }

            if (i > 0 && less(e, heap[(i - 1) / 2], graph)) return false;
        }
    }

    for (int e = 0; e < m; ++e) {
        if (seen[e] != 1) return false;

        const Edge& edge = graph.getEdge(e);
        if (owner_[e] != edge.u && owner_[e] != edge.v) return false;
    }

    return true;
}

void DDFLPD::unlink(int v) {
    const int p = listOwner_[v];
    if (p == -1) return;

    const int previous = prevSibling_[v];
    const int next = nextSibling_[v];

    if (previous == -1) {
        firstChild_[p] = next;
    } else {
        nextSibling_[previous] = next;
    }

    if (next != -1) prevSibling_[next] = previous;

    listOwner_[v] = -1;
    prevSibling_[v] = -1;
    nextSibling_[v] = -1;
}

void DDFLPD::link(int v, int p) {
    nextSibling_[v] = firstChild_[p];
    prevSibling_[v] = -1;

    if (firstChild_[p] != -1) prevSibling_[firstChild_[p]] = v;

    firstChild_[p] = v;
    listOwner_[v] = p;
}

void DDFLPD::moveTo(int v, int p) {
    unlink(v);
    link(v, p);
}

bool DDFLPD::charge() {
    if (!counting_) return true;

    if (!checkedIncrement(wR_)) {
        stopReason_ = RepairResult::Overflow;
        return false;
    }

    if (wR_ > limit_) {
        stopReason_ = RepairResult::LimitExceeded;
        return false;
    }

    return true;
}

Status DDFLPD::buildSupport(const Graph& graph, unsigned long long& work) {
    const int n = graph.getVertexCount();
    const int m = graph.getEdgeCount();
    const int source = graph.getSource();

    work = 0;                                          // B1

    firstChild_.assign(n, -1);                          // B2
    nextSibling_.assign(n, -1);
    prevSibling_.assign(n, -1);
    listOwner_.assign(n, -1);
    store_.clear(n, m);

    for (int v = 0; v < n; ++v) {                       // B3
        if (v == source) continue;

        // Graph prep provides a connected graph, so every non-source vertex
        // must have a parent after a successful Dijkstra run.
        assert(parent_[v] >= 0 && parent_[v] < n);
        link(v, parent_[v]);
    }

    for (int e = 0; e < m; ++e) {
        if (!checkedIncrement(work)) return Status::Overflow; // B4

        const Edge& edge = graph.getEdge(e);
        const int owner = store_.chooseOwner(edge.u, edge.v);

        store_.add(e, owner, graph);
        if (!checkedIncrement(work)) return Status::Overflow; // B5
    }

    return Status::Ok;
}

Status DDFLPD::initialize(const Graph& graph) {
    const int n = graph.getVertexCount();

    Q_.setup(n);
    color_.assign(n, 0);
    colored_.clear();

    redCount_ = 0;
    wR_ = 0;
    wD_ = 0;
    wB_ = 0;
    stopReason_ = RepairResult::Finished;

    if (Dijkstra::run(graph, Q_, distance_, parent_, wD_, true)
        != Status::Ok) {
        return Status::Overflow;
    }

    return buildSupport(graph, wB_);
}

bool DDFLPD::checkPink(const Graph& graph, int z, bool& pink) {
    pink = false;

    for (const Adjacent& entry : graph.neighborsOf(z)) {
        const int u = entry.neighbor;

        if (u == parent_[z]) continue; // CP1: skip parent before counting.
        if (!charge()) return false;  // One non-parent edge look.

        // CP2: finite distances are nonnegative, so subtraction is safe.
        if (color_[u] != 2 &&
            distance_[z] != Graph::INF_DISTANCE &&
            distance_[u] != Graph::INF_DISTANCE &&
            distance_[z] - distance_[u] == graph.getWeight(entry.edgeId)) {

            moveTo(z, u);                              // CP3
            parent_[z] = u;

            if (color_[z] == 0) colored_.push_back(z);
            color_[z] = 1;

            pink = true;
            return true;
        }
    }

    return true; // CP4: no tight alternative; caller will color z red.
}

bool DDFLPD::checkBestNonRed(const Graph& graph, int z) {
    long long best = Graph::INF_DISTANCE;               // CB1
    int bestParent = -1;

    for (const Adjacent& entry : graph.neighborsOf(z)) {
        if (!charge()) return false;                   // CB2: every edge look.

        const int u = entry.neighbor;
        if (color_[u] == 2) continue;

        const long long weight = graph.getWeight(entry.edgeId);

        if (distance_[u] == Graph::INF_DISTANCE ||
            weight >= Graph::INF_DISTANCE - distance_[u]) {
            stopReason_ = RepairResult::Overflow;
            return false;
        }

        const long long candidate = distance_[u] + weight; // CB3

        if (candidate < best) {
            best = candidate;
            bestParent = u;
        }
    }

    unlink(z);                                         // CB4: not counted.
    distance_[z] = best;
    parent_[z] = bestParent;

    if (bestParent != -1) link(z, bestParent);

    return true;
}

bool DDFLPD::processRed(const Graph& graph, int z) {
    for (const Adjacent& entry : graph.neighborsOf(z)) {
        if (!charge()) return false;                   // PR1: every edge look.

        const int q = entry.neighbor;
        if (color_[q] != 2 || !Q_.contains(q)) continue;

        const long long weight = graph.getWeight(entry.edgeId);

        if (distance_[z] == Graph::INF_DISTANCE ||
            weight >= Graph::INF_DISTANCE - distance_[z]) {
            stopReason_ = RepairResult::Overflow;
            return false;
        }

        const long long candidate = distance_[z] + weight; // PR2

        if (candidate < distance_[q]) {
            distance_[q] = candidate;
            parent_[q] = z;

            moveTo(q, z);

            Q_.decreaseKey(q, candidate);               // PR3
            if (!charge()) return false;
        }
    }

    return true;
}

RepairResult DDFLPD::repair(Graph& graph, const Batch& batch, bool counting,
                            unsigned long long limit) {
    assert(Q_.empty());
    assert(distance_.size() ==
           static_cast<std::size_t>(graph.getVertexCount()));

    counting_ = counting;
    limit_ = limit;
    wR_ = 0;
    redCount_ = 0;
    stopReason_ = RepairResult::Finished;
    colored_.clear();

    // Phase 1: apply increases and queue affected tree children (not counted).
    for (const BatchUpdate& update : batch.updates) {
        assert(update.edgeId >= 0 && update.edgeId < graph.getEdgeCount());
        assert(update.oldWeight == graph.getWeight(update.edgeId));
        assert(update.newWeight > update.oldWeight);

        graph.setWeight(update.edgeId, update.newWeight);
        store_.increaseKey(update.edgeId, graph);

        const Edge& edge = graph.getEdge(update.edgeId);
        int child = -1;

        if (parent_[edge.v] == edge.u) {
            child = edge.v;
        } else if (parent_[edge.u] == edge.v) {
            child = edge.u;
        }

        if (child != -1 && !Q_.contains(child)) {
            Q_.insert(child, distance_[child]);
        }
    }

    // Phase 2: find pink and red vertices.
    while (!Q_.empty()) {
        const int z = Q_.extractMin();
        if (!charge()) return stopReason_;

        bool pink = false;

        if (!checkPink(graph, z, pink)) return stopReason_;
        if (pink) continue;

        if (color_[z] == 0) colored_.push_back(z);
        color_[z] = 2;
        ++redCount_;

        for (int child = firstChild_[z]; child != -1;
             child = nextSibling_[child]) {
            if (!Q_.contains(child)) {
                Q_.insert(child, distance_[child]);
                if (!charge()) return stopReason_;
            }
        }
    }

    // Phase 3: find replacements for red vertices, then relax red neighbors.
    for (int z : colored_) {
        if (color_[z] != 2) continue;

        if (!checkBestNonRed(graph, z)) return stopReason_;

        Q_.insert(z, distance_[z]);
        if (!charge()) return stopReason_;
    }

    while (!Q_.empty()) {
        const int z = Q_.extractMin();
        if (!charge()) return stopReason_;

        if (!processRed(graph, z)) return stopReason_;
    }

    // Line 16: cleanup is not counted. Keep colored_ for untimed metrics.
    for (int z : colored_) color_[z] = 0;

    return RepairResult::Finished;
}

void DDFLPD::abandonRepair() {
    Q_.clear();                                        // Line 18

    for (int z : colored_) color_[z] = 0;
}

Status DDFLPD::rebuild(const Graph& graph, unsigned long long& work) {
    unsigned long long dijkstraWork = 0;
    unsigned long long supportWork = 0;
    work = 0;

    // abandonRepair already emptied Q; setup belongs only in initialize.
    assert(Q_.empty());

    if (Dijkstra::run(graph, Q_, distance_, parent_, dijkstraWork, true)
        != Status::Ok) {
        return Status::Overflow;
    }

    if (buildSupport(graph, supportWork) != Status::Ok) {
        return Status::Overflow;
    }

    if (supportWork > std::numeric_limits<unsigned long long>::max() -
                      dijkstraWork) {
        return Status::Overflow;
    }

    work = dijkstraWork + supportWork;
    return Status::Ok;
}

bool DDFLPD::checkSupport(const Graph& graph) const {
    const int n = graph.getVertexCount();
    const int source = graph.getSource();

    if (static_cast<int>(parent_.size()) != n ||
        static_cast<int>(distance_.size()) != n ||
        static_cast<int>(color_.size()) != n ||
        static_cast<int>(firstChild_.size()) != n ||
        static_cast<int>(nextSibling_.size()) != n ||
        static_cast<int>(prevSibling_.size()) != n ||
        static_cast<int>(listOwner_.size()) != n ||
        source < 0 || source >= n || parent_[source] != -1 ||
        listOwner_[source] != -1 || prevSibling_[source] != -1 ||
        nextSibling_[source] != -1 || !Q_.empty() ||
        !store_.check(graph)) {
        return false;
    }

    std::vector<int> seen(n, 0);
    int count = 0;

    for (int p = 0; p < n; ++p) {
        if (color_[p] != 0) return false;

        int previous = -1;

        for (int v = firstChild_[p]; v != -1; v = nextSibling_[v]) {
            if (v < 0 || v >= n || v == source || ++seen[v] != 1 ||
                listOwner_[v] != p || parent_[v] != p ||
                prevSibling_[v] != previous) {
                return false;
            }

            previous = v;
            ++count;
        }
    }

    if (count != n - 1) return false;

    for (int v = 0; v < n; ++v) {
        if (v != source && (seen[v] != 1 || listOwner_[v] != parent_[v])) {
            return false;
        }
    }

    return true;
}

const std::vector<long long>& DDFLPD::distances() const {
    return distance_;
}

const std::vector<int>& DDFLPD::parents() const {
    return parent_;
}

unsigned long long DDFLPD::getWD() const {
    return wD_;
}

unsigned long long DDFLPD::getWB() const {
    return wB_;
}

unsigned long long DDFLPD::lastWR() const {
    return wR_;
}

std::size_t DDFLPD::lastAffected() const {
    return colored_.size();
}

int DDFLPD::lastRed() const {
    return redCount_;
}

int DDFLPD::edgeOwner(int edgeId) const {
    return store_.ownerOf(edgeId);
}

unsigned long long DDFLPD::lastDegreeSum(const Graph& graph) const {
    unsigned long long result = 0;

    for (int v : colored_) result += graph.neighborsOf(v).size();

    return result;
}