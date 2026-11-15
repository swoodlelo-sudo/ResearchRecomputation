// Function definitions declared in Graph.h, the addEdge function takes (u, v, weight) appends the edge to the edge
// list, and then pushes an entry to both u and v's adjacency lists.

#include "Graph.h"

#include <fstream>

Graph::Graph() : source(0) {}

Graph::Graph(const int vertexCount, const int sourceVertex)
{
    reset(vertexCount, sourceVertex);
}

void Graph::reset(const int vertexCount, const int sourceVertex)
{
    source = sourceVertex;
    edges.clear();
    adjacency.assign(vertexCount, std::vector<Adjacent>());
}

int Graph::addEdge(const int u, const int v, const long long weight)
{
    const int edgeId = static_cast<int>(edges.size());

    edges.push_back(Edge{u, v, weight});
    adjacency[u].push_back(Adjacent{v, edgeId});
    adjacency[v].push_back(Adjacent{u, edgeId});

    return edgeId;
}

int Graph::getVertexCount() const
{
    return static_cast<int>(adjacency.size());
}

int Graph::getEdgeCount() const
{
    return static_cast<int>(edges.size());
}

int Graph::getSource() const
{
    return source;
}

const Edge &Graph::getEdge(const int edgeId) const
{
    return edges[edgeId];
}

long long Graph::getWeight(const int edgeId) const
{
    return edges[edgeId].weight;
}

void Graph::setWeight(const int edgeId, const long long weight)
{
    edges[edgeId].weight = weight;
}

int Graph::otherEndpoint(const int edgeId, const int vertex) const
{
    return edges[edgeId].u == vertex ? edges[edgeId].v : edges[edgeId].u;
}

const std::vector<Adjacent> &Graph::neighborsOf(const int vertex) const
{
    return adjacency[vertex];
}

int Graph::degreeOf(const int vertex) const
{
    return static_cast<int>(adjacency[vertex].size());
}

bool Graph::load(const std::string &fileName)
{
    std::ifstream in(fileName);
    if (!in)
    {
        return false;
    }

    int vertexCount = 0;
    int edgeCount = 0;
    int sourceVertex = 0;

    // Read and check the header before resetting the graph.
    // A DIMACS file starts with text so it fails this read.
    if (!(in >> vertexCount >> edgeCount >> sourceVertex))
    {
        return false;
    }

    // We need at least one vertex to have a valid fixed source.
    if (vertexCount <= 0 || edgeCount < 0 ||
        sourceVertex < 0 || sourceVertex >= vertexCount)
    {
        return false;
    }

    reset(vertexCount, sourceVertex);

    for (int i = 0; i < edgeCount; i++)
    {
        int u = 0;
        int v = 0;
        long long weight = 0;

        // Stops if an edge is missing or a value cannot be read.
        if (!(in >> u >> v >> weight))
        {
            return false;
        }

        // This is the if statement that checks u and v while loading,
        // prevents out-of-bounds access to adjacency[u] or adjacency[v].
        if (u < 0 || u >= vertexCount || v < 0 || v >= vertexCount)
        {
            return false;
        }

        // Our graph files must use positive edge weights.
        if (weight <= 0)
        {
            return false;
        }

        // The conversion tool should have removed self-loops.
        if (u == v)
        {
            return false;
        }

        addEdge(u, v, weight);
    }

    return true;
}

bool Graph::save(const std::string &fileName) const
{
    std::ofstream out(fileName);
    if (!out)
    {
        return false;
    }

    out << getVertexCount() << " " << getEdgeCount() << " "
        << source << "\n";

    for (const Edge &edge : edges)
    {
        out << edge.u << " " << edge.v << " " << edge.weight << "\n";
    }

    // Closing also flushes buffered output then checks for write errors after it.
    out.close();
    return !out.fail();
}