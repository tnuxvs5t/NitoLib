#pragma once
#include "func.hpp"

struct nto_self {
    template <class E>
    constexpr decltype(auto) operator()(E&& edge) const {
        return forward<E>(edge);
    }
};

struct nordinal {
    template <class V>
    constexpr int operator()(V&& vertex) const { return int(vertex); }
};

/*
Minimal graph descriptor.  vertices enumerates semantic vertex keys, next(vertex)
returns any range-for compatible adjacency object, and to(edge) returns its target key.
Algorithms additionally receive index(key)->[0,vertices.len()) when keys are not dense
integers.  No graph type, iterator category, edge record or ownership model is imposed.
*/
template <class V, class N, class To = nto_self>
struct ngraph {
    V vertices;
    mutable N next;
    mutable To to{};

    template <class K>
    constexpr decltype(auto) edges(K&& vertex) const {
        return invoke(next, forward<K>(vertex));
    }

    template <class E>
    constexpr decltype(auto) target(E&& edge) const {
        return invoke(to, forward<E>(edge));
    }
};

template <class V, class N>
ngraph(V, N) -> ngraph<V, N>;

template <class V, class N, class To>
ngraph(V, N, To) -> ngraph<V, N, To>;

/* Distances are stored by dense position.  Duplicate sources are harmless. */
template <class G, class R, class Id>
vector<int> nbfs_many(G&& graph, R sources, Id index) {
    vector<int> distance(graph.vertices.len(), -1), queue;
    queue.reserve(graph.vertices.len());
    for (int i = 0; i < sources.len(); ++i) {
        int source = invoke(index, sources[i]);
        if (distance[source] < 0) distance[source] = 0, queue.push_back(source);
    }
    for (int at = 0; at < int(queue.size()); ++at) {
        int from = queue[at];
        for (auto&& edge : graph.edges(graph.vertices[from])) {
            int to = invoke(index, graph.target(edge));
            if (distance[to] < 0) distance[to] = distance[from] + 1, queue.push_back(to);
        }
    }
    return distance;
}

template <class G, class R>
vector<int> nbfs_many(G&& graph, R sources) {
    return nbfs_many(forward<G>(graph), move(sources), nordinal{});
}

template <class G, class K, class Id = nordinal>
vector<int> nbfs(G&& graph, K source, Id index = {}) {
    auto sources = ntabulate(1, [source = move(source)](int) mutable -> decltype(auto) {
        return (source);
    });
    return nbfs_many(forward<G>(graph), move(sources), move(index));
}

/*
A rooted projection owns only traversal metadata and its vertex/index descriptors.
Its nfunc/nview accessors borrow *this and therefore expire when it moves or dies.
parent[root]==root; unseen vertices have parent/depth/component/subtree == -1/0.
*/
template <class V, class Id>
struct nrooted {
    V vertices;
    mutable Id index;
    vector<int> parent_position, depth_value, component_position;
    vector<int> preorder_position, subtree_value, root_position;
    vector<int> child_offset, child_position;

    int len() const { return vertices.len(); }

    auto keys() const {
        return ntabulate(len(), [this](int i) -> decltype(auto) { return vertices[i]; });
    }

    auto locate() const {
        return [this](auto&& key) {
            return invoke(index, forward<decltype(key)>(key));
        };
    }

    auto positions() const { return nfunc{keys(), locate()}; }

    auto parents() const {
        return nmap_values(nfunc_bind(keys(), nall(parent_position), locate()),
                           [this](int position) -> decltype(auto) {
                               return vertices[position];
                           });
    }

    auto depths() const {
        return nfunc_bind(keys(), nall(depth_value), locate());
    }

    auto components() const {
        return nmap_values(nfunc_bind(keys(), nall(component_position), locate()),
                           [this](int position) -> decltype(auto) {
                               return vertices[position];
                           });
    }

    auto subtree_sizes() const {
        return nfunc_bind(keys(), nall(subtree_value), locate());
    }

    auto order() const {
        return nproject(nall(preorder_position),
                        [this](int position) -> decltype(auto) { return vertices[position]; });
    }

    auto roots() const {
        return nproject(nall(root_position),
                        [this](int position) -> decltype(auto) { return vertices[position]; });
    }

    template <class K>
    auto children(K&& key) const {
        int position = invoke(index, forward<K>(key));
        return nproject(nsub(nall(child_position), child_offset[position],
                             child_offset[position + 1]),
                        [this](int child) -> decltype(auto) { return vertices[child]; });
    }
};

/*
nroot builds a first-discovery forest from the supplied roots.  It is valid on directed
or cyclic graphs: already discovered arcs are ignored.  Tree algorithms that interpret
subtree metadata as original-tree structure must separately rely on the input being a
forest.  Roots need not cover every vertex; uncovered metadata remains unseen.
*/
template <class G, class R, class Id>
auto nroot(G graph, R roots, Id index) {
    int n = graph.vertices.len();
    vector<int> parent(n, -1), depth(n, -1), component(n, -1), order, subtree(n);
    vector<int> root_positions;
    order.reserve(n);
    for (int i = 0; i < roots.len(); ++i) {
        int root = invoke(index, roots[i]);
        if (parent[root] >= 0) continue;
        parent[root] = root;
        depth[root] = 0;
        component[root] = root;
        root_positions.push_back(root);
        vector<int> stack{root};
        while (!stack.empty()) {
            int from = stack.back();
            stack.pop_back();
            order.push_back(from);
            for (auto&& edge : graph.edges(graph.vertices[from])) {
                int to = invoke(index, graph.target(edge));
                if (parent[to] >= 0) continue;
                parent[to] = from;
                depth[to] = depth[from] + 1;
                component[to] = root;
                stack.push_back(to);
            }
        }
    }
    for (auto it = order.rbegin(); it != order.rend(); ++it) {
        int vertex = *it;
        subtree[vertex] += 1;
        if (parent[vertex] != vertex) subtree[parent[vertex]] += subtree[vertex];
    }
    vector<int> child_offset(n + 1), child_position;
    child_position.reserve(order.size() - root_positions.size());
    for (int vertex : order)
        if (parent[vertex] != vertex) ++child_offset[parent[vertex] + 1];
    partial_sum(child_offset.begin(), child_offset.end(), child_offset.begin());
    vector<int> cursor = child_offset;
    child_position.resize(child_offset.back());
    for (int vertex : order)
        if (parent[vertex] != vertex) child_position[cursor[parent[vertex]]++] = vertex;
    using V = decltype(graph.vertices);
    return nrooted<V, Id>{move(graph.vertices), move(index), move(parent), move(depth),
                          move(component), move(order), move(subtree), move(root_positions),
                          move(child_offset), move(child_position)};
}

template <class G, class R>
auto nroot(G graph, R roots) {
    return nroot(move(graph), move(roots), nordinal{});
}
