#pragma once
#include "graph.hpp"
#include "func.hpp"

/*
A rooted projection owns only traversal metadata and its invertible vertex descriptor.
Its nfunc/nview accessors borrow *this and therefore expire when it moves or dies.
parent[root]==root; unseen vertices have parent/depth/component/subtree == -1/0.
Key-valued parents()/components() are only defined for covered vertices.
*/
template <class V>
struct nrooted {
    V vertices;
    vector<nidx_t> parent_position, depth_value, component_position;
    vector<nidx_t> preorder_position, subtree_value, root_position;
    vector<nidx_t> child_offset, child_position;

    nidx_t len() const { return vertices.len(); }

    auto keys() const { return nall(vertices); }

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
Discovery follows recursive DFS adjacency order and uses O(height) call stack.
Outstanding adjacency ranges must survive nested graph.edges calls.
*/
template <class G, class R>
auto nroot(G graph, R&& roots) {
    nidx_t n = graph.vertices.len();
    vector<nidx_t> parent(n, -1), depth(n, -1), component(n, -1), order, subtree(n);
    vector<nidx_t> root_positions;
    order.reserve(n);
    auto dfs = [&](auto&& self, nidx_t from) -> void {
        order.push_back(from);
        subtree[from] = 1;
        for (auto&& edge : graph.edges(graph.vertices[from])) {
            nidx_t to = graph.vertices.inverse(graph.target(edge));
            if (parent[to] >= 0) continue;
            parent[to] = from;
            depth[to] = depth[from] + 1;
            component[to] = component[from];
            self(self, to);
            subtree[from] += subtree[to];
        }
    };
    for (nidx_t i = 0; i < nlen(roots); ++i) {
        nidx_t root = graph.vertices.inverse(roots[i]);
        if (parent[root] >= 0) continue;
        parent[root] = root;
        depth[root] = 0;
        component[root] = root;
        root_positions.push_back(root);
        dfs(dfs, root);
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
