#include "../src-v3/fhq.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

struct value {
    long long x;
    long long sum;
    bool reverse = false;
};

int main() {
    auto flip = [](auto& q, nidx_t root) {
        if (root >= 0) q[root].value.reverse ^= true;
    };
    auto pull = [](auto& q, nidx_t root) {
        auto& node = q[root];
        node.value.sum = node.value.x;
        if (node.left >= 0) node.value.sum += q[node.left].value.sum;
        if (node.right >= 0) node.value.sum += q[node.right].value.sum;
    };
    auto push = [flip](auto& q, nidx_t root) {
        auto& node = q[root];
        if (!node.value.reverse) return;
        q.swap_children(root);
        flip(q, node.left);
        flip(q, node.right);
        node.value.reverse = false;
    };
    auto q = nmake_fhq<value>(pull, push, 123456789);
    mt19937 rng(0xBAD5EED);
    vector<long long> reference;
    nidx_t root = -1;

    auto split3 = [&](nidx_t tree, nidx_t left, nidx_t right) {
        auto [ab, c] = q.split(tree, right);
        auto [a, b] = q.split(ab, left);
        return tuple(a, b, c);
    };
    auto verify = [&] {
        CHECK(q.size(root) == nidx_t(reference.size()));
        auto sequence = q.sequence(root);
        long long sum = 0;
        for (nidx_t i = 0; i < sequence.len(); ++i) {
            CHECK(sequence[i].x == reference[i]);
            nidx_t handle = q.kth(root, i);
            CHECK(q.rank(handle) == i && q.root_of(handle) == root);
            sum += reference[i];
        }
        CHECK((root < 0 ? 0 : q[root].value.sum) == sum);
    };

    for (nidx_t round = 0; round < 30000; ++round) {
        nidx_t operation = nidx_t(rng() % 6);
        if (reference.empty()) operation = 0;
        if (reference.size() > 500) operation = 1 + nidx_t(rng() % 5);

        if (operation == 0) {
            nidx_t at = nidx_t(rng() % (reference.size() + 1));
            long long x = nidx_t(rng() % 2001) - 1000;
            auto [left, right] = q.split(root, at);
            root = q.merge(q.merge(left, q.make(value{x, x})), right);
            reference.insert(reference.begin() + at, x);
        } else if (operation == 1) {
            nidx_t left = nidx_t(rng() % reference.size());
            nidx_t right = left + 1 + nidx_t(rng() % (reference.size() - left));
            auto [a, b, c] = split3(root, left, right);
            root = q.merge(a, c);
            reference.erase(reference.begin() + left, reference.begin() + right);
            (void)b;
        } else if (operation == 2) {
            nidx_t left = nidx_t(rng() % (reference.size() + 1));
            nidx_t right = left + nidx_t(rng() % (reference.size() - left + 1));
            auto [a, b, c] = split3(root, left, right);
            flip(q, b);
            root = q.merge(q.merge(a, b), c);
            reverse(reference.begin() + left, reference.begin() + right);
        } else if (operation == 3) {
            nidx_t position = nidx_t(rng() % reference.size());
            long long x = nidx_t(rng() % 2001) - 1000;
            nidx_t handle = q.kth(root, position);
            q[handle].value.x = x;
            q.rebuild(handle);
            reference[position] = x;
        } else if (operation == 4) {
            nidx_t left = nidx_t(rng() % (reference.size() + 1));
            nidx_t right = left + nidx_t(rng() % (reference.size() - left + 1));
            auto [a, b, c] = split3(root, left, right);
            long long got = b < 0 ? 0 : q[b].value.sum;
            long long expected = accumulate(reference.begin() + left,
                                            reference.begin() + right, 0LL);
            CHECK(got == expected);
            root = q.merge(q.merge(a, b), c);
        } else {
            nidx_t left = nidx_t(rng() % (reference.size() + 1));
            nidx_t right = left + nidx_t(rng() % (reference.size() - left + 1));
            nidx_t at = nidx_t(rng() % (reference.size() - (right - left) + 1));
            auto [a, b, c] = split3(root, left, right);
            nidx_t rest = q.merge(a, c);
            auto [x, y] = q.split(rest, at);
            root = q.merge(q.merge(x, b), y);
            vector<long long> moved(reference.begin() + left, reference.begin() + right);
            reference.erase(reference.begin() + left, reference.begin() + right);
            reference.insert(reference.begin() + at, moved.begin(), moved.end());
        }
        if (round % 101 == 0) verify();
    }
    verify();

    nfhq<unique_ptr<nidx_t>> move_only;
    nidx_t one = move_only.make(make_unique<nidx_t>(7));
    nidx_t two = move_only.make(make_unique<nidx_t>(9));
    nidx_t both = move_only.merge(one, two);
    CHECK(*move_only[move_only.kth(both, 0)].value == 7);
    CHECK(*move_only[move_only.kth(both, 1)].value == 9);

    auto move_policy = [state = make_unique<nidx_t>()](auto&, nidx_t) mutable { ++*state; };
    auto custom = nmake_fhq<nidx_t>(move(move_policy));
    nidx_t custom_root = custom.merge(custom.make(1), custom.make(2));
    CHECK(custom.size(custom_root) == 2);
}
