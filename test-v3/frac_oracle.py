#!/usr/bin/env python3
"""Unbounded Fraction differential oracle; standard Python/C++, no Boost dependency."""

from fractions import Fraction
from pathlib import Path
import os
import random
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
LOW, HIGH = -(1 << 63), (1 << 63) - 1
RNG = random.Random(0xF12AC2026)
cases = []
expected = []


def add(op, a, b, c=0, d=1):
    cases.append(f"{op} {a} {b} {c} {d}\n")
    try:
        left = Fraction(a, b)
        if op in "cu":
            value = left
        else:
            right = Fraction(c, d)
            if op == "<":
                expected.append(str((left > right) - (left < right)))
                return
            if op == "+":
                value = left + right
            elif op == "-":
                value = left - right
            elif op == "*":
                value = left * right
            else:
                value = left / right
        if LOW <= value.numerator <= HIGH and value.denominator <= HIGH:
            expected.append(f"{value.numerator} {value.denominator}")
        else:
            expected.append("overflow")
    except ZeroDivisionError:
        expected.append("domain")


edges = [LOW, LOW + 1, -2, -1, 0, 1, 2, HIGH - 1, HIGH]
for a in edges:
    for b in edges:
        add("c", a, b)
for a in [0, 1, HIGH, HIGH + 1, (1 << 64) - 1]:
    for b in [0, 1, HIGH, HIGH + 1, (1 << 64) - 1]:
        add("u", a, b)
for trial in range(5000):
    a, c = RNG.randint(LOW, HIGH), RNG.randint(LOW, HIGH)
    b, d = RNG.randint(1, HIGH), RNG.randint(1, HIGH)
    if trial % 4 == 0:
        a, c = RNG.choice(edges), RNG.choice(edges)
        b, d = RNG.choice([1, 2, HIGH - 1, HIGH]), RNG.choice([1, 2, HIGH - 1, HIGH])
    if trial % 4 == 1:
        d = b  # Addition cancellation / unreduced common denominators.
    if trial % 4 == 2:
        c, d = b, a  # Reciprocal products, but operands must be representable.
        right = Fraction(c, d) if d else Fraction(0)
        if not (LOW <= right.numerator <= HIGH and right.denominator <= HIGH):
            c, d = 0, 1
        else:
            c, d = right.numerator, right.denominator
    for op in "+-*/<":
        add(op, a, b, c, d)
    add("c", a, RNG.randint(LOW, HIGH))
    add("u", RNG.getrandbits(64), RNG.getrandbits(64))

DRIVER = r'''
#include "src-v3/frac.hpp"
int main() {
    char op;
    string sa, sb;
    long long c, d;
    while (cin >> op >> sa >> sb >> c >> d) {
        try {
            nfrac<> a = op == 'u' ? nfrac<>(stoull(sa), stoull(sb))
                                  : nfrac<>(stoll(sa), stoll(sb));
            if (op != 'c' && op != 'u') {
                nfrac<> b(c, d);
                if (op == '<') { cout << ((a > b) - (a < b)) << '\n'; continue; }
                if (op == '+') a += b;
                if (op == '-') a -= b;
                if (op == '*') a *= b;
                if (op == '/') a /= b;
            }
            cout << a.numerator() << ' ' << a.denominator() << '\n';
        } catch (const overflow_error&) { cout << "overflow\n"; }
          catch (const domain_error&) { cout << "domain\n"; }
    }
}
'''

with tempfile.TemporaryDirectory(prefix="nitori-frac-oracle-") as directory:
    source = Path(directory) / "driver.cpp"
    binary = Path(directory) / "driver"
    source.write_text(DRIVER)
    subprocess.run([os.environ.get("CXX", "g++"), "-std=c++23", "-O2",
                    "-Wall", "-Wextra", "-Wpedantic", "-Wshadow", "-Werror",
                    "-I", str(ROOT), str(source), "-o", str(binary)], check=True)
    actual = subprocess.run([str(binary)], input="".join(cases), text=True,
                            capture_output=True, check=True).stdout.splitlines()
    if len(actual) != len(expected):
        raise SystemExit(f"result count mismatch: {len(actual)} != {len(expected)}")
    for case, got, want in zip(cases, actual, expected):
        if got != want:
            raise SystemExit(f"{case.strip()}: got {got}, expected {want}")
print(f"nfrac unbounded Python Fraction oracle passed: {len(cases)} cases")
