#include "../src-v3/mdview.hpp"
#include "../src-v3/bag.hpp"
#include "../src-v3/vec_bag.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __LINE__ << ": " #x "\n"; abort(); } } while (false)

struct direction {
    bool descending = false;
    bool operator()(int a, int b) const { return descending ? a > b : a < b; }
};

int main() {
    vector<int> values{1, 4, 2, 2};
    nbag<int, direction> bag(values, direction{true});
    nvec_bag<int, direction> contiguous(values, direction{true});
    auto bag_copy = bag;
    auto contiguous_copy = contiguous;
    bag_copy.insert(3);
    contiguous_copy.insert(3);
    CHECK(bag_copy.kth(0) == 4 && bag_copy.kth(1) == 3);
    CHECK(contiguous_copy.kth(0) == 4 && contiguous_copy.kth(1) == 3);
    CHECK(bag.len() == 4 && contiguous.len() == 4);
    vector<nidx_t> plan{1, 1};
    auto gathered = ngather(span{values}, span{plan});
    static_assert(is_same_v<decltype(gathered), nindexed_span<int>>);
    CHECK(&gathered[0] == &gathered[1]);

    array<int, 6> storage{1, 2, 3, 4, 5, 6};
    auto grid = nmdview(span{storage}, array<nidx_t, 2>{2, 3}, array<long long, 2>{-3, 8})
                    .permute({1, 0}).reverse(0).rebase({10, -8});
    auto domain = nproduct(nrange(10, 13), nrange(-8, -6));
    auto function = nfunc{domain, [grid](auto coordinate) -> int& {
        return grid(get<0>(coordinate), get<1>(coordinate));
    }};
    auto mapped = nmap_values(function, [](int& value) -> int& { return value; });
    mapped(tuple{10, -8}) = 30;
    CHECK(storage[2] == 30 && grid(10, -8) == 30);
    auto picked = nselect(mapped, vector<nidx_t>{0, 0, 5});
    picked[1] = 31;
    CHECK(picked[0] == 31 && storage[2] == 31);
    CHECK(picked[2] == 4);
}
