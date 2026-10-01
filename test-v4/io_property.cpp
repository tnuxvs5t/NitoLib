#include "../src-v4/io.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

using i128 = __int128_t;
using u128 = __uint128_t;

string decimal(i128 value) { ostringstream out; nwrite(out, value); return out.str(); }
string decimal(u128 value) { ostringstream out; nwrite(out, value); return out.str(); }

int main() {
    constexpr i128 minimum = numeric_limits<i128>::min();
    constexpr i128 maximum = numeric_limits<i128>::max();
    constexpr u128 unsigned_maximum = numeric_limits<u128>::max();
    CHECK(decimal(minimum) == "-170141183460469231731687303715884105728");
    CHECK(decimal(maximum) == "170141183460469231731687303715884105727");
    CHECK(decimal(unsigned_maximum) == "340282366920938463463374607431768211455");

    stringstream extrema(decimal(minimum) + " " + decimal(maximum) + " " + decimal(unsigned_maximum));
    i128 got_minimum = 0, got_maximum = 0;
    u128 got_unsigned_maximum = 0;
    CHECK(nscan(extrema, got_minimum, got_maximum, got_unsigned_maximum));
    CHECK(got_minimum == minimum && got_maximum == maximum && got_unsigned_maximum == unsigned_maximum);

    stringstream output;
    nprint(output, minimum, 7, unsigned_maximum);
    CHECK(output.str() == "-170141183460469231731687303715884105728 7 "
                         "340282366920938463463374607431768211455");
    stringstream line;
    nprintln(line, 1, -2, maximum);
    CHECK(line.str() == "1 -2 170141183460469231731687303715884105727\n");

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
    nidx_t plus = 0;
    CHECK(nread(boundary, plus) && plus == 42 && boundary.eof() && !boundary.fail());

    stringstream malformed("+ x");
    nidx_t unchanged = 17;
    CHECK(!nread(malformed, unchanged) && unchanged == 17 && malformed.fail());
    malformed.clear();
    CHECK(!nread(malformed, unchanged) && unchanged == 17 && malformed.fail());

    mt19937_64 rng(0x10f45a57);
    for (nidx_t round = 0; round < 20000; ++round) {
        u128 bits = u128(rng()) << 64 | rng();
        i128 signed_value = i128(bits >> 1);
        if (rng() & 1) signed_value = -signed_value;
        stringstream input(decimal(signed_value) + " " + decimal(bits));
        i128 got_signed = 0;
        u128 got_unsigned = 0;
        CHECK(nscan(input, got_signed, got_unsigned));
        CHECK(got_signed == signed_value && got_unsigned == bits);
    }
    cout << "v4 io: bounded decimal scan/write, stream interop and wide integers passed\n";
}
