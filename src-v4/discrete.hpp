#pragma once
#include "func.hpp"
#include "sequence.hpp"

/*
   Discrete operations build reusable position plans.  Ordinary traversal remains
   ordinary STL; these functions are here because a plan can be saved, composed,
   applied to a view, or used to reorder an nfunc's semantic domain.
*/
template <class S, class I>
constexpr auto nselect(S source, I positions) {
    return ngather(move(source), move(positions));
}

template <class D, class F, class I>
requires requires(I& positions) { positions.len(); }
constexpr auto nselect(nfunc<D, F> function, I positions) {
    auto domain = ngather(move(function.domain), move(positions));
    return nfunc{move(domain), move(function.eval)};
}

template <class S>
constexpr auto nselect(S source, vector<nidx_t> positions) {
    nidx_t n = nidx_t(positions.size());
    return nselect(move(source), ntabulate(n, [positions = move(positions)](nidx_t i) {
        return positions[i];
    }));
}

template <class S>
constexpr auto nslice(S source, nidx_t left, nidx_t right) {
    return nselect(move(source), nrange(left, right));
}

template <class S, class A, class B>
requires (nidx_wider_v<A> || nidx_wider_v<B>)
constexpr auto nslice(S, A, B) = delete;

template <class S, class T = remove_cvref_t<decltype(declval<S&>()[0])>,
          class F = plus<>>
constexpr vector<T> nprefix(S&& source, T identity = {}, F operation = {}) {
    vector<T> result;
    result.reserve(size_t(nlen(source)) + 1);
    result.push_back(move(identity));
    for (nidx_t i = 0; i < nlen(source); ++i)
        result.push_back(invoke(operation, as_const(result.back()), source[i]));
    return result;
}

template <class S, class T = remove_cvref_t<decltype(declval<S&>()[0])>,
          class F = plus<>>
constexpr vector<T> nsuffix(S&& source, T identity = {}, F operation = {}) {
    vector<T> result;
    result.reserve(size_t(nlen(source)) + 1);
    result.push_back(move(identity));
    for (nidx_t i = nlen(source); i-- > 0;)
        result.push_back(invoke(operation, source[i], as_const(result.back())));
    ranges::reverse(result);
    return result;
}

template <class S>
constexpr auto nstride(S source, nidx_t first, nidx_t last, nidx_t step) {
    nidx_t distance = step > 0 ? max(nidx_t(0), last - first)
                               : max(nidx_t(0), first - last);
    nuidx_t width = step > 0 ? nuidx_t(step) : nuidx_t(0) - nuidx_t(step);
    nidx_t count = nidx_t(nuidx_t(distance) / width +
                          (nuidx_t(distance) % width != 0));
    return nselect(move(source), ntabulate(
        count,
        [first, step](nidx_t i) { return first + i * step; },
        [first, step](nidx_t position) { return (position - first) / step; }
    ));
}

template <class S, class A, class B, class C>
requires (nidx_wider_v<A> || nidx_wider_v<B> || nidx_wider_v<C>)
constexpr auto nstride(S, A, B, C) = delete;

template <class S>
constexpr auto nstride(S source, nidx_t step) {
    nidx_t n = nlen(source);
    return step > 0 ? nstride(move(source), 0, n, step)
                    : nstride(move(source), n - 1, -1, step);
}

template <class S, class I>
requires nidx_wider_v<I>
constexpr auto nstride(S, I) = delete;

template <class S, class P>
vector<nidx_t> npositions(S&& source, P predicate) {
    vector<nidx_t> positions;
    positions.reserve(nlen(source));
    for (nidx_t i = 0; i < nlen(source); ++i)
        if (invoke(predicate, source[i])) positions.push_back(i);
    return positions;
}

template <class S, class P>
auto nfilter(S source, P predicate) {
    auto positions = npositions(source, move(predicate));
    return nselect(move(source), move(positions));
}

template <class S, class P = equal_to<>>
auto nunique(S source, P together = {}) {
    auto positions = npositions(nrange(nlen(source)), [&](nidx_t i) {
        return !i || !invoke(together, source[i - 1], source[i]);
    });
    return nselect(move(source), move(positions));
}

template <class S>
constexpr auto nindexed(S source) {
    return nzip(nrange(nlen(source)), move(source));
}

