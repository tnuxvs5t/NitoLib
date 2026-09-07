#pragma once
#include "graph.hpp"

struct npath_piece {
    nidx_t left, right;
    bool reverse;
};

/*
HLD metadata owns dense arrays but only owns one invertible vertex descriptor, not its
referents.  lca/path require vertices in one component.  path returns [left,right)
pieces in traversal order from the first vertex to the second; reverse means read that
base interval right-to-left.  This preserves noncommutative vertex-path aggregates.
*/
template <class V>
struct nhld_layout {
    V vertices;
    vector<nidx_t> parent_position, depth_value, subtree_value, heavy_position;
    vector<nidx_t> head_position, position_value, vertex_at_position, root_position;

    nidx_t len() const { return vertices.len(); }
    auto keys() const { return nall(vertices); }
    auto locate() const {
        return nlocate(vertices);
    }
    auto positions() const { return nanchors(keys(), nall(position_value)); }
    auto depths() const { return nanchors(keys(), nall(depth_value)); }
    auto subtree_sizes() const { return nanchors(keys(), nall(subtree_value)); }

    auto parents() const {
        return nmap_values(nanchors(keys(), nall(parent_position)),
                           [this](nidx_t position) -> decltype(auto) {
                               return vertices[position];
                           });
    }

    auto heads() const {
        return nmap_values(nanchors(keys(), nall(head_position)),
                           [this](nidx_t position) -> decltype(auto) {
                               return vertices[position];
                           });
    }

    auto order() const {
        return nproject(nall(vertex_at_position),
                        [this](nidx_t position) -> decltype(auto) { return vertices[position]; });
    }

    template <class X, class Y>
    decltype(auto) lca(X&& x, Y&& y) const {
        nidx_t a = vertices.inverse(forward<X>(x)), b = vertices.inverse(forward<Y>(y));
        while (head_position[a] != head_position[b]) {
            if (depth_value[head_position[a]] < depth_value[head_position[b]]) swap(a, b);
            a = parent_position[head_position[a]];
        }
        return vertices[depth_value[a] < depth_value[b] ? a : b];
    }

    /* Ordered streaming path decomposition, without heap allocation. */
    template <class X, class Y, class F>
    void visit_path(X&& x, Y&& y, F visit) const {
        nidx_t a = vertices.inverse(forward<X>(x)), b = vertices.inverse(forward<Y>(y));
        array<npath_piece, numeric_limits<nidx_t>::digits + 1> right;
        nidx_t count = 0;
        while (head_position[a] != head_position[b]) {
            if (depth_value[head_position[a]] >= depth_value[head_position[b]]) {
                invoke(visit, npath_piece{position_value[head_position[a]], position_value[a] + 1, true});
                a = parent_position[head_position[a]];
            } else {
                right[count++] = {position_value[head_position[b]], position_value[b] + 1, false};
                b = parent_position[head_position[b]];
            }
        }
        if (depth_value[a] >= depth_value[b])
            invoke(visit, npath_piece{position_value[b], position_value[a] + 1, true});
        else
            right[count++] = {position_value[a], position_value[b] + 1, false};
        while (count) invoke(visit, right[--count]);
    }

    template <class X, class Y>
    vector<npath_piece> path(X&& x, Y&& y) const {
        vector<npath_piece> result;
        visit_path(forward<X>(x), forward<Y>(y), [&](npath_piece piece) { result.push_back(piece); });
        return result;
    }
};

namespace nhld_detail {
template <class V, class C>
auto layout(V&& vertices, vector<nidx_t> roots, C children,
            vector<nidx_t> parent, vector<nidx_t> depth, vector<nidx_t> subtree,
            vector<nidx_t> heavy) {
    nidx_t n = vertices.len(), timer = 0;
    vector<nidx_t> head(n), position(n), at(n);
    for (nidx_t root : roots) {
        vector<pair<nidx_t, nidx_t>> tasks{{root, root}};
        while (!tasks.empty()) {
            auto [start, chain] = tasks.back();
            tasks.pop_back();
            for (nidx_t vertex = start; vertex >= 0; vertex = heavy[vertex]) {
                head[vertex] = chain;
                position[vertex] = timer;
                at[timer++] = vertex;
                invoke(children, vertex, [&](nidx_t child) {
                    if (child != heavy[vertex]) tasks.emplace_back(child, child);
                });
            }
        }
    }
    return nhld_layout<remove_cvref_t<V>>{forward<V>(vertices), move(parent), move(depth),
        move(subtree), move(heavy), move(head), move(position), move(at), move(roots)};
}

template <class R, class V>
auto rooted(R&& tree, V&& vertices) {
    vector<nidx_t> heavy(tree.len(), -1);
    for (nidx_t child : tree.child_position) {
        nidx_t parent = tree.parent_position[child];
        if (heavy[parent] < 0 || tree.subtree_value[heavy[parent]] < tree.subtree_value[child])
            heavy[parent] = child;
    }
    auto children = [&](nidx_t position, auto visit) {
        for (nidx_t child : nsub(nall(tree.child_position), tree.child_offset[position],
                                 tree.child_offset[position + 1])) invoke(visit, child);
    };
    return layout(forward<V>(vertices), forward<R>(tree).root_position, children,
                  forward<R>(tree).parent_position, forward<R>(tree).depth_value,
                  forward<R>(tree).subtree_value, move(heavy));
}
}

