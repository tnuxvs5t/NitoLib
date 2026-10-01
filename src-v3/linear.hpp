#pragma once
#include "math.hpp"

template <class T>
struct nmatrix {
    nidx_t rows = 0, columns = 0;
    vector<T> data;

    nmatrix() = default;
    explicit nmatrix(nidx_t row_count, nidx_t column_count, const T& value = T{})
        : rows(row_count), columns(column_count), data(size_t(rows) * columns, value) {}

    nidx_t len() const { return rows; }
    T& operator()(nidx_t row, nidx_t column) { return data[size_t(row) * columns + column]; }
    const T& operator()(nidx_t row, nidx_t column) const { return data[size_t(row) * columns + column]; }
    auto operator[](nidx_t row) { return span(data).subspan(size_t(row) * columns, columns); }
    auto operator[](nidx_t row) const { return span(data).subspan(size_t(row) * columns, columns); }

    static nmatrix identity(nidx_t size) {
        nmatrix result(size, size);
        for (nidx_t i = 0; i < size; ++i) result(i, i) = T(1);
        return result;
    }
};

/* Dimensions agree; value operations support zero, multiplication and +=. */
template <class A, class B>
auto nmatmul(const A& left, const B& right) {
    using T = remove_cvref_t<decltype(left(0, 0) * right(0, 0))>;
    nmatrix<T> result(left.rows, right.columns);
    for (nidx_t row = 0; row < left.rows; ++row)
        for (nidx_t middle = 0; middle < left.columns; ++middle)
            for (nidx_t column = 0; column < right.columns; ++column)
                result(row, column) += left(row, middle) * right(middle, column);
    return result;
}

/* Square matrix; exponent is nonnegative and supports bit testing and right shift. */
template <class T, class E>
nmatrix<T> nmatpow(nmatrix<T> base, E exponent) {
    auto one = nmatrix<T>::identity(base.rows);
    return npow(move(base), exponent, move(one),
                [](const auto& a, const auto& b) { return nmatmul(a, b); });
}

template <class T>
struct nrref_result {
    nmatrix<T> matrix;
    vector<nidx_t> pivot;
    nidx_t rank() const { return nidx_t(pivot.size()); }
};

/*
Gauss-Jordan over a field.  Zero comparison is exact; floating tolerances belong in T
or in a separate numerical routine.  Only columns before pivot_columns may be pivots,
but row operations cover the whole matrix (useful for augmented systems).
*/
template <class T>
nrref_result<T> nrref(nmatrix<T> matrix, nidx_t pivot_columns = -1) {
    if (pivot_columns < 0) pivot_columns = matrix.columns;
    vector<nidx_t> pivot;
    nidx_t next_row = 0;
    for (nidx_t column = 0; column < pivot_columns && next_row < matrix.rows; ++column) {
        nidx_t chosen = next_row;
        while (chosen < matrix.rows && matrix(chosen, column) == T{}) ++chosen;
        if (chosen == matrix.rows) continue;
        for (nidx_t c = 0; c < matrix.columns; ++c) swap(matrix(next_row, c), matrix(chosen, c));
        T scale = T(1) / matrix(next_row, column);
        for (nidx_t c = 0; c < matrix.columns; ++c) matrix(next_row, c) *= scale;
        for (nidx_t row = 0; row < matrix.rows; ++row) if (row != next_row) {
            T factor = matrix(row, column);
            if (factor == T{}) continue;
            for (nidx_t c = 0; c < matrix.columns; ++c)
                matrix(row, c) -= factor * matrix(next_row, c);
        }
        pivot.push_back(column);
        ++next_row;
    }
    return {move(matrix), move(pivot)};
}

