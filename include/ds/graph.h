#ifndef DS_GRAPH_H
#define DS_GRAPH_H

#include "array.h"
#include "queue.h"

class Graph {
    int nodeCount_;
    Array<Array<int>> adj_;

public:
    explicit Graph(int nodeCount = 0) : nodeCount_(nodeCount) {
        for (int i = 0; i < nodeCount_; ++i) {
            adj_.push(Array<int>());
        }
    }

    void addEdge(int from, int to, bool bidirectional = true) {
        adj_[from].push(to);
        if (bidirectional) {
            adj_[to].push(from);
        }
    }

    int nodeCount() const {
        return nodeCount_;
    }

    const Array<int>& neighbors(int node) const {
        return adj_[node];
    }

    Array<int> bfs(int start) const {
        Array<int> order;
        Array<int> visited;
        for (int i = 0; i < nodeCount_; ++i) {
            visited.push(0);
        }
        Queue<int> pending;
        pending.enqueue(start);
        visited[start] = 1;
        while (!pending.empty()) {
            const int node = pending.front();
            pending.dequeue();
            order.push(node);
            const Array<int>& edges = adj_[node];
            for (int i = 0; i < edges.size(); ++i) {
                const int next = edges[i];
                if (visited[next] == 0) {
                    visited[next] = 1;
                    pending.enqueue(next);
                }
            }
        }
        return order;
    }

    Array<int> bfsPath(int start, int goal) const {
        Array<int> path;
        if (start < 0 || goal < 0 || start >= nodeCount_ || goal >= nodeCount_) {
            return path;
        }

        Array<int> visited;
        Array<int> parent;
        for (int i = 0; i < nodeCount_; ++i) {
            visited.push(0);
            parent.push(-1);
        }

        Queue<int> pending;
        pending.enqueue(start);
        visited[start] = 1;
        while (!pending.empty()) {
            const int node = pending.front();
            pending.dequeue();
            if (node == goal) {
                break;
            }
            const Array<int>& edges = adj_[node];
            for (int i = 0; i < edges.size(); ++i) {
                const int next = edges[i];
                if (visited[next] == 0) {
                    visited[next] = 1;
                    parent[next] = node;
                    pending.enqueue(next);
                }
            }
        }

        if (visited[goal] == 0) {
            return path;
        }

        Array<int> reverse;
        int node = goal;
        while (node != -1) {
            reverse.push(node);
            if (node == start) {
                break;
            }
            node = parent[node];
        }
        for (int i = reverse.size() - 1; i >= 0; --i) {
            path.push(reverse[i]);
        }
        return path;
    }
};

#endif
