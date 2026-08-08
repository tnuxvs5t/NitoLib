#include "common.hpp"

struct nconcat_words {
    string id() const { return {}; }
    string operator()(string left, const string& right) const { return left += right; }
};

int main() {
    nvector<string> words{"a", "bb", "c", "ddd", "e", "ff", "g"};
    nsparse<string, nconcat_words> table(words);
    for (int left = 0; left <= words.len(); ++left)
        for (int right = left; right <= words.len(); ++right) {
            string expected;
            for (int index = left; index < right; ++index)
                expected += words[index];
            ntest(table.fold(left, right) == expected);
        }

    mt19937 random(0x36c1a55U);
    for (int repeat = 0; repeat < 200; ++repeat) {
        int n = int(random() % 80);
        nvector<int> values(n);
        for (int index = 0; index < n; ++index)
            values[index] = int(random() % 1000) - 500;
        nsparse<int> minimum(values);
        for (int attempt = 0; attempt < 200; ++attempt) {
            int left = int(random() % (n + 1));
            int right = left + int(random() % (n - left + 1));
            int expected = numeric_limits<int>::max();
            for (int index = left; index < right; ++index)
                expected = min(expected, values[index]);
            ntest(minimum.fold(left, right) == expected);
        }
    }

    npotential_dsu<long long> potential(7);
    ntest(potential.bind(0, 1, 4));
    ntest(potential.bind(1, 2, -9));
    ntest(potential.diff(0, 2).val() == -5);
    ntest(potential.diff(2, 0).val() == 5);
    ntest(potential.bind(0, 2, -5));
    ntest(!potential.bind(0, 2, 6));
    ntest(!potential.diff(0, 6));

    for (int repeat = 0; repeat < 250; ++repeat) {
        int n = 2 + int(random() % 30);
        nvector<long long> hidden(n);
        for (int index = 0; index < n; ++index)
            hidden[index] = int(random() % 2001) - 1000;
        npotential_dsu<long long> dsu(n);
        for (int index = 1; index < n; ++index) {
            int parent = int(random() % index);
            ntest(dsu.bind(parent, index, hidden[index] - hidden[parent]));
        }
        for (int attempt = 0; attempt < 200; ++attempt) {
            int left = int(random() % n), right = int(random() % n);
            ntest(dsu.diff(left, right).val() == hidden[right] - hidden[left]);
        }
    }
}
