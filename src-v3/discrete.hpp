#pragma once
#include "func.hpp"

/*
This module treats an index list as a reusable structural plan.  Applying the plan
to an nview reorders positions; applying it to an nfunc reorders its semantic domain
and keeps the evaluator.  A vector<int> plan is moved into the returned descriptor,
so a locally computed filter/order never leaves a dangling index view.

Sources are descriptors passed by value.  Borrow an ordinary owner with nall(owner).
All positions and [left,right) intervals must be valid; stride is nonzero.  Selection
is lazy, O(1) per access, and repeated positions deliberately alias the same lvalue.
*/
template <class S, class I>
constexpr auto nselect(S source, I positions) {
    return ngather(move(source), move(positions));
}

template <class D, class F, class I>
constexpr auto nselect(nfunc<D, F> function, I positions) {
    auto domain = ngather(move(function.domain), move(positions));
    return nfunc{move(domain), move(function.eval)};
}

template <class S>
constexpr auto nselect(S source, vector<int> positions) {
    int n = int(positions.size());
    return nview{n, [source = move(source), positions = move(positions)](int i) mutable
                        -> decltype(auto) { return source[positions[i]]; }};
}

template <class D, class F>
constexpr auto nselect(nfunc<D, F> function, vector<int> positions) {
    auto domain = nselect(move(function.domain), move(positions));
    return nfunc{move(domain), move(function.eval)};
}

template <class S>
constexpr auto nslice(S source, int left, int right) {
    return nselect(move(source), nrange(left, right));
}

/*
Prefix/suffix scans include the identity, so both results have source.len()+1 values.
The default is plus<> with value-initialized element type.  A custom identity selects
the accumulator type.  operation creates a new accumulator without mutating the saved
one; prefix uses operation(accumulator,value), suffix operation(value,accumulator).
*/
template <class S, class T = remove_cvref_t<decltype(declval<S&>()[0])>,
          class F = plus<>>
constexpr vector<T> nprefix(S source, T identity = {}, F operation = {}) {
    int n = nlen(source);
    vector<T> result;
    result.reserve(size_t(n) + 1);
    result.push_back(move(identity));
    for (int i = 0; i < n; ++i) {
        T next = invoke(operation, as_const(result.back()), source[i]);
        result.push_back(move(next));
    }
    return result;
}

template <class S, class T = remove_cvref_t<decltype(declval<S&>()[0])>,
          class F = plus<>>
constexpr vector<T> nsuffix(S source, T identity = {}, F operation = {}) {
    int n = nlen(source);
    vector<T> result;
    result.reserve(size_t(n) + 1);
    result.push_back(move(identity));
    for (int i = n; i-- > 0;) {
        T next = invoke(operation, source[i], as_const(result.back()));
        result.push_back(move(next));
    }
    ranges::reverse(result);
    return result;
}

template <class S>
constexpr auto nstride(S source, int first, int last, int step) {
    int distance = step > 0 ? max(0, last - first) : max(0, first - last);
    int width = abs(step);
    int count = distance / width + (distance % width != 0);
    return nselect(move(source), ntabulate(
        count,
        [first, step](int i) { return first + i * step; },
        [first, step](int position) { return (position - first) / step; }
    ));
}

template <class S>
constexpr auto nstride(S source, int step) {
    int n = nlen(source);
    return step > 0 ? nstride(move(source), 0, n, step)
                    : nstride(move(source), n - 1, -1, step);
}

template <class S, class P>
auto nfilter(S source, P predicate) {
    vector<int> positions;
    positions.reserve(nlen(source));
    for (int i = 0; i < nlen(source); ++i)
        if (invoke(predicate, source[i])) positions.push_back(i);
    return nselect(move(source), move(positions));
}

/* Positional enumeration is lazy; tuple element 1 preserves the source result category. */
template <class S>
constexpr auto nindexed(S source) {
    int n = nlen(source);
    return nzip(nrange(n), move(source));
}

/* ncollect recursively removes references inside pair/tuple results such as nzip. */
template <class T = void, class S>
auto ncollect(S source) {
    using inferred = nview_detail::owned_t<decltype(source[0])>;
    using result = conditional_t<is_void_v<T>, inferred, T>;
    vector<result> values;
    values.reserve(nlen(source));
    for (int i = 0; i < nlen(source); ++i) values.emplace_back(source[i]);
    return values;
}

