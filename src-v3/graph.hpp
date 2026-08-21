#pragma once
#include "func.hpp"

struct nto_self {
    template <class E>
    constexpr decltype(auto) operator()(E&& edge) const {
        return forward<E>(edge);
    }
};

/*
Minimal graph descriptor.  vertices enumerates semantic vertex keys, next(vertex)
returns any range-for compatible adjacency object, and to(edge) returns its target key.
vertices.inverse(key) returns its dense position.  No graph type, iterator category,
edge record or ownership model is imposed.
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
template <class G, class R>
vector<nidx_t> nbfs_many(G&& graph, R sources) {
    vector<nidx_t> distance(graph.vertices.len(), -1), queue;
    queue.reserve(graph.vertices.len());
    for (nidx_t i = 0; i < sources.len(); ++i) {
        nidx_t source = graph.vertices.inverse(sources[i]);
        if (distance[source] < 0) distance[source] = 0, queue.push_back(source);
    }
    for (nidx_t at = 0; at < nidx_t(queue.size()); ++at) {
        nidx_t from = queue[at];
        for (auto&& edge : graph.edges(graph.vertices[from])) {
            nidx_t to = graph.vertices.inverse(graph.target(edge));
            if (distance[to] < 0) distance[to] = distance[from] + 1, queue.push_back(to);
        }
    }
    return distance;
}

template <class G, class K>
vector<nidx_t> nbfs(G&& graph, K source) {
    auto sources = ntabulate(1, [source = move(source)](nidx_t) mutable -> decltype(auto) {
        return (source);
    });
    return nbfs_many(forward<G>(graph), move(sources));
}

/*
A rooted projection owns only traversal metadata and its invertible vertex descriptor.
Its nfunc/nview accessors borrow *this and therefore expire when it moves or dies.
parent[root]==root; unseen vertices have parent/depth/component/subtree == -1/0.
*/
template <class V>
struct nrooted {
    V vertices;
    vector<nidx_t> parent_position, depth_value, component_position;
    vector<nidx_t> preorder_position, subtree_value, root_position;
    vector<nidx_t> child_offset, child_position;

    nidx_t len() const { return vertices.len(); }

    auto keys() const {
        return ntabulate(
            len(),
            [this](nidx_t i) -> decltype(auto) { return vertices[i]; },
            [this](auto&& key) {
                return vertices.inverse(forward<decltype(key)>(key));
            }
        );
    }

    auto locate() const {
        return nlocate(vertices);
    }

    auto positions() const { return nfunc{keys(), locate()}; }

    auto parents() const {
        return nmap_values(nanchors(keys(), nall(parent_position)),
                           [this](nidx_t position) -> decltype(auto) {
                               return vertices[position];
                           });
    }

    auto depths() const {
        return nanchors(keys(), nall(depth_value));
    }

    auto components() const {
        return nmap_values(nanchors(keys(), nall(component_position)),
                           [this](nidx_t position) -> decltype(auto) {
                               return vertices[position];
                           });
    }

    auto subtree_sizes() const {
        return nanchors(keys(), nall(subtree_value));
    }

    auto order() const {
        return nproject(nall(preorder_position),
                        [this](nidx_t position) -> decltype(auto) { return vertices[position]; });
    }

    auto roots() const {
        return nproject(nall(root_position),
                        [this](nidx_t position) -> decltype(auto) { return vertices[position]; });
    }

    template <class K>
    auto children(K&& key) const {
        nidx_t position = vertices.inverse(forward<K>(key));
        return nproject(nsub(nall(child_position), child_offset[position],
                             child_offset[position + 1]),
                        [this](nidx_t child) -> decltype(auto) { return vertices[child]; });
    }
};

/*
nroot builds a first-discovery forest from the supplied roots.  It is valid on directed
or cyclic graphs: already discovered arcs are ignored.  Tree algorithms that interpret
subtree metadata as original-tree structure must separately rely on the input being a
forest.  Roots need not cover every vertex; uncovered metadata remains unseen.
*/
template <class G, class R>
auto nroot(G graph, R roots) {
    nidx_t n = graph.vertices.len();
    vector<nidx_t> parent(n, -1), depth(n, -1), component(n, -1), order, subtree(n);
    vector<nidx_t> root_positions;
    order.reserve(n);
    for (nidx_t i = 0; i < roots.len(); ++i) {
        nidx_t root = graph.vertices.inverse(roots[i]);
        if (parent[root] >= 0) continue;
        parent[root] = root;
        depth[root] = 0;
        component[root] = root;
        root_positions.push_back(root);
        vector<nidx_t> stack{root};
        while (!stack.empty()) {
            nidx_t from = stack.back();
            stack.pop_back();
            order.push_back(from);
            for (auto&& edge : graph.edges(graph.vertices[from])) {
                nidx_t to = graph.vertices.inverse(graph.target(edge));
                if (parent[to] >= 0) continue;
                parent[to] = from;
                depth[to] = depth[from] + 1;
                component[to] = root;
                stack.push_back(to);
            }
        }
    }
    for (auto it = order.rbegin(); it != order.rend(); ++it) {
        nidx_t vertex = *it;
        subtree[vertex] += 1;
        if (parent[vertex] != vertex) subtree[parent[vertex]] += subtree[vertex];
    }
    vector<nidx_t> child_offset(n + 1), child_position;
    child_position.reserve(order.size() - root_positions.size());
    for (nidx_t vertex : order)
        if (parent[vertex] != vertex) ++child_offset[parent[vertex] + 1];
    partial_sum(child_offset.begin(), child_offset.end(), child_offset.begin());
    vector<nidx_t> cursor = child_offset;
    child_position.resize(child_offset.back());
    for (nidx_t vertex : order)
        if (parent[vertex] != vertex) child_position[cursor[parent[vertex]]++] = vertex;
    using V = decltype(graph.vertices);
    return nrooted<V>{move(graph.vertices), move(parent), move(depth), move(component),
                      move(order), move(subtree), move(root_positions), move(child_offset),
                      move(child_position)};
}
