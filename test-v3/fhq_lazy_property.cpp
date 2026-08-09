#include "../src-v3/fhq.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

struct value {
    long long x;
    long long sum;
    bool reverse = false;
};

int main() {
    auto flip = [](auto& q, int root) {
        if (root >= 0) q[root].value.reverse ^= true;
    };
    auto pull = [](auto& q, int root) {
        auto& node = q[root];
        node.value.sum = node.value.x;
        if (node.left >= 0) node.value.sum += q[node.left].value.sum;
        if (node.right >= 0) node.value.sum += q[node.right].value.sum;
    };
    auto push = [flip](auto& q, int root) {
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
    int root = -1;

    auto split3 = [&](int tree, int left, int right) {
        auto [ab, c] = q.split(tree, right);
        auto [a, b] = q.split(ab, left);
        return tuple(a, b, c);
    };
    auto verify = [&] {
        CHECK(q.size(root) == int(reference.size()));
        auto sequence = q.sequence(root);
        long long sum = 0;
        for (int i = 0; i < sequence.len(); ++i) {
            CHECK(sequence[i].x == reference[i]);
            int handle = q.kth(root, i);
            CHECK(q.rank(handle) == i && q.root_of(handle) == root);
            sum += reference[i];
        }
        CHECK((root < 0 ? 0 : q[root].value.sum) == sum);
    };

    for (int round = 0; round < 30000; ++round) {
        int operation = int(rng() % 6);
        if (reference.empty()) operation = 0;
        if (reference.size() > 500) operation = 1 + int(rng() % 5);

        if (operation == 0) {
            int at = int(rng() % (reference.size() + 1));
            long long x = int(rng() % 2001) - 1000;
            auto [left, right] = q.split(root, at);
            root = q.merge(q.merge(left, q.make(value{x, x})), right);
            reference.insert(reference.begin() + at, x);
        } else if (operation == 1) {
            int left = int(rng() % reference.size());
            int right = left + 1 + int(rng() % (reference.size() - left));
            auto [a, b, c] = split3(root, left, right);
            root = q.merge(a, c);
            reference.erase(reference.begin() + left, reference.begin() + right);
            (void)b;
        } else if (operation == 2) {
            int left = int(rng() % (reference.size() + 1));
            int right = left + int(rng() % (reference.size() - left + 1));
            auto [a, b, c] = split3(root, left, right);
            flip(q, b);
            root = q.merge(q.merge(a, b), c);
            reverse(reference.begin() + left, reference.begin() + right);
        } else if (operation == 3) {
            int position = int(rng() % reference.size());
            long long x = int(rng() % 2001) - 1000;
            int handle = q.kth(root, position);
            q[handle].value.x = x;
            q.rebuild(handle);
            reference[position] = x;
        } else if (operation == 4) {
            int left = int(rng() % (reference.size() + 1));
            int right = left + int(rng() % (reference.size() - left + 1));
            auto [a, b, c] = split3(root, left, right);
            long long got = b < 0 ? 0 : q[b].value.sum;
            long long expected = accumulate(reference.begin() + left,
                                            reference.begin() + right, 0LL);
            CHECK(got == expected);
            root = q.merge(q.merge(a, b), c);
        } else {
            int left = int(rng() % (reference.size() + 1));
            int right = left + int(rng() % (reference.size() - left + 1));
            int at = int(rng() % (reference.size() - (right - left) + 1));
            auto [a, b, c] = split3(root, left, right);
            int rest = q.merge(a, c);
            auto [x, y] = q.split(rest, at);
            root = q.merge(q.merge(x, b), y);
            vector<long long> moved(reference.begin() + left, reference.begin() + right);
            reference.erase(reference.begin() + left, reference.begin() + right);
            reference.insert(reference.begin() + at, moved.begin(), moved.end());
        }
        if (round % 101 == 0) verify();
    }
    verify();

    nfhq<unique_ptr<int>> move_only;
    int one = move_only.make(make_unique<int>(7));
    int two = move_only.make(make_unique<int>(9));
    int both = move_only.merge(one, two);
    CHECK(*move_only[move_only.kth(both, 0)].value == 7);
    CHECK(*move_only[move_only.kth(both, 1)].value == 9);

    auto move_policy = [state = make_unique<int>()](auto&, int) mutable { ++*state; };
    auto custom = nmake_fhq<int>(move(move_policy));
    int custom_root = custom.merge(custom.make(1), custom.make(2));
    CHECK(custom.size(custom_root) == 2);
}
