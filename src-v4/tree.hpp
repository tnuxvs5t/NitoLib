#pragma once
#include "rooted.hpp"

struct npath_piece {
    nidx_t left, right;
    // reverse=true means consume this HLD segment right-to-left.
    bool reverse;
};

template <class V>
struct nhld_layout {
    // Projections borrow this layout; lca/path endpoints must be reached vertices
    // in the same component. Construction uses O(height) recursive call stack.
    V vertices;
    vector<nidx_t> par, dep, sz, heavy;
    vector<nidx_t> head, pos, at, rt;

    nidx_t len() const { return vertices.len(); }
    auto keys() const { return nall(vertices); }
    auto locate() const { return nlocate(vertices); }

    pair<nidx_t, nidx_t> subtree(nidx_t v) const {
        return {pos[v], pos[v] + sz[v]};
    }

    auto positions() const { return nanchors(keys(), nall(pos), locate()); }
    auto depths() const { return nanchors(keys(), nall(dep), locate()); }
    auto subtree_sizes() const { return nanchors(keys(), nall(sz), locate()); }

    auto parents() const {
        return nmap_values(nanchors(keys(), nall(par), locate()),
                           [this](nidx_t v) -> decltype(auto) { return vertices[v]; });
    }

    auto heads() const {
        return nmap_values(nanchors(keys(), nall(head), locate()),
                           [this](nidx_t v) -> decltype(auto) { return vertices[v]; });
    }

    auto order() const {
        return nproject(nall(at),
                        [this](nidx_t v) -> decltype(auto) { return vertices[v]; });
    }

    auto roots() const {
        return nproject(nall(rt),
                        [this](nidx_t v) -> decltype(auto) { return vertices[v]; });
    }

    template <class X, class Y>
    decltype(auto) lca(X&& x, Y&& y) const {
        nidx_t a = vertices.inverse(forward<X>(x));
        nidx_t b = vertices.inverse(forward<Y>(y));
        while (head[a] != head[b]) {
            if (dep[head[a]] < dep[head[b]]) swap(a, b);
            a = par[head[a]];
        }
        return vertices[dep[a] < dep[b] ? a : b];
    }

    template <class X, class Y, class F>
    void visit_path(X&& x, Y&& y, F visit) const {
        nidx_t a = vertices.inverse(forward<X>(x));
        nidx_t b = vertices.inverse(forward<Y>(y));
        array<npath_piece, numeric_limits<nidx_t>::digits + 1> back;
        nidx_t count = 0;
        while (head[a] != head[b]) {
            if (dep[head[a]] >= dep[head[b]]) {
                invoke(visit, npath_piece{pos[head[a]], pos[a] + 1, true});
                a = par[head[a]];
            } else {
                back[count++] = {pos[head[b]], pos[b] + 1, false};
                b = par[head[b]];
            }
        }
        if (dep[a] >= dep[b])
            invoke(visit, npath_piece{pos[b], pos[a] + 1, true});
        else
            back[count++] = {pos[a], pos[b] + 1, false};
        while (count) invoke(visit, back[--count]);
    }

    template <class X, class Y>
    vector<npath_piece> path(X&& x, Y&& y) const {
        vector<npath_piece> result;
        visit_path(forward<X>(x), forward<Y>(y),
                   [&](npath_piece piece) { result.push_back(piece); });
        return result;
    }
};

template <class V, class R, class C>
requires requires(V& vertices) { vertices.len(); vertices.inverse(vertices[0]); }
auto nhld(V vertices, R roots, C children) {
    nidx_t n = vertices.len();
    vector<nidx_t> par(n, -1), dep(n, -1), sz(n), heavy(n, -1), rt, ord;
    rt.reserve(nlen(roots));
    ord.reserve(n);

    {
        auto dfs = [&](auto&& self, nidx_t v) -> void {
            ord.push_back(v);
            sz[v] = 1;
            for (auto&& key : invoke(children, vertices[v])) {
                nidx_t u = vertices.inverse(key);
                if (par[u] >= 0) continue;
                par[u] = v;
                dep[u] = dep[v] + 1;
                self(self, u);
                sz[v] += sz[u];
                if (heavy[v] < 0 || sz[heavy[v]] <= sz[u]) heavy[v] = u;
            }
        };

        for (nidx_t i = 0; i < nlen(roots); ++i) {
            nidx_t root = vertices.inverse(roots[i]);
            if (par[root] >= 0) continue;
            par[root] = root;
            dep[root] = 0;
            rt.push_back(root);
            dfs(dfs, root);
        }
    }

    vector<nidx_t> head(n, -1), pos(n, -1), at(ord.size());
    nidx_t timer = 0;
    {
        auto dfs = [&](auto&& self, nidx_t v, nidx_t top) -> void {
            head[v] = top;
            pos[v] = timer;
            at[timer++] = v;
            if (heavy[v] >= 0) self(self, heavy[v], top);
            for (auto&& key : invoke(children, vertices[v])) {
                nidx_t u = vertices.inverse(key);
                if (par[u] == v && u != heavy[v]) self(self, u, u);
            }
        };

        for (nidx_t root : rt) dfs(dfs, root, root);
    }
    at.resize(timer);
    return nhld_layout<remove_cvref_t<V>>{
        move(vertices), move(par), move(dep), move(sz), move(heavy),
        move(head), move(pos), move(at), move(rt)
    };
}

