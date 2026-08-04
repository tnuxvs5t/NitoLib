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

    ndeque_ring<string> ring{"b", "c"};
    ring.pushl("a");
    ring.pushr("d");
    auto copy = ring;
    auto moved = move(copy);
    ntest(moved.len() == 4 && moved[0] == "a" && moved[3] == "d");
    ntest(copy.empty());
    nsort(moved, ngreater<>());
    ntest(moved[0] == "d" && moved[3] == "a");

    ndeque_stl<int> reference_backend{3, 1, 2};
    nsort(reference_backend);
    ntest(reference_backend[0] == 1 && reference_backend[2] == 3);
}
