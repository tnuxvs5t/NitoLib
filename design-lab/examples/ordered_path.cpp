#include "../include/hld.hpp"
#include "../include/segtree.hpp"
#include <iostream>
#include <string>

// Direction is part of the value, not a hidden requirement on the tree.
struct Both { std::string forward, backward; };
struct Concat {
    using value_type = Both;
    Both id() const { return {}; }
    Both join(const Both& a, const Both& b) const {
        return {a.forward + b.forward, b.backward + a.backward};
    }
};

int main() {
    std::vector<std::vector<int>> adj{{1, 2}, {0, 3, 4}, {0, 5}, {1}, {1}, {2}};
    hld tree(6, 0, [&](int v) -> const auto& { return adj[v]; });
    std::vector<Both> base(6);
    for (int v = 0; v < 6; ++v) {
        std::string label(1, char('A' + v));
        base[tree.pos[v]] = {label, label};
    }
    segtree<Concat> seg(base);
    auto path = [&](int a, int b, bool edges = false) {
        std::string answer;
        tree.path(a, b, [&](path_piece part) {
            auto value = seg.fold(part.left, part.right);
            answer += part.reversed ? value.backward : value.forward;
        }, edges);
        return answer;
    };
    std::cout << path(3, 5) << '\n';
    std::cout << path(3, 5, true) << '\n';
    seg.set(tree.pos[1], {"X", "X"});
    std::cout << path(3, 5) << '\n';
}