template <class V>
auto nhld(const nrooted<V>& tree) {
    return nhld(tree.vertices, tree.roots(),
                [&tree](auto&& key) { return tree.children(forward<decltype(key)>(key)); });
}

template <class V>
auto nhld(nrooted<V>&& tree) {
    V vertices = move(tree.vertices);
    nidx_t n = tree.n;
    auto par = move(tree.par), dep = move(tree.dep), sz = move(tree.sz);
    auto ord = move(tree.ord), rt = move(tree.rt);
    auto off = move(tree.off), ch = move(tree.ch);
    vector<nidx_t> heavy(n, -1);
    for (nidx_t v : ord) {
        if (par[v] == v) continue;
        nidx_t p = par[v];
        if (heavy[p] < 0 || sz[heavy[p]] <= sz[v]) heavy[p] = v;
    }

    vector<nidx_t> head(n, -1), pos(n, -1), at(ord.size());
    nidx_t timer = 0;
    {
        auto dfs = [&](auto&& self, nidx_t v, nidx_t top) -> void {
            head[v] = top;
            pos[v] = timer;
            at[timer++] = v;
            if (heavy[v] >= 0) self(self, heavy[v], top);
            for (nidx_t i = off[v]; i < off[v + 1]; ++i) {
                nidx_t u = ch[i];
                if (u != heavy[v]) self(self, u, u);
            }
        };

        for (nidx_t root : rt) dfs(dfs, root, root);
    }
    at.resize(timer);
    return nhld_layout<V>{move(vertices), move(par), move(dep), move(sz), move(heavy),
                           move(head), move(pos), move(at), move(rt)};
}

template <class C>
auto nhld(nidx_t n, nidx_t root, C next) {
    return nhld(nroot(ngraph{n, move(next)}, array<nidx_t, 1>{root}));
}

// Symmetric forest: each undirected edge occurs once in each direction, and
// adjacency is repeatable and survives nested calls. merge is associative with
// id(); base(key) is first, then contributions in local adjacency order.
// lift(state, from_key, edge_from_to) sends from's state with to excluded.
// Answers use dense positions. O(V+E) state operations/space, O(height) call stack.
template <class G, class Base, class Lift, class M>
auto nreroot(G&& graph, Base base, Lift lift, M merge) {
    using T = remove_cvref_t<invoke_result_t<Base&, decltype(graph.vertices[0])>>;
    nidx_t n = graph.vertices.len();
    vector<nidx_t> par(n, -1), order;
    vector<T> down(n, merge.id()), up(n, merge.id()), answer(n, merge.id());
    order.reserve(n);

    auto dfs = [&](auto&& self, nidx_t v) -> void {
        order.push_back(v);
        T state = invoke(base, graph.vertices[v]);
        for (auto&& edge : graph.edges(graph.vertices[v])) {
            nidx_t u = graph.vertices.inverse(graph.target(edge));
            if (u == par[v]) continue;
            par[u] = v;
            self(self, u);
            state = invoke(merge, move(state), down[u]);
        }
        if (par[v] != v)
            for (auto&& edge : graph.edges(graph.vertices[v]))
                if (graph.vertices.inverse(graph.target(edge)) == par[v]) {
                    down[v] = invoke(lift, move(state), graph.vertices[v], edge);
                    break;
                }
    };
    for (nidx_t v = 0; v < n; ++v) if (par[v] < 0) {
        par[v] = v;
        dfs(dfs, v);
    }

    for (nidx_t v : order) {
        vector<T> part;
        for (auto&& edge : graph.edges(graph.vertices[v])) {
            nidx_t u = graph.vertices.inverse(graph.target(edge));
            part.push_back(u == par[v] ? up[v] : down[u]);
        }
        vector<T> suffix(part.size() + 1, merge.id());
        for (size_t i = part.size(); i-- > 0;)
            suffix[i] = invoke(merge, part[i], suffix[i + 1]);
        T prefix = invoke(base, graph.vertices[v]);
        size_t i = 0;
        for (auto&& edge : graph.edges(graph.vertices[v])) {
            nidx_t u = graph.vertices.inverse(graph.target(edge));
            if (u != par[v])
                up[u] = invoke(lift, invoke(merge, prefix, suffix[i + 1]),
                               graph.vertices[v], edge);
            prefix = invoke(merge, move(prefix), part[i++]);
        }
        answer[v] = move(prefix);
    }
    return answer;
}
