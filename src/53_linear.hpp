template <class T> class nmatrix {
    int rows_ = 0, columns_ = 0;
    nvector<T> storage_;

    static int volume(int rows, int columns) {
        npre(rows >= 0 && columns >= 0);
        npre(rows == 0 || columns <= INT_MAX / rows);
        return rows * columns;
    }

  public:
    using value_type = T;

    nmatrix() = default;
    nmatrix(int rows, int columns) : rows_(rows), columns_(columns), storage_(volume(rows, columns)) {}
    nmatrix(int rows, int columns, const T& value)
        : rows_(rows), columns_(columns), storage_(volume(rows, columns), value) {}
    nmatrix(initializer_list<initializer_list<T>> rows) {
        npre(rows.size() <= size_t(INT_MAX));
        rows_ = int(rows.size());
        npre(!rows_ || rows.begin()->size() <= size_t(INT_MAX));
        columns_ = rows_ ? int(rows.begin()->size()) : 0;
        storage_.reserve(volume(rows_, columns_));
        for (const auto& row : rows) {
            npre(row.size() == size_t(columns_));
            for (const T& value : row)
                storage_.push(value);
        }
    }

    int rows() const noexcept { return rows_; }
    int cols() const noexcept { return columns_; }
    int len() const noexcept { return storage_.len(); }
    bool empty() const noexcept { return storage_.empty(); }
    T* data() noexcept { return storage_.data(); }
    const T* data() const noexcept { return storage_.data(); }
    T* row_data(int row) {
        npre(0 <= row && row < rows_);
        return columns_ ? storage_.data() + ptrdiff_t(row) * columns_ : storage_.data();
    }
    const T* row_data(int row) const {
        npre(0 <= row && row < rows_);
        return columns_ ? storage_.data() + ptrdiff_t(row) * columns_ : storage_.data();
    }

    T& operator[](int i) { return storage_[i]; }
    const T& operator[](int i) const { return storage_[i]; }
    T& operator()(int row, int column) {
        npre(0 <= row && row < rows_ && 0 <= column && column < columns_);
        return storage_[row * columns_ + column];
    }
    const T& operator()(int row, int column) const {
        npre(0 <= row && row < rows_ && 0 <= column && column < columns_);
        return storage_[row * columns_ + column];
    }

    auto view() & { return nview(storage_.data(), rows_, columns_); }
    auto view() const& { return nview(storage_.data(), rows_, columns_); }
    auto view() && = delete;

    auto row(int index) & {
        return nrow(view(), index);
    }
    auto row(int index) const& { return nrow(view(), index); }
    auto row(int) && = delete;

    auto column(int index) & { return ncolumn(view(), index); }
    auto column(int index) const& { return ncolumn(view(), index); }
    auto column(int) && = delete;

    // offset > 0 selects a diagonal above the main one; offset < 0 selects one below it.
    auto diagonal(int offset = 0) & { return ndiagonal(view(), offset); }
    auto diagonal(int offset = 0) const& { return ndiagonal(view(), offset); }
    auto diagonal(int = 0) && = delete;

    friend bool operator==(const nmatrix&, const nmatrix&) = default;
};

template <class A>
concept nmatrix_like = requires(const A& matrix, int row, int column) {
    { matrix.rows() } -> same_as<int>;
    { matrix.cols() } -> same_as<int>;
    matrix(row, column);
};

template <class T, class Add = nadd<T>, class Mul = nmul<T>>
    requires nsemiring<Add, Mul, T>
nmatrix<T> nmatrix_identity(int n, Add add = {}, Mul multiply = {}) {
    npre(n >= 0);
    nmatrix<T> result(n, n, add.id());
    for (int i = 0; i < n; ++i)
        result(i, i) = multiply.id();
    return result;
}

template <nmatrix_like A, nmatrix_like B, class Add = nadd<remove_cvref_t<decltype(declval<const A&>()(0, 0))>>,
          class Mul = nmul<remove_cvref_t<decltype(declval<const A&>()(0, 0))>>>
    requires nsemiring<Add, Mul, remove_cvref_t<decltype(declval<const A&>()(0, 0))>> &&
             same_as<remove_cvref_t<decltype(declval<const A&>()(0, 0))>,
                     remove_cvref_t<decltype(declval<const B&>()(0, 0))>>
