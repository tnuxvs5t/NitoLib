#pragma once
#include "arena.hpp"

template <class T>
struct nadd {
    constexpr T id() const { return T{}; }
    constexpr T operator()(T left, const T& right) const { return left += right; }
};

template <class A, class B>
struct nadd<pair<A, B>> {
    constexpr pair<A, B> id() const { return {nadd<A>{}.id(), nadd<B>{}.id()}; }
    constexpr pair<A, B> operator()(pair<A, B> left, const pair<A, B>& right) const {
        return {nadd<A>{}(move(left.first), right.first),
                nadd<B>{}(move(left.second), right.second)};
    }
};

template <class... A>
struct nadd<tuple<A...>> {
    constexpr tuple<A...> id() const { return {nadd<A>{}.id()...}; }
    constexpr tuple<A...> operator()(tuple<A...> left, const tuple<A...>& right) const {
        return [&]<size_t... I>(index_sequence<I...>) {
            return tuple<A...>{nadd<A>{}(move(get<I>(left)), get<I>(right))...};
        }(index_sequence_for<A...>{});
    }
};

template <class T>
struct nmin {
    constexpr T id() const {
        if constexpr (numeric_limits<T>::has_infinity) return numeric_limits<T>::infinity();
        else return numeric_limits<T>::max();
    }
    constexpr T operator()(T left, const T& right) const {
        return right < left ? right : move(left);
    }
};

template <class A, class B>
struct nmin<pair<A, B>> {
    constexpr pair<A, B> id() const { return {nmin<A>{}.id(), nmin<B>{}.id()}; }
    constexpr pair<A, B> operator()(pair<A, B> left, const pair<A, B>& right) const {
        return right < left ? right : move(left);
    }
};

template <class... A>
struct nmin<tuple<A...>> {
    constexpr tuple<A...> id() const { return {nmin<A>{}.id()...}; }
    constexpr tuple<A...> operator()(tuple<A...> left, const tuple<A...>& right) const {
        return right < left ? right : move(left);
    }
};

template <class T>
struct nmax {
    constexpr T id() const {
        if constexpr (numeric_limits<T>::has_infinity) return -numeric_limits<T>::infinity();
        else return numeric_limits<T>::lowest();
    }
    constexpr T operator()(T left, const T& right) const {
        return left < right ? right : move(left);
    }
};

template <class A, class B>
struct nmax<pair<A, B>> {
    constexpr pair<A, B> id() const { return {nmax<A>{}.id(), nmax<B>{}.id()}; }
    constexpr pair<A, B> operator()(pair<A, B> left, const pair<A, B>& right) const {
        return left < right ? right : move(left);
    }
};

template <class... A>
struct nmax<tuple<A...>> {
    constexpr tuple<A...> id() const { return {nmax<A>{}.id()...}; }
    constexpr tuple<A...> operator()(tuple<A...> left, const tuple<A...>& right) const {
        return left < right ? right : move(left);
    }
};

namespace ndetail {

template <class F, class I>
constexpr void nsegment_emit(F& visit, nidx_t node, I left, I right) {
    if constexpr (requires { invoke(visit, node, left, right); })
        invoke(visit, node, left, right);
    else
        invoke(visit, node);
}

} // namespace ndetail

template <class I, class C, class F>
constexpr void nsegment_trace(nidx_t root, I left, I right, I position,
                              C&& child, F&& visit) {
    for (nidx_t node = root; node >= 0;) {
        ndetail::nsegment_emit(visit, node, left, right);
        if (left + 1 == right) break;
        I middle = midpoint(left, right);
        nidx_t side = position < middle ? 0 : 1;
        node = invoke(child, node, side);
        if (side) left = middle;
        else right = middle;
    }
}

template <class I, class C, class F>
constexpr void nsegment_cover(nidx_t root, I left, I right, I query_left,
                              I query_right, C&& child, F&& visit) {
    if (root < 0 || query_left == query_right) return;
    auto walk = [&](auto&& self, nidx_t node, I node_left, I node_right) -> void {
        if (node < 0 || query_right <= node_left || node_right <= query_left) return;
        if (query_left <= node_left && node_right <= query_right) {
            ndetail::nsegment_emit(visit, node, node_left, node_right);
            return;
        }
        I middle = midpoint(node_left, node_right);
        if (query_left < middle)
            self(self, invoke(child, node, 0), node_left, middle);
        if (middle < query_right)
            self(self, invoke(child, node, 1), middle, node_right);
    };
    walk(walk, root, left, right);
}

