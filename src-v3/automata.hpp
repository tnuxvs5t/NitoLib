#pragma once
#include "view.hpp"

/* Common lowercase mapping; custom alphabets are ordinary callables returning [0,sigma). */
struct nlowercase {
    constexpr int operator()(char symbol) const { return symbol - 'a'; }
};

/*
Aho-Corasick trie/failure kernel.  Add every pattern before the single build(); after
build, missing transitions have been completed and structural insertion is invalid.
add returns the terminal state, so callers can attach any payload externally instead
of paying for a mandatory output representation.  occurrences(text)[state] counts how
often that trie prefix occurs as a suffix; state 0 (the empty pattern) counts n+1.
*/
template <class Map = nlowercase>
struct nac {
    int sigma;
    [[no_unique_address]] mutable Map map;
    vector<int> transition, failure, order;

    explicit nac(int alphabet_size = 26, Map symbol_map = {})
        : sigma(alphabet_size), map(move(symbol_map)), transition(alphabet_size, -1),
          failure(1) {}

    int states() const { return int(failure.size()); }
    int& edge(int state, int symbol) { return transition[state * sigma + symbol]; }
    int edge(int state, int symbol) const { return transition[state * sigma + symbol]; }

    int make_state() {
        int state = states();
        failure.push_back(0);
        transition.resize(size_t(states()) * sigma, -1);
        return state;
    }

    template <class V>
    int add(V pattern) {
        int state = 0;
        for (int i = 0; i < pattern.len(); ++i) {
            int symbol = invoke(map, pattern[i]);
            int child = edge(state, symbol);
            if (child < 0) child = make_state(), edge(state, symbol) = child;
            state = child;
        }
        return state;
    }

    void build() {
        order.clear();
        order.reserve(states());
        order.push_back(0);
        for (int symbol = 0; symbol < sigma; ++symbol) {
            int& child = edge(0, symbol);
            if (child < 0) child = 0;
            else failure[child] = 0, order.push_back(child);
        }
        for (int at = 1; at < int(order.size()); ++at) {
            int state = order[at];
            for (int symbol = 0; symbol < sigma; ++symbol) {
                int& child = edge(state, symbol);
                if (child < 0)
                    child = edge(failure[state], symbol);
                else {
                    failure[child] = edge(failure[state], symbol);
                    order.push_back(child);
                }
            }
        }
    }

    template <class Symbol>
    int step(int state, Symbol&& symbol) const {
        return edge(state, invoke(map, forward<Symbol>(symbol)));
    }

    template <class V>
    vector<int> walk(V text) const {
        vector<int> result(text.len());
        int state = 0;
        for (int i = 0; i < text.len(); ++i) result[i] = state = step(state, text[i]);
        return result;
    }

    template <class V>
    vector<long long> occurrences(V text) const {
        vector<long long> count(states());
        int state = 0;
        ++count[0];
        for (int i = 0; i < text.len(); ++i) ++count[state = step(state, text[i])];
        for (auto it = order.rbegin(); it != order.rend(); ++it)
            if (*it) count[failure[*it]] += count[*it];
        return count;
    }
};
