#pragma once
#include "rooted.hpp"

struct npath_piece {
    nidx_t left, right;
    bool reverse;
};

template <class V>
struct nhld_layout {
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
