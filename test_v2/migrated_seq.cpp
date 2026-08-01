#include "common.hpp"

int main() {
    nvector<int> a{1, 2, 3};
    a.push(4);
    ntest(a.len() == 4 && a.pop() == 4);
    ntest(a.get(9) == nullptr && a.get(9, 7) == 7);

    ndeque<int> d;
    deque<int> reference;
    for (int i = 0; i < 1000; ++i) {
        if (i & 1) {
            d.pushl(i);
            reference.push_front(i);
        } else {
            d.pushr(i);
            reference.push_back(i);
        }
    }
    ntest(d.len() == int(reference.size()));
    for (int i = 0; i < d.len(); ++i)
        ntest(d[i] == reference[i]);

    for (int i = 0; i < 500; ++i) {
        ntest(d.popl() == reference.front());
        reference.pop_front();
        ntest(d.popr() == reference.back());
        reference.pop_back();
    }
    ntest(d.empty() && d.popl(8) == 8);
}
