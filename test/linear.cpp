#include "common.hpp"

int main() {
    nmatrix<int> matrix{{9, 2, 7, 4}, {6, 8, 3, 5}, {1, 0, 11, 10}};
    auto column = matrix.column(1);
    nsort(column);
    ntest(matrix(0, 1) == 0 && matrix(1, 1) == 2 && matrix(2, 1) == 8);
    auto diagonal = matrix.diagonal();
    nsort(diagonal);
    ntest(matrix(0, 0) == 2 && matrix(1, 1) == 9 && matrix(2, 2) == 11);
    auto upper = matrix.diagonal(1);
    nreverse_inplace(upper);
    ntest(matrix(0, 1) == 10 && matrix(1, 2) == 3 && matrix(2, 3) == 0);
    nsort(matrix.row(0));
    ntest((matrix.row(0)[0] == 2 && matrix.row(0)[3] == 10));

    nmatrix<int> empty_columns(3, 0);
    auto empty_row = empty_columns.row(2);
    static_assert(ncontiguous_indexed<decltype(empty_row)>);
    ntest(empty_row.empty() && empty_row.data() == empty_columns.data());

    using mint = nmodint<1000000007>;
    nmatrix<mint> fibonacci{{1, 1}, {1, 0}};
    auto power = nmatpow(fibonacci, 50);
    ntest(power(0, 1).val() == 586268941);

    using field = nmodint<101>;
    nmatrix<field> a{{2, 3, 1}, {4, 1, 5}, {7, 2, 6}};
    ntest(ndeterminant(a).val() == 26);
    nvector<field> wanted{11, 17, 29};
    auto solution = nlinear_solve(a, wanted);
    ntest(solution.ok() && solution->basis.empty());
    for (int row = 0; row < a.rows(); ++row) {
        field sum;
        for (int column_index = 0; column_index < a.cols(); ++column_index)
            sum += a(row, column_index) * solution->particular[column_index];
        ntest(sum == wanted[row]);
    }

    nmatrix<field> under{{1, 2, 3}, {2, 4, 6}};
    nvector<field> rhs{4, 8};
    auto family = nlinear_solve(under, rhs);
    ntest(family.ok() && family->basis.len() == 2 && family->rank == 1 &&
          family->one == family->particular);
    for (int vector = 0; vector < family->basis.len(); ++vector)
        for (int row = 0; row < under.rows(); ++row) {
            field sum;
            for (int col = 0; col < under.cols(); ++col)
                sum += under(row, col) * family->basis[vector][col];
            ntest(sum == field{});
        }
    ntest(!nlinear_solve(nmatrix<field>{{1}, {1}}, nvector<field>{3, 4}));

    mt19937 random(0x5311eaU);
    for (int repeat = 0; repeat < 300; ++repeat) {
        int rows = 1 + random() % 5, middle = 1 + random() % 5, cols = 1 + random() % 5;
        nmatrix<long long> left(rows, middle), right(middle, cols);
        for (int i = 0; i < left.len(); ++i)
            left[i] = int(random() % 13) - 6;
        for (int i = 0; i < right.len(); ++i)
            right[i] = int(random() % 13) - 6;
        auto product = nmatmul(left, right);
        for (int i = 0; i < rows; ++i)
            for (int j = 0; j < cols; ++j) {
                long long brute = 0;
                for (int k = 0; k < middle; ++k)
                    brute += left(i, k) * right(k, j);
                ntest(product(i, j) == brute);
            }
    }

    nmat<long long> legacy{{1, 2}, {3, 4}};
    ntest((legacy * nmat<long long>::eye(2)) == legacy);
    ntest(legacy.trans() == nmat<long long>({{1, 3}, {2, 4}}));
    ntest(legacy.get(9, 9, 77) == 77);

    nmat<field> invertible{{2, 3}, {5, 7}};
    ntest(ndet(invertible) == field{-1});
    auto inverse = ninverse(invertible);
    ntest(inverse && invertible * inverse.val() == nmat<field>::eye(2));
    ntest(!ninverse(nmat<field>{{1, 2}, {2, 4}}));

    auto legacy_solution = ngauss(nmat<field>{{1, 2, 3}, {2, 4, 6}}, rhs);
    ntest(legacy_solution.consistent && legacy_solution.rank == 1 &&
          legacy_solution.basis.len() == 2 && legacy_solution.one == legacy_solution.particular);
    auto impossible = ngauss(nmat<field>{{1}, {1}}, nvector<field>{3, 4});
    ntest(!impossible.consistent && impossible.rank == 1);
}
