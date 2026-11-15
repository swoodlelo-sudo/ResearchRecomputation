// This file has the functions declared in GraphPrep.h.
// The input keeps integer travel times. Each arc becomes one undirected pair.

#include "GraphPrep.h"

#include <algorithm>
#include <fstream>
#include <limits>
#include <tuple>
#include <vector>

namespace {
struct RawEdge {
    int u;
    int v;
    long long weight;
};
}

bool GraphPrep::convertDimacs(const std::string& fileName, Graph& full,
                              PrepStats& stats) {
    stats = PrepStats{};
    std::ifstream in(fileName);
    if (!in) return false;

    bool hasHeader = false;
    int n = 0;
    long long expectedArcs = 0;
    std::vector<RawEdge> raw;
    std::string tag;

    while (in >> tag) {
        if (tag == "c") {
            std::string rest;
            std::getline(in, rest);
        } else if (tag == "p") {
            std::string format;
            long long vertexCount;

            if (hasHeader || !(in >> format >> vertexCount >> expectedArcs) ||
                format != "sp" || vertexCount <= 0 ||
                vertexCount > std::numeric_limits<int>::max() ||
                expectedArcs < 0) {
                return false;
            }

            n = static_cast<int>(vertexCount);
            hasHeader = true;
        } else if (tag == "a") {
            long long u, v, w;

            if (!hasHeader || !(in >> u >> v >> w) ||
                u < 1 || u > n || v < 1 || v > n || w <= 0 ||
                stats.arcLines == std::numeric_limits<long long>::max()) {
                return false;
            }

            ++stats.arcLines;
            if (stats.arcLines > expectedArcs) return false;

            if (u == v) {
                ++stats.selfLoops;
            } else {
                raw.push_back({
                    static_cast<int>(std::min(u, v)),
                    static_cast<int>(std::max(u, v)),
                    w
                });
            }
        } else {
            return false;
        }
    }

    if (in.bad() || !in.eof() || !hasHeader ||
        stats.arcLines != expectedArcs) {
        return false;
    }

    // Weight is the third key, so the first copy of a pair is its lightest.
    std::sort(raw.begin(), raw.end(), [](const RawEdge& a, const RawEdge& b) {
        return std::tie(a.u, a.v, a.weight) < std::tie(b.u, b.v, b.weight);
    });

    std::vector<RawEdge> kept;
    kept.reserve(raw.size());

    for (const RawEdge& edge : raw) {
        if (kept.empty() || kept.back().u != edge.u ||
            kept.back().v != edge.v) {
            kept.push_back(edge);
        }
    }

    stats.duplicatesMerged = static_cast<long long>(raw.size() - kept.size());
    stats.edges = static_cast<long long>(kept.size());

    if (kept.size() >
        static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        return false; // Graph uses int edge IDs and counts.
    }

    // This insertion order also makes every full adjacency list ascending.
    full.reset(n, 0);

    for (const RawEdge& edge : kept) {
        full.addEdge(edge.u - 1, edge.v - 1, edge.weight);
    }

    return true;
}

void GraphPrep::bfsOrder(const Graph& graph, int source,
                         std::vector<int>& order) {
    order.clear();

    const int n = graph.getVertexCount();
    if (source < 0 || source >= n) return;

    std::vector<char> seen(n, 0);
    seen[source] = 1;
    order.push_back(source);

    for (std::size_t head = 0; head < order.size(); ++head) {
        const int u = order[head];

        for (const Adjacent& entry : graph.neighborsOf(u)) {
            const int v = entry.neighbor;

            if (!seen[v]) {
                seen[v] = 1;
                order.push_back(v);
            }
        }
    }
}

bool GraphPrep::neighborsAscending(const Graph& graph) {
    for (int u = 0; u < graph.getVertexCount(); ++u) {
        int previous = -1;

        for (const Adjacent& entry : graph.neighborsOf(u)) {
            if (entry.neighbor <= previous) return false;
            previous = entry.neighbor;
        }
    }

    return true;
}

bool GraphPrep::cutGraph(const Graph& full, const std::vector<int>& order,
                         int N, Graph& cut, std::vector<int>& localToFull) {
    if (N <= 0 || static_cast<std::size_t>(N) > order.size()) return false;
    if (order.front() != full.getSource()) return false;

    const int n = full.getVertexCount();
    std::vector<int> fullToLocal(n, -1);

    for (int i = 0; i < N; ++i) {
        const int v = order[i];

        if (v < 0 || v >= n || fullToLocal[v] != -1) return false;
        fullToLocal[v] = i;
    }

    localToFull.assign(order.begin(), order.begin() + N);
    cut.reset(N, 0);

    // Scanning full edge IDs preserves their relative order in every cut.
    for (int e = 0; e < full.getEdgeCount(); ++e) {
        const Edge& edge = full.getEdge(e);
        const int u = fullToLocal[edge.u];
        const int v = fullToLocal[edge.v];

        if (u != -1 && v != -1) {
            cut.addEdge(u, v, edge.weight);
        }
    }

    return true;
}

bool GraphPrep::saveMapping(const std::vector<int>& localToFull,
                            const std::string& fileName) {
    std::ofstream out(fileName);
    if (!out) return false;

    out << localToFull.size() << '\n';

    for (std::size_t i = 0; i < localToFull.size(); ++i) {
        out << i << ' ' << (localToFull[i] + 1) << '\n';
    }

    out.close();
    return !out.fail();
}