#pragma once
#include "graph.hpp"

struct npath_piece {
    int left, right;
    bool reverse;
};

/*
HLD metadata owns dense arrays but only owns vertex/index descriptors, not their
referents.  lca/path require vertices in one component.  path returns [left,right)
pieces in traversal order from the first vertex to the second; reverse means read that
base interval right-to-left.  This preserves noncommutative vertex-path aggregates.
*/
template <class V, class Id>
struct nhld_layout {
    V vertices;
    mutable Id index;
    vector<int> parent_position, depth_value, subtree_value, heavy_position;
    vector<int> head_position, position_value, vertex_at_position, root_position;

    int len() const { return vertices.len(); }
    auto keys() const {
        return ntabulate(len(), [this](int i) -> decltype(auto) { return vertices[i]; });
    }
    auto locate() const {
        return [this](auto&& key) {
            return invoke(index, forward<decltype(key)>(key));
        };
    }
    auto positions() const { return nfunc_bind(keys(), nall(position_value), locate()); }
    auto depths() const { return nfunc_bind(keys(), nall(depth_value), locate()); }
    auto subtree_sizes() const { return nfunc_bind(keys(), nall(subtree_value), locate()); }

    auto parents() const {
        return nmap_values(nfunc_bind(keys(), nall(parent_position), locate()),
                           [this](int position) -> decltype(auto) {
                               return vertices[position];
                           });
    }

    auto heads() const {
        return nmap_values(nfunc_bind(keys(), nall(head_position), locate()),
                           [this](int position) -> decltype(auto) {
                               return vertices[position];
                           });
    }

    auto order() const {
        return nproject(nall(vertex_at_position),
                        [this](int position) -> decltype(auto) { return vertices[position]; });
    }

    template <class X, class Y>
    decltype(auto) lca(X&& x, Y&& y) const {
        int a = invoke(index, forward<X>(x)), b = invoke(index, forward<Y>(y));
        while (head_position[a] != head_position[b]) {
            if (depth_value[head_position[a]] < depth_value[head_position[b]]) swap(a, b);
            a = parent_position[head_position[a]];
        }
        return vertices[depth_value[a] < depth_value[b] ? a : b];
    }

    template <class X, class Y>
    vector<npath_piece> path(X&& x, Y&& y) const {
        int a = invoke(index, forward<X>(x)), b = invoke(index, forward<Y>(y));
        vector<npath_piece> left, right;
        while (head_position[a] != head_position[b]) {
            if (depth_value[head_position[a]] >= depth_value[head_position[b]]) {
                left.push_back({position_value[head_position[a]], position_value[a] + 1, true});
                a = parent_position[head_position[a]];
            } else {
                right.push_back({position_value[head_position[b]], position_value[b] + 1, false});
                b = parent_position[head_position[b]];
            }
        }
        if (depth_value[a] >= depth_value[b])
            left.push_back({position_value[b], position_value[a] + 1, true});
        else
            right.push_back({position_value[a], position_value[b] + 1, false});
        while (!right.empty()) left.push_back(right.back()), right.pop_back();
        return left;
    }
};

/*
The construction port is vertices, roots, children(vertex), index(vertex).  children
must describe a rooted forest, be repeatable, and enumerate every non-root exactly once.
No concrete graph/tree owner, parent array type or adjacency representation is required.
*/
template <class V, class R, class C, class Id>
auto nhld(V vertices, R roots, C children, Id index) {
    int n = vertices.len();
    vector<int> parent(n, -1), depth(n), subtree(n, 1), heavy(n, -1), traversal, root_position;
    traversal.reserve(n);
    for (int i = 0; i < roots.len(); ++i) {
        int root = invoke(index, roots[i]);
        root_position.push_back(root);
        parent[root] = root;
        vector<int> stack{root};
        while (!stack.empty()) {
            int from = stack.back();
            stack.pop_back();
            traversal.push_back(from);
            for (auto&& child_key : invoke(children, vertices[from])) {
                int child = invoke(index, child_key);
                parent[child] = from;
                depth[child] = depth[from] + 1;
                stack.push_back(child);
            }
        }
    }
    for (auto it = traversal.rbegin(); it != traversal.rend(); ++it) {
        int vertex = *it;
        if (parent[vertex] == vertex) continue;
        int p = parent[vertex];
        subtree[p] += subtree[vertex];
        if (heavy[p] < 0 || subtree[heavy[p]] < subtree[vertex]) heavy[p] = vertex;
    }

    vector<int> head(n), position(n), at(n);
    int timer = 0;
    for (int root : root_position) {
        vector<pair<int, int>> tasks{{root, root}};
        while (!tasks.empty()) {
            auto [start, chain] = tasks.back();
            tasks.pop_back();
            for (int vertex = start; vertex >= 0; vertex = heavy[vertex]) {
                head[vertex] = chain;
                position[vertex] = timer;
                at[timer++] = vertex;
                for (auto&& child_key : invoke(children, vertices[vertex])) {
                    int child = invoke(index, child_key);
                    if (child != heavy[vertex]) tasks.emplace_back(child, child);
                }
            }
        }
    }
    return nhld_layout<V, Id>{move(vertices), move(index), move(parent), move(depth),
                              move(subtree), move(heavy), move(head), move(position),
                              move(at), move(root_position)};
}