template <class F>
constexpr void nsegment_trace(nidx_t base, nidx_t position, F&& visit) {
    nsegment_trace(nidx_t(1), nidx_t(0), base, position,
                   [](nidx_t node, nidx_t side) { return node * 2 + side; },
                   forward<F>(visit));
}

template <class F>
constexpr void nsegment_cover(nidx_t base, nidx_t left, nidx_t right, F&& visit) {
    nsegment_cover(nidx_t(1), nidx_t(0), base, left, right,
                   [](nidx_t node, nidx_t side) { return node * 2 + side; },
                   forward<F>(visit));
}

template <class T, class M = nadd<T>>
struct nseg {
    [[no_unique_address]] mutable M merge;
    nidx_t length = 0, base = 1;
    vector<T> tree;

    explicit nseg(nidx_t n = 0, M operation = {})
        : merge(move(operation)), length(n),
          base(nidx_t(bit_ceil(nuidx_t(max(nidx_t(1), n))))),
          tree(size_t(2) * base, merge.id()) {}

    template <class V>
    requires requires(V& source) { source[0]; }
    explicit nseg(const V& source, M operation = {}) : nseg(nlen(source), move(operation)) {
        for (nidx_t i = 0; i < length; ++i) tree[base + i] = source[i];
        for (nidx_t i = base; --i;)
            tree[i] = invoke(merge, tree[i << 1], tree[i << 1 | 1]);
    }

    nidx_t len() const { return length; }
    bool empty() const { return length == 0; }
    const T& get(nidx_t position) const { return tree[base + position]; }

    void set(nidx_t position, T value) {
        nidx_t node = base + position;
        tree[node] = move(value);
        while (node >>= 1)
            tree[node] = invoke(merge, tree[node << 1], tree[node << 1 | 1]);
    }

    T fold(nidx_t left, nidx_t right) const {
        T prefix = merge.id(), suffix = merge.id();
        for (left += base, right += base; left < right; left >>= 1, right >>= 1) {
            if (left & 1) prefix = invoke(merge, move(prefix), tree[left++]);
            if (right & 1) suffix = invoke(merge, tree[--right], move(suffix));
        }
        return invoke(merge, move(prefix), move(suffix));
    }

    T fold() const { return length ? tree[1] : merge.id(); }

    template <class P>
    nidx_t max_right(nidx_t left, P predicate) const {
        T aggregate = merge.id();
        assert(invoke(predicate, aggregate));
        if (left == length) return length;
        nidx_t node = left + base;
        do {
            while (!(node & 1)) node >>= 1;
            T candidate = invoke(merge, aggregate, tree[node]);
            if (!invoke(predicate, candidate)) {
                while (node < base) {
                    node <<= 1;
                    candidate = invoke(merge, aggregate, tree[node]);
                    if (invoke(predicate, candidate)) aggregate = move(candidate), ++node;
                }
                return min(length, node - base);
            }
            aggregate = move(candidate);
            ++node;
        } while ((node & -node) != node);
        return length;
    }

    template <class P>
    nidx_t min_left(nidx_t right, P predicate) const {
        T aggregate = merge.id();
        assert(invoke(predicate, aggregate));
        if (!right) return 0;
        nidx_t node = right + base;
        do {
            --node;
            while (node > 1 && (node & 1)) node >>= 1;
            T candidate = invoke(merge, tree[node], aggregate);
            if (!invoke(predicate, candidate)) {
                while (node < base) {
                    node = node << 1 | 1;
                    candidate = invoke(merge, tree[node], aggregate);
                    if (invoke(predicate, candidate)) aggregate = move(candidate), --node;
                }
                return max(nidx_t(0), node + 1 - base);
            }
            aggregate = move(candidate);
        } while ((node & -node) != node);
        return 0;
    }

    void pointwise(const nseg& other) {
        for (nidx_t i = 0; i < base; ++i)
            tree[base + i] = invoke(merge, move(tree[base + i]), other.tree[base + i]);
        for (nidx_t i = base; --i;)
            tree[i] = invoke(merge, tree[i << 1], tree[i << 1 | 1]);
    }
};

