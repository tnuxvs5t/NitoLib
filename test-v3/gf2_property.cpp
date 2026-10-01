#include "../src-v3/linear.hpp"
#include "../src-v3/view.hpp"

#define CHECK(x) do { if (!(x)) { cerr << __LINE__ << ": " #x "\n"; abort(); } } while (false)

using grid = vector<vector<unsigned char>>;

nmatrix<uint64_t> pack(const grid& values, nidx_t columns, bool dirty_padding = false) {
    nidx_t words = columns / 64 + (columns % 64 != 0);
    nmatrix<uint64_t> result(nlen(values), words);
    for (nidx_t i = 0; i < nlen(values); ++i) {
        for (nidx_t j = 0; j < columns; ++j)
            if (values[i][j]) result(i, j / 64) |= uint64_t(1) << (j % 64);
        if (dirty_padding && columns % 64)
            result(i, words - 1) |= UINT64_MAX << (columns % 64);
    }
    return result;
}

// Independent byte-by-byte Gaussian elimination, not packed row operations.
pair<grid, vector<nidx_t>> scalar_rref(grid a, nidx_t columns, nidx_t allowed) {
    vector<nidx_t> pivot;
    nidx_t row = 0;
    for (nidx_t c = 0; c < allowed && row < nlen(a); ++c) {
        nidx_t next = row;
        while (next < nlen(a) && !a[next][c]) ++next;
        if (next == nlen(a)) continue;
        swap(a[row], a[next]);
        for (nidx_t i = 0; i < nlen(a); ++i) if (i != row && a[i][c])
            for (nidx_t j = 0; j < columns; ++j) a[i][j] = (a[i][j] + a[row][j]) % 2;
        pivot.push_back(c);
        ++row;
    }
    return {a, pivot};
}

template <class V>
bool solves(const grid& a, nidx_t columns, const vector<unsigned char>& b, const V& x) {
    for (nidx_t i = 0; i < nlen(a); ++i) {
        nidx_t sum = 0;
        for (nidx_t j = 0; j < columns; ++j) sum += a[i][j] * ((x[j / 64] >> (j % 64)) & 1);
        if (sum % 2 != b[i]) return false;
    }
    return true;
}

void verify(const grid& a, nidx_t columns, const vector<unsigned char>& right, bool exhaustive) {
    auto matrix = pack(a, columns, true);
    auto before = matrix.data;
    auto actual = ngf2_rref(matrix, columns);
    auto [expected, pivot] = scalar_rref(a, columns, columns);
    CHECK(actual.pivot == pivot);
    CHECK(actual.matrix.data == pack(expected, columns).data);
    nidx_t allowed = columns / 2;
    auto partial = ngf2_rref(matrix, columns, allowed);
    auto [partial_expected, partial_pivot] = scalar_rref(a, columns, allowed);
    CHECK(partial.pivot == partial_pivot);
    CHECK(partial.matrix.data == pack(partial_expected, columns).data);
    auto solution = ngf2_solve(matrix, columns, nall(right));
    CHECK(matrix.data == before);
    CHECK(solution.rank == nlen(pivot));
    auto augmented = a;
    for (nidx_t i = 0; i < nlen(a); ++i) augmented[i].push_back(right[i]);
    auto [augmented_expected, coefficient_pivot] = scalar_rref(augmented, columns + 1, columns);
    bool consistent = true;
    for (nidx_t i = nlen(coefficient_pivot); i < nlen(a); ++i)
        if (augmented_expected[i][columns]) consistent = false;
    CHECK(solution.consistent == consistent);
    if (!consistent) {
        CHECK(solution.particular.empty() && solution.basis.data.empty() && solution.basis.rows == 0);
    } else {
        CHECK(nlen(solution.particular) == matrix.columns);
        CHECK(solution.basis.rows == columns - solution.rank && solution.basis.columns == matrix.columns);
        CHECK(solves(a, columns, right, solution.particular));
        vector<unsigned char> zero(nlen(a));
        for (nidx_t i = 0; i < solution.basis.rows; ++i)
            CHECK(solves(a, columns, zero, solution.basis[i]));
        grid directions(solution.basis.rows, vector<unsigned char>(columns));
        for (nidx_t i = 0; i < solution.basis.rows; ++i)
            for (nidx_t j = 0; j < columns; ++j)
                directions[i][j] = (solution.basis(i, j / 64) >> (j % 64)) & 1;
        CHECK(nlen(scalar_rref(directions, columns, columns).second) == solution.basis.rows);
        if (columns % 64) {
            CHECK((solution.particular.back() >> (columns % 64)) == 0);
            for (nidx_t i = 0; i < solution.basis.rows; ++i)
                CHECK((solution.basis(i, matrix.columns - 1) >> (columns % 64)) == 0);
        }
    }
    if (exhaustive) {
        vector<uint64_t> expected_solutions, found;
        for (uint64_t mask = 0; mask < (uint64_t(1) << columns); ++mask)
            if (solves(a, columns, right, array{mask})) expected_solutions.push_back(mask);
        if (consistent) {
            uint64_t base = columns ? solution.particular[0] : 0;
            for (uint64_t mask = 0; mask < (uint64_t(1) << solution.basis.rows); ++mask) {
                uint64_t value = base;
                for (nidx_t j = 0; j < solution.basis.rows; ++j)
                    if ((mask >> j) & 1) value ^= solution.basis(j, 0);
                found.push_back(value);
            }
            sort(found.begin(), found.end());
        }
        CHECK(found == expected_solutions);
    }
}

