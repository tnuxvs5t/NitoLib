#pragma once
#include "arena.hpp"
#include "view.hpp"

template <class T>
struct nadd {
    constexpr T id() const { return T{}; }
    constexpr T operator()(T left, const T& right) const { return left += right; }
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
A supplies tag_id(), compose(newer,older), and apply(aggregate,tag,length).
Composition means older executes first.  apply must distribute over interval merge.
Queries push tags and therefore are logically const but physically mutating.
*/
template <class S, class F, class M, class A>
struct nlazyseg {
    [[no_unique_address]] M merge;
    [[no_unique_address]] A action;
    nidx_t length = 0, base = 1;
    vector<S> tree;
    vector<F> lazy;
    vector<unsigned char> pending;

    explicit nlazyseg(nidx_t n = 0, M operation = {}, A action_policy = {})
        : merge(move(operation)), action(move(action_policy)), length(n),
          base(nidx_t(bit_ceil(nuidx_t(max(nidx_t(1), n))))),
          tree(size_t(2) * base, merge.id()),
          lazy(size_t(2) * base, this->action.tag_id()), pending(size_t(2) * base) {}

    template <class V>
    explicit nlazyseg(V source, M operation = {}, A action_policy = {})
        : nlazyseg(source.len(), move(operation), move(action_policy)) {
        for (nidx_t i = 0; i < length; ++i) tree[base + i] = source[i];
        for (nidx_t i = base; --i;) pull(i);
    }

    nidx_t len() const { return length; }
    bool empty() const { return !length; }

    void put(nidx_t node, nidx_t width, const F& tag) {
        tree[node] = action.apply(move(tree[node]), tag, width);
        if (pending[node])
            lazy[node] = action.compose(tag, lazy[node]);
        else
            lazy[node] = tag, pending[node] = true;
    }

    void push(nidx_t node, nidx_t left, nidx_t right) {
        if (!pending[node] || left + 1 == right) return;
        nidx_t middle = midpoint(left, right);
        put(node << 1, middle - left, lazy[node]);
        put(node << 1 | 1, right - middle, lazy[node]);
        lazy[node] = action.tag_id();
        pending[node] = false;
    }

    void pull(nidx_t node) { tree[node] = invoke(merge, tree[node << 1], tree[node << 1 | 1]); }

  private:
    void apply0(nidx_t node, nidx_t left, nidx_t right, nidx_t query_left, nidx_t query_right, const F& tag) {
        if (query_left <= left && right <= query_right) return put(node, right - left, tag);
        push(node, left, right);
        nidx_t middle = midpoint(left, right);
        if (query_left < middle) apply0(node << 1, left, middle, query_left, query_right, tag);
        if (middle < query_right) apply0(node << 1 | 1, middle, right, query_left, query_right, tag);
        pull(node);
    }

    S fold0(nidx_t node, nidx_t left, nidx_t right, nidx_t query_left, nidx_t query_right) {
        if (query_left <= left && right <= query_right) return tree[node];
        push(node, left, right);
        nidx_t middle = midpoint(left, right);
        if (query_right <= middle) return fold0(node << 1, left, middle, query_left, query_right);
        if (middle <= query_left) return fold0(node << 1 | 1, middle, right, query_left, query_right);
        return invoke(merge, fold0(node << 1, left, middle, query_left, query_right),
                      fold0(node << 1 | 1, middle, right, query_left, query_right));
    }

    void set0(nidx_t node, nidx_t left, nidx_t right, nidx_t position, S value) {
        if (left + 1 == right) {
            tree[node] = move(value);
            lazy[node] = action.tag_id();
            pending[node] = false;
            return;
        }
        push(node, left, right);
        nidx_t middle = midpoint(left, right);
        if (position < middle)
            set0(node << 1, left, middle, position, move(value));
        else
            set0(node << 1 | 1, middle, right, position, move(value));
        pull(node);
    }

  public:
    void apply(nidx_t left, nidx_t right, const F& tag) {
        if (left < right) apply0(1, 0, base, left, right, tag);
    }

    S fold(nidx_t left, nidx_t right) {
        return left == right ? merge.id() : fold0(1, 0, base, left, right);
    }

    S fold() const { return length ? tree[1] : merge.id(); }
    S get(nidx_t position) { return fold(position, position + 1); }
    void set(nidx_t position, S value) { set0(1, 0, base, position, move(value)); }
};

template <class T>
struct naddsum_action {
    constexpr T tag_id() const { return T{}; }
    constexpr T compose(const T& newer, const T& older) const { return older + newer; }
    constexpr T apply(T sum, const T& tag, nidx_t length) const { return sum + tag * T(length); }
};

template <class T>
using nlazy_addsum = nlazyseg<T, T, nadd<T>, naddsum_action<T>>;

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