template <class T = void, class S>
auto ncollect(S&& source) {
    using item = nowned_t<decltype(source[0])>;
    using result = conditional_t<is_void_v<T>, item, T>;
    vector<result> values;
    values.reserve(nlen(source));
    for (nidx_t i = 0; i < nlen(source); ++i) values.emplace_back(source[i]);
    return values;
}

template <class D, class F>
constexpr void nassign(D&& destination, F value_at) {
    for (nidx_t i = 0; i < nlen(destination); ++i)
        destination[i] = invoke(value_at, i);
}

template <class D, class T>
constexpr void nfill(D&& destination, T value) {
    nassign(forward<D>(destination), [&](nidx_t) -> const T& { return value; });
}

template <class S, class D>
constexpr void ncopy(S&& source, D&& destination) {
    nidx_t n = nlen(source);
    for (nidx_t i = 0; i < n; ++i) destination[i] = source[i];
}

template <class S, class D, class F>
constexpr void ntransform(S&& source, D&& destination, F operation) {
    nidx_t n = nlen(source);
    for (nidx_t i = 0; i < n; ++i)
        destination[i] = invoke(operation, source[i]);
}

template <class A, class B, class D, class F>
constexpr void ntransform(A&& first, B&& second, D&& destination, F operation) {
    nidx_t n = nlen(first);
    for (nidx_t i = 0; i < n; ++i)
        destination[i] = invoke(operation, first[i], second[i]);
}

template <class S, class T, class F = plus<>>
constexpr T naccumulate(S&& source, T initial, F operation = {}) {
    for (nidx_t i = 0; i < nlen(source); ++i)
        initial = invoke(operation, move(initial), source[i]);
    return initial;
}

template <class S, class F>
constexpr F neach(S&& source, F action) {
    for (nidx_t i = 0; i < nlen(source); ++i) invoke(action, source[i]);
    return action;
}

template <class S, class P>
constexpr nidx_t nfind_if(S&& source, P predicate) {
    nidx_t n = nlen(source);
    for (nidx_t i = 0; i < n; ++i)
        if (invoke(predicate, source[i])) return i;
    return n;
}

template <class S, class T, class C = equal_to<>, class P = identity>
constexpr bool ncontains(S&& source, const T& target, C compare = {}, P projection = {}) {
    return nfind_if(source, [&](auto&& value) {
        return invoke(compare, invoke(projection, forward<decltype(value)>(value)), target);
    }) != nlen(source);
}

template <class S, class P>
constexpr nidx_t ncount_if(S&& source, P predicate) {
    nidx_t count = 0;
    for (nidx_t i = 0; i < nlen(source); ++i)
        count += bool(invoke(predicate, source[i]));
    return count;
}

template <class S, class P>
constexpr bool nall_of(S&& source, P predicate) {
    return nfind_if(source, [&](auto&& value) {
        return !invoke(predicate, forward<decltype(value)>(value));
    }) == nlen(source);
}

template <class S, class P>
constexpr bool nany_of(S&& source, P predicate) {
    return nfind_if(source, move(predicate)) != nlen(source);
}

template <class S, class P>
constexpr bool nnone_of(S&& source, P predicate) {
    return !nany_of(source, move(predicate));
}

template <class S, class C = less<>, class P = identity>
constexpr nidx_t nargmin(S&& source, C compare = {}, P projection = {}) {
    nidx_t n = nlen(source), best = n ? 0 : n;
    for (nidx_t i = 1; i < n; ++i)
        if (invoke(compare, invoke(projection, source[i]),
                    invoke(projection, source[best]))) best = i;
    return best;
}

template <class S, class C = less<>, class P = identity>
constexpr nidx_t nargmax(S&& source, C compare = {}, P projection = {}) {
    return nargmin(source, [&](auto&& left, auto&& right) {
        return invoke(compare, forward<decltype(right)>(right),
                       forward<decltype(left)>(left));
    }, move(projection));
}

template <class F>
constexpr nidx_t nboundary(nidx_t length, F before) {
    nidx_t left = 0, right = length;
    while (left < right) {
        nidx_t middle = left + (right - left) / 2;
        if (invoke(before, middle)) left = middle + 1;
        else right = middle;
    }
    return left;
}

