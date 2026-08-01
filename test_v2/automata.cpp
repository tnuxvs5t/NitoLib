#include "common.hpp"

static auto letters(string& text) { return nproject(text, [](char c) { return c - 'a'; }); }

struct nnoncopy_match_sink {
    int calls = 0;
    nnoncopy_match_sink() = default;
    nnoncopy_match_sink(const nnoncopy_match_sink&) = delete;
    void operator()(int, int) { ++calls; }
};

int main() {
    ntrie<26> trie;
    string apple = "apple", app = "app", ape = "ape";
    trie.add(letters(apple));
    trie.add(letters(app));
    trie.add(letters(app));
    trie.add(letters(ape));
    ntest(trie.count(letters(app)) == 2);
    ntest(trie.count_prefix(letters(app)) == 3);
    string absent = "apply";
    ntest(trie.find(letters(absent)) == npos);
    nvector<unsigned long long> oversized_symbol{1ULL << 32};
    ntest(trie.find(oversized_symbol, 77) == 77);

    nac<26> automaton;
    nvector<string> patterns{"he", "she", "hers", "his"};
    for (int i = 0; i < patterns.len(); ++i)
        ntest(automaton.add(letters(patterns[i])) == i);
    automaton.build();
    string sample = "ushers";
    nvector<pair<int, int>> matches;
    automaton.match(letters(sample), [&](int end, int id) { matches.push(end, id); });
    ntest(matches.len() == 3 && automaton.count(letters(sample)) == 3);
    nnoncopy_match_sink sink;
    automaton.match(letters(sample), sink);
    ntest(sink.calls == 3);

    mt19937 random(0x61acU);
    for (int repeat = 0; repeat < 300; ++repeat) {
        int count = 1 + random() % 10, text_length = random() % 35;
        nvector<nvector<int>> words(count);
        nac<3> ac;
        for (int id = 0; id < count; ++id) {
            int length = 1 + random() % 8;
            words[id].resize(length);
            for (int i = 0; i < length; ++i)
                words[id][i] = random() % 3;
            ntest(ac.add(words[id]) == id);
        }
        ac.build();
        nvector<int> text(text_length), got(count), want(count);
        for (int i = 0; i < text_length; ++i)
            text[i] = random() % 3;
        ac.match(text, [&](int, int id) { ++got[id]; });
        for (int id = 0; id < count; ++id)
            for (int first = 0; first + words[id].len() <= text.len(); ++first) {
                bool equal = true;
                for (int j = 0; j < words[id].len(); ++j)
                    equal &= text[first + j] == words[id][j];
                want[id] += equal;
            }
        ntest(got == want);
    }
}
