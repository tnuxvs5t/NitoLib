#include "common.hpp"

int main() {
    string text = "ababaabababa";
    string pattern = "ababa";
    ntest((nkmp_find(text, pattern) == nvector<int>{0, 5, 7}));
    ntest(nkmp(text, pattern) == nkmp_find(text, pattern));
    ntest((nkmp_find(text, string{}) == nvector<int>{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}));

    auto prefix = nprefix_function(pattern);
    ntest((prefix == nvector<int>{0, 0, 1, 2, 3}));
    ntest(nprefix(pattern) == prefix);
    auto z = nz_function(pattern);
    ntest((z == nvector<int>{5, 0, 3, 0, 1}));
    ntest(nzfunc(pattern) == z);

    auto palindrome = nmanacher(text);
    static_assert(same_as<decltype(palindrome), nmanacher_result>);
    ntest(palindrome.pal(0, 5) && palindrome.pal(5, 12));
    ntest(!palindrome.pal(0, 6) && palindrome.pal(3, 3));

    string banana = "banana";
    auto suffix = nsuffix_array(banana);
    ntest((suffix == nvector<int>{5, 3, 1, 0, 4, 2}));
    ntest((nlcp_array(banana, suffix) == nvector<int>{0, 1, 3, 0, 0, 2}));

    ndeque<char> non_contiguous;
    for (char c : banana)
        non_contiguous.pushr(c);
    ntest(nsuffix_array(non_contiguous) == suffix);
}