/*
The construction port is invertible vertices, roots and children(vertex).  children
must describe a rooted forest, be repeatable, and enumerate every non-root exactly once.
No concrete graph/tree owner, parent array type or adjacency representation is required.
*/
template <class V, class R, class C>
auto nhld(V vertices, R roots, C children) {
    nidx_t n = vertices.len();
    vector<nidx_t> parent(n, -1), depth(n), subtree(n, 1), heavy(n, -1), traversal, root_position;
    traversal.reserve(n);
    for (nidx_t i = 0; i < roots.len(); ++i) {
        nidx_t root = vertices.inverse(roots[i]);
        root_position.push_back(root);
        parent[root] = root;
        vector<nidx_t> stack{root};
        while (!stack.empty()) {
            nidx_t from = stack.back();
            stack.pop_back();
            traversal.push_back(from);
            for (auto&& child_key : invoke(children, vertices[from])) {
                nidx_t child = vertices.inverse(child_key);
                parent[child] = from;
                depth[child] = depth[from] + 1;
                stack.push_back(child);
            }
        }
    }
    for (auto it = traversal.rbegin(); it != traversal.rend(); ++it) {
        nidx_t vertex = *it;
        if (parent[vertex] == vertex) continue;
        nidx_t p = parent[vertex];
        subtree[p] += subtree[vertex];
        if (heavy[p] < 0 || subtree[heavy[p]] < subtree[vertex]) heavy[p] = vertex;
    }

    auto child_positions = [&](nidx_t vertex, auto visit) {
        for (auto&& key : invoke(children, vertices[vertex]))
            invoke(visit, vertices.inverse(key));
    };
    return nhld_detail::layout(move(vertices), move(root_position), child_positions,
                               move(parent), move(depth), move(subtree), move(heavy));
}

template <class V>
auto nhld(const nrooted<V>& tree) {
    return nhld_detail::rooted(tree, tree.keys());
}

/* Consume rooted metadata; the result no longer borrows the nrooted object. */
template <class V>
auto nhld(nrooted<V>&& tree) {
    return nhld_detail::rooted(move(tree), move(tree.vertices));
}

/*
Rerooting over a symmetric forest graph.  Every undirected edge appears once in each
direction and adjacency is repeatable.  merge supplies id() and is associative; order
is the local adjacency order, so commutativity is not required.  base(vertex) creates
the vertex state.  lift(state,from,edge_from_to) maps the aggregate at from with `to`
excluded into its contribution to `to`.  Returns answers by dense vertex position.
*/
template <class G, class Base, class Lift, class M>
auto nreroot(G&& graph, Base base, Lift lift, M merge) {
    using S = remove_cvref_t<decltype(invoke(base, graph.vertices[0]))>;
    nidx_t n = graph.vertices.len();
    vector<nidx_t> parent(n, -1), order;
    order.reserve(n);
    for (nidx_t source = 0; source < n; ++source) {
        if (parent[source] >= 0) continue;
        parent[source] = source;
        vector<nidx_t> stack{source};
        while (!stack.empty()) {
            nidx_t from = stack.back();
            stack.pop_back();
            order.push_back(from);
            for (auto&& edge : graph.edges(graph.vertices[from])) {
                nidx_t to = graph.vertices.inverse(graph.target(edge));
                if (parent[to] < 0) parent[to] = from, stack.push_back(to);
            }
        }
    }

    vector<S> toward_parent(n, merge.id());
    for (auto it = order.rbegin(); it != order.rend(); ++it) {
        nidx_t from = *it;
        S state = invoke(base, graph.vertices[from]);
        for (auto&& edge : graph.edges(graph.vertices[from])) {
            nidx_t to = graph.vertices.inverse(graph.target(edge));
            if (parent[to] == from) state = invoke(merge, move(state), toward_parent[to]);
        }
        if (parent[from] != from)
            for (auto&& edge : graph.edges(graph.vertices[from]))
                if (graph.vertices.inverse(graph.target(edge)) == parent[from]) {
                    toward_parent[from] = invoke(lift, state, graph.vertices[from], edge);
                    break;
                }
    }

    vector<S> from_parent(n, merge.id()), answer(n, merge.id());
    for (nidx_t from : order) {
        vector<S> contribution;
        for (auto&& edge : graph.edges(graph.vertices[from])) {
            nidx_t to = graph.vertices.inverse(graph.target(edge));
            contribution.push_back(to == parent[from] ? from_parent[from] : toward_parent[to]);
        }
        nidx_t degree = nidx_t(contribution.size());
        vector<S> suffix(degree + 1, merge.id());
        for (nidx_t i = degree; i--;) suffix[i] = invoke(merge, contribution[i], suffix[i + 1]);
        S prefix = invoke(base, graph.vertices[from]);
        nidx_t position = 0;
        for (auto&& edge : graph.edges(graph.vertices[from])) {
            nidx_t to = graph.vertices.inverse(graph.target(edge));
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
