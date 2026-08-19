#pragma once
#include "view.hpp"

template <class T>
struct nsum_group {
    constexpr T id() const { return T{}; }
    constexpr T operator()(T left, const T& right) const { return left += right; }
    constexpr T inverse(T value) const { return -value; }
};

/*
Fenwick updates and range subtraction rely on an Abelian group.  lower additionally
requires prefixes to be monotone under less and returns len() when no position reaches
the target.  No syntax machinery attempts to prove these algebraic laws.
*/
template <class T, class G = nsum_group<T>>
struct nfenwick {
    [[no_unique_address]] mutable G group;
    vector<T> tree;

    explicit nfenwick(nidx_t n = 0, G operation = {})
        : group(move(operation)), tree(n + 1, group.id()) {}

    template <class V>
    explicit nfenwick(V source, G operation = {}) : nfenwick(source.len(), move(operation)) {
        for (nidx_t i = 0; i < source.len(); ++i) tree[i + 1] = source[i];
        for (nidx_t i = 1; i < nidx_t(tree.size()); ++i) {
            nidx_t parent = i + (i & -i);
            if (parent < nidx_t(tree.size())) tree[parent] = invoke(this->group, tree[parent], tree[i]);
        }
    }

    nidx_t len() const { return nidx_t(tree.size()) - 1; }

    void add(nidx_t position, const T& delta) {
        for (++position; position < nidx_t(tree.size()); position += position & -position)
            tree[position] = invoke(group, move(tree[position]), delta);
    }

    T prefix(nidx_t right) const {
        T result = group.id();
        for (; right; right -= right & -right) result = invoke(group, move(result), tree[right]);
        return result;
    }

    T fold(nidx_t left, nidx_t right) const {
        return invoke(group, group.inverse(prefix(left)), prefix(right));
    }

    T get(nidx_t position) const { return fold(position, position + 1); }

    void set(nidx_t position, const T& value) {
        add(position, invoke(group, group.inverse(get(position)), value));
    }

    template <class Less = less<>>
    nidx_t lower(const T& target, Less less = {}) const {
        nidx_t position = 0;
        T prefix_value = group.id();
        for (nidx_t step = nidx_t(bit_floor(nuidx_t(len()))); step; step >>= 1) {
            nidx_t next = position + step;
            if (next <= len()) {
                T candidate = invoke(group, prefix_value, tree[next]);
                if (invoke(less, candidate, target)) position = next, prefix_value = move(candidate);
            }
        }
        return position;
    }
};

struct ndsu {
    vector<nidx_t> parent;
    explicit ndsu(nidx_t n = 0) : parent(n, -1) {}
    nidx_t len() const { return nidx_t(parent.size()); }
    nidx_t find(nidx_t vertex) {
        return parent[vertex] < 0 ? vertex : parent[vertex] = find(parent[vertex]);
    }
    bool same(nidx_t a, nidx_t b) { return find(a) == find(b); }
    nidx_t size(nidx_t vertex) { return -parent[find(vertex)]; }
    nidx_t merge(nidx_t a, nidx_t b) {
        a = find(a), b = find(b);
        if (a == b) return a;
        if (parent[a] > parent[b]) swap(a, b);
        parent[a] += parent[b];
        parent[b] = a;
        return a;
    }
};

/*
Weighted DSU over an Abelian group.  potential[x] stores value(x)-value(parent(x));
merge(a,b,delta) imposes value(b)-value(a)=delta and returns whether the new equation is
consistent (true also when it was already implied).  difference is nullopt across sets.
*/
template <class T, class G = nsum_group<T>>
struct npotential_dsu {
    [[no_unique_address]] mutable G group;
    vector<nidx_t> parent;
    vector<T> potential;

    explicit npotential_dsu(nidx_t n = 0, G operation = {})
        : group(move(operation)), parent(n, -1), potential(n, group.id()) {}

