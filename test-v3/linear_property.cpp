#include "../src-v3/linear.hpp"
#include "../src-v3/math.hpp"

using mint = nmodint<1000000007>;

void check(bool condition, const char* message) {
    if (!condition) throw runtime_error(message);
}

vector<mint> apply(const nmatrix<mint>& matrix, const vector<mint>& input) {
    vector<mint> result(matrix.rows);
    for (nidx_t row = 0; row < matrix.rows; ++row)
        for (nidx_t column = 0; column < matrix.columns; ++column)
            result[row] += matrix(row, column) * input[column];
    return result;
}

mint brute_determinant(const nmatrix<mint>& matrix) {
    nidx_t n = matrix.rows;
    vector<nidx_t> permutation(n);
    iota(permutation.begin(), permutation.end(), 0);
    mint answer = 0;
    do {
        nidx_t inversions = 0;
        mint product = 1;
        for (nidx_t i = 0; i < n; ++i) {
            product *= matrix(i, permutation[i]);
            for (nidx_t j = 0; j < i; ++j) inversions += permutation[j] > permutation[i];
        }
        answer += inversions & 1 ? -product : product;
    } while (next_permutation(permutation.begin(), permutation.end()));
    return answer;
}

int main() {
    mt19937 random(0x1a2b3c4dU);
    for (nidx_t trial = 0; trial < 1200; ++trial) {
        nidx_t rows = nidx_t(random() % 7), columns = nidx_t(random() % 7);
        nmatrix<mint> matrix(rows, columns);
        for (mint& value : matrix.data) value = nidx_t(random() % 11) - 5;
        vector<mint> chosen(columns);
        for (mint& value : chosen) value = nidx_t(random() % 11) - 5;
        auto right = apply(matrix, chosen);
        auto solution = nlinear_solve(matrix, nall(right));
        check(solution.consistent, "constructed system consistency");
        check(apply(matrix, solution.particular) == right, "particular solution");
        for (const auto& direction : solution.basis)
            check(apply(matrix, direction) == vector<mint>(rows), "nullspace basis");

        auto reduced = nrref(matrix);
        check(nidx_t(solution.basis.size()) == columns - reduced.rank(), "nullity");
        for (nidx_t row = 0; row < reduced.rank(); ++row) {
            nidx_t pivot = reduced.pivot[row];
            check(reduced.matrix(row, pivot) == mint(1), "unit pivot");
            for (nidx_t other = 0; other < rows; ++other)
                if (other != row) check(reduced.matrix(other, pivot) == mint(0), "clean pivot");
        }
    }

    for (nidx_t trial = 0; trial < 500; ++trial) {
        nidx_t n = nidx_t(random() % 7);
        nmatrix<mint> matrix(n, n);
        for (mint& value : matrix.data) value = nidx_t(random() % 9) - 4;
        check(ndeterminant(matrix) == brute_determinant(matrix), "determinant");
        auto inverse = ninverse(matrix);
        check(bool(inverse) == (ndeterminant(matrix) != mint(0)), "inverse presence");
        if (inverse) {
            auto product = nmatmul(matrix, *inverse);
            check(product.data == nmatrix<mint>::identity(n).data, "inverse product");
        }
    }

    nmatrix<mint> fibonacci(2, 2);
    fibonacci(0, 0) = fibonacci(0, 1) = fibonacci(1, 0) = 1;
    auto power = nmatpow(fibonacci, 50);
    vector<mint> state{1, 0};
    for (nidx_t i = 0; i < 50; ++i) state = apply(fibonacci, state);
    check(apply(power, vector<mint>{1, 0}) == state, "matrix power");

    __int128_t huge = 1;
    for (nidx_t i = 0; i < 36; ++i) huge *= 10;
    auto translation = nmatrix<mint>::identity(2);
    translation(0, 1) = 1;
    auto huge_power = nmatpow(translation, huge);
    check(huge_power(0, 0) == mint(1) && huge_power(1, 1) == mint(1),
          "wide matrix exponent diagonal");
    check(huge_power(0, 1) == mint(huge) && huge_power(1, 0) == mint(0),
          "wide matrix exponent translation");

    nmatrix<mint> impossible(1, 1);
    vector<mint> one{1};
    auto no = nlinear_solve(impossible, nall(one));
    check(!no.consistent, "inconsistent system");
}