template <class V, class M>
nseg(V, M) -> nseg<remove_cvref_t<decltype(declval<V>()[0])>, M>;

/* Lazy traversal is policy-driven: the policy owns tags, make/join, and action order. */
template <class T, class Ops>
struct nlazyseg {
    [[no_unique_address]] Ops ops;
    nidx_t length = 0, base = 1;
    vector<T> tree;

    explicit nlazyseg(nidx_t n = 0, Ops policy = {})
        : ops(move(policy)), length(n),
          base(nidx_t(bit_ceil(nuidx_t(max(nidx_t(1), n))))),
          tree(size_t(2) * base, ops.identity()) {
        if constexpr (requires { ops.init(*this); }) ops.init(*this);
    }

    template <class V>
    requires requires(V& source) { source[0]; }
    explicit nlazyseg(const V& source, Ops policy = {})
        : nlazyseg(nlen(source), move(policy)) {
        for (nidx_t i = 0; i < length; ++i) tree[base + i] = ops.make(source[i]);
        for (nidx_t i = base; --i;) pull(i);
    }

    nidx_t len() const { return length; }
    bool empty() const { return length == 0; }
    T& operator[](nidx_t node) { return tree[node]; }
    const T& operator[](nidx_t node) const { return tree[node]; }

    void push(nidx_t node, nidx_t left, nidx_t right) {
        if constexpr (requires { ops.push(*this, node, left, right); })
            if (left + 1 < right) ops.push(*this, node, left, right);
    }

    void pull(nidx_t node) {
        if constexpr (requires { ops.pull(*this, node); }) ops.pull(*this, node);
        else tree[node] = ops.join(tree[node << 1], tree[node << 1 | 1]);
    }

    template <class V>
    void walk(nidx_t left, nidx_t right, V&& visit) {
        if (left == right) return;
        auto descend = [&](auto&& self, nidx_t node, nidx_t lo, nidx_t hi) -> void {
            if (right <= lo || hi <= left) return;
            if (invoke(visit, *this, node, lo, hi, left <= lo && hi <= right)) return;
            assert(lo + 1 < hi);
            if (lo + 1 == hi) return;
            push(node, lo, hi);
            nidx_t middle = midpoint(lo, hi);
            self(self, node << 1, lo, middle);
            self(self, node << 1 | 1, middle, hi);
            pull(node);
        };
        descend(descend, 1, 0, base);
    }

    template <class C>
    void apply(nidx_t left, nidx_t right, const C& command) {
        walk(left, right, [&](auto&, nidx_t node, nidx_t lo, nidx_t hi, bool full) {
            return full && ops.try_apply(*this, node, lo, hi, command);
        });
    }

    T fold(nidx_t left, nidx_t right) {
        T result = ops.identity();
        walk(left, right, [&](auto&, nidx_t node, nidx_t, nidx_t, bool full) {
            if (full) result = ops.join(move(result), tree[node]);
            return full;
        });
        return result;
    }

    T fold() const { return tree[1]; }
    T get(nidx_t position) { return fold(position, position + 1); }

    template <class U>
    void set(nidx_t position, U&& value) {
        walk(position, position + 1,
             [&](auto&, nidx_t node, nidx_t, nidx_t, bool full) {
                 if (full) tree[node] = ops.make(forward<U>(value));
                 return full;
             });
    }

    template <class P>
    nidx_t max_right(nidx_t left, P predicate) {
        T aggregate = ops.identity();
        assert(invoke(predicate, aggregate));
        auto descend = [&](auto&& self, nidx_t node, nidx_t lo, nidx_t hi) -> nidx_t {
            if (hi <= left || length <= lo) return -1;
            if (left <= lo && hi <= length) {
                T candidate = ops.join(aggregate, tree[node]);
                if (invoke(predicate, candidate)) {
                    aggregate = move(candidate);
                    return -1;
                }
                if (lo + 1 == hi) return lo;
            }
            push(node, lo, hi);
            nidx_t middle = midpoint(lo, hi);
            nidx_t answer = self(self, node << 1, lo, middle);
            if (answer < 0) answer = self(self, node << 1 | 1, middle, hi);
            pull(node);
            return answer;
        };
        nidx_t answer = descend(descend, 1, 0, base);
        return answer < 0 ? length : answer;
    }

