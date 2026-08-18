#include "../src-v3/io.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

using i128 = __int128_t;
using u128 = __uint128_t;

string decimal(i128 value) {
    ostringstream out;
    nwrite(out, value);
    return out.str();
}

string decimal(u128 value) {
    ostringstream out;
    nwrite(out, value);
    return out.str();
}

int main() {
    constexpr i128 minimum = numeric_limits<i128>::min();
    constexpr i128 maximum = numeric_limits<i128>::max();
    constexpr u128 unsigned_maximum = numeric_limits<u128>::max();
    CHECK(decimal(minimum) == "-170141183460469231731687303715884105728");
    CHECK(decimal(maximum) == "170141183460469231731687303715884105727");
    CHECK(decimal(unsigned_maximum) == "340282366920938463463374607431768211455");
    stringstream extrema(decimal(minimum) + " " + decimal(maximum) + " " +
                         decimal(unsigned_maximum));
    i128 got_minimum = 0, got_maximum = 0;
    u128 got_unsigned_maximum = 0;
    CHECK(nscan(extrema, got_minimum, got_maximum, got_unsigned_maximum));
    CHECK(got_minimum == minimum && got_maximum == maximum &&
          got_unsigned_maximum == unsigned_maximum);

    stringstream mixed("12 1000000000000000000000000000000000000 -7 42");
    int a = 0, c = 0;
    i128 huge = 0;
    long long d = 0;
    mixed >> a;
    CHECK(nread(mixed, huge));
    mixed >> c;
    CHECK(nscan(mixed, d));
    CHECK(a == 12 && huge > i128(0) && c == -7 && d == 42);

    stringstream output;
    output << "prefix ";
    nprint(output, minimum, 7, unsigned_maximum);
    output << " suffix";
    CHECK(output.str() ==
          "prefix -170141183460469231731687303715884105728 7 "
          "340282366920938463463374607431768211455 suffix");

    stringstream line;
    nprintln(line, 1, -2, maximum);
    CHECK(line.str() == "1 -2 170141183460469231731687303715884105727\n");

    stringstream default_input("5 1000000000000000000000000000000000000 6");
    stringstream default_output;
    auto* old_input = cin.rdbuf(default_input.rdbuf());
    auto* old_output = cout.rdbuf(default_output.rdbuf());
    cin.clear();
    int default_left = 0, default_right = 0;
    i128 default_huge = 0;
    cin >> default_left;
    CHECK(nread(default_huge));
    CHECK(nscan(default_right));
    cout << "std ";
    nprintln(default_huge, default_right);
    cout << "tail";
    cin.rdbuf(old_input);
    cout.rdbuf(old_output);
    cin.clear();
    CHECK(default_left == 5 && default_huge > 0 && default_right == 6);
    CHECK(default_output.str() ==
          "std 1000000000000000000000000000000000000 6\ntail");

    stringstream boundary("-128 127 255 128 -129 256 -1 +42");
    signed char smin = 0, smax = 0, overflow_signed = 9;
    unsigned char umax = 0, overflow_unsigned = 9, negative_unsigned = 9;
    CHECK(nscan(boundary, smin, smax, umax));
    CHECK(smin == -128 && smax == 127 && umax == 255);
    CHECK(!nread(boundary, overflow_signed) && overflow_signed == 9 && boundary.fail());
    boundary.clear();
    CHECK(!nread(boundary, overflow_signed) && overflow_signed == 9 && boundary.fail());
    boundary.clear();
    CHECK(!nread(boundary, overflow_unsigned) && overflow_unsigned == 9 && boundary.fail());
    boundary.clear();
    CHECK(!nread(boundary, negative_unsigned) && negative_unsigned == 9 && boundary.fail());
    boundary.clear();
    int plus = 0;
    CHECK(nread(boundary, plus) && plus == 42 && boundary.eof() && !boundary.fail());

    stringstream malformed("+ x");
    int unchanged = 17;
    CHECK(!nread(malformed, unchanged) && unchanged == 17 && malformed.fail());
    malformed.clear();
    CHECK(!nread(malformed, unchanged) && unchanged == 17 && malformed.fail());

    mt19937_64 rng(0x10f45a57);
    for (int round = 0; round < 100000; ++round) {
        u128 bits = u128(rng()) << 64 | rng();
        i128 signed_value = i128(bits >> 1);
        if (rng() & 1) signed_value = -signed_value;
        string text = decimal(signed_value) + " " + decimal(bits);
        stringstream input(text);
        i128 got_signed = 0;
        u128 got_unsigned = 0;
        CHECK(nscan(input, got_signed, got_unsigned));
        CHECK(got_signed == signed_value && got_unsigned == bits);
    }
}