/*
Position write kernel.  nassign(destination,value_at) assigns value_at(position) and is
the primitive behind fill/copy/transform.  Destinations yield assignable lvalues;
ncopy/ntransform require destination.len() >= source.len(), and binary transform also
requires equal input lengths.  Calls proceed left-to-right.  Descriptors may be temporary,
but their borrowed owners outlive the call.  Overlapping source/destination storage has
sequential rather than snapshot semantics, so use ncollect first when detached input is
required.
*/
namespace ndiscrete_detail {
template <class D, class F>
constexpr void assign(D&& destination, int count, F& value_at) {
    for (int i = 0; i < count; ++i) destination[i] = invoke(value_at, i);
}
}

template <class D, class F>
constexpr void nassign(D&& destination, F value_at) {
    int n = nlen(destination);
    ndiscrete_detail::assign(forward<D>(destination), n, value_at);
}

template <class D, class T>
constexpr void nfill(D&& destination, T value) {
    nassign(forward<D>(destination), [&](int) -> const T& { return value; });
}

template <class S, class D>
constexpr void ncopy(S&& source, D&& destination) {
    int n = nlen(source);
    auto value_at = [&](int i) -> decltype(auto) { return source[i]; };
    ndiscrete_detail::assign(forward<D>(destination), n, value_at);
}

template <class S, class D, class F>
constexpr void ntransform(S&& source, D&& destination, F operation) {
    int n = nlen(source);
    auto value_at = [&](int i) -> decltype(auto) {
        return invoke(operation, source[i]);
    };
    ndiscrete_detail::assign(forward<D>(destination), n, value_at);
}

template <class A, class B, class D, class F>
constexpr void ntransform(A&& first, B&& second, D&& destination, F operation) {
    int n = nlen(first);
    auto value_at = [&](int i) -> decltype(auto) {
        return invoke(operation, first[i], second[i]);
    };
    ndiscrete_detail::assign(forward<D>(destination), n, value_at);
}

/* Left-to-right scalar fold.  operation(accumulator,value) must return assignable T. */
template <class S, class T, class F = plus<>>
constexpr T naccumulate(S source, T initial, F operation = {}) {
    for (int i = 0; i < nlen(source); ++i)
        initial = invoke(operation, move(initial), source[i]);
    return initial;
}

/* Like std::for_each, neach returns the possibly stateful action after ordered calls. */
template <class S, class F>
constexpr F neach(S source, F action) {
    for (int i = 0; i < nlen(source); ++i) invoke(action, source[i]);
    return action;
}

template <class S, class P>
constexpr int nfind_if(S source, P predicate) {
    int n = nlen(source);
    for (int i = 0; i < n; ++i)
        if (invoke(predicate, source[i])) return i;
    return n;
}

template <class S, class P>
constexpr int ncount_if(S source, P predicate) {
    int count = 0;
    for (int i = 0; i < nlen(source); ++i) count += bool(invoke(predicate, source[i]));
    return count;
}

template <class S, class P>
constexpr bool nall_of(S source, P predicate) {
    int n = nlen(source);
    return nfind_if(move(source), [&](auto&& value) {
               return !invoke(predicate, forward<decltype(value)>(value));
           }) == n;
}

template <class S, class P>
constexpr bool nany_of(S source, P predicate) {
    int n = nlen(source);
    return nfind_if(move(source), move(predicate)) != n;
}

template <class S, class P>
constexpr bool nnone_of(S source, P predicate) {
    return !nany_of(move(source), move(predicate));
}

/* Extrema return the positional index, or len() for an empty source. */
template <class S, class C = less<>, class P = identity>
constexpr int nargmin(S source, C compare = {}, P projection = {}) {
    int n = nlen(source), best = n ? 0 : n;
    for (int i = 1; i < n; ++i)
        if (invoke(compare, invoke(projection, source[i]),
                    invoke(projection, source[best]))) best = i;
    return best;
}

template <class S, class C = less<>, class P = identity>
constexpr int nargmax(S source, C compare = {}, P projection = {}) {
    return nargmin(move(source), [&](auto&& left, auto&& right) {
        return invoke(compare, forward<decltype(right)>(right),
                       forward<decltype(left)>(left));
    }, move(projection));
}