    template <class P>
    nidx_t min_left(nidx_t right, P predicate) {
        T aggregate = ops.identity();
        assert(invoke(predicate, aggregate));
        auto descend = [&](auto&& self, nidx_t node, nidx_t lo, nidx_t hi) -> nidx_t {
            if (right <= lo) return -1;
            if (hi <= right) {
                T candidate = ops.join(tree[node], aggregate);
                if (invoke(predicate, candidate)) {
                    aggregate = move(candidate);
                    return -1;
                }
                if (lo + 1 == hi) return hi;
            }
            push(node, lo, hi);
            nidx_t middle = midpoint(lo, hi);
            nidx_t answer = self(self, node << 1 | 1, middle, hi);
            if (answer < 0) answer = self(self, node << 1, lo, middle);
            pull(node);
            return answer;
        };
        nidx_t answer = descend(descend, 1, 0, base);
        return answer < 0 ? 0 : answer;
    }
};

template <class V, class Ops>
nlazyseg(V, Ops) -> nlazyseg<
    remove_cvref_t<decltype(declval<Ops&>().make(declval<V&>()[0]))>, Ops>;

template <class M, class A>
struct nlazy_ops {
    using T = remove_cvref_t<decltype(declval<M&>().id())>;
    using F = remove_cvref_t<decltype(declval<A&>().tag_id())>;
    [[no_unique_address]] M merge;
    [[no_unique_address]] A action;
    vector<F> lazy;
    vector<unsigned char> pending;

    nlazy_ops(M operation = {}, A policy = {})
        : merge(move(operation)), action(move(policy)) {}

    T identity() { return merge.id(); }
    T make(T value) { return value; }
    T join(T left, const T& right) { return invoke(merge, move(left), right); }

    template <class Q>
    void init(Q& q) {
        lazy.assign(q.base, action.tag_id());
        pending.assign(q.base, 0);
    }

    template <class Q>
    bool try_apply(Q& q, nidx_t node, nidx_t left, nidx_t right, const F& tag) {
        q[node] = action.apply(move(q[node]), tag, right - left);
        if (node < q.base) {
            // compose(newer, older): older is applied first.
            lazy[node] = pending[node] ? action.compose(tag, lazy[node]) : tag;
            pending[node] = true;
        }
        return true;
    }

    template <class Q>
    void push(Q& q, nidx_t node, nidx_t left, nidx_t right) {
        if (!pending[node]) return;
        nidx_t middle = midpoint(left, right);
        try_apply(q, node << 1, left, middle, lazy[node]);
        try_apply(q, node << 1 | 1, middle, right, lazy[node]);
        pending[node] = false;
    }
};

template <class T>
struct naddsum_action {
    constexpr T tag_id() const { return T{}; }
    constexpr T compose(const T& newer, const T& older) const { return older + newer; }
    constexpr T apply(T sum, const T& tag, nidx_t length) const {
        return sum + tag * T(length);
    }
};

template <class T>
using nlazy_addsum = nlazyseg<T, nlazy_ops<nadd<T>, naddsum_action<T>>>;

/* Sparse segment roots over a long-long coordinate domain. Missing subtrees are id().
   Destructive methods reuse nodes; *_copy methods allocate a persistent result and may
   share untouched subtrees. */
template <class T, class M = nadd<T>>
struct nsparse_seg {
    struct node {
        T aggregate;
        nidx_t left = -1, right = -1;
    };

    narena<node> pool;
    long long lo, hi;
    [[no_unique_address]] mutable M merge_values;

    explicit nsparse_seg(long long left_bound, long long right_bound, M operation = {})
        : lo(left_bound), hi(right_bound), merge_values(move(operation)) {}

    nidx_t nodes() const { return pool.len(); }
    void reserve(nidx_t n) { pool.reserve(n); }
    node& operator[](nidx_t root) { return pool[root]; }
    const node& operator[](nidx_t root) const { return pool[root]; }

    nidx_t make(T value, nidx_t left = -1, nidx_t right = -1) {
        return pool.make(node{move(value), left, right});
    }
    nidx_t make() { return make(merge_values.id()); }

    T aggregate(nidx_t root) const {
        return root < 0 ? merge_values.id() : pool[root].aggregate;
    }

