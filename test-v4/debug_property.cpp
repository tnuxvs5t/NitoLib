#include "../src-v4/debug.hpp"
#include "../src-v4/view.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

struct debug_leaf { nidx_t value; };
ostream& operator<<(ostream& out, const debug_leaf& leaf) { return out << "leaf(" << leaf.value << ')'; }

namespace custom_debug {
struct point { nidx_t x, y; };
void ndebug_repr(ndebug_writer& out, const point& value) {
    out.object("point", [&](auto field) { field("x", value.x); field("y", value.y); });
}
struct packet { point where; vector<nidx_t> payload; };
void ndebug_repr(ndebug_writer& out, const packet& value) {
    out.object("packet", [&](auto field) { field("where", value.where); field("payload", value.payload); });
}
struct both { nidx_t value; };
ostream& operator<<(ostream& out, const both&) { return out << "wrong"; }
void ndebug_repr(ndebug_writer& out, const both& value) {
    out.object("both", [&](auto field) { field("value", value.value); });
}
}

enum class code : unsigned { ready = 7 };

int main() {
    stringstream scalar;
    ndebug(scalar, "scalar =", true, false, '\n', static_cast<signed char>(-2),
           static_cast<unsigned char>(250), numeric_limits<__int128_t>::min(), code::ready,
           debug_leaf{9}, nullptr);
    CHECK(scalar.str() == "scalar = true false '\\n' -2 250 "
                         "-170141183460469231731687303715884105728 7 leaf(9) nullptr\n");

    vector<pair<nidx_t, tuple<string, vector<bool>>>> nested{{1, {"a\n\"b", {true, false}}},
                                                              {2, {"河童", {false}}}};
    stringstream structure;
    ndebug(structure, "nested =", nested, tuple<>{}, tuple{5});
    CHECK(structure.str() == "nested = [(1, (\"a\\n\\\"b\", [true, false])), "
                         "(2, (\"河童\", [false]))] () (5,)\n");

    nidx_t calls = 0;
    auto view = ntabulate(4, [&](nidx_t i) { ++calls; return pair{i, i * i}; });
    stringstream lazy;
    ndebug(lazy, "view =", view);
    CHECK(lazy.str() == "view = nview[(0, 0), (1, 1), (2, 4), (3, 9)]\n" && calls == 4);

    nidx_t key_calls = 0, eval_calls = 0;
    auto function = nfunc{ntabulate(3, [&](nidx_t i) { ++key_calls; return i == 2 ? 0 : i; }),
                          [&](nidx_t key) { ++eval_calls; return vector{key, key + 10}; }};
    stringstream keyed;
    ndebug(keyed, "f =", function);
    CHECK(keyed.str() == "f = nfunc[(0, [0, 10]), (1, [1, 11]), (0, [0, 10])]\n");
    CHECK(key_calls == 3 && eval_calls == 3);

    stringstream raw;
    ndebug(raw, string("raw\nlabel"), vector<string>{"quoted\nvalue", "x\\y", string("a\0b", 3)});
    CHECK(raw.str() == "raw\nlabel [\"quoted\\nvalue\", \"x\\\\y\", \"a\\0b\"]\n");

    stringstream custom;
    ndebug(custom, "objects =", custom_debug::point{2, 3},
           custom_debug::packet{{4, 5}, {8, 13}}, custom_debug::both{21});
    CHECK(custom.str() == "objects = point{x=2, y=3} packet{where=point{x=4, y=5}, payload=[8, 13]} "
                         "both{value=21}\n");
    cout << "v4 debug: recursive values, views/functions, escaping and ADL objects passed\n";
}