/* Square matrix over a field; empty determinant is one. */
template <class T>
T ndeterminant(nmatrix<T> matrix) {
    T result = T(1);
    for (nidx_t column = 0; column < matrix.rows; ++column) {
        nidx_t chosen = column;
        while (chosen < matrix.rows && matrix(chosen, column) == T{}) ++chosen;
        if (chosen == matrix.rows) return T{};
        if (chosen != column) {
            for (nidx_t c = column; c < matrix.columns; ++c)
                swap(matrix(column, c), matrix(chosen, c));
            result = -result;
        }
        T pivot = matrix(column, column);
        result *= pivot;
        T inverse = T(1) / pivot;
        for (nidx_t row = column + 1; row < matrix.rows; ++row) {
            T factor = matrix(row, column) * inverse;
            for (nidx_t c = column + 1; c < matrix.columns; ++c)
                matrix(row, c) -= factor * matrix(column, c);
        }
    }
    return result;
}

template <class T>
optional<nmatrix<T>> ninverse(nmatrix<T> matrix) {
    nidx_t n = matrix.rows;
    nmatrix<T> augmented(n, 2 * n);
    for (nidx_t row = 0; row < n; ++row)
        for (nidx_t column = 0; column < n; ++column) {
            augmented(row, column) = matrix(row, column);
            augmented(row, n + column) = row == column ? T(1) : T{};
        }
    auto reduced = nrref(move(augmented), n);
    if (reduced.rank() != n) return nullopt;
    nmatrix<T> result(n, n);
    for (nidx_t row = 0; row < n; ++row)
        for (nidx_t column = 0; column < n; ++column)
            result(row, column) = reduced.matrix(row, n + column);
    return result;
}

template <class T>
struct nlinear_solution {
    bool consistent;
    vector<T> particular;
    vector<vector<T>> basis;
};

/* Returns one solution and a nullspace basis for coefficients*x=right. */
template <class T, class V>
nlinear_solution<T> nlinear_solve(nmatrix<T> coefficients, V&& right) {
    nidx_t equations = coefficients.rows, variables = coefficients.columns;
    nmatrix<T> augmented(equations, variables + 1);
    for (nidx_t row = 0; row < equations; ++row) {
        for (nidx_t column = 0; column < variables; ++column)
            augmented(row, column) = coefficients(row, column);
        augmented(row, variables) = right[row];
    }
    auto reduced = nrref(move(augmented), variables);
    for (nidx_t row = reduced.rank(); row < equations; ++row)
        if (reduced.matrix(row, variables) != T{}) return {false, {}, {}};
    vector<T> particular(variables);
    vector<unsigned char> is_pivot(variables);
    for (nidx_t row = 0; row < reduced.rank(); ++row) {
        nidx_t column = reduced.pivot[row];
        is_pivot[column] = true;
        particular[column] = reduced.matrix(row, variables);
    }
    vector<vector<T>> basis;
    for (nidx_t free = 0; free < variables; ++free) if (!is_pivot[free]) {
        vector<T> direction(variables);
        direction[free] = T(1);
        for (nidx_t row = 0; row < reduced.rank(); ++row)
            direction[reduced.pivot[row]] = -reduced.matrix(row, free);
        basis.push_back(move(direction));
    }
    return {true, move(particular), move(basis)};
}

/* GF(2) Gauss-Jordan on packed rows. Logical columns are [0,columns), with bit j
   stored in word j/64, bit j%64 (least significant bit first). matrix.columns is
   the WORD count ceil(columns/64), not the logical column count. Padding bits are
   ignored and cleared. Dimensions are nonnegative; -1 means all logical columns
   may pivot, otherwise 0 <= pivot_columns <= columns. Input is copied unless moved.
   Returns the same packed layout and logical pivot indices. Earlier columns in a
   pivot row are zero, so XOR can start at the word containing the current pivot.
   With m rows, n logical columns and rank r: O(m*n + m*r*ceil(n/64)) word operations,
   O(m*ceil(n/64)+r) storage including the returned working matrix. */
