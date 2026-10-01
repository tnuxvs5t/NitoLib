#include "../src-v4/tree.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __LINE__ << ": " #x << '\n'; abort(); } } while (false)

struct keys {
    vector<string>* data;
    nidx_t len() const { return nidx_t(data->size()); }
    const string& operator[](nidx_t i) const { return (*data)[i]; }
    nidx_t inverse(const string& key) const {
        return nidx_t(find(data->begin(), data->end(), key) - data->begin());
    }
};

static_assert(ranges::random_access_range<decltype(nall(declval<vector<int>&>()))>);

int main() {
    vector<string> keys{"root", "left", "right", "leaf"};
    vector<vector<string>> children{{"left", "right"}, {"leaf"}, {}, {}};
    unordered_map<string, nidx_t> at;
    for (nidx_t i = 0; i < nidx_t(keys.size()); ++i) at[keys[i]] = i;

    auto vertices = ::keys{&keys};
    auto next = [&](const string& key) -> const auto& { return children[at[key]]; };
    auto rooted = nroot(ngraph{vertices, next}, vector<string>{"root"});
    CHECK(rooted.parents()(string("leaf")) == "left");
    CHECK(rooted.depths()(string("leaf")) == 2);
    CHECK(rooted.subtree_sizes()(string("root")) == 4);
    CHECK(rooted.positions()(string("right")) == 2);
    vector<string> rooted_order;
    for (auto&& key : rooted.order()) rooted_order.push_back(key);
    CHECK((rooted_order == vector<string>{"root", "left", "leaf", "right"}));

    auto copied = nhld(rooted);
    auto moved = nhld(move(rooted));
    CHECK(copied.lca(string("leaf"), string("left")) == "left");
    CHECK(moved.lca(string("leaf"), string("left")) == "left");
    CHECK(copied.positions()(string("leaf")) >= 0);

    vector<string> path;
    copied.visit_path(string("leaf"), string("right"), [&](npath_piece piece) {
        for (nidx_t i = piece.left; i < piece.right; ++i)
            path.push_back(copied.order()[piece.reverse ? piece.right - 1 - (i - piece.left) : i]);
    });
    CHECK((path == vector<string>{"leaf", "left", "root", "right"}));
    cout << "v4 key tree: key domain, anchors, HLD path passed\n";
}