int main() {
    nmatrix<uint64_t> single(1, 1);
    single(0, 0) = 1;
    auto boolean_rhs = ngf2_solve(single, 1, vector<bool>{true});
    CHECK(boolean_rhs.consistent && boolean_rhs.particular == vector<uint64_t>({1}));
    verify({}, 0, {}, true);
    verify({}, 8, {}, true);
    verify(grid(2), 0, {0, 0}, true);
    verify(grid(2), 0, {0, 1}, true);
    verify({{1, 1}, {1, 1}}, 2, {0, 1}, true);
    for (nidx_t pattern = 0; pattern < (1 << 12); ++pattern) {
        grid a(3, vector<unsigned char>(3));
        vector<unsigned char> b(3);
        for (nidx_t i = 0; i < 3; ++i) {
            for (nidx_t j = 0; j < 3; ++j) a[i][j] = (pattern >> (3 * i + j)) & 1;
            b[i] = (pattern >> (9 + i)) & 1;
        }
        verify(a, 3, b, true);
    }
    mt19937_64 rng(0x6f22026);
    for (nidx_t trial = 0; trial < 600; ++trial) {
        nidx_t rows = nidx_t(rng() % 10), columns = nidx_t(rng() % 9);
        grid a(rows, vector<unsigned char>(columns));
        vector<unsigned char> b(rows);
        for (auto& row : a) for (auto& value : row) value = rng() & 1;
        for (auto& value : b) value = rng() & 1;
        verify(a, columns, b, true);
    }
    for (nidx_t columns : {0, 1, 63, 64, 65, 127, 128, 129, 193}) {
        verify({}, columns, {}, false);
        grid identity(columns, vector<unsigned char>(columns));
        vector<unsigned char> b(columns);
        for (nidx_t i = 0; i < columns; ++i) identity[i][i] = 1, b[i] = i % 2;
        verify(identity, columns, b, false);
        for (nidx_t trial = 0; trial < 8; ++trial) {
            nidx_t rows = columns + trial - 3;
            rows = max(nidx_t(0), rows);
            grid a(rows, vector<unsigned char>(columns));
            b.resize(rows);
            for (auto& row : a) for (auto& value : row) value = rng() & 1;
            for (auto& value : b) value = rng() & 1;
            verify(a, columns, b, false);
        }
    }
}
