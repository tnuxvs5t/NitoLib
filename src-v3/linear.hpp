#pragma once
#include "view.hpp"

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
    auto operator[](nidx_t row) { return nsub(nall(data), row * columns, (row + 1) * columns); }
    auto operator[](nidx_t row) const { return nsub(nall(data), row * columns, (row + 1) * columns); }

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
    nmatrix<T> result = nmatrix<T>::identity(base.rows);
    while (exponent) {
        if (exponent & 1) result = nmatmul(result, base);
        exponent >>= 1;
        if (exponent) base = nmatmul(base, base);
    }
    return result;
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
nlinear_solution<T> nlinear_solve(nmatrix<T> coefficients, V right) {
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
