#include "../src-v3/automata.hpp"

void check(bool condition, const char* message) {
    if (!condition) throw runtime_error(message);
}

int main() {
    mt19937 random(0xacac2026U);
    for (nidx_t trial = 0; trial < 1500; ++trial) {
        nidx_t pattern_count = 1 + nidx_t(random() % 25);
        vector<string> patterns(pattern_count);
        nac automaton(3, [](char symbol) { return symbol - 'a'; });
        vector<nidx_t> terminal(pattern_count);
        for (nidx_t i = 0; i < pattern_count; ++i) {
            nidx_t length = nidx_t(random() % 9);
            for (nidx_t j = 0; j < length; ++j) patterns[i] += char('a' + random() % 3);
            terminal[i] = automaton.add(nall(patterns[i]));
        }
        automaton.build();
        nidx_t text_length = nidx_t(random() % 80);
        string text;
        for (nidx_t i = 0; i < text_length; ++i) text += char('a' + random() % 3);
        auto count = automaton.occurrences(nall(text));
        auto states = automaton.walk(nall(text));
        check(nidx_t(states.size()) == text_length, "walk length");
        for (nidx_t i = 0; i < pattern_count; ++i) {
            long long expected = 0;
            if (patterns[i].empty()) expected = text_length + 1;
            else for (nidx_t at = 0; at + nidx_t(patterns[i].size()) <= text_length; ++at)
                expected += equal(patterns[i].begin(), patterns[i].end(), text.begin() + at);
            check(count[terminal[i]] == expected, "occurrence count");
        }
    }

    vector<vector<nidx_t>> patterns{{0, 1}, {1}, {0, 1, 0}, {}};
    vector<nidx_t> text{0, 1, 0, 1, 0};
    nac automaton(2, [](nidx_t symbol) { return symbol; });
    vector<nidx_t> terminal;
    for (auto& pattern : patterns) terminal.push_back(automaton.add(nall(pattern)));
    automaton.build();
    auto count = automaton.occurrences(nall(text));
    check(count[terminal[0]] == 2, "integer alphabet ab");
    check(count[terminal[1]] == 2, "integer alphabet b");
    check(count[terminal[2]] == 2, "integer alphabet aba");
    check(count[terminal[3]] == 6, "integer alphabet empty");
}