    nidx_t len() const { return nidx_t(parent.size()); }
    nidx_t find(nidx_t vertex) {
        if (parent[vertex] < 0) return vertex;
        nidx_t old_parent = parent[vertex];
        nidx_t root = find(old_parent);
        potential[vertex] = invoke(group, move(potential[vertex]), potential[old_parent]);
        return parent[vertex] = root;
    }
    T weight(nidx_t vertex) {
        find(vertex);
        return potential[vertex];
    }
    bool same(nidx_t a, nidx_t b) { return find(a) == find(b); }
    nidx_t size(nidx_t vertex) { return -parent[find(vertex)]; }

    optional<T> difference(nidx_t a, nidx_t b) {
        if (find(a) != find(b)) return nullopt;
        return invoke(group, group.inverse(potential[a]), potential[b]);
    }

    bool merge(nidx_t a, nidx_t b, const T& delta) {
        nidx_t root_a = find(a), root_b = find(b);
        T relation = invoke(group, invoke(group, delta, potential[a]),
                            group.inverse(potential[b]));
        if (root_a == root_b) return relation == group.id();
        if (parent[root_a] > parent[root_b]) {
            swap(root_a, root_b);
            relation = group.inverse(move(relation));
        }
        parent[root_a] += parent[root_b];
        parent[root_b] = root_a;
        potential[root_b] = move(relation);
        return true;
    }
};

/* time() counts successful merges; failed same-component merges write no history. */
struct nrollback_dsu {
    struct change { nidx_t child, old_size; };
    vector<nidx_t> parent;
    vector<change> history;
    explicit nrollback_dsu(nidx_t n = 0) : parent(n, -1) {}
    nidx_t find(nidx_t vertex) const {
        while (parent[vertex] >= 0) vertex = parent[vertex];
        return vertex;
    }
    bool same(nidx_t a, nidx_t b) const { return find(a) == find(b); }
    nidx_t size(nidx_t vertex) const { return -parent[find(vertex)]; }
    nidx_t time() const { return nidx_t(history.size()); }
    bool merge(nidx_t a, nidx_t b) {
        a = find(a), b = find(b);
        if (a == b) return false;
        if (parent[a] > parent[b]) swap(a, b);
        history.push_back({b, parent[b]});
        parent[a] += parent[b];
        parent[b] = a;
        return true;
    }
    void undo() {
        auto [child, old_size] = history.back();
        history.pop_back();
        nidx_t root = parent[child];
        parent[root] -= old_size;
        parent[child] = old_size;
    }
    void rollback(nidx_t target) {
        while (time() > target) undo();
    }
};

/* Ordered queue aggregation.  M has the same id/associativity contract as nseg. */
template <class T, class M>
struct nqueue_agg {
    struct node { T value, aggregate; };
    [[no_unique_address]] mutable M merge;
    vector<node> input, output;

    explicit nqueue_agg(M operation = {}) : merge(move(operation)) {}
    nidx_t len() const { return nidx_t(input.size() + output.size()); }
    bool empty() const { return input.empty() && output.empty(); }

    void push(T value) {
        T aggregate = input.empty() ? value : invoke(merge, input.back().aggregate, value);
        input.push_back({move(value), move(aggregate)});
    }

    void transfer() {
        if (!output.empty()) return;
        while (!input.empty()) {
            T value = move(input.back().value);
            input.pop_back();
            T aggregate = output.empty() ? value : invoke(merge, value, output.back().aggregate);
            output.push_back({move(value), move(aggregate)});
        }
    }

    const T& front() { transfer(); return output.back().value; }
    void pop() { transfer(); output.pop_back(); }
    T fold() const {
        if (output.empty()) return input.empty() ? merge.id() : input.back().aggregate;
        if (input.empty()) return output.back().aggregate;
        return invoke(merge, output.back().aggregate, input.back().aggregate);
    }
};

/*
Ordered deque aggregation.  M supplies id() and associative M(left,right); order is
never assumed commutative.  The deque is reverse(left) followed by right.  Rebuilding
an empty requested side gives it half the elements, so all operations are amortized
O(1), while one endpoint access/pop may take O(n).  Endpoints and pops require nonempty.

operator[] is deliberately read-only.  References may be invalidated by any operation
that changes or rebuilds storage, and an nall descriptor must not outlive a structural
mutation because it captured the old length.
*/
template <class T, class M>
struct ndeque_agg {
private:
    struct node { T value, aggregate; };
    [[no_unique_address]] mutable M merge;
    vector<node> left, right;

