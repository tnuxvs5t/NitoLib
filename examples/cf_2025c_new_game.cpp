#include "Nitori.h"

struct Item {
    int value, count;
};

int main() {
    int tests;
    nin >> tests;
    while (tests--) {
        int n, k;
        nin >> n >> k;
        nvector<int> cards(n);
        nrep(i, n)
            nin >> cards[i];
        nsort(cards);

        auto frequencies = nmap_values(nruns(cards), [](auto run) {
            return Item{run[0], run.len()};
        });
        auto consecutive = nruns(frequencies, [](const Item& left, const Item& right) {
            return right.value == left.value + 1;
        });

        int answer = 0;
        nfor(chain, consecutive) {
            int sum = 0;
            nfori(i, item, chain) {
                sum += item.count;
                if (i >= k)
                    sum -= chain[i - k].count;
                nchmax(answer, sum);
            }
        }
        nprintln(answer);
    }
}