auto nmatmul(const A& a, const B& b, Add add = {}, Mul multiply = {}) {
    using T = remove_cvref_t<decltype(a(0, 0))>;
    npre(a.cols() == b.rows());
    nmatrix<T> result(a.rows(), b.cols(), add.id());
    for (int i = 0; i < a.rows(); ++i)
        for (int k = 0; k < a.cols(); ++k)
            for (int j = 0; j < b.cols(); ++j)
                result(i, j) = add(move(result(i, j)), multiply(a(i, k), b(k, j)));
    return result;
}

template <class T, class Add = nadd<T>, class Mul = nmul<T>>
    requires nsemiring<Add, Mul, T>
nmatrix<T> nmatpow(nmatrix<T> base, uint64_t exponent, Add add = {}, Mul multiply = {}) {
    npre(base.rows() == base.cols());
    auto result = nmatrix_identity<T>(base.rows(), add, multiply);
    while (exponent) {
        if (exponent & 1)
            result = nmatmul(result, base, add, multiply);
        exponent >>= 1;
        if (exponent)
            base = nmatmul(base, base, add, multiply);
    }
    return result;
}

template <class T, class Add = nadd<T>, class Mul = nmul<T>>
    requires nmonoid<Add, T> && nmonoid<Mul, T>
class nmat : public nmatrix<T> {
    using base = nmatrix<T>;

  public:
    using base::base;
    nmat() = default;
    explicit nmat(base matrix) : base(move(matrix)) {}

    static nmat eye(int size) {
        npre(size >= 0);
        Add add;
        Mul multiply;
        nmat result(size, size, add.id());
        for (int index = 0; index < size; ++index)
            result(index, index) = multiply.id();
        return result;
    }
    T get(int row, int column, T fallback = Add{}.id()) const {
        return 0 <= row && row < this->rows() && 0 <= column && column < this->cols()
                   ? (*this)(row, column)
                   : move(fallback);
    }
    nmat& operator+=(const nmat& other) {
        npre(this->rows() == other.rows() && this->cols() == other.cols());
        Add add;
        for (int index = 0; index < this->len(); ++index)
            (*this)[index] = add(move((*this)[index]), other[index]);
        return *this;
    }
    friend nmat operator+(nmat left, const nmat& right) { return left += right; }
    friend nmat operator*(const nmat& left, const nmat& right) {
        npre(left.cols() == right.rows());
        Add add;
        Mul multiply;
        nmat result(left.rows(), right.cols(), add.id());
        for (int row = 0; row < left.rows(); ++row)
            for (int middle = 0; middle < left.cols(); ++middle) {
                const T& value = left(row, middle);
                for (int column = 0; column < right.cols(); ++column)
                    result(row, column) =
                        add(move(result(row, column)), multiply(value, right(middle, column)));
            }
        return result;
    }
    nmat& operator*=(const nmat& other) { return *this = *this * other; }
    nmat pow(long long exponent) const {
        npre(this->rows() == this->cols() && exponent >= 0);
        nmat base = *this, result = eye(this->rows());
        uint64_t remaining = uint64_t(exponent);
        while (remaining) {
            if (remaining & 1)
                result *= base;
            remaining >>= 1;
            if (remaining)
                base *= base;
        }
        return result;
    }
    nmat trans() const {
        nmat result(this->cols(), this->rows(), Add{}.id());
        for (int row = 0; row < this->rows(); ++row)
            for (int column = 0; column < this->cols(); ++column)
                result(column, row) = (*this)(row, column);
        return result;
    }
    friend bool operator==(const nmat& left, const nmat& right) {
        return static_cast<const base&>(left) == static_cast<const base&>(right);
    }
};