    nidx_t set0(nidx_t root, long long left, long long right, long long position, T value) {
        if (root < 0) root = make(merge_values.id());
        if (left + 1 == right) {
            pool[root].aggregate = move(value);
            return root;
        }
        long long middle = midpoint(left, right);
        if (position < middle)
            pool[root].left = set0(pool[root].left, left, middle, position, move(value));
        else
            pool[root].right = set0(pool[root].right, middle, right, position, move(value));
        pull(root);
        return root;
    }

    nidx_t combine0(nidx_t root, long long left, long long right, long long position, const T& value) {
        if (root < 0) root = make(merge_values.id());
        if (left + 1 == right) {
            pool[root].aggregate = invoke(merge_values, move(pool[root].aggregate), value);
            return root;
        }
        long long middle = midpoint(left, right);
        if (position < middle)
            pool[root].left = combine0(pool[root].left, left, middle, position, value);
        else
            pool[root].right = combine0(pool[root].right, middle, right, position, value);
        pull(root);
        return root;
    }

    nidx_t set_copy0(nidx_t root, long long left, long long right, long long position, const T& value) {
        if (left + 1 == right) return make(value);
        nidx_t a = root < 0 ? -1 : pool[root].left;
        nidx_t b = root < 0 ? -1 : pool[root].right;
        long long middle = midpoint(left, right);
        if (position < middle)
            a = set_copy0(a, left, middle, position, value);
        else
            b = set_copy0(b, middle, right, position, value);
        return make(invoke(merge_values, aggregate(a), aggregate(b)), a, b);
    }

    nidx_t combine_copy0(nidx_t root, long long left, long long right, long long position,
                      const T& value) {
        if (left + 1 == right) return make(invoke(merge_values, aggregate(root), value));
        nidx_t a = root < 0 ? -1 : pool[root].left;
        nidx_t b = root < 0 ? -1 : pool[root].right;
        long long middle = midpoint(left, right);
        if (position < middle)
            a = combine_copy0(a, left, middle, position, value);
        else
            b = combine_copy0(b, middle, right, position, value);
        return make(invoke(merge_values, aggregate(a), aggregate(b)), a, b);
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

    nidx_t merge0(nidx_t left_root, nidx_t right_root, long long left, long long right) {
        if (left_root < 0) return right_root;
        if (right_root < 0) return left_root;
        if (left + 1 == right) {
            pool[left_root].aggregate = invoke(merge_values, move(pool[left_root].aggregate),
                                               pool[right_root].aggregate);
            return left_root;
        }
        long long middle = midpoint(left, right);
        pool[left_root].left = merge0(pool[left_root].left, pool[right_root].left, left, middle);
        pool[left_root].right = merge0(pool[left_root].right, pool[right_root].right, middle, right);
        pull(left_root);
        return left_root;
    }

    nidx_t merge_copy0(nidx_t left_root, nidx_t right_root, long long left, long long right) {
        if (left_root < 0) return right_root;
        if (right_root < 0) return left_root;
        if (left + 1 == right)
            return make(invoke(merge_values, pool[left_root].aggregate,
                               pool[right_root].aggregate));
        long long middle = midpoint(left, right);
        nidx_t a = merge_copy0(pool[left_root].left, pool[right_root].left, left, middle);
        nidx_t b = merge_copy0(pool[left_root].right, pool[right_root].right, middle, right);
        return make(invoke(merge_values, aggregate(a), aggregate(b)), a, b);
    }

    nidx_t clone0(nidx_t root) {
        if (root < 0) return -1;
        nidx_t left = clone0(pool[root].left), right = clone0(pool[root].right);
        return make(pool[root].aggregate, left, right);
    }

  public:
    nidx_t set(nidx_t root, long long position, T value) {
        return set0(root, lo, hi, position, move(value));
    }

    nidx_t combine(nidx_t root, long long position, const T& value) {
        return combine0(root, lo, hi, position, value);
    }

    nidx_t set_copy(nidx_t root, long long position, const T& value) {
        return set_copy0(root, lo, hi, position, value);
    }

    nidx_t combine_copy(nidx_t root, long long position, const T& value) {
        return combine_copy0(root, lo, hi, position, value);
    }

    T fold(nidx_t root, long long left, long long right) const {
        return fold0(root, lo, hi, left, right);
    }

    T fold(nidx_t root) const { return aggregate(root); }
    T get(nidx_t root, long long position) const { return fold(root, position, position + 1); }

    nidx_t merge(nidx_t left_root, nidx_t right_root) {
        return merge0(left_root, right_root, lo, hi);
    }

    nidx_t merge_copy(nidx_t left_root, nidx_t right_root) {
        return merge_copy0(left_root, right_root, lo, hi);
    }

    nidx_t clone(nidx_t root) { return clone0(root); }
};
