#include "../src-v4/tree.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __LINE__ << ": " #x << '\n'; abort(); } } while (false)

struct arc { nidx_t to; int weight; };
struct sum_state {
    long long count, distance;
    friend bool operator==(sum_state, sum_state) = default;
};
struct sum_merge {
    sum_state id() const { return {0, 0}; }
    sum_state operator()(sum_state a, sum_state b) const {
        return {a.count + b.count, a.distance + b.distance};
    }
};
struct concat {
    string id() const { return {}; }
    string operator()(string a, const string& b) const { return a += b; }
};

int main() {
    mt19937 rng(0x7234);
    for (int trial = 0; trial < 700; ++trial) {
        nidx_t n = nidx_t(rng() % 19);
        auto keys = nrange(100, 100 + n);
        vector<vector<arc>> adj(n);
        for (nidx_t v = 1; v < n; ++v) if (rng() % 5) {
            nidx_t p = nidx_t(rng() % v);
            int weight = int(rng() % 21) - 10;
            adj[p].push_back({100 + v, weight});
            adj[v].push_back({100 + p, weight});
        }
        for (auto& bucket : adj) shuffle(bucket.begin(), bucket.end(), rng);
        auto graph = ngraph{keys, [&](nidx_t key) -> const auto& { return adj[key - 100]; },
                            [](arc e) { return e.to; }};
        auto distance = nreroot(graph, [](nidx_t) { return sum_state{1, 0}; },
            [](sum_state state, nidx_t, arc e) {
                state.distance += state.count * e.weight;
                return state;
            }, sum_merge{});

        auto base = [](nidx_t key) { return to_string(key); };
        auto lift = [](string state, nidx_t from, arc e) {
            return "[" + to_string(from) + ">" + to_string(e.to) + ":" + state + "]";
        };
        auto ordered = nreroot(graph, base, lift, concat{});
        for (nidx_t root = 0; root < n; ++root) {
            vector<unsigned char> seen(n);
            vector<pair<nidx_t, long long>> queue{{root, 0}};
            seen[root] = true;
            long long total = 0;
            for (size_t i = 0; i < queue.size(); ++i) {
                auto [v, d] = queue[i];
                total += d;
                for (arc e : adj[v]) if (!seen[e.to - 100]) {
                    seen[e.to - 100] = true;
                    queue.emplace_back(e.to - 100, d + e.weight);
                }
            }
            CHECK((distance[root] == sum_state{static_cast<long long>(queue.size()), total}));
            auto oracle = [&](auto&& self, nidx_t v, nidx_t parent) -> string {
                string state = base(v + 100);
                for (arc e : adj[v]) if (e.to - 100 != parent)
                    state += lift(self(self, e.to - 100, v), e.to,
                                  arc{v + 100, e.weight});
                return state;
            };
            CHECK(ordered[root] == oracle(oracle, root, -1));
        }
    }

    vector<string> names{"a", "b", "c"};
    auto domain = ninvert(nall(names));
    vector<vector<string>> adj{{"c", "b"}, {"a"}, {"a"}};
    auto graph = ngraph{domain, [&](const string& key) -> auto& { return adj[domain.inverse(key)]; }};
    auto answer = nreroot(graph, [](const string& key) { return key; },
                          [](string state, const string&, const string&) { return "(" + state + ")"; },
                          concat{});
    CHECK((answer == vector<string>{"a(c)(b)", "b(a(c))", "c(a(b))"}));
    cout << "v4 reroot: empty/keyed forests, signed weighted distances and adjacency order passed\n";
}
