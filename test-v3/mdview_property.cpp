#include "../src-v3/mdview.hpp"
#include "../src-v3/segment.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __LINE__ << ": " #x "\n"; abort(); } } while (false)

using key = array<long long, 3>;

int main() {
    array<int, 6> fixed{0, 1, 2, 3, 4, 5};
    nmdview matrix(span{fixed}, array<nidx_t, 2>{2, 3}, array<long long, 2>{-2, 10});
    auto composed = matrix.sub({-1, 11}, {0, 13}).permute({1, 0}).reverse(0).rebase({100, -7});
    CHECK(composed(100, -7) == 5 && composed(101, -7) == 4);
    composed(101, -7) = 41;
    CHECK(fixed[4] == 41);
    nmdview readonly(span{as_const(fixed)}, array<nidx_t, 1>{6});
    static_assert(is_same_v<decltype(readonly[0]), const int&>);
    CHECK(readonly[4] == 41);

    mt19937 rng(321789);
    for (int round = 0; round < 500; ++round) {
        array<nidx_t, 3> shape{nidx_t(rng() % 5), nidx_t(rng() % 5), nidx_t(rng() % 5)};
        if (round & 1) for (auto& extent : shape) ++extent;
        key origin{-1000000000000000000LL, 13, 1000000000000000000LL};
        vector<int> storage(shape[0] * shape[1] * shape[2]);
        iota(storage.begin(), storage.end(), 0);
        nmdview view(span{storage}, shape, origin);
        // Oracle stores explicit semantic coordinates and original storage identities.
        // It never uses a stride or the view's coordinate/position functions.
        map<key, size_t> oracle;
        size_t id = 0;
        for (long long x = 0; x < shape[0]; ++x)
            for (long long y = 0; y < shape[1]; ++y)
                for (long long z = 0; z < shape[2]; ++z)
                    oracle[{origin[0] + x, origin[1] + y, origin[2] + z}] = id++;
        for (int step = 0; step < 35; ++step) {
            CHECK(size_t(view.size()) == oracle.size());
            CHECK(view.shape == shape && view.bias == origin);
            nidx_t flat = 0;
            long long sum = 0;
            for (auto [coordinate, slot] : oracle) {
                CHECK(view.position(coordinate) == flat);
                CHECK(view.coordinate(flat) == coordinate);
                CHECK(&view[flat] == &storage[slot]);
                view(coordinate) += step;
                CHECK(view[flat] == storage[slot]);
                sum += storage[slot];
                ++flat;
            }
            nseg<long long> aggregate(view);
            CHECK(aggregate.fold() == sum);
            map<key, size_t> next;
            int operation = int(rng() % 4);
            if (operation == 0) {
                key lower = origin, upper = origin;
                for (size_t axis = 0; axis < 3; ++axis) {
                    nidx_t a = nidx_t(rng() % (shape[axis] + 1));
                    nidx_t b = nidx_t(rng() % (shape[axis] + 1));
                    if (round & 1) {
                        a = nidx_t(rng() % shape[axis]);
                        b = a + 1 + nidx_t(rng() % (shape[axis] - a));
                    }
                    lower[axis] += min(a, b);
                    upper[axis] += max(a, b);
                    shape[axis] = max(a, b) - min(a, b);
                }
                for (auto [coordinate, slot] : oracle) {
                    bool inside = true;
                    for (size_t axis = 0; axis < 3; ++axis)
                        inside &= lower[axis] <= coordinate[axis] && coordinate[axis] < upper[axis];
                    if (inside) next[coordinate] = slot;
                }
                view = view.sub(lower, upper);
                origin = lower;
            } else if (operation == 1) {
                key new_origin{static_cast<long long>(rng() % 41) - 20, -1000000000000000000LL, 8};
                for (auto [old_coordinate, slot] : oracle) {
                    key coordinate = old_coordinate;
                    for (size_t axis = 0; axis < 3; ++axis)
                        coordinate[axis] = new_origin[axis] + (coordinate[axis] - origin[axis]);
                    next[coordinate] = slot;
                }
                view = view.rebase(new_origin);
                origin = new_origin;
            } else if (operation == 2) {
                array<size_t, 3> axes{0, 1, 2};
                shuffle(axes.begin(), axes.end(), rng);
                for (auto [coordinate, slot] : oracle)
                    next[{coordinate[axes[0]], coordinate[axes[1]], coordinate[axes[2]]}] = slot;
                view = view.permute(axes);
                origin = {origin[axes[0]], origin[axes[1]], origin[axes[2]]};
                shape = {shape[axes[0]], shape[axes[1]], shape[axes[2]]};
            } else {
                size_t axis = rng() % 3;
                for (auto [old_coordinate, slot] : oracle) {
                    key coordinate = old_coordinate;
                    coordinate[axis] = origin[axis] + shape[axis] - 1 - (coordinate[axis] - origin[axis]);
                    next[coordinate] = slot;
                }
                view = view.reverse(axis);
            }
            oracle = move(next);
        }
    }
}
