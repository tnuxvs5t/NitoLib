#include "../src-v3/ds.hpp"

void check(bool condition, const char* message) {
    if (!condition) throw runtime_error(message);
}

struct xor_group {
    nidx_t id() const { return 0; }
    nidx_t operator()(nidx_t a, nidx_t b) const { return a ^ b; }
    nidx_t inverse(nidx_t value) const { return value; }
};

template <class T, class G>
void random_test(G group, uint64_t seed) {
    mt19937_64 random(seed);
    for (nidx_t trial = 0; trial < 500; ++trial) {
        nidx_t n = 1 + nidx_t(random() % 24);
        npotential_dsu<T, G> dsu(n, group);
        vector<vector<pair<nidx_t, T>>> graph(n);

        auto brute = [&](nidx_t source, nidx_t target) -> optional<T> {
            vector<unsigned char> seen(n);
            vector<T> value(n, group.id());
            vector<nidx_t> queue{source};
            seen[source] = true;
            for (nidx_t at = 0; at < nidx_t(queue.size()); ++at) {
                nidx_t from = queue[at];
                for (auto [to, delta] : graph[from]) if (!seen[to]) {
                    seen[to] = true;
                    value[to] = group(value[from], delta);
                    queue.push_back(to);
                }
            }
            if (!seen[target]) return nullopt;
            return group(group.inverse(value[source]), value[target]);
        };

        for (nidx_t step = 0; step < 600; ++step) {
            nidx_t a = nidx_t(random() % n), b = nidx_t(random() % n);
            if (random() % 3) {
                T delta = T(random() % 31);
                auto old = brute(a, b);
                bool expected = !old || *old == delta;
                check(dsu.merge(a, b, delta) == expected, "merge consistency");
                if (!old) {
                    graph[a].push_back({b, delta});
                    graph[b].push_back({a, group.inverse(delta)});
                }
            } else {
                auto expected = brute(a, b);
                auto actual = dsu.difference(a, b);
                check(bool(actual) == bool(expected), "difference presence");
                if (actual) check(*actual == *expected, "difference value");
                check(dsu.same(a, b) == bool(expected), "same");
            }
        }
    }
}

int main() {
    random_test<long long>(nsum_group<long long>{}, 0x51ed5eedULL);
    random_test<nidx_t>(xor_group{}, 0xa11ceULL);
}