    void add_left(vector<node>& side, T value) {
        T aggregate = side.empty() ? value : invoke(merge, value, side.back().aggregate);
        side.push_back({move(value), move(aggregate)});
    }
    void add_right(vector<node>& side, T value) {
        T aggregate = side.empty() ? value : invoke(merge, side.back().aggregate, value);
        side.push_back({move(value), move(aggregate)});
    }

    void ensure_front() {
        if (!left.empty() || right.empty()) return;
        nidx_t n = nidx_t(right.size()), left_count = (n + 1) / 2;
        vector<node> next_left, next_right;
        next_left.reserve(left_count);
        next_right.reserve(n - left_count);
        for (nidx_t i = left_count; i > 0; --i)
            add_left(next_left, move(right[i - 1].value));
        for (nidx_t i = left_count; i < n; ++i)
            add_right(next_right, move(right[i].value));
        left.swap(next_left);
        right.swap(next_right);
    }

    void ensure_back() {
        if (!right.empty() || left.empty()) return;
        nidx_t n = nidx_t(left.size()), right_count = (n + 1) / 2;
        vector<node> next_left, next_right;
        next_left.reserve(n - right_count);
        next_right.reserve(right_count);
        for (nidx_t i = right_count; i < n; ++i)
            add_left(next_left, move(left[i].value));
        for (nidx_t i = right_count; i > 0; --i)
            add_right(next_right, move(left[i - 1].value));
        left.swap(next_left);
        right.swap(next_right);
    }

public:
    explicit ndeque_agg(M operation = {}) : merge(move(operation)) {}
    nidx_t len() const { return nidx_t(left.size() + right.size()); }
    bool empty() const { return left.empty() && right.empty(); }

    const T& operator[](nidx_t position) const {
        nidx_t left_count = nidx_t(left.size());
        return position < left_count ? left[left_count - 1 - position].value
                                     : right[position - left_count].value;
    }

    void push_front(T value) { add_left(left, move(value)); }
    void push_back(T value) { add_right(right, move(value)); }
    const T& front() { ensure_front(); return left.back().value; }
    const T& back() { ensure_back(); return right.back().value; }
    void pop_front() { ensure_front(); left.pop_back(); }
    void pop_back() { ensure_back(); right.pop_back(); }

    T fold() const {
        if (left.empty()) return right.empty() ? merge.id() : right.back().aggregate;
        if (right.empty()) return left.back().aggregate;
        return invoke(merge, left.back().aggregate, right.back().aggregate);
    }
};

/* O must be associative, commutative and idempotent; queries are nonempty [l,r). */
template <class T, class O>
struct nsparse_table {
    [[no_unique_address]] mutable O operation;
    vector<vector<T>> table;

    template <class V>
    explicit nsparse_table(V source, O merge = {}) : operation(move(merge)) {
        nidx_t n = source.len(), levels = n ? bit_width(nuidx_t(n)) : 0;
        if (!n) return;
        vector<T> first;
        first.reserve(n);
        for (nidx_t i = 0; i < n; ++i) first.push_back(source[i]);
        table.push_back(move(first));
        for (nidx_t level = 1; level < levels; ++level) {
            table.push_back(table[0]);
            for (nidx_t i = 0; i + (nidx_t(1) << level) <= n; ++i)
                table[level][i] = invoke(this->operation, table[level - 1][i],
                                          table[level - 1][i + (nidx_t(1) << (level - 1))]);
        }
    }

    nidx_t len() const { return table.empty() ? 0 : nidx_t(table[0].size()); }
    const T& get(nidx_t position) const { return table[0][position]; }
    T fold(nidx_t left, nidx_t right) const {
        nidx_t level = bit_width(nuidx_t(right - left)) - 1;
        return invoke(operation, table[level][left],
                      table[level][right - (nidx_t(1) << level)]);
    }
};

template <class V, class O>
nsparse_table(V, O) -> nsparse_table<remove_cvref_t<decltype(declval<V>()[0])>, O>;