template <nexact_field_element T> int nrref(nmatrix<T>& matrix, nvector<int>* pivot_columns = nullptr) {
    if (pivot_columns)
        pivot_columns->clear();
    int row = 0;
    for (int column = 0; column < matrix.cols() && row < matrix.rows(); ++column) {
        int pivot = row;
        while (pivot < matrix.rows() && matrix(pivot, column) == T{})
            ++pivot;
        if (pivot == matrix.rows())
            continue;
        if (pivot != row)
            for (int j = 0; j < matrix.cols(); ++j)
                swap(matrix(row, j), matrix(pivot, j));
        T inverse = T{1} / matrix(row, column);
        for (int j = column; j < matrix.cols(); ++j)
            matrix(row, j) *= inverse;
        for (int i = 0; i < matrix.rows(); ++i)
            if (i != row && matrix(i, column) != T{}) {
                T factor = matrix(i, column);
                for (int j = column; j < matrix.cols(); ++j)
                    matrix(i, j) -= factor * matrix(row, j);
            }
        if (pivot_columns)
            pivot_columns->push(column);
        ++row;
    }
    return row;
}

template <nexact_field_element T> T ndeterminant(nmatrix<T> matrix) {
    npre(matrix.rows() == matrix.cols());
    T determinant{1};
    for (int column = 0; column < matrix.cols(); ++column) {
        int pivot = column;
        while (pivot < matrix.rows() && matrix(pivot, column) == T{})
            ++pivot;
        if (pivot == matrix.rows())
            return T{};
        if (pivot != column) {
            for (int j = column; j < matrix.cols(); ++j)
                swap(matrix(column, j), matrix(pivot, j));
            determinant = -determinant;
        }
        T value = matrix(column, column);
        determinant *= value;
        for (int row = column + 1; row < matrix.rows(); ++row) {
            T factor = matrix(row, column) / value;
            for (int j = column; j < matrix.cols(); ++j)
                matrix(row, j) -= factor * matrix(column, j);
        }
    }
    return determinant;
}

template <nexact_field_element T> T ndet(nmatrix<T> matrix) {
    return ndeterminant(move(matrix));
}

template <nexact_field_element T> nmaybe<nmatrix<T>> ninverse(nmatrix<T> matrix) {
    npre(matrix.rows() == matrix.cols());
    int size = matrix.rows();
    nmatrix<T> inverse(size, size, T{});
    for (int index = 0; index < size; ++index)
        inverse(index, index) = T{1};
    for (int column = 0; column < size; ++column) {
        int pivot = column;
        while (pivot < size && matrix(pivot, column) == T{})
            ++pivot;
        if (pivot == size)
            return {};
        if (pivot != column)
            for (int index = 0; index < size; ++index) {
                swap(matrix(column, index), matrix(pivot, index));
                swap(inverse(column, index), inverse(pivot, index));
            }
        T scale = T{1} / matrix(column, column);
        for (int index = 0; index < size; ++index) {
            matrix(column, index) *= scale;
            inverse(column, index) *= scale;
        }
        for (int row = 0; row < size; ++row)
            if (row != column && matrix(row, column) != T{}) {
                T factor = matrix(row, column);
                for (int index = 0; index < size; ++index) {
                    matrix(row, index) -= factor * matrix(column, index);
                    inverse(row, index) -= factor * inverse(column, index);
                }
            }
    }
    return inverse;
}

template <nexact_field_element T>
nmatrix<T> ninverse(nmatrix<T> matrix, nmatrix<T> fallback) {
    auto result = ninverse(move(matrix));
    return result ? move(result.val()) : move(fallback);
}

template <nexact_field_element T, class Add, class Mul>
T ndet(nmat<T, Add, Mul> matrix) {
    nmatrix<T> storage = move(matrix);
    return ndeterminant(move(storage));
}

template <nexact_field_element T, class Add, class Mul>
nmaybe<nmat<T, Add, Mul>> ninverse(nmat<T, Add, Mul> matrix) {
    nmatrix<T> storage = move(matrix);
    auto result = ninverse(move(storage));
    if (!result)
        return {};
    return nmat<T, Add, Mul>(move(result.val()));
}

template <nexact_field_element T, class Add, class Mul>
nmat<T, Add, Mul> ninverse(nmat<T, Add, Mul> matrix, nmat<T, Add, Mul> fallback) {
    auto result = ninverse(move(matrix));
    return result ? move(result.val()) : move(fallback);
}

template <class T> struct nlinear_solution {
    bool consistent = true;
    int rank = 0;
    nvector<T> particular;
    nvector<T> one;
    nvector<nvector<T>> basis;
};

