#include "common.hpp"

static vector<int> values_of(const nseq_fhq<int>& sequence) {
    vector<int> result;
    nfor(value, sequence)
        result.push_back(value);
    return result;
}

int main() {
    mt19937 random(0x3d0a1U);
    auto domain = nseq_fhq<int>{}.domain();
    nseq_fhq<int> left(domain), right(domain);
    vector<int> reference;
    for (int i = 0; i < 80; ++i) {
        int value = int(random() % 1000);
        left.push(value);
        reference.push_back(value);
    }

    for (int step = 0; step < 600; ++step) {
        int cut = int(random() % (reference.size() + 1));
        auto pieces = move(left).split_at(cut);
        left = move(pieces.first);
        right = move(pieces.second);
        vector<int> prefix(reference.begin(), reference.begin() + cut);
        vector<int> suffix(reference.begin() + cut, reference.end());
        ntest(values_of(left) == prefix && values_of(right) == suffix);

        left.merge_from(move(right));
        ntest(values_of(left) == reference && right.empty());

        if (random() & 1) {
            int at = int(random() % (reference.size() + 1));
            int value = int(random() % 1000);
            left.ins(at, value);
            reference.insert(reference.begin() + at, value);
        } else if (!reference.empty()) {
            int at = int(random() % reference.size());
            left.del(at);
            reference.erase(reference.begin() + at);
        }
        ntest(values_of(left) == reference);
    }
}
