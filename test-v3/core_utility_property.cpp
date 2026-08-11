#include "../src-v3/core.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

int main() {
    int value = 5;
    CHECK(nchmin(value, 3) && value == 3);
    CHECK(!nchmin(value, 7) && value == 3);
    CHECK(nchmax(value, 8) && value == 8);
    CHECK(!nchmax(value, 4) && value == 8);

    long long mixed = 10;
    CHECK(nchmin(mixed, 7) && mixed == 7);
    CHECK(nchmax(mixed, 11) && mixed == 11);

    string text = "m";
    CHECK(nchmin(text, string("a")) && text == "a");
    CHECK(nchmax(text, string("z")) && text == "z");

    mt19937 rng(0xC0DE);
    for (int round = 0; round < 20000; ++round) {
        int left = int(rng() % 2001) - 1000;
        int right = int(rng() % 2001) - 1000;

        int expected_min = min(left, right);
        bool changed_min = right < left;
        CHECK(nchmin(left, right) == changed_min && left == expected_min);

        left = int(rng() % 2001) - 1000;
        expected_min = max(left, right);
        bool changed_max = left < right;
        CHECK(nchmax(left, right) == changed_max && left == expected_min);
    }
}