/* Binary bounds use positional sorted order and return an insertion position. */
template <class S, class T, class C = less<>, class P = identity>
constexpr int nlower(S source, const T& value, C compare = {}, P projection = {}) {
    int left = 0, right = nlen(source);
    while (left < right) {
        int middle = left + (right - left) / 2;
        if (invoke(compare, invoke(projection, source[middle]), value)) left = middle + 1;
        else right = middle;
    }
    return left;
}

template <class S, class T, class C = less<>, class P = identity>
constexpr int nupper(S source, const T& value, C compare = {}, P projection = {}) {
    int left = 0, right = nlen(source);
    while (left < right) {
        int middle = left + (right - left) / 2;
        if (!invoke(compare, value, invoke(projection, source[middle]))) left = middle + 1;
        else right = middle;
    }
    return left;
}

/* nargsort is a positional plan; norder applies it without moving source values. */
template <class S, class C = less<>, class P = identity>
vector<int> nargsort(const S& source, C compare = {}, P projection = {}) {
    vector<int> order(nlen(source));
    iota(order.begin(), order.end(), 0);
    ranges::sort(order, [&](int left, int right) {
        return invoke(compare, invoke(projection, source[left]),
                       invoke(projection, source[right]));
    });
    return order;
}

template <class S, class C = less<>, class P = identity>
auto norder(S source, C compare = {}, P projection = {}) {
    auto order = nargsort(source, move(compare), move(projection));
    return nselect(move(source), move(order));
}

/*
These two algorithms mutate yielded lvalues, never keys or descriptor structure.  A
temporary descriptor is safe for the duration of the call; its borrowed owner must
still outlive the call.  Values must be swappable and sorting requires a strict order.
For sorting, positions describe distinct storage; repeated aliases are not a permutation.
*/
template <class S, class C = less<>, class P = identity>
constexpr void nsort(S&& source, C compare = {}, P projection = {}) {
    auto values = ntabulate(nlen(source), [p = addressof(source)](int i) -> decltype(auto) {
        return (*p)[i];
    });
    ranges::sort(values, [&](auto&& left, auto&& right) {
        return invoke(compare, invoke(projection, forward<decltype(left)>(left)),
                       invoke(projection, forward<decltype(right)>(right)));
    });
}

template <class S>
constexpr void nreverse_inplace(S&& source) {
    int n = nlen(source);
    for (int i = 0; i < n / 2; ++i) swap(source[i], source[n - 1 - i]);
}

/*
A chunk domain is keyed by its full [left,right) interval.  Evaluating that key creates
a slice, so no fake start-key locator or lower_bound is needed.  Child slices detach
from the chunk function; therefore its ordinary source descriptor must be copyable.
*/
template <class S, class I>
constexpr auto nchunks(S source, I intervals) {
    return nfunc{move(intervals), [source = move(source)](pair<int, int> interval) mutable {
                     return nslice(source, interval.first, interval.second);
                 }};
}

/* Width is positive; the final valid fixed-width block may be shorter. */
template <class S>
constexpr auto nblock(S source, int width, int index) {
    int n = nlen(source), left = index * width;
    return nslice(move(source), left, min(n, left + width));
}

template <class S>
constexpr auto nblocks(S source, int width) {
    int n = nlen(source), count = n / width + (n % width != 0);
    auto intervals = ntabulate(count, [n, width](int i) {
        return pair{i * width, min(n, (i + 1) * width)};
    });
    return nchunks(move(source), move(intervals));
}

/* width and step are positive.  Only complete windows are enumerated. */
template <class S>
constexpr auto nwindows(S source, int width, int step = 1) {
    int n = nlen(source), count = width <= n ? 1 + (n - width) / step : 0;
    auto intervals = ntabulate(count, [width, step](int i) {
        int left = i * step;
        return pair{left, left + width};
    });
    return nchunks(move(source), move(intervals));
}

/* Maximal adjacent runs.  together(previous,current) defines run membership. */
template <class S, class P = equal_to<>>
auto nruns(S source, P together = {}) {
    vector<pair<int, int>> bounds;
    int n = nlen(source), left = 0;
    for (int i = 1; i <= n; ++i)
        if (i == n || !invoke(together, source[i - 1], source[i]))
            bounds.push_back({left, i}), left = i;
    int count = int(bounds.size());
    auto intervals = ntabulate(count,
                               [bounds = move(bounds)](int i) { return bounds[i]; });
    return nchunks(move(source), move(intervals));
}