    void pull(nidx_t root) {
        pool[root].aggregate = invoke(merge_values, aggregate(pool[root].left),
                                      aggregate(pool[root].right));
    }

private:
    template <bool Copy>
    nidx_t store(nidx_t root, T value, nidx_t left = -1, nidx_t right = -1) {
        if constexpr (Copy) return make(move(value), left, right);
        else {
            if (root < 0) return make(move(value), left, right);
            pool[root] = node{move(value), left, right};
            return root;
        }
    }

    template <bool Copy, class F>
    nidx_t update0(nidx_t root, long long left, long long right,
                   long long position, F& edit) {
        if (left + 1 == right) return store<Copy>(root, invoke(edit, root));
        nidx_t child_left = root < 0 ? -1 : pool[root].left;
        nidx_t child_right = root < 0 ? -1 : pool[root].right;
        long long middle = midpoint(left, right);
        if (position < middle)
            child_left = update0<Copy>(child_left, left, middle, position, edit);
        else
            child_right = update0<Copy>(child_right, middle, right, position, edit);
        return store<Copy>(root, invoke(merge_values, aggregate(child_left),
                                        aggregate(child_right)),
                           child_left, child_right);
    }

    T fold0(nidx_t root, long long left, long long right,
            long long query_left, long long query_right) const {
        if (root < 0 || query_right <= left || right <= query_left)
            return merge_values.id();
        if (query_left <= left && right <= query_right) return pool[root].aggregate;
        long long middle = midpoint(left, right);
        return invoke(merge_values,
                      fold0(pool[root].left, left, middle, query_left, query_right),
                      fold0(pool[root].right, middle, right, query_left, query_right));
    }

    template <bool Copy>
    nidx_t merge0(nidx_t left_root, nidx_t right_root,
                  long long left, long long right) {
        if (left_root < 0) return right_root;
        if (right_root < 0) return left_root;
        if (left + 1 == right) {
            T value = invoke(merge_values, pool[left_root].aggregate,
                             pool[right_root].aggregate);
            return store<Copy>(left_root, move(value));
        }
        long long middle = midpoint(left, right);
        nidx_t child_left = merge0<Copy>(pool[left_root].left,
                                         pool[right_root].left, left, middle);
        nidx_t child_right = merge0<Copy>(pool[left_root].right,
                                          pool[right_root].right, middle, right);
        return store<Copy>(left_root,
                           invoke(merge_values, aggregate(child_left),
                                  aggregate(child_right)),
                           child_left, child_right);
    }

    nidx_t clone0(nidx_t root) {
        if (root < 0) return -1;
        nidx_t left = clone0(pool[root].left);
        nidx_t right = clone0(pool[root].right);
        return make(pool[root].aggregate, left, right);
    }

public:
    nidx_t set(nidx_t root, long long position, T value) {
        auto edit = [&](nidx_t) { return move(value); };
        return update0<false>(root, lo, hi, position, edit);
    }

    nidx_t combine(nidx_t root, long long position, const T& value) {
        auto edit = [&](nidx_t leaf) {
            return invoke(merge_values, leaf < 0 ? merge_values.id()
                                                 : move(pool[leaf].aggregate), value);
        };
        return update0<false>(root, lo, hi, position, edit);
    }

    nidx_t set_copy(nidx_t root, long long position, const T& value) {
        auto edit = [&](nidx_t) { return value; };
        return update0<true>(root, lo, hi, position, edit);
    }

    nidx_t combine_copy(nidx_t root, long long position, const T& value) {
        auto edit = [&](nidx_t leaf) {
            return invoke(merge_values, aggregate(leaf), value);
        };
        return update0<true>(root, lo, hi, position, edit);
    }

    T fold(nidx_t root, long long left, long long right) const {
        return fold0(root, lo, hi, left, right);
    }
    T fold(nidx_t root) const { return aggregate(root); }
    T get(nidx_t root, long long position) const { return fold(root, position, position + 1); }

    nidx_t merge(nidx_t left_root, nidx_t right_root) {
        return merge0<false>(left_root, right_root, lo, hi);
    }
    nidx_t merge_copy(nidx_t left_root, nidx_t right_root) {
        return merge0<true>(left_root, right_root, lo, hi);
    }
    nidx_t clone(nidx_t root) { return clone0(root); }
};
