#include "../src-v3/reftree.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

using U = uint64_t;

struct count_ops {
    U count(const auto& q, nidx_t p, nidx_t h) const {
        return q[p].value << (h - q[p].height);
    }
    U join(const auto& q, nidx_t h, nidx_t l, nidx_t r) const {
        return count(q, l, h - 1) + count(q, r, h - 1);
    }
};

using bits = nreftree<U, count_ops>;

void check(const bits& q, nidx_t p, nidx_t h, const vector<U>& a) {
    nidx_t before = q.nodes();
    auto count = [&](nidx_t root, nidx_t height) { return q.ops.count(q, root, height); };
    auto has = [&](nidx_t root, nidx_t height) { return count(root, height) != 0; };
    CHECK(count(p, h) == accumulate(a.begin(), a.end(), U(0)));
    vector<U> selected;
    for (U x = 0; x < a.size(); ++x) {
        CHECK(q[q.leaf_at(p, x)].value == a[x]);
        U expected = x;
        while (expected < a.size() && !a[expected]) ++expected;
        CHECK(q.find_first(p, h, x, has) == expected);
        if (a[x]) selected.push_back(x);
    }
    for (U k = 0; k < selected.size(); ++k) CHECK(q.kth(p, h, k, count) == selected[k]);
    CHECK(q.find_first(p, h, a.size(), has) == a.size());
    CHECK(q.find_first(p, h, numeric_limits<U>::max(), has) == a.size());
    CHECK(q.nodes() == before);
}

void bit_properties() {
    bits q;
    // Deliberately reverse Boolean handle order: handle zero is not an empty root.
    nidx_t one = q.leaf(1), zero = q.leaf(0);
    nidx_t odd = q.join(1, zero, one), even = q.join(1, one, zero);
    CHECK(q[odd].value == q[even].value && odd != even);
    nidx_t mixed = q.join(2, odd, even);
    check(q, mixed, 2, {0, 1, 1, 0});
    auto repeated = q.split(odd, 60);
    CHECK(repeated.first == odd && repeated.second == odd);
    CHECK(q.join(60, odd, odd) == odd);
    nidx_t duplicate = q.join(1, zero, one);
    CHECK(duplicate != odd); // No semantic interning promised.
    check(q, duplicate, 3, {0, 1, 0, 1, 0, 1, 0, 1});

    for (nidx_t h = 0; h <= 3; ++h) {
        U n = U(1) << h;
        for (U mask = 0; mask < (U(1) << n); ++mask) {
            vector<U> a(n);
            nidx_t p = zero;
            for (U x = 0; x < n; ++x) {
                a[x] = (mask >> x) & 1;
                if (a[x]) p = q.set(p, h, x, one);
            }
            check(q, p, h, a);
            for (nidx_t k = 0; k <= h; ++k) {
                U width = U(1) << k;
                for (U x = 0; x < n; x += width) {
                    nidx_t before = q.nodes();
                    nidx_t piece = q.block(p, x, k);
                    CHECK(q.nodes() == before);
                    check(q, piece, k, vector<U>(a.begin() + x, a.begin() + x + width));
                }
            }
        }
    }

    mt19937_64 rng(0x52454654524545ULL);
    for (nidx_t h = 1; h <= 8; ++h) {
        U n = U(1) << h;
        vector<nidx_t> roots{zero, one};
        vector<vector<U>> values{vector<U>(n, 0), vector<U>(n, 1)};
        for (nidx_t round = 0; round < 800; ++round) {
            size_t base = rng() % roots.size(), from = rng() % roots.size();
            nidx_t k = nidx_t(rng() % U(h + 1)), sk = nidx_t(rng() % U(k + 1));
            U width = U(1) << k, sw = U(1) << sk;
            U x = rng() % (n / width) * width, y = rng() % (n / sw) * sw;
            nidx_t src = q.block(roots[from], y, sk);
            auto expected = values[base];
            for (U j = 0; j < width; ++j) expected[x + j] = values[from][y + j % sw];
            nidx_t before = q.nodes();
            nidx_t p = q.paste(roots[base], h, x, k, src);
            CHECK(q.nodes() - before <= h - k);
            check(q, p, h, expected);
            check(q, roots[base], h, values[base]);
            size_t historical = rng() % roots.size();
            check(q, roots[historical], h, values[historical]);
            roots.push_back(p);
            values.push_back(move(expected));
        }
    }
    U end = U(1) << 60;
    auto count = [&](nidx_t p, nidx_t h) { return q.ops.count(q, p, h); };
    auto has = [&](nidx_t p, nidx_t h) { return count(p, h) != 0; };
    CHECK(count(odd, 60) == end / 2);
    CHECK(q.kth(odd, 60, end / 2 - 1, count) == end - 1);
    CHECK(q.find_first(odd, 60, end - 2, has) == end - 1);
    nidx_t changed = q.set(odd, 60, end - 1, zero);
    CHECK(count(changed, 60) == end / 2 - 1);
    CHECK(q.find_first(changed, 60, end - 2, has) == end);
    CHECK(q.leaf_at(odd, end - 1) == one);
    CHECK(q.paste(odd, 60, U(1) << 59, 30, odd) == odd);
    CHECK(q.paste(changed, 60, 0, 60, one) == one);
    CHECK(q.set(zero, 0, 0, one) == one);
    nidx_t before = q.nodes();
    q.reserve(before + 100);
    CHECK(q.leaf_at(changed, end - 1) == zero);
    CHECK(q.nodes() == before);
    // Public construction appends parents after children, including skipped levels.
    for (nidx_t p = 0; p < q.nodes(); ++p) {
        const auto& a = q[p];
        if (!a.height) continue;
        CHECK(a.left < p && a.right < p);
        CHECK(q[a.left].height < a.height && q[a.right].height < a.height);
        CHECK(a.value == q.ops.join(q, a.height, a.left, a.right));
    }
}

