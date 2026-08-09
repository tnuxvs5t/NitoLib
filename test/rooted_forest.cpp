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

int main() {
    static_assert(ngraph_like<nrooted_forest<int>>);

    nrooted_forest<int> empty(0);
    ntest(empty.empty() && empty.len() == 0 && empty.vertices() == 0);
    ntest(empty.components() == 0 && empty.edges() == 0 && empty.layout().adjacency.empty());

    nrooted_forest<int> isolated(4);
    ntest(isolated.components() == 4 && !isolated.connected());
    ntest(collect_ints(isolated.roots()) == vector<int>({0, 1, 2, 3}));
    ntest(collect_ints(isolated.order()) == vector<int>({0, 1, 2, 3}));
    for (int vertex = 0; vertex < isolated.len(); ++vertex) {
        ntest(isolated.parent(vertex) == vertex && isolated.root(vertex) == vertex);
        ntest(isolated.depth(vertex) == 0 && isolated.subtree_size(vertex) == 1);
        ntest(isolated.children(vertex).empty());
    }

    nvector<int> multi_parent{0, 0, 2, 2, 4};
    nrooted_forest<int> multi(multi_parent);
    ntest(multi.components() == 3 && multi.edges() == 2);
    ntest(collect_ints(multi.roots()) == vector<int>({0, 2, 4}));
    ntest(collect_ints(multi.order()) == vector<int>({0, 1, 2, 3, 4}));
    ntest(multi.subtree_size(2) == 2 && collect_ints(multi.children(2)) == vector<int>({3}));

    nvector<int> parent{0, 0, 0, 1, 1, 3, 5};
    nrooted_forest<int> tree(parent);
    ntest(tree.len() == 7 && tree.vertices() == 7 && tree.edges() == 6);
    ntest(tree.components() == 1 && tree.connected());
    ntest(tree.root(6) == 0 && tree.parent(6) == 5 && tree.depth(6) == 4);
    ntest(tree.position(0) == 0 && tree.vertex(6) == 6 && tree.subtree_size(1) == 5);
    ntest(collect_ints(tree.children(0)) == vector<int>({1, 2}));
    ntest(collect_ints(tree.children(1)) == vector<int>({3, 4}));
    ntest(tree.child_count(6) == 0);
    ntest(tree.vertex_node(0).val().kind == ngraph_node_kind::vertex);

    auto layout = tree.layout();
    ntest(layout.parent == parent);
    ntest(layout.order == nvector<int>({0, 1, 2, 3, 4, 5, 6}));
    ntest(layout.adjacency[1] == vector<int>({0, 3, 4}));

    ngraph_list<int> symmetric(7);
    for (int vertex = 1; vertex < parent.len(); ++vertex)
        symmetric.add2(parent[vertex], vertex);
    nrooted_forest<int> from_symmetric(symmetric, 0, true);
    ntest(from_symmetric.layout().parent == parent);
    ntest(from_symmetric.layout().adjacency == layout.adjacency);
    ntest(ncollect(from_symmetric.order()) == ncollect(tree.order()));
    ntest(nbfs(from_symmetric, 6) == nbfs(symmetric, 6));

    auto lca = nlca(tree);
    ntest(lca(6, 4) == 1 && lca.distance(6, 2) == 5);
    ntest(lca.kth_on_path(6, 2, 0) == 6 && lca.kth_on_path(6, 2, 5) == 2);

    nhld decomposition(tree);
    ntest(path_vertices(decomposition, 6, 2) == vector<int>({6, 5, 3, 1, 0, 2}));
    ntest(decomposition.subtree(1).second - decomposition.subtree(1).first == 5);
    ntest(reroot_words(tree) == reroot_words(symmetric));

    ngraph_list<int> directed(8);
    directed.add(0, 1);
    directed.add(0, 2);
    directed.add(1, 3);
    directed.add(4, 5);
    directed.add(5, 6);
    directed.add(5, 7);
    nrooted_forest<int> forest(directed, 0);
    ntest(forest.components() == 2 && !forest.connected());
    ntest(forest.root(3) == 0 && forest.root(7) == 4);
    ntest(forest.parent(7) == 5 && forest.depth(7) == 2);
    ntest(forest.edges() == 6);
    nlca_binary<int> forest_lca(forest);
    ntest(forest_lca.same(3, 7) == false && forest_lca.lca(3, 7) == npos);
    ntest(forest_lca.lca(3, 2) == 0 && forest_lca.kth(4, 7, 2) == 7);

    ngraph_list<int> mixed(6);
    mixed.add(0, 1);          // directed component
    mixed.add2(2, 3);         // symmetric component
    mixed.add(4, 5);          // another directed component
    nrooted_forest<int> mixed_forest(mixed, 0);
    ntest(mixed_forest.components() == 3 && mixed_forest.edges() == 3);
    ntest(collect_ints(mixed_forest.roots()) == vector<int>({0, 2, 4}));
    ntest(mixed_forest.layout().adjacency[0] == vector<int>({1}));
    ntest(mixed_forest.layout().adjacency[1] == vector<int>({0}));

    tree.set_vertex(0, 42);
    auto current = tree.vertex_node(0);
    ntest(current.current() && current.val().payload && *current.val().payload == 42);

    auto shared = tree.domain();
    auto stale = tree.vertex_node(1);
    nrooted_forest<int> sibling(shared, parent);
    ntest(tree.same_domain(sibling) && !stale.current());

    auto original = tree.vertex_node(2);
    nrooted_forest<int> copy = tree;
    ntest(original.current() && !tree.same_domain(copy) && copy.vertex_node(2).current());
    nrooted_forest<int> moved = move(tree);
    ntest(!original.current() && moved.vertex_node(2).current());
}
