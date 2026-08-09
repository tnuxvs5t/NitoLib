#pragma once
#include "arena.hpp"

template <class T>
struct nline {
    T slope, intercept;
    template <class X>
    constexpr auto operator()(const X& x) const { return slope * x + intercept; }
};

struct nline_eval {
    template <class L, class X>
    constexpr decltype(auto) operator()(const L& line, const X& x) const {
        return invoke(line, x);
    }
};

/*
Sparse Li Chao kernel on integral [lo,hi).  Any two admissible functions cross at most
once; eval(line,x) returns Y and better(a,b) selects the desired minimum/maximum.
Roots are destructive handles in one arena, -1 is empty, and distinct live roots must
not share nodes.  add_segment accepts a valid half-open subrange.  Each operation costs
O(log(hi-lo)); an empty-root query returns infinity.
*/
template <class Line, class X, class Y, class Eval = nline_eval, class Better = less<>>
struct nlichao {
    struct node { optional<Line> line; int left = -1, right = -1; };
    narena<node> pool;
    X lo, hi;
    Y infinity;
    [[no_unique_address]] mutable Eval evaluate;
    [[no_unique_address]] mutable Better better;

    nlichao(X left, X right, Y identity, Eval evaluator = {}, Better comparison = {})
        : lo(left), hi(right), infinity(move(identity)), evaluate(move(evaluator)),
          better(move(comparison)) {}

    int nodes() const { return pool.len(); }
    void reserve(int count) { pool.reserve(count); }

  private:
    int add0(int root, X left, X right, Line line) {
        if (root < 0) return pool.make(node{move(line)});
        if (!pool[root].line) {
            pool[root].line.emplace(move(line));
            return root;
        }
        X middle = midpoint(left, right);
        bool wins_left = invoke(better, invoke(evaluate, line, left),
                                invoke(evaluate, *pool[root].line, left));
        bool wins_middle = invoke(better, invoke(evaluate, line, middle),
                                  invoke(evaluate, *pool[root].line, middle));
        if (wins_middle) swap(line, *pool[root].line);
        if (left + 1 == right) return root;
        if (wins_left != wins_middle)
            pool[root].left = add0(pool[root].left, left, middle, move(line));
        else
            pool[root].right = add0(pool[root].right, middle, right, move(line));
        return root;
    }

    int add_segment0(int root, X left, X right, X query_left, X query_right, const Line& line) {
        if (query_right <= left || right <= query_left) return root;
        if (query_left <= left && right <= query_right) return add0(root, left, right, line);
        if (root < 0) root = pool.make(node{});
        X middle = midpoint(left, right);
        pool[root].left = add_segment0(pool[root].left, left, middle,
                                       query_left, query_right, line);
        pool[root].right = add_segment0(pool[root].right, middle, right,
                                        query_left, query_right, line);
        return root;
    }

    Y query0(int root, X left, X right, X position) const {
        if (root < 0) return infinity;
        Y result = pool[root].line ? invoke(evaluate, *pool[root].line, position) : infinity;
        if (left + 1 == right) return result;
        X middle = midpoint(left, right);
        Y child = position < middle ? query0(pool[root].left, left, middle, position)
                                    : query0(pool[root].right, middle, right, position);
        return invoke(better, child, result) ? child : result;
    }

  public:
    int add(int root, Line line) { return add0(root, lo, hi, move(line)); }
    int add_segment(int root, X left, X right, const Line& line) {
        return add_segment0(root, lo, hi, left, right, line);
    }
    Y query(int root, X position) const { return query0(root, lo, hi, position); }
};