struct text_ops {
    string observed(const auto& q, nidx_t p, nidx_t h) const {
        string value = q[p].value;
        for (nidx_t i = q[p].height; i < h; ++i) value += value;
        return value;
    }
    string join(const auto& q, nidx_t h, nidx_t l, nidx_t r) const {
        return observed(q, l, h - 1) + observed(q, r, h - 1);
    }
};

void ordered_properties() {
    nreftree<string, text_ops> q;
    array<nidx_t, 3> letters{q.leaf("a"), q.leaf("b"), q.leaf("c")};
    nidx_t ab = q.join(1, letters[0], letters[1]);
    CHECK(q.ops.observed(q, ab, 3) == "abababab");
    CHECK(q[q.join(2, ab, q.join(1, letters[1], letters[0]))].value == "abba");
    mt19937 rng(51237);
    constexpr nidx_t H = 6;
    vector<nidx_t> roots{ab};
    vector<string> values{q.ops.observed(q, ab, H)};
    for (nidx_t round = 0; round < 600; ++round) {
        size_t base = rng() % roots.size(), from = rng() % roots.size();
        nidx_t k = nidx_t(rng() % (H + 1)), sk = nidx_t(rng() % (k + 1));
        U width = U(1) << k, sw = U(1) << sk;
        U x = rng() % (64 / width) * width, y = rng() % (64 / sw) * sw;
        string expected = values[base];
        nidx_t src = q.block(roots[from], y, sk);
        if (round % 5 == 0) {
            size_t letter = rng() % letters.size();
            src = letters[letter];
            expected.replace(x, width, size_t(width), char('a' + letter));
        } else {
            for (U j = 0; j < width; ++j) expected[x + j] = values[from][y + j % sw];
        }
        nidx_t p = q.paste(roots[base], H, x, k, src);
        CHECK(q.ops.observed(q, p, H) == expected);
        CHECK(q.ops.observed(q, roots[base], H) == values[base]);
        roots.push_back(p);
        values.push_back(move(expected));
    }
    // Terminals with equal information retain distinct identity; no Info equality needed.
    nidx_t another_a = q.leaf("a");
    CHECK(another_a != letters[0]);
    CHECK(q.leaf_at(q.join(1, letters[0], another_a), 1) == another_a);
}

struct paint_ops {
    using cost = pair<U, U>;
    cost observed(const auto& q, nidx_t p, nidx_t h) const {
        auto [a, b] = q[p].value;
        U low = min(a, b) << (h - q[p].height);
        return {low + U(a > b), low + U(b > a)};
    }
    cost join(const auto& q, nidx_t h, nidx_t l, nidx_t r) const {
        auto [a, b] = observed(q, l, h - 1);
        auto [c, d] = observed(q, r, h - 1);
        return {min(a + c, 1 + b + d), min(b + d, 1 + a + c)};
    }
};

