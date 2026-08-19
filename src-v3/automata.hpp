#pragma once
#include "view.hpp"

/* Common lowercase mapping; custom alphabets are ordinary callables returning [0,sigma). */
struct nlowercase {
    constexpr nidx_t operator()(char symbol) const { return symbol - 'a'; }
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
    nidx_t sigma;
    [[no_unique_address]] mutable Map map;
    vector<nidx_t> transition, failure, order;

    explicit nac(nidx_t alphabet_size = 26, Map symbol_map = {})
        : sigma(alphabet_size), map(move(symbol_map)), transition(alphabet_size, -1),
          failure(1) {}

    nidx_t states() const { return nidx_t(failure.size()); }
    nidx_t& edge(nidx_t state, nidx_t symbol) { return transition[state * sigma + symbol]; }
    nidx_t edge(nidx_t state, nidx_t symbol) const { return transition[state * sigma + symbol]; }

    nidx_t make_state() {
        nidx_t state = states();
        failure.push_back(0);
        transition.resize(size_t(states()) * sigma, -1);
        return state;
    }

    template <class V>
    nidx_t add(V pattern) {
        nidx_t state = 0;
        for (nidx_t i = 0; i < pattern.len(); ++i) {
            nidx_t symbol = invoke(map, pattern[i]);
            nidx_t child = edge(state, symbol);
            if (child < 0) child = make_state(), edge(state, symbol) = child;
            state = child;
        }
        return state;
    }

    void build() {
        order.clear();
        order.reserve(states());
        order.push_back(0);
        for (nidx_t symbol = 0; symbol < sigma; ++symbol) {
            nidx_t& child = edge(0, symbol);
            if (child < 0) child = 0;
            else failure[child] = 0, order.push_back(child);
        }
        for (nidx_t at = 1; at < nidx_t(order.size()); ++at) {
            nidx_t state = order[at];
            for (nidx_t symbol = 0; symbol < sigma; ++symbol) {
                nidx_t& child = edge(state, symbol);
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
    nidx_t step(nidx_t state, Symbol&& symbol) const {
        return edge(state, invoke(map, forward<Symbol>(symbol)));
    }

    template <class V>
    vector<nidx_t> walk(V text) const {
        vector<nidx_t> result(text.len());
        nidx_t state = 0;
        for (nidx_t i = 0; i < text.len(); ++i) result[i] = state = step(state, text[i]);
        return result;
    }

    template <class V>
    vector<long long> occurrences(V text) const {
        vector<long long> count(states());
        nidx_t state = 0;
        ++count[0];
        for (nidx_t i = 0; i < text.len(); ++i) ++count[state = step(state, text[i])];
        for (auto it = order.rbegin(); it != order.rend(); ++it)
            if (*it) count[failure[*it]] += count[*it];
        return count;
    }
};
