#include "common.hpp"

int main() {
    using field = nmodint<101>;
    nmatrix<field> coefficients(0, INT_MAX);
    return nlinear_solve(coefficients, nvector<field>{})->basis.len();
}
