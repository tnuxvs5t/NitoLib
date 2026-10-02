#pragma once
#include "graph.hpp"
#include "func.hpp"

// Arrays use dense positions. Only vertices reached from the supplied roots have
// metadata; projections borrow this object. DFS needs O(height) call stack.
template <class V>
struct nrooted {
    V vertices;
    nidx_t n;
    vector<nidx_t> par, dep, comp, ord, sz;
    vector<nidx_t> rt, off, ch;

    nidx_t len() const { return n; }

    auto keys() const { return nall(vertices); }
    auto locate() const { return nlocate(vertices); }
    auto positions() const { return nfunc{keys(), locate()}; }

    auto roots() const {
        return nproject(nall(rt), [this](nidx_t i) -> decltype(auto) {
            return vertices[i];
        });
    }

    auto order() const {
        return nproject(nall(ord),
                        [this](nidx_t v) -> decltype(auto) { return vertices[v]; });
    }

    auto parents() const {
        return nmap_values(nanchors(keys(), nall(par), locate()),
                           [this](nidx_t v) -> decltype(auto) { return vertices[v]; });
    }

    auto depths() const { return nanchors(keys(), nall(dep), locate()); }

    auto components() const {
        return nmap_values(nanchors(keys(), nall(comp), locate()),
                           [this](nidx_t v) -> decltype(auto) { return vertices[v]; });
    }

    auto subtree_sizes() const { return nanchors(keys(), nall(sz), locate()); }

    template <class K>
    auto children(K&& key) const {
        nidx_t v = vertices.inverse(forward<K>(key));
        return nproject(nsub(nall(ch), off[v], off[v + 1]),
                        [this](nidx_t u) -> decltype(auto) { return vertices[u]; });
    }
};

template <class G, class R>
auto nroot(G graph, R&& roots) {
    nidx_t n = graph.vertices.len();
    vector<nidx_t> par(n, -1), dep(n, -1), comp(n, -1), ord, sz(n);
    vector<nidx_t> rt;
    ord.reserve(n);
    rt.reserve(nlen(roots));

    {
        auto dfs = [&](auto&& self, nidx_t v) -> void {
            ord.push_back(v);
            sz[v] = 1;
            for (auto&& edge : graph.edges(graph.vertices[v])) {
                nidx_t u = graph.vertices.inverse(graph.target(edge));
                if (par[u] >= 0) continue;
                par[u] = v;
                dep[u] = dep[v] + 1;
                comp[u] = comp[v];
                self(self, u);
                sz[v] += sz[u];
            }
        };

        for (nidx_t i = 0; i < nlen(roots); ++i) {
            nidx_t root = graph.vertices.inverse(roots[i]);
            if (par[root] >= 0) continue;
            par[root] = root;
            dep[root] = 0;
            comp[root] = root;
            rt.push_back(root);
            dfs(dfs, root);
        }
    }

    vector<nidx_t> off(n + 1), ch;
    ch.reserve(ord.size() - rt.size());
    for (nidx_t v : ord)
        if (par[v] != v) ++off[par[v] + 1];
    partial_sum(off.begin(), off.end(), off.begin());
    vector<nidx_t> at = off;
    ch.resize(off.back());
    for (nidx_t v : ord)
        if (par[v] != v) ch[at[par[v]]++] = v;

    using V = remove_cvref_t<decltype(graph.vertices)>;
    return nrooted<V>{move(graph.vertices), n, move(par), move(dep), move(comp), move(ord), move(sz),
            move(rt), move(off), move(ch)};
}
