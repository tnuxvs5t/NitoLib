#include "../v2_unsafe/Nitori.h"

template <class F> long long elapsed_ms(F operation) {
    auto start = chrono::steady_clock::now();
    operation();
    return chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - start).count();
}

int main() {
    mt19937 random(0x20c0ffeeU);
    uint64_t checksum = 0;

    nvector<int> sequence(400000);
    for (int i = 0; i < sequence.len(); ++i)
        sequence[i] = int(random());
    long long sort_ms = elapsed_ms([&] { nsort(sequence); });
    for (int i = 0; i < sequence.len(); i += 4096)
        checksum = checksum * 1000003 + unsigned(sequence[i]);

    nfenwick<long long> fenwick(300000);
    long long fenwick_ms = elapsed_ms([&] {
        for (int operation = 0; operation < 600000; ++operation) {
            int index = int(random() % 300000);
            if (operation & 1)
                checksum += uint64_t(fenwick.prefix(index + 1));
            else
                fenwick.add(index, int(random() % 101) - 50);
        }
    });

    using mint = nmodint<998244353>;
    nvector<mint> left(1 << 17), right(1 << 17);
    for (int i = 0; i < left.len(); ++i) {
        left[i] = random();
        right[i] = random();
    }
    nvector<mint> convolution;
    long long ntt_ms = elapsed_ms([&] { convolution = nconv(left, right); });
    for (int i = 0; i < convolution.len(); i += 4096)
        checksum = checksum * 1000003 + convolution[i].val();

    nlichao<long long> lichao(-1000000, 1000001);
    long long lichao_ms = elapsed_ms([&] {
        for (int operation = 0; operation < 200000; ++operation) {
            if (operation & 1) {
                long long x = int(random() % 2000001) - 1000000;
                checksum += uint64_t(lichao.query(x).val());
            } else {
                long long slope = int(random() % 2001) - 1000;
                long long intercept = int(random() % 2000001) - 1000000;
                lichao.add(slope, intercept);
            }
        }
    });

    printf("sort_400k_ms=%lld\n", sort_ms);
    printf("fenwick_600k_ms=%lld\n", fenwick_ms);
    printf("ntt_131k_x2_ms=%lld\n", ntt_ms);
    printf("lichao_200k_ms=%lld\n", lichao_ms);
    printf("checksum=%llu\n", (unsigned long long)checksum);
}
