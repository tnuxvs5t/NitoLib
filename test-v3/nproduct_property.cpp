#include "../src-v3/func.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

int main() {
    auto product = nproduct(nrange(2), nrange(3), nrange(2));
    vector<tuple<nidx_t, nidx_t, nidx_t>> expected;
    for (nidx_t a = 0; a < 2; ++a)
        for (nidx_t b = 0; b < 3; ++b)
            for (nidx_t c = 0; c < 2; ++c)
                expected.emplace_back(a, b, c);

    CHECK(product.len() == nidx_t(expected.size()));
    for (nidx_t i = 0; i < product.len(); ++i)
        CHECK(product[i] == expected[i]);

    auto anchored = nanchors(move(product), nrange(nidx_t(expected.size())));
    for (nidx_t i = 0; i < nidx_t(expected.size()); ++i) {
        CHECK(anchored.key(i) == expected[i]);
        CHECK(anchored[i] == i);
        CHECK(anchored(expected[i]) == i);
    }

    auto four = nproduct(nrange(2), nrange(2), nrange(2), nrange(2));
    CHECK(four.len() == 16);
    CHECK((four[13] == tuple{1, 1, 0, 1}));
    CHECK(nproduct(nrange(2), nrange(0), nrange(3)).empty());

    vector<nidx_t> a{4, 5};
    vector<char> b{'x', 'y'};
    vector<nidx_t> c{8, 9};
    auto references = nproduct(nall(a), nall(b), nall(c));
    get<0>(references[3]) = 40;
    get<1>(references[4]) = 'z';
    get<2>(references[7]) = 90;
    CHECK(a[0] == 40 && b[0] == 'z' && c[1] == 90);

    auto owned_anchor = nanchors(move(references), nrange(8));
    CHECK(owned_anchor(tuple{40, 'z', 90}) == 1);
}
