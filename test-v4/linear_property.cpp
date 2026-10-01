#include "../src-v4/linear.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; abort(); } } while (false)

using mint = nmodint<1000000007>;

vector<mint> apply_matrix(const nmatrix<mint>& matrix, const vector<mint>& input) {
    vector<mint> result(matrix.rows);
    for (nidx_t row = 0; row < matrix.rows; ++row)
        for (nidx_t column = 0; column < matrix.columns; ++column)
            result[row] += matrix(row, column) * input[column];
    return result;
}

mint brute_det(const nmatrix<mint>& matrix) {
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

uint64_t parity(uint64_t value) {
    return popcount(value) & 1;
}

int main() {
    mt19937 rng(0x1a2b3c4dU);
    for (nidx_t round = 0; round < 1000; ++round) {
        nidx_t rows = nidx_t(rng() % 7), columns = nidx_t(rng() % 7);
        nmatrix<mint> matrix(rows, columns);
        for (auto& value : matrix.data) value = nidx_t(rng() % 11) - 5;
        vector<mint> chosen(columns);
        for (auto& value : chosen) value = nidx_t(rng() % 11) - 5;
        auto right = apply_matrix(matrix, chosen);
        auto solution = nlinear_solve(matrix, right);
        CHECK(solution.consistent && apply_matrix(matrix, solution.particular) == right);
        for (const auto& direction : solution.basis)
            CHECK(apply_matrix(matrix, direction) == vector<mint>(rows));
        auto reduced = nrref(matrix);
        CHECK(nidx_t(solution.basis.size()) == columns - reduced.rank());
    }

    for (nidx_t round = 0; round < 450; ++round) {
        nidx_t n = nidx_t(rng() % 7);
        nmatrix<mint> matrix(n, n);
        for (auto& value : matrix.data) value = nidx_t(rng() % 9) - 4;
        CHECK(ndeterminant(matrix) == brute_det(matrix));
        auto inverse = ninverse(matrix);
        CHECK(bool(inverse) == (ndeterminant(matrix) != mint(0)));
        if (inverse) CHECK(nmatmul(matrix, *inverse).data == nmatrix<mint>::identity(n).data);
    }

    nmatrix<mint> fibonacci(2, 2);
    fibonacci(0, 0) = fibonacci(0, 1) = fibonacci(1, 0) = 1;
    auto power = nmatpow(fibonacci, 50);
    vector<mint> state{1, 0};
    for (nidx_t i = 0; i < 50; ++i) state = apply_matrix(fibonacci, state);
    CHECK(apply_matrix(power, vector<mint>{1, 0}) == state);
    __int128_t huge = 1;
    for (nidx_t i = 0; i < 36; ++i) huge *= 10;
    auto translation = nmatrix<mint>::identity(2);
    translation(0, 1) = 1;
    auto huge_power = nmatpow(translation, huge);
    CHECK(huge_power(0, 1) == mint(huge));

    for (nidx_t round = 0; round < 500; ++round) {
        nidx_t variables = nidx_t(rng() % 130), equations = nidx_t(rng() % 11);
        nidx_t words = variables / 64 + (variables % 64 != 0);
        nmatrix<uint64_t> coefficients(equations, words);
        vector<unsigned char> right(equations);
        for (auto& value : coefficients.data) value = uint64_t(rng()) * rng();
        for (auto& value : right) value = rng() & 1;
        auto solution = ngf2_solve(coefficients, variables, right);
        auto check_vector = [&](const vector<uint64_t>& x, const vector<unsigned char>& target) {
            for (nidx_t row = 0; row < equations; ++row) {
                uint64_t value = 0;
                for (nidx_t word = 0; word < words; ++word)
                    value ^= coefficients(row, word) & x[word];
                CHECK(parity(value) == target[row]);
            }
        };
        if (solution.consistent) {
            check_vector(solution.particular, right);
            for (nidx_t row = 0; row < solution.basis.rows; ++row) {
                vector<unsigned char> zero(right.size());
                check_vector(vector<uint64_t>(solution.basis[row].begin(), solution.basis[row].end()), zero);
            }
        }
    }

    nmatrix<uint64_t> equations(2, 1);
    equations(0, 0) = 0b011;
    equations(1, 0) = 0b110;
    auto solved = ngf2_solve(equations, 3, vector<unsigned char>{1, 0});
    CHECK(solved.consistent && solved.rank == 2 && solved.basis.rows == 1);
    CHECK(solved.particular[0] == 1 && solved.basis(0, 0) == 7);

    cout << "v4 linear: matrices, fields, packed GF(2) elimination and solves passed\n";
}
