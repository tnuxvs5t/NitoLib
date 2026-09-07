#pragma once
#include "arena.hpp"
#include "view.hpp"

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

template <class... T>
struct nadd<tuple<T...>> {
    constexpr tuple<T...> id() const { return {nadd<T>{}.id()...}; }
    constexpr tuple<T...> operator()(tuple<T...> left, const tuple<T...>& right) const {
        return [&]<size_t... I>(index_sequence<I...>) {
            return tuple<T...>{nadd<T>{}(move(get<I>(left)), get<I>(right))...};
        }(index_sequence_for<T...>{});
    }
};

/* Numeric extrema adapters.  T has numeric_limits bounds; values exclude NaN. */
template <class T>
struct nmin {
    constexpr T id() const {
        if constexpr (numeric_limits<T>::has_infinity)
            return numeric_limits<T>::infinity();
        else
            return numeric_limits<T>::max();
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

template <class... T>
struct nmin<tuple<T...>> {
    constexpr tuple<T...> id() const { return {nmin<T>{}.id()...}; }
    constexpr tuple<T...> operator()(tuple<T...> left, const tuple<T...>& right) const {
        return right < left ? right : move(left);
    }
};

template <class T>
struct nmax {
    constexpr T id() const {
        if constexpr (numeric_limits<T>::has_infinity)
            return -numeric_limits<T>::infinity();
        else
            return numeric_limits<T>::lowest();
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

template <class... T>
struct nmax<tuple<T...>> {
    constexpr tuple<T...> id() const { return {nmax<T>{}.id()...}; }
    constexpr tuple<T...> operator()(tuple<T...> left, const tuple<T...>& right) const {
        return left < right ? right : move(left);
    }
};

namespace nsegment_detail {
template <class F, class I>
constexpr void emit(F& visit, nidx_t node, I left, I right) {
    if constexpr (requires { invoke(visit, node, left, right); })
        invoke(visit, node, left, right);
    else
        invoke(visit, node);
}
}

/*
Pure segment-topology walks.  root < 0 is absent; child(node,side) returns the existing
or newly opened child and may return a negative handle to stop.  trace visits root to
leaf.  cover visits the left-to-right canonical partition of [query_left,query_right).
The visitor may accept node alone or (node,left,right).  Coordinates and the query are
valid half-open intervals; no aggregate, tag, push, pull or storage policy is implied.
*/
template <class I, class C, class F>
constexpr void nsegment_trace(nidx_t root, I left, I right, I position,
                              C&& child, F&& visit) {
    for (nidx_t node = root; node >= 0;) {
        nsegment_detail::emit(visit, node, left, right);
        if (left + 1 == right) break;
        I middle = midpoint(left, right);
        nidx_t side = position < middle ? 0 : 1;
        node = invoke(child, node, side);
        if (side) left = middle;
        else right = middle;
    }
}

template <class I, class C, class F>
constexpr void nsegment_cover(nidx_t root, I left, I right, I query_left, I query_right,
                              C&& child, F&& visit) {
    if (root < 0 || query_left == query_right) return;
    auto walk = [&](auto&& self, nidx_t node, I node_left, I node_right) -> void {
        if (node < 0 || query_right <= node_left || node_right <= query_left) return;
        if (query_left <= node_left && node_right <= query_right) {
            nsegment_detail::emit(visit, node, node_left, node_right);
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

/* Static heap topology: root 1 covers [0,base), children are node*2+side. */
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

/* M supplies id() and associative M(left,right); order is never assumed commutative. */
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
    explicit nseg(V source, M operation = {}) : nseg(source.len(), move(operation)) {
        for (nidx_t i = 0; i < length; ++i) tree[base + i] = source[i];
        for (nidx_t i = base; --i;) tree[i] = invoke(merge, tree[i << 1], tree[i << 1 | 1]);
    }

    nidx_t len() const { return length; }
    bool empty() const { return !length; }
    const T& get(nidx_t position) const { return tree[base + position]; }

    void set(nidx_t position, T value) {
        nidx_t node = base + position;
        tree[node] = move(value);
        while (node >>= 1) tree[node] = invoke(merge, tree[node << 1], tree[node << 1 | 1]);
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

    /* Pointwise leaf merge; lengths and operation meanings must agree. */
    void pointwise(const nseg& other) {
        for (nidx_t i = 0; i < base; ++i)
            tree[base + i] = invoke(merge, move(tree[base + i]), other.tree[base + i]);
        for (nidx_t i = base; --i;) tree[i] = invoke(merge, tree[i << 1], tree[i << 1 | 1]);
    }
};

template <class V, class M>
nseg(V, M) -> nseg<remove_cvref_t<decltype(declval<V>()[0])>, M>;

/*
One policy owns node semantics: identity(), make(value), join(left,right), and optional
init(tree), pull(tree,node), push(tree,node,left,right).  Missing pull uses join.
try_apply(tree,node,left,right,command) completes a whole node or returns false without
changing its semantic state; valid commands must succeed at leaves.  Deferred state
belongs to the policy/payload, not to the traversal.  Padding is identity, never updated.
All query ranges lie in [0,len()).  Cost is visited nodes times local policy cost;
extra descent requires an algorithm-specific amortized proof.  Queries may push.
*/
template <class T, class Ops>
struct nlazyseg {
    [[no_unique_address]] Ops ops;
    nidx_t length = 0, base = 1;
    vector<T> tree;

    /* n identity leaves; use a source when identity is not a valid element. */
    explicit nlazyseg(nidx_t n = 0, Ops policy = {})
        : ops(move(policy)), length(n),
          base(nidx_t(bit_ceil(nuidx_t(max(nidx_t(1), n))))),
          tree(size_t(2) * base, ops.identity()) {
        if constexpr (requires { ops.init(*this); }) ops.init(*this);
    }

    template <class V>
    requires requires(V& source) { source.len(); source[0]; }
    explicit nlazyseg(V source, Ops policy = {})
        : nlazyseg(source.len(), move(policy)) {
        for (nidx_t i = 0; i < length; ++i) tree[base + i] = ops.make(source[i]);
        for (nidx_t i = base; --i;) pull(i);
    }

    nidx_t len() const { return length; }
    bool empty() const { return !length; }
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

    /*
    visit(tree,node,left,right,full) returns true to prune, false to descend left then
    right with push/pull.  It must stop at leaves.  A partial node may be inspected or
    skipped, but must not be changed as though fully covered.  No topology mutation.
    Public push/pull and heap indices also permit custom branch order or multi-tree walks.
    */
    template <class V>
    void walk(nidx_t left, nidx_t right, V&& visit) {
        if (left == right) return;
        auto descend = [&](auto&& self, nidx_t node, nidx_t lo, nidx_t hi) -> void {
            if (right <= lo || hi <= left) return;
            if (invoke(visit, *this, node, lo, hi, left <= lo && hi <= right)) return;
            assert(lo + 1 < hi);
            if (lo + 1 == hi) return;
            push(node, lo, hi);
            nidx_t mid = midpoint(lo, hi);
            self(self, node << 1, lo, mid);
            self(self, node << 1 | 1, mid, hi);
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
        walk(position, position + 1, [&](auto&, nidx_t node, nidx_t, nidx_t, bool full) {
            if (full) tree[node] = ops.make(forward<U>(value));
            return full;
        });
    }
};

template <class V, class Ops>
nlazyseg(V, Ops) -> nlazyseg<remove_cvref_t<decltype(declval<Ops&>().make(declval<V&>()[0]))>, Ops>;

/* Ordinary lazy assembly.  M is an ordered monoid.  A supplies tag_id(),
compose(newer,older) (older first), apply(value,tag,length), distributing over M.
Only internal nodes store tags.  Point replacement therefore has no stale leaf tag.
*/
template <class M, class A>
struct nlazy_ops {
    using T = remove_cvref_t<decltype(declval<M&>().id())>;
    using F = remove_cvref_t<decltype(declval<A&>().tag_id())>;
    [[no_unique_address]] M merge;
    [[no_unique_address]] A action;
    vector<F> lazy;
    vector<unsigned char> pending;

    nlazy_ops(M operation = {}, A action_policy = {})
        : merge(move(operation)), action(move(action_policy)) {}
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
            lazy[node] = pending[node] ? action.compose(tag, lazy[node]) : tag;
            pending[node] = true;
        }
        return true;
    }
    template <class Q>
    void push(Q& q, nidx_t node, nidx_t left, nidx_t right) {
        if (!pending[node]) return;
        nidx_t mid = midpoint(left, right);
        try_apply(q, node << 1, left, mid, lazy[node]);
        try_apply(q, node << 1 | 1, mid, right, lazy[node]);
        pending[node] = false;
    }
};

template <class T>
struct naddsum_action {
    constexpr T tag_id() const { return T{}; }
    constexpr T compose(const T& newer, const T& older) const { return older + newer; }
    constexpr T apply(T sum, const T& tag, nidx_t length) const { return sum + tag * T(length); }
};

template <class T>
using nlazy_addsum = nlazyseg<T, nlazy_ops<nadd<T>, naddsum_action<T>>>;

/*
Sparse ordered segment kernel on [lo,hi).  -1 is an absent identity subtree and roots
are plain handles into one append-only arena.  set/combine/merge mutate reachable nodes
and require their input roots to have disjoint ownership.  set_copy/combine_copy and
merge_copy never mutate old nodes and may share untouched subtrees.  Mixing a shared
persistent root into a destructive operation is a contract violation; clone first.
*/
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

    /*
    Structural escape hatch.  If aggregate operations remain in use, value and children
    already satisfy their aggregate invariant.  A later make may invalidate references,
    never integer handles.  The zero-argument form creates an identity aggregate.
    */
    nidx_t make(T value, nidx_t left = -1, nidx_t right = -1) {
        return pool.make(node{move(value), left, right});
    }

    nidx_t make() { return make(merge_values.id()); }
    T aggregate(nidx_t root) const { return root < 0 ? merge_values.id() : pool[root].aggregate; }
    void pull(nidx_t root) {
        pool[root].aggregate = invoke(merge_values, aggregate(pool[root].left),
                                     aggregate(pool[root].right));
    }

  private:

    template <bool Copy>
    nidx_t store(nidx_t root, T value, nidx_t a = -1, nidx_t b = -1) {
        if constexpr (Copy) return make(move(value), a, b);
        else {
            if (root < 0) return make(move(value), a, b);
            pool[root] = node{move(value), a, b};
            return root;
        }
    }

    /* Only handles cross recursion: child allocation may relocate the arena. */
    template <bool Copy, class F>
    nidx_t update0(nidx_t root, long long left, long long right, long long position, F& edit) {
        if (left + 1 == right) return store<Copy>(root, invoke(edit, root));
        nidx_t a = root < 0 ? -1 : pool[root].left;
        nidx_t b = root < 0 ? -1 : pool[root].right;
        long long middle = midpoint(left, right);
        if (position < middle)
            a = update0<Copy>(a, left, middle, position, edit);
        else
            b = update0<Copy>(b, middle, right, position, edit);
        return store<Copy>(root, invoke(merge_values, aggregate(a), aggregate(b)), a, b);
    }

    T fold0(nidx_t root, long long left, long long right,
            long long query_left, long long query_right) const {
        if (root < 0 || query_right <= left || right <= query_left) return merge_values.id();
        if (query_left <= left && right <= query_right) return pool[root].aggregate;
        long long middle = midpoint(left, right);
        return invoke(merge_values,
                      fold0(pool[root].left, left, middle, query_left, query_right),
                      fold0(pool[root].right, middle, right, query_left, query_right));
    }

    template <bool Copy>
    nidx_t merge0(nidx_t left_root, nidx_t right_root, long long left, long long right) {
        if (left_root < 0) return right_root;
        if (right_root < 0) return left_root;
        if (left + 1 == right) {
            if constexpr (Copy)
                return make(invoke(merge_values, pool[left_root].aggregate, pool[right_root].aggregate));
            else
                return store<false>(left_root, invoke(merge_values, move(pool[left_root].aggregate),
                                                     pool[right_root].aggregate));
        }
        long long middle = midpoint(left, right);
        nidx_t a = merge0<Copy>(pool[left_root].left, pool[right_root].left, left, middle);
        nidx_t b = merge0<Copy>(pool[left_root].right, pool[right_root].right, middle, right);
        return store<Copy>(left_root, invoke(merge_values, aggregate(a), aggregate(b)), a, b);
    }

    nidx_t clone0(nidx_t root) {
        if (root < 0) return -1;
        nidx_t left = clone0(pool[root].left), right = clone0(pool[root].right);
        return make(pool[root].aggregate, left, right);
    }

  public:
    nidx_t set(nidx_t root, long long position, T value) {
        auto edit = [&](nidx_t) { return move(value); };
        return update0<false>(root, lo, hi, position, edit);
    }

    nidx_t combine(nidx_t root, long long position, const T& value) {
        auto edit = [&](nidx_t leaf) {
            return invoke(merge_values, leaf < 0 ? merge_values.id() : move(pool[leaf].aggregate), value);
        };
        return update0<false>(root, lo, hi, position, edit);
    }

    nidx_t set_copy(nidx_t root, long long position, const T& value) {
        auto edit = [&](nidx_t) { return value; };
        return update0<true>(root, lo, hi, position, edit);
    }

    nidx_t combine_copy(nidx_t root, long long position, const T& value) {
        auto edit = [&](nidx_t leaf) { return invoke(merge_values, aggregate(leaf), value); };
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
