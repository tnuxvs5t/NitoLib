#include "../src-v4/topk.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

struct candidate {
    nidx_t score, key, source;
    friend bool operator==(const candidate&, const candidate&) = default;
};
struct better {
    bool operator()(const candidate& left, const candidate& right) const {
        return tie(left.score, left.source) > tie(right.score, right.source);
    }
};
struct key_of { nidx_t operator()(const candidate& value) const { return value.key; } };
struct key_better {
    bool operator()(const candidate& left, const candidate& right) const {
        return tie(left.score, left.key, left.source) >
               tie(right.score, right.key, right.source);
    }
};

template <size_t K>
vector<candidate> values(const ntopk<candidate, K>& state) {
    vector<candidate> result;
    for (nidx_t i = 0; i < state.len(); ++i) result.push_back(state[i]);
    return result;
}

template <size_t K>
vector<candidate> ordinary_oracle(vector<candidate> source) {
    ranges::sort(source, better{});
    if (source.size() > K) source.resize(K);
    return source;
}

template <size_t K>
vector<candidate> keyed_oracle(vector<candidate> source) {
    vector<candidate> best;
    for (auto value : source) {
        auto it = find_if(best.begin(), best.end(), [&](const candidate& x) {
            return x.key == value.key;
        });
        if (it == best.end()) best.push_back(value);
        else if (key_better{}(value, *it)) *it = value;
    }
    ranges::sort(best, key_better{});
    if (best.size() > K) best.resize(K);
    return best;
}

int main() {
    ntopk_merge<candidate, 0, better> zero;
    auto empty = zero.single({1, 1, 1});
    CHECK(empty.empty() && !zero.update(empty, {2, 2, 2}));

    using ordinary = ntopk_merge<candidate, 4, better>;
    using keyed = ntopk_by_merge<candidate, 4, key_better, key_of>;
    ordinary op;
    keyed kop;
    mt19937 rng(0x70F);
    for (nidx_t round = 0; round < 3000; ++round) {
        vector<candidate> a, b, c;
        for (nidx_t i = 0; i < 12; ++i)
            a.push_back({nidx_t(rng() % 20) - 10, nidx_t(rng() % 6), i});
        for (nidx_t i = 0; i < 12; ++i)
            b.push_back({nidx_t(rng() % 20) - 10, nidx_t(rng() % 6), 100 + i});
        for (nidx_t i = 0; i < 12; ++i)
            c.push_back({nidx_t(rng() % 20) - 10, nidx_t(rng() % 6), 200 + i});

        auto fill = [&](auto& operation, const auto& source) {
            auto state = operation.id();
            for (auto value : source) operation.update(state, value);
            return state;
        };
        auto sa = fill(op, a), sb = fill(op, b), sc = fill(op, c);
        auto all = a;
        all.insert(all.end(), b.begin(), b.end());
        all.insert(all.end(), c.begin(), c.end());
        CHECK(values(op(op(sa, sb), sc)) == ordinary_oracle<4>(all));
        CHECK(values(op(sa, op(sb, sc))) == ordinary_oracle<4>(all));

        auto ka = fill(kop, a), kb = fill(kop, b), kc = fill(kop, c);
        CHECK(values(kop(kop(ka, kb), kc)) == keyed_oracle<4>(all));
        CHECK(values(kop(ka, kop(kb, kc))) == keyed_oracle<4>(all));
    }
    cout << "v4 topk: ordinary and keyed fixed-capacity summaries passed\n";
}