template <class S, class T, class C = less<>, class P = identity>
constexpr nidx_t nlower(S&& source, const T& value, C compare = {}, P projection = {}) {
    return nboundary(nlen(source), [&](nidx_t i) {
        return invoke(compare, invoke(projection, source[i]), value);
    });
}

template <class S, class T, class C = less<>, class P = identity>
constexpr nidx_t nupper(S&& source, const T& value, C compare = {}, P projection = {}) {
    return nboundary(nlen(source), [&](nidx_t i) {
        return !invoke(compare, value, invoke(projection, source[i]));
    });
}

template <class S, class C = less<>, class P = identity>
auto norder(S source, C compare = {}, P projection = {}) {
    auto order = nargsort(source, move(compare), move(projection));
    return nselect(move(source), move(order));
}

template <class S, class C = less<>, class P = identity>
constexpr void nsort(S&& source, C compare = {}, P projection = {}) {
    auto values = ntabulate(nlen(source), [p = addressof(source)](nidx_t i)
            -> decltype(auto) { return (*p)[i]; });
    ranges::sort(values, [&](auto&& left, auto&& right) {
        return invoke(compare, invoke(projection, forward<decltype(left)>(left)),
                       invoke(projection, forward<decltype(right)>(right)));
    });
}

template <class S>
constexpr void nreverse_inplace(S&& source) {
    for (nidx_t i = 0, n = nlen(source); i < n / 2; ++i)
        swap(source[i], source[n - 1 - i]);
}

/* A retained descriptor is the one place where shared ownership is semantic: chunks
   may be detached from their parent function while still borrowing external owners. */
template <class S>
auto nretain(shared_ptr<S> source) {
    return nview{
        nlen(*source),
        [source = move(source)](nidx_t i) -> decltype(auto) { return (*source)[i]; }
    };
}

template <class D, class F>
auto nretain(shared_ptr<nfunc<D, F>> source) {
    auto domain = nretain(shared_ptr<D>(source, addressof(source->domain)));
    return nfunc{
        move(domain),
        [source = move(source)](auto&& key) -> decltype(auto) {
            return invoke(source->eval, forward<decltype(key)>(key));
        }
    };
}

template <class S, class I>
auto nchunks(S source, I intervals) {
    auto retained = nretain(make_shared<S>(move(source)));
    return nfunc{
        move(intervals),
        [source = move(retained)](pair<nidx_t, nidx_t> interval) mutable {
            return nslice(source, interval.first, interval.second);
        }
    };
}

template <class S>
constexpr auto nblock(S source, nidx_t width, nidx_t index) {
    nidx_t n = nlen(source);
    nidx_t left = index * width;
    return nslice(move(source), left, left + min(width, n - left));
}

template <class S>
constexpr auto nblocks(S source, nidx_t width) {
    nidx_t n = nlen(source), count = n / width + (n % width != 0);
    auto intervals = ntabulate(count, [n, width](nidx_t i) {
        nidx_t left = i * width;
        return pair{left, left + min(width, n - left)};
    });
    return nchunks(move(source), move(intervals));
}

template <class S>
constexpr auto nwindows(S source, nidx_t width, nidx_t step = 1) {
    nidx_t n = nlen(source), count = width <= n ? 1 + (n - width) / step : 0;
    auto intervals = ntabulate(count, [width, step](nidx_t i) {
        nidx_t left = i * step;
        return pair{left, left + width};
    });
    return nchunks(move(source), move(intervals));
}

template <class S, class P = nullptr_t>
auto nruns(S source, P operation = {}) {
    vector<pair<nidx_t, nidx_t>> bounds;
    nidx_t n = nlen(source), left = 0;
    auto accept = [&](nidx_t right) {
        if constexpr (same_as<P, nullptr_t>)
            return right - left == 1 || source[right - 2] == source[right - 1];
        else
            return bool(invoke(operation, left, right));
    };
    for (nidx_t i = 0; i < n; ++i) {
        if (!accept(i + 1)) {
            bounds.push_back({left, i});
            left = i;
            (void)accept(i + 1);
        }
    }
    if (n) bounds.push_back({left, n});
    nidx_t count = nidx_t(bounds.size());
    auto intervals = ntabulate(count,
                               [bounds = move(bounds)](nidx_t i) { return bounds[i]; });
    return nchunks(move(source), move(intervals));
}
