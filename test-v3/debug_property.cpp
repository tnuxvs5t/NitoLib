#include "../src-v3/debug.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

struct debug_leaf {
    nidx_t value;
};

ostream& operator<<(ostream& out, const debug_leaf& leaf) {
    return out << "leaf(" << leaf.value << ')';
}

namespace custom_debug {

struct point {
    nidx_t x, y;
};

void ndebug_repr(ndebug_writer& out, const point& value) {
    out.object("point", [&](auto field) {
        field("x", value.x);
        field("y", value.y);
    });
}

struct packet {
    point where;
    vector<nidx_t> payload;
};

void ndebug_repr(ndebug_writer& out, const packet& value) {
    out.object("packet", [&](auto field) {
        field("where", value.where);
        field("payload", value.payload);
    });
}

struct both_interfaces {
    nidx_t value;
};

ostream& operator<<(ostream& out, const both_interfaces&) {
    return out << "wrong-leaf";
}

void ndebug_repr(ndebug_writer& out, const both_interfaces& value) {
    out.object("both", [&](auto field) {
        field("value", value.value);
    });
}

struct ticking {
    nidx_t value;
    nidx_t* calls;

    nidx_t read() const {
        ++*calls;
        return value;
    }
};

void ndebug_repr(ndebug_writer& out, const ticking& value) {
    out.object("ticking", [&](auto field) {
        field("read", value.read());
    });
}

} // namespace custom_debug

enum class debug_code : unsigned { ready = 7 };

struct sync_buffer : stringbuf {
    nidx_t synchronizations = 0;
    int sync() override {
        ++synchronizations;
        return stringbuf::sync();
    }
};

int main() {
    stringstream scalar;
    __int128_t minimum = numeric_limits<__int128_t>::min();
    ndebug(scalar, "scalar =", true, false, '\n', static_cast<signed char>(-2),
           static_cast<unsigned char>(250),
           minimum, debug_code::ready, debug_leaf{9}, nullptr);
    CHECK(scalar.str() ==
          "scalar = true false '\\n' -2 250 "
          "-170141183460469231731687303715884105728 7 leaf(9) nullptr\n");

    vector<pair<nidx_t, tuple<string, vector<bool>>>> nested{
        {1, {"a\n\"b", {true, false}}},
        {2, {"河童", {false}}}
    };
    stringstream structure;
    ndebug(structure, "nested =", nested, tuple<>{}, tuple{5});
    CHECK(structure.str() ==
          "nested = [(1, (\"a\\n\\\"b\", [true, false])), "
          "(2, (\"河童\", [false]))] () (5,)\n");

    nidx_t view_calls = 0;
    auto view = ntabulate(4, [&](nidx_t i) {
        ++view_calls;
        return pair{i, i * i};
    });
    stringstream lazy;
    ndebug(lazy, "view =", view);
    CHECK(lazy.str() == "view = nview[(0, 0), (1, 1), (2, 4), (3, 9)]\n");
    CHECK(view_calls == 4);

    nidx_t key_calls = 0, eval_calls = 0;
    auto function = nfunc{
        ntabulate(3, [&](nidx_t i) {
            ++key_calls;
            return i == 2 ? 0 : i;
        }),
        [&](nidx_t key) {
            ++eval_calls;
            return vector{key, key + 10};
        }
    };
    stringstream keyed;
    ndebug(keyed, "f =", function);
    CHECK(keyed.str() ==
          "f = nfunc[(0, [0, 10]), (1, [1, 11]), (0, [0, 10])]\n");
    CHECK(key_calls == 3 && eval_calls == 3);

    auto move_view = ntabulate(2, [owner = make_unique<nidx_t>(6)](nidx_t i) {
        return *owner + i;
    });
    auto move_function = nfunc{
        nrange(2),
        [owner = make_unique<nidx_t>(8)](nidx_t key) { return *owner + key; }
    };
    stringstream move_only;
    ndebug(move_only, move_view, move_function);
    CHECK(move_only.str() == "nview[6, 7] nfunc[(0, 8), (1, 9)]\n");

    string top = "raw\nlabel";
    stringstream raw;
    ndebug(raw, top, vector<string>{"quoted\nvalue", "x\\y", string("a\0b", 3)});
    CHECK(raw.str() ==
          "raw\nlabel [\"quoted\\nvalue\", \"x\\\\y\", \"a\\0b\"]\n");

    auto composed = tuple{
        nrange(2),
        nfunc{nrange(2), [](nidx_t key) { return pair{key, char('a' + key)}; }}
    };
    stringstream recursive;
    ndebug(recursive, composed);
    CHECK(recursive.str() ==
          "(nview[0, 1], nfunc[(0, (0, 'a')), (1, (1, 'b'))])\n");

    sync_buffer buffer;
    ostream flushed(&buffer);
    ndebug(flushed, "now", 42);
    CHECK(buffer.str() == "now 42\n" && buffer.synchronizations == 1);

    custom_debug::point point{2, 3};
    custom_debug::packet packet{{4, 5}, {8, 13}};
    custom_debug::both_interfaces both{21};
    stringstream custom;
    ndebug(custom, "objects =", point, packet, both);
    CHECK(custom.str() ==
          "objects = point{x=2, y=3} packet{where=point{x=4, y=5}, payload=[8, 13]} "
          "both{value=21}\n");

    nidx_t ticking_calls = 0;
    custom_debug::ticking ticking{34, &ticking_calls};
    stringstream custom_once;
    ndebug(custom_once, ticking);
    CHECK(custom_once.str() == "ticking{read=34}\n" && ticking_calls == 1);

    mt19937 rng(0xD38A6);
    for (nidx_t round = 0; round < 1000; ++round) {
        nidx_t n = nidx_t(rng() % 21);
        vector<nidx_t> keys(n);
        for (nidx_t& key : keys) key = nidx_t(rng() % 11) - 5;

        nidx_t random_key_calls = 0, random_eval_calls = 0;
        auto random_function = nfunc{
            ntabulate(n, [&](nidx_t i) {
                ++random_key_calls;
                return keys[i];
            }),
            [&](nidx_t key) {
                ++random_eval_calls;
                return pair{key * key, -key};
            }
        };

        stringstream got, expected;
        ndebug(got, random_function);
        expected << "nfunc[";
        for (nidx_t i = 0; i < n; ++i) {
            if (i) expected << ", ";
            nidx_t key = keys[i];
            expected << '(' << key << ", (" << key * key << ", " << -key << "))";
        }
        expected << "]\n";
        CHECK(got.str() == expected.str());
        CHECK(random_key_calls == n && random_eval_calls == n);
    }
}