// Independent oracle: BFS over actual lamp states and every legal paint operation.
vector<U> paint_bfs(nidx_t h, U initial) {
    U n = U(1) << h, states = U(1) << n;
    vector<U> distance(states, numeric_limits<U>::max());
    queue<U> pending;
    distance[initial] = 0;
    pending.push(initial);
    while (!pending.empty()) {
        U state = pending.front(); pending.pop();
        for (nidx_t k = 0; k <= h; ++k) {
            U width = U(1) << k;
            for (U x = 0; x < n; x += width) {
                U mask = ((U(1) << width) - 1) << x;
                for (U next : {state & ~mask, state | mask}) {
                    if (distance[next] != numeric_limits<U>::max()) continue;
                    distance[next] = distance[state] + 1;
                    pending.push(next);
                }
            }
        }
    }
    return distance;
}

void paint_properties() {
    nreftree<paint_ops::cost, paint_ops> q;
    nidx_t zero = q.leaf(0, 1), one = q.leaf(1, 0);
    for (nidx_t h = 0; h <= 3; ++h) {
        U n = U(1) << h, states = U(1) << n;
        auto from_zero = paint_bfs(h, 0), from_one = paint_bfs(h, states - 1);
        for (U mask = 0; mask < states; ++mask) {
            nidx_t p = zero;
            for (U x = 0; x < n; ++x) if ((mask >> x) & 1) p = q.set(p, h, x, one);
            auto [a, b] = q.ops.observed(q, p, h);
            CHECK(a == from_zero[mask] && b == from_one[mask]);
        }
    }
    nidx_t p = q.join(2, one, q.join(1, one, zero)); // 1110
    CHECK(q.ops.observed(q, p, 2) == paint_ops::cost(2, 1));
    CHECK(q.ops.observed(q, p, 3) == paint_ops::cost(3, 2));
    CHECK(q.ops.observed(q, p, 60) == paint_ops::cost((U(1) << 58) + 1, U(1) << 58));
}

struct move_info {
    unique_ptr<U> value;
    explicit move_info(U v) : value(make_unique<U>(v)) {}
    move_info(move_info&&) = default;
    move_info& operator=(move_info&&) = delete;
};

struct move_ops {
    unique_ptr<U> joins;
    explicit move_ops(U v) : joins(make_unique<U>(v)) {}
    move_info join(const auto& q, nidx_t h, nidx_t l, nidx_t r) {
        ++*joins;
        return move_info((*q[l].value.value << (h - 1 - q[l].height)) +
                         (*q[r].value.value << (h - 1 - q[r].height)));
    }
};

void freedom_properties() {
    nreftree<> bare;
    nidx_t a = bare.leaf(), b = bare.leaf();
    nidx_t p = bare.join(1, a, b);
    CHECK(bare.leaf_at(p, 6) == a && bare.leaf_at(p, 7) == b);
    static_assert(is_const_v<remove_reference_t<decltype(bare[p])>>);
    nreftree<move_info, move_ops> q(move_ops(0));
    nidx_t zero = q.leaf(0), one = q.leaf(1);
    nidx_t odd = q.join(1, zero, one);
    CHECK(*q[odd].value.value == 1 && *q.ops.joins == 1);
    CHECK(q.join(60, odd, odd) == odd && *q.ops.joins == 1);
    // Callbacks are borrowed; move-only callables need neither copying nor registration.
    auto has = [state = make_unique<U>(0), &q](nidx_t root, nidx_t) mutable {
        ++*state;
        return *q[root].value.value != 0;
    };
    auto count = [state = make_unique<U>(0), &q](nidx_t root, nidx_t h) mutable {
        ++*state;
        return *q[root].value.value << (h - q[root].height);
    };
    CHECK(q.find_first(odd, 60, 0, has) == 1);
    CHECK(q.kth(odd, 60, 100, count) == 201);
    auto moved = move(q);
    CHECK(*moved[odd].value.value == 1);
    nidx_t changed = moved.set(odd, 4, 15, zero);
    CHECK(*moved[changed].value.value == 7);
}

int main() {
    bit_properties();
    ordered_properties();
    paint_properties();
    freedom_properties();
    cout << "reftree: exhaustive bits/blocks, historical paste/search/select, ordered text, paint BFS, move-only OK\n";
}
