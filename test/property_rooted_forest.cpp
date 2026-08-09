#include "common.hpp"

template <class A> static vector<int> collect_ints(const A& values) {
    vector<int> result;
    nfor(value, values)
        result.push_back(value);
    return result;
}

static vector<int> path_vertices(const nhld& decomposition, int from, int to) {
    vector<int> result;
    nfor(segment, decomposition.path(from, to)) {
        if (segment.rev) {
            for (int position = segment.r; position-- > segment.l;)
                result.push_back(decomposition.vertex(position));
        } else {
            for (int position = segment.l; position < segment.r; ++position)
                result.push_back(decomposition.vertex(position));
        }
    }
    return result;
}

static nvector<string> reroot_words(const auto& graph) {
    return nreroot(
        graph, string{}, [](string left, string right) { return left + right; },
        [](string aggregate, int vertex) {
            return "(" + to_string(vertex) + ":" + aggregate + ")";
        },
        [](string state, int from, int to) {
            return "[" + to_string(from) + ">" + to_string(to) + ":" + state + "]";
        });
}

static int check_forest(const nrooted_forest<int>& actual,
                        const nrooted_forest<int>& expected,
                        const nvector<int>& parent) {
    ntest(actual.len() == expected.len() && actual.len() == parent.len());
    ntest(actual.components() == expected.components());
    ntest(ncollect(actual.order()) == ncollect(expected.order()));
    ntest(ncollect(actual.roots()) == ncollect(expected.roots()));
    for (int vertex = 0; vertex < actual.len(); ++vertex) {
        ntest(actual.parent(vertex) == parent[vertex]);
        ntest(actual.component(vertex) == expected.component(vertex));
        ntest(actual.root(vertex) == expected.root(vertex));
        ntest(actual.depth(vertex) == expected.depth(vertex));
        ntest(actual.position(vertex) == expected.position(vertex));
        ntest(actual.subtree_size(vertex) == expected.subtree_size(vertex));
        ntest(collect_ints(actual.children(vertex)) == collect_ints(expected.children(vertex)));
    }
    return 0;
}

int main() {
    mt19937 rng(0x6e69746fU);
    for (int trial = 0; trial < 260; ++trial) {
        int vertices = 1 + int(rng() % 42);
        nvector<int> parent(vertices, 0);
        parent[0] = 0;
        for (int vertex = 1; vertex < vertices; ++vertex)
            parent[vertex] = (rng() % 4 == 0) ? vertex : int(rng() % vertex);

        nrooted_forest<int> from_parent(parent);
        ngraph_list<int> symmetric(vertices), directed(vertices);
        for (int vertex = 1; vertex < vertices; ++vertex) {
            if (parent[vertex] == vertex)
                continue;
            symmetric.add2(parent[vertex], vertex);
            directed.add(parent[vertex], vertex);
        }
        nrooted_forest<int> from_symmetric(symmetric, 0, true);
        nrooted_forest<int> from_directed(directed, 0, false);
        ntest(check_forest(from_symmetric, from_parent, parent) == 0);
        ntest(check_forest(from_directed, from_parent, parent) == 0);
        ntest(ncollect(from_parent.order()) == ncollect(from_symmetric.order()));

        for (int source = 0; source < vertices; ++source) {
            ntest(nbfs(from_parent, source) == nbfs(symmetric, source));
            nlca_binary<int> left(from_parent), right(symmetric);
            for (int target = 0; target < vertices; ++target) {
                ntest(left.same(source, target) == right.same(source, target));
                ntest(left.lca(source, target) == right.lca(source, target));
                ntest(left.dist(source, target, -77) == right.dist(source, target, -77));
                int steps = int(rng() % (vertices + 3));
                ntest(left.kth(source, target, steps) == right.kth(source, target, steps));
            }
        }
    }

    for (int trial = 0; trial < 180; ++trial) {
        int vertices = 1 + int(rng() % 55);
        nvector<int> parent(vertices, 0);
        parent[0] = 0;
        for (int vertex = 1; vertex < vertices; ++vertex)
            parent[vertex] = int(rng() % vertex);
        nrooted_forest<int> actual(parent);
        ngraph_list<int> reference(vertices);
        for (int vertex = 1; vertex < vertices; ++vertex)
            reference.add2(parent[vertex], vertex);

        nhld left(actual), right(reference);
        for (int query = 0; query < 160; ++query) {
            int from = int(rng() % vertices), to = int(rng() % vertices);
            ntest(path_vertices(left, from, to) == path_vertices(right, from, to));
            ntest(left.lca(from, to) == right.lca(from, to));
        }
        ntest(reroot_words(actual) == reroot_words(reference));
    }
}
