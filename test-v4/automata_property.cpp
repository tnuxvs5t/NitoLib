#include "../src-v4/automata.hpp"
#include "../src-v4/view.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

int main() {
    mt19937 rng(0xacac2026U);
    for (nidx_t round = 0; round < 1200; ++round) {
        nidx_t pattern_count = 1 + nidx_t(rng() % 25);
        vector<string> patterns(pattern_count);
        nac automaton(3, [](char symbol) { return symbol - 'a'; });
        vector<nidx_t> terminal(pattern_count);
        for (nidx_t i = 0; i < pattern_count; ++i) {
            nidx_t length = nidx_t(rng() % 9);
            for (nidx_t j = 0; j < length; ++j) patterns[i] += char('a' + rng() % 3);
            terminal[i] = automaton.add(nall(patterns[i]));
        }
        automaton.build();
        string text;
        for (nidx_t i = 0, n = rng() % 80; i < n; ++i) text += char('a' + rng() % 3);
        auto count = automaton.occurrences(nall(text));
        CHECK(nidx_t(automaton.walk(nall(text)).size()) == nidx_t(text.size()));
        for (nidx_t i = 0; i < pattern_count; ++i) {
            long long expected = 0;
            if (patterns[i].empty()) expected = text.size() + 1;
            else for (nidx_t at = 0; at + nidx_t(patterns[i].size()) <= nidx_t(text.size()); ++at)
                expected += equal(patterns[i].begin(), patterns[i].end(), text.begin() + at);
            CHECK(count[terminal[i]] == expected);
        }
    }

    vector<vector<nidx_t>> patterns{{0, 1}, {1}, {0, 1, 0}, {}};
    vector<nidx_t> text{0, 1, 0, 1, 0};
    nac automaton(2, [](nidx_t symbol) { return symbol; });
    vector<nidx_t> terminal;
    for (auto& pattern : patterns) terminal.push_back(automaton.add(nall(pattern)));
    automaton.build();
    auto count = automaton.occurrences(nall(text));
    CHECK(count[terminal[0]] == 2 && count[terminal[1]] == 2);
    CHECK(count[terminal[2]] == 2 && count[terminal[3]] == 6);

    cout << "v4 automata: configurable alphabet, failure build and occurrence counts passed\n";
}
