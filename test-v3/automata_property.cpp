#include "../src-v3/automata.hpp"

void check(bool condition, const char* message) {
    if (!condition) throw runtime_error(message);
}

int main() {
    mt19937 random(0xacac2026U);
    for (int trial = 0; trial < 1500; ++trial) {
        int pattern_count = 1 + int(random() % 25);
        vector<string> patterns(pattern_count);
        nac automaton(3, [](char symbol) { return symbol - 'a'; });
        vector<int> terminal(pattern_count);
        for (int i = 0; i < pattern_count; ++i) {
            int length = int(random() % 9);
            for (int j = 0; j < length; ++j) patterns[i] += char('a' + random() % 3);
            terminal[i] = automaton.add(nall(patterns[i]));
        }
        automaton.build();
        int text_length = int(random() % 80);
        string text;
        for (int i = 0; i < text_length; ++i) text += char('a' + random() % 3);
        auto count = automaton.occurrences(nall(text));
        auto states = automaton.walk(nall(text));
        check(int(states.size()) == text_length, "walk length");
        for (int i = 0; i < pattern_count; ++i) {
            long long expected = 0;
            if (patterns[i].empty()) expected = text_length + 1;
            else for (int at = 0; at + int(patterns[i].size()) <= text_length; ++at)
                expected += equal(patterns[i].begin(), patterns[i].end(), text.begin() + at);
            check(count[terminal[i]] == expected, "occurrence count");
        }
    }

    vector<vector<int>> patterns{{0, 1}, {1}, {0, 1, 0}, {}};
    vector<int> text{0, 1, 0, 1, 0};
    nac automaton(2, [](int symbol) { return symbol; });
    vector<int> terminal;
    for (auto& pattern : patterns) terminal.push_back(automaton.add(nall(pattern)));
    automaton.build();
    auto count = automaton.occurrences(nall(text));
    check(count[terminal[0]] == 2, "integer alphabet ab");
    check(count[terminal[1]] == 2, "integer alphabet b");
    check(count[terminal[2]] == 2, "integer alphabet aba");
    check(count[terminal[3]] == 6, "integer alphabet empty");
}