inline nrref_result<uint64_t> ngf2_rref(nmatrix<uint64_t> matrix, nidx_t columns,
                                       nidx_t pivot_columns = -1) {
    if (pivot_columns < 0) pivot_columns = columns;
    if (columns % 64)
        for (nidx_t row = 0; row < matrix.rows; ++row)
            matrix(row, matrix.columns - 1) &= (uint64_t(1) << (columns % 64)) - 1;
    vector<nidx_t> pivot;
    nidx_t next_row = 0;
    for (nidx_t column = 0; column < pivot_columns && next_row < matrix.rows; ++column) {
        nidx_t word = column / 64;
        uint64_t mask = uint64_t(1) << (column % 64);
        nidx_t chosen = next_row;
        while (chosen < matrix.rows && !(matrix(chosen, word) & mask)) ++chosen;
        if (chosen == matrix.rows) continue;
        for (nidx_t w = 0; w < matrix.columns; ++w) swap(matrix(next_row, w), matrix(chosen, w));
        for (nidx_t row = 0; row < matrix.rows; ++row)
            if (row != next_row && (matrix(row, word) & mask))
                for (nidx_t w = word; w < matrix.columns; ++w)
                    matrix(row, w) ^= matrix(next_row, w);
        pivot.push_back(column);
        ++next_row;
    }
    return {move(matrix), move(pivot)};
}

struct ngf2_solution {
    bool consistent;
    nidx_t rank;
    vector<uint64_t> particular;
    nmatrix<uint64_t> basis;
};

/* coefficients is m x ceil(variables/64) words, RHS has m boolean (0/1) entries.
   variables >= 0 and variables+1 fits nidx_t. Input padding is ignored; inputs are
   not modified. On success, particular is ceil(variables/64) words and each row of
   basis is a packed nullspace direction. All padding is zero. Solutions are the
   particular XOR any subset of directions; there are 2^(variables-rank) of them.
   On inconsistency, rank is the coefficient rank and particular/basis are empty.
   Uses ngf2_rref with one non-pivot RHS column, then O(variables*rank) bit extraction
   plus O((variables-rank)*ceil(variables/64)) output initialization. Zero variables
   and zero equations are supported without inventing a free-variable sentinel. */
template <class V>
ngf2_solution ngf2_solve(const nmatrix<uint64_t>& coefficients, nidx_t variables, const V& right) {
    nidx_t words = variables / 64 + (variables % 64 != 0);
    nmatrix<uint64_t> augmented(coefficients.rows, variables / 64 + 1);
    for (nidx_t row = 0; row < coefficients.rows; ++row) {
        for (nidx_t w = 0; w < words; ++w) augmented(row, w) = coefficients(row, w);
        if (variables % 64) augmented(row, words - 1) &= (uint64_t(1) << (variables % 64)) - 1;
        if (right[row]) augmented(row, variables / 64) |= uint64_t(1) << (variables % 64);
    }
    auto reduced = ngf2_rref(move(augmented), variables + 1, variables);
    auto bit = [&](nidx_t row, nidx_t column) {
        return (reduced.matrix(row, column / 64) >> (column % 64)) & uint64_t(1);
    };
    nidx_t rank = reduced.rank();
    for (nidx_t row = rank; row < coefficients.rows; ++row)
        if (bit(row, variables)) return {false, rank, {}, {}};
    vector<uint64_t> particular(words);
    vector<unsigned char> is_pivot(variables);
    for (nidx_t row = 0; row < rank; ++row) {
        nidx_t column = reduced.pivot[row];
        is_pivot[column] = true;
        particular[column / 64] |= bit(row, variables) << (column % 64);
    }
    nmatrix<uint64_t> basis(variables - rank, words);
    nidx_t direction = 0;
    for (nidx_t free = 0; free < variables; ++free) if (!is_pivot[free]) {
        basis(direction, free / 64) |= uint64_t(1) << (free % 64);
        for (nidx_t row = 0; row < rank; ++row) {
            nidx_t column = reduced.pivot[row];
            basis(direction, column / 64) |= bit(row, free) << (column % 64);
        }
        ++direction;
    }
    return {true, rank, move(particular), move(basis)};
}
