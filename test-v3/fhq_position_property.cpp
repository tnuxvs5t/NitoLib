#include "../src-v3/fhq.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

struct affine_position { long long a = 1, p = 0, b = 0; };
struct reverse_command {};
struct atom { long long x; char label; };
struct record {
    atom own;
    long long sum = 0;
    string forward, backward;
    affine_position pending;
    bool reverse = false;
};

// x_i -> a*x_i + p*i + b, with indices local to the updated subtree.
struct position_ops {
    unique_ptr<nidx_t> pushes = make_unique<nidx_t>();

    void pull(auto& q, nidx_t root) const {
        auto& node = q[root];
        auto& s = node.value;
        s.sum = s.own.x;
        s.forward = s.backward = string(1, s.own.label);
        if (node.left >= 0) {
            const auto& child = q[node.left].value;
            s.sum += child.sum;
            s.forward = child.forward + s.forward;
            s.backward += child.backward;
        }
        if (node.right >= 0) {
            const auto& child = q[node.right].value;
            s.sum += child.sum;
            s.forward += child.forward;
            s.backward = child.backward + s.backward;
        }
    }

    bool try_apply(auto& q, nidx_t root, affine_position tag) const {
        auto& s = q[root].value;
        long long n = q.size(root), own_position = q.size(q[root].left);
        s.own.x = tag.a * s.own.x + tag.p * own_position + tag.b;
        s.sum = tag.a * s.sum + tag.p * (n * (n - 1) / 2) + tag.b * n;
        auto old = s.pending;
        s.pending = {tag.a * old.a, tag.a * old.p + tag.p, tag.a * old.b + tag.b};
        return true;
    }

    bool try_apply(auto& q, nidx_t root, reverse_command) const {
        auto& s = q[root].value;
        q.swap_children(root);
        swap(s.forward, s.backward);
        s.reverse ^= true;
        s.pending.b += s.pending.p * (q.size(root) - 1);
        s.pending.p = -s.pending.p;
        return true;
    }

    void push(auto& q, nidx_t root) {
        ++*pushes;
        auto tag = q[root].value.pending;
        bool reverse = q[root].value.reverse;
        nidx_t left = q[root].left, right = q[root].right;
        for (nidx_t child : {left, right}) if (child >= 0) {
            if (reverse) try_apply(q, child, reverse_command{});
            auto shifted = tag;
            if (child == right) shifted.b += shifted.p * (q.size(left) + 1);
            try_apply(q, child, shifted);
        }
        q[root].value.pending = {};
        q[root].value.reverse = false;
    }

    // Both commands always complete a whole subtree, so fallback is unreachable.
    void apply_one(auto&, nidx_t, const auto&) const { abort(); }
};

int main() {
    auto q = nmake_fhq<record>(position_ops{}, 7654321);
    vector<atom> reference;
    nidx_t root = -1;
    auto insert = [&](nidx_t at, atom value) {
        root = q.edit(root, at, at, [&](auto& tree, nidx_t middle) {
            CHECK(middle == -1);
            return tree.make(record{value, 0, {}, {}, {}, false});
        });
        reference.insert(reference.begin() + at, value);
    };
    auto update = [&](nidx_t left, nidx_t right, const auto& command) {
        root = q.edit(root, left, right, [&](auto& tree, nidx_t middle) {
            tree.apply(middle, command);
            return middle;
        });
    };
    auto verify = [&] {
        CHECK(q.size(root) == nidx_t(reference.size()));
        long long sum = 0;
        string forward;
        for (auto x : reference) { sum += x.x; forward += x.label; }
        CHECK((root < 0 ? 0 : q[root].value.sum) == sum);
        CHECK((root < 0 ? string{} : q[root].value.forward) == forward);
        reverse(forward.begin(), forward.end());
        CHECK((root < 0 ? string{} : q[root].value.backward) == forward);
    };

    for (nidx_t i = 0; i < 7; ++i) insert(i, atom{i + 1, char('a' + i)});
    // Pending progression is conjugated by reverse, then shifted when splitting.
    update(0, 7, affine_position{1, 3, 2});
    for (nidx_t i = 0; i < 7; ++i) reference[i].x += 3 * i + 2;
    update(0, 7, reverse_command{});
    reverse(reference.begin(), reference.end());
    update(1, 6, affine_position{-1, 2, 5});
    for (nidx_t i = 1; i < 6; ++i) reference[i].x = -reference[i].x + 2 * (i - 1) + 5;
    verify();

    mt19937 rng(188331);
    for (nidx_t step = 0; step < 20000; ++step) {
        nidx_t n = nidx_t(reference.size());
        nidx_t left = nidx_t(rng() % (n + 1)), right = nidx_t(rng() % (n + 1));
        if (left > right) swap(left, right);
        nidx_t op = nidx_t(rng() % 7);
        if (!n) op = 0;
        if (n > 100 && op == 0) op = 1;
        if (op == 0) {
            insert(left, atom{nidx_t(rng() % 101) - 50, char('a' + rng() % 26)});
        } else if (op == 1) {
            root = q.edit(root, left, right, [](auto&, nidx_t) { return nidx_t(-1); });
            reference.erase(reference.begin() + left, reference.begin() + right);
        } else if (op == 2) {
            update(left, right, reverse_command{});
            reverse(reference.begin() + left, reference.begin() + right);
        } else if (op == 3) {
            affine_position tag{nidx_t(rng() % 3) - 1, nidx_t(rng() % 7) - 3,
                                nidx_t(rng() % 11) - 5};
            update(left, right, tag);
            for (nidx_t i = left; i < right; ++i)
                reference[i].x = tag.a * reference[i].x + tag.p * (i - left) + tag.b;
        } else if (op == 4) {
            nidx_t at = nidx_t(rng() % n), handle = q.kth(root, at);
            q[handle].value.own.x = reference[at].x = nidx_t(rng() % 201) - 100;
            q.rebuild(handle);
        } else if (op == 5) {
            auto [ab, c] = q.split(root, right);
            auto [a, b] = q.split(ab, left);
            root = q.merge(b, q.merge(a, c));
            rotate(reference.begin(), reference.begin() + left, reference.begin() + right);
        } else {
            root = q.edit(root, left, right, [&](auto& tree, nidx_t middle) {
                long long sum = 0;
                string forward;
                for (nidx_t i = left; i < right; ++i) {
                    sum += reference[i].x;
                    forward += reference[i].label;
                }
                CHECK((middle < 0 ? 0 : tree[middle].value.sum) == sum);
                CHECK((middle < 0 ? string{} : tree[middle].value.forward) == forward);
                return middle;
            });
        }
        verify();
        if (step % 37 == 0) for (nidx_t i = 0; i < q.size(root); ++i) {
            nidx_t handle = q.kth(root, i);
            CHECK(q[handle].value.own.x == reference[i].x);
            CHECK(q[handle].value.own.label == reference[i].label);
            CHECK(q.rank(handle) == i && q.root_of(handle) == root);
        }
    }
    auto moved = move(q);
    CHECK(*moved.ops.pushes > 0);
    CHECK(moved.size(root) == nidx_t(reference.size()));
}
