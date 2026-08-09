#pragma once
#include "segment.hpp"

/*
Link-cut forest for dynamic connectivity and ordered vertex-path aggregates.  M supplies
id() and associative M(left,right); commutativity is not required because every auxiliary
node stores both path directions.  Vertices are stable dense positions.  link joins two
components, cut removes an existing direct edge, and all operations are amortized O(log n).
*/
template <class T, class M = nadd<T>>
struct nlct {
    struct node {
        T value, forward, reverse;
        int left = -1, right = -1, parent = -1, size = 1;
        bool reversed = false;
    };
    [[no_unique_address]] mutable M merge;
    vector<node> nodes;

    explicit nlct(int n = 0, M operation = {}) : merge(move(operation)) {
        nodes.reserve(n);
        for (int i = 0; i < n; ++i) {
            T value = merge.id();
            nodes.push_back({value, value, move(value)});
        }
    }

    template <class V>
    explicit nlct(V values, M operation = {}) : merge(move(operation)) {
        nodes.reserve(values.len());
        for (int i = 0; i < values.len(); ++i) {
            T value = values[i];
            nodes.push_back({value, value, move(value)});
        }
    }

    int len() const { return int(nodes.size()); }
    int size(int vertex) const { return vertex < 0 ? 0 : nodes[vertex].size; }
    T forward(int vertex) const { return vertex < 0 ? merge.id() : nodes[vertex].forward; }
    T reverse(int vertex) const { return vertex < 0 ? merge.id() : nodes[vertex].reverse; }

    bool auxiliary_root(int vertex) const {
        int parent = nodes[vertex].parent;
        return parent < 0 || (nodes[parent].left != vertex && nodes[parent].right != vertex);
    }

    void pull(int vertex) {
        auto& current = nodes[vertex];
        current.size = 1 + size(current.left) + size(current.right);
        current.forward = invoke(merge, invoke(merge, forward(current.left), current.value),
                                 forward(current.right));
        current.reverse = invoke(merge, invoke(merge, reverse(current.right), current.value),
                                 reverse(current.left));
    }

    void toggle(int vertex) {
        if (vertex < 0) return;
        swap(nodes[vertex].left, nodes[vertex].right);
        swap(nodes[vertex].forward, nodes[vertex].reverse);
        nodes[vertex].reversed = !nodes[vertex].reversed;
    }

    void push(int vertex) {
        if (!nodes[vertex].reversed) return;
        toggle(nodes[vertex].left);
        toggle(nodes[vertex].right);
        nodes[vertex].reversed = false;
    }

    void rotate(int vertex) {
        int parent = nodes[vertex].parent, grandparent = nodes[parent].parent;
        bool right_child = nodes[parent].right == vertex;
        int middle = right_child ? nodes[vertex].left : nodes[vertex].right;
        if (!auxiliary_root(parent)) {
            if (nodes[grandparent].left == parent) nodes[grandparent].left = vertex;
            else nodes[grandparent].right = vertex;
        }
        nodes[vertex].parent = grandparent;
        if (right_child) {
            nodes[vertex].left = parent;
            nodes[parent].right = middle;
        } else {
            nodes[vertex].right = parent;
            nodes[parent].left = middle;
        }
        nodes[parent].parent = vertex;
        if (middle >= 0) nodes[middle].parent = parent;
        pull(parent);
        pull(vertex);
    }

    void splay(int vertex) {
        vector<int> path{vertex};
        for (int at = vertex; !auxiliary_root(at); at = nodes[at].parent)
            path.push_back(nodes[at].parent);
        for (auto it = path.rbegin(); it != path.rend(); ++it) push(*it);
        while (!auxiliary_root(vertex)) {
            int parent = nodes[vertex].parent;
            if (!auxiliary_root(parent)) {
                int grandparent = nodes[parent].parent;
                bool zigzig = (nodes[parent].left == vertex) ==
                              (nodes[grandparent].left == parent);
                rotate(zigzig ? parent : vertex);
            }
            rotate(vertex);
        }
    }

    int access(int vertex) {
        int previous = -1;
        for (int current = vertex; current >= 0; current = nodes[current].parent) {
            splay(current);
            nodes[current].right = previous;
            if (previous >= 0) nodes[previous].parent = current;
            pull(current);
            previous = current;
        }
        splay(vertex);
        return previous;
    }

    void make_root(int vertex) {
        access(vertex);
        toggle(vertex);
    }

    int find_root(int vertex) {
        access(vertex);
        push(vertex);
        while (nodes[vertex].left >= 0) vertex = nodes[vertex].left, push(vertex);
        splay(vertex);
        return vertex;
    }

    bool connected(int a, int b) {
        if (a == b) return true;
        make_root(a);
        return find_root(b) == a;
    }

    void link(int a, int b) {
        make_root(a);
        nodes[a].parent = b;
    }

    void cut(int a, int b) {
        make_root(a);
        access(b);
        nodes[b].left = -1;
        nodes[a].parent = -1;
        pull(b);
    }

    void set(int vertex, T value) {
        access(vertex);
        nodes[vertex].value = move(value);
        pull(vertex);
    }

    T get(int vertex) {
        access(vertex);
        return nodes[vertex].value;
    }

    T fold(int a, int b) {
        make_root(a);
        access(b);
        return nodes[b].forward;
    }

    int path_size(int a, int b) {
        make_root(a);
        access(b);
        return nodes[b].size;
    }
};

template <class V, class M>
nlct(V, M) -> nlct<remove_cvref_t<decltype(declval<V>()[0])>, M>;