template <class V, class Id>
auto nhld(const nrooted<V, Id>& tree) {
    auto children = [&tree](auto&& vertex) {
        return tree.children(forward<decltype(vertex)>(vertex));
    };
    return nhld(tree.keys(), tree.roots(), move(children), tree.locate());
}

/*
Rerooting over a symmetric forest graph.  Every undirected edge appears once in each
direction and adjacency is repeatable.  merge supplies id() and is associative; order
is the local adjacency order, so commutativity is not required.  base(vertex) creates
the vertex state.  lift(state,from,edge_from_to) maps the aggregate at from with `to`
excluded into its contribution to `to`.  Returns answers by dense vertex position.
*/
template <class G, class Base, class Lift, class M, class Id = nordinal>
auto nreroot(G&& graph, Base base, Lift lift, M merge, Id index = {}) {
    using S = remove_cvref_t<decltype(invoke(base, graph.vertices[0]))>;
    int n = graph.vertices.len();
    vector<int> parent(n, -1), order;
    order.reserve(n);
    for (int source = 0; source < n; ++source) {
        if (parent[source] >= 0) continue;
        parent[source] = source;
        vector<int> stack{source};
        while (!stack.empty()) {
            int from = stack.back();
            stack.pop_back();
            order.push_back(from);
            for (auto&& edge : graph.edges(graph.vertices[from])) {
                int to = invoke(index, graph.target(edge));
                if (parent[to] < 0) parent[to] = from, stack.push_back(to);
            }
        }
    }

    vector<S> toward_parent(n, merge.id());
    for (auto it = order.rbegin(); it != order.rend(); ++it) {
        int from = *it;
        S state = invoke(base, graph.vertices[from]);
        for (auto&& edge : graph.edges(graph.vertices[from])) {
            int to = invoke(index, graph.target(edge));
            if (parent[to] == from) state = invoke(merge, move(state), toward_parent[to]);
        }
        if (parent[from] != from)
            for (auto&& edge : graph.edges(graph.vertices[from]))
                if (invoke(index, graph.target(edge)) == parent[from]) {
                    toward_parent[from] = invoke(lift, state, graph.vertices[from], edge);
                    break;
                }
    }

    vector<S> from_parent(n, merge.id()), answer(n, merge.id());
    for (int from : order) {
        vector<S> contribution;
        for (auto&& edge : graph.edges(graph.vertices[from])) {
            int to = invoke(index, graph.target(edge));
            contribution.push_back(to == parent[from] ? from_parent[from] : toward_parent[to]);
        }
        int degree = int(contribution.size());
        vector<S> suffix(degree + 1, merge.id());
        for (int i = degree; i--;) suffix[i] = invoke(merge, contribution[i], suffix[i + 1]);
        S prefix = invoke(base, graph.vertices[from]);
        int position = 0;
        for (auto&& edge : graph.edges(graph.vertices[from])) {
            int to = invoke(index, graph.target(edge));
            if (parent[to] == from) {
                S without = invoke(merge, prefix, suffix[position + 1]);
                from_parent[to] = invoke(lift, without, graph.vertices[from], edge);
            }
            prefix = invoke(merge, move(prefix), contribution[position++]);
        }
        answer[from] = move(prefix);
    }
    return answer;
}