template <nexact_field_element T, nindexed B>
nmaybe<nlinear_solution<T>> nlinear_solve(nmatrix<T> coefficients, const B& values) {
    npre(coefficients.rows() == nlen(values));
    int equations = coefficients.rows(), variables = coefficients.cols();
    npre(variables < INT_MAX);
    nmatrix<T> augmented(equations, variables + 1);
    for (int i = 0; i < equations; ++i) {
        for (int j = 0; j < variables; ++j)
            augmented(i, j) = coefficients(i, j);
        augmented(i, variables) = values[i];
    }
    nvector<int> pivots;
    nrref(augmented, &pivots);
    for (int row = 0; row < equations; ++row) {
        bool zero = true;
        for (int column = 0; column < variables; ++column)
            zero &= augmented(row, column) == T{};
        if (zero && augmented(row, variables) != T{})
            return {};
    }

    nlinear_solution<T> result;
    result.particular = nvector<T>(variables);
    nvector<int> pivot_row(variables, npos);
    for (int row = 0; row < pivots.len() && pivots[row] < variables; ++row) {
        pivot_row[pivots[row]] = row;
        result.particular[pivots[row]] = augmented(row, variables);
        ++result.rank;
    }
    for (int free = 0; free < variables; ++free)
        if (pivot_row[free] == npos) {
            nvector<T> direction(variables);
            direction[free] = T{1};
            for (int pivot = 0; pivot < variables; ++pivot)
                if (pivot_row[pivot] != npos)
                    direction[pivot] = -augmented(pivot_row[pivot], free);
            result.basis.push(move(direction));
        }
    result.one = result.particular;
    return result;
}

template <nexact_field_element T, nindexed B>
nlinear_solution<T> ngauss(nmatrix<T> coefficients, const B& values) {
    npre(coefficients.rows() == nlen(values));
    int equations = coefficients.rows(), variables = coefficients.cols(), row = 0;
    nvector<T> right = ncollect<T>(values);
    nvector<int> pivot_row(variables, npos);
    for (int column = 0; column < variables && row < equations; ++column) {
        int pivot = row;
        while (pivot < equations && coefficients(pivot, column) == T{})
            ++pivot;
        if (pivot == equations)
            continue;
        if (pivot != row) {
            for (int index = 0; index < variables; ++index)
                swap(coefficients(row, index), coefficients(pivot, index));
            swap(right[row], right[pivot]);
        }
        T scale = T{1} / coefficients(row, column);
        for (int index = column; index < variables; ++index)
            coefficients(row, index) *= scale;
        right[row] *= scale;
        for (int other = 0; other < equations; ++other)
            if (other != row && coefficients(other, column) != T{}) {
                T factor = coefficients(other, column);
                for (int index = column; index < variables; ++index)
                    coefficients(other, index) -= factor * coefficients(row, index);
                right[other] -= factor * right[row];
            }
        pivot_row[column] = row++;
    }

    nlinear_solution<T> result;
    result.rank = row;
    for (int equation = 0; equation < equations; ++equation) {
        bool zero = true;
        for (int column = 0; column < variables; ++column)
            zero &= coefficients(equation, column) == T{};
        if (zero && right[equation] != T{}) {
            result.consistent = false;
            return result;
        }
    }
    result.particular = nvector<T>(variables);
    for (int column = 0; column < variables; ++column)
        if (pivot_row[column] != npos)
            result.particular[column] = right[pivot_row[column]];
    for (int free = 0; free < variables; ++free)
        if (pivot_row[free] == npos) {
            nvector<T> direction(variables);
            direction[free] = T{1};
            for (int pivot = 0; pivot < variables; ++pivot)
                if (pivot_row[pivot] != npos)
                    direction[pivot] = -coefficients(pivot_row[pivot], free);
            result.basis.push(move(direction));
        }
    result.one = result.particular;
    return result;
}

template <nexact_field_element T, class Add, class Mul, nindexed B>
nlinear_solution<T> ngauss(nmat<T, Add, Mul> coefficients, const B& values) {
    nmatrix<T> storage = move(coefficients);
    return ngauss(move(storage), values);
}
