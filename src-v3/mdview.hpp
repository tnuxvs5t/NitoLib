#pragma once
#include "core.hpp"

/* Borrowed affine multidimensional view. shape is the number of positions per
   axis, bias is its semantic coordinate origin, stride is the physical element
   step. Keys/coordinates use long long independently of nidx_t lengths.
   Data and every derived view share storage; no operation allocates or retains
   an owner. Owner relocation/destruction invalidates all views. Constness is
   shallow; instantiate T=const U for read-only elements.

   D>0, extents are nonnegative, lengths/stride products fit nidx_t/ptrdiff_t,
   bias+shape fits long long, and all coordinates/subranges are within the domain.
   The initial row-major span holds at least product(shape) elements. Empty views
   may be sliced/permuted/rebased but not indexed. No arbitrary overlapping layout
   constructor or automatic hash inverse: the affine map stays algebraic. */
template <class T, size_t D>
struct nmdview {
    static_assert(D > 0);
    using key_type = array<long long, D>;
    T* data;
    array<nidx_t, D> shape;
    key_type bias;
    array<ptrdiff_t, D> stride;

    template <size_t N>
    nmdview(span<T, N> storage, array<nidx_t, D> extents, key_type origin = {})
        : data(storage.data()), shape(extents), bias(origin) {
        ptrdiff_t step = 1;
        for (size_t axis = D; axis--;) {
            stride[axis] = step;
            step *= shape[axis];
        }
        assert(step >= 0 && size_t(step) <= storage.size());
    }

    nidx_t size() const {
        nidx_t n = 1;
        for (nidx_t extent : shape) n *= extent;
        return n;
    }

    // Logical row-major position, independent of physical strides. O(D).
    nidx_t position(const key_type& key) const {
        nidx_t flat = 0;
        for (size_t axis = 0; axis < D; ++axis)
            flat = flat * shape[axis] + nidx_t(key[axis] - bias[axis]);
        return flat;
    }

    key_type coordinate(nidx_t flat) const {
        key_type key;
        for (size_t axis = D; axis--;) {
            key[axis] = bias[axis] + flat % shape[axis];
            flat /= shape[axis];
        }
        return key;
    }

    T& operator()(const key_type& key) const {
        ptrdiff_t offset = 0;
        for (size_t axis = 0; axis < D; ++axis)
            offset += ptrdiff_t(key[axis] - bias[axis]) * stride[axis];
        return data[offset];
    }
    template <class... I>
    requires (sizeof...(I) == D && (convertible_to<I, long long> && ...))
    T& operator()(I... coordinates) const { return (*this)(key_type{static_cast<long long>(coordinates)...}); }

    T& operator[](nidx_t flat) const { return (*this)(coordinate(flat)); }

    // Coordinates retain their labels; lower/upper are semantic half-open bounds.
    nmdview sub(const key_type& lower, const key_type& upper) const {
        nmdview result = *this;
        ptrdiff_t offset = 0;
        for (size_t axis = 0; axis < D; ++axis) {
            assert(bias[axis] <= lower[axis] && lower[axis] <= upper[axis]);
            assert(upper[axis] - bias[axis] <= shape[axis]);
            offset += ptrdiff_t(lower[axis] - bias[axis]) * stride[axis];
            result.shape[axis] = nidx_t(upper[axis] - lower[axis]);
        }
        result.bias = lower;
        if (result.size()) result.data += offset;
        return result;
    }

    // Only labels change. Storage, shape and logical element order do not.
    nmdview rebase(key_type origin) const {
        nmdview result = *this;
        result.bias = origin;
        return result;
    }

    // axes[new_axis] = old_axis; axes is a permutation of [0,D).
    nmdview permute(const array<size_t, D>& axes) const {
        nmdview result = *this;
        for (size_t axis = 0; axis < D; ++axis) {
            result.shape[axis] = shape[axes[axis]];
            result.bias[axis] = bias[axes[axis]];
            result.stride[axis] = stride[axes[axis]];
        }
        return result;
    }

    // Reverse values along one axis, preserving its coordinate labels.
    nmdview reverse(size_t axis) const {
        nmdview result = *this;
        if (size()) result.data += (shape[axis] - 1) * stride[axis];
        result.stride[axis] = -stride[axis];
        return result;
    }
};

template <class T, size_t N, size_t D>
nmdview(span<T, N>, array<nidx_t, D>, array<long long, D> = {}) -> nmdview<T, D>;
