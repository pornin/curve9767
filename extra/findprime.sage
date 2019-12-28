#! /usr/bin/env sage

import sys
import hashlib
from sage.all import *

# For degree n (prime) and constant c (2 or more), find the largest prime
# p such that:
#   p >= 5
#   p >= min_p
#   2 <= c <= p-2
#   (1+c*(n-1))*p^2 + (2^16-1)*p < 2^32+2^16
#   z^n-c is irreducible over GF(p)[z]
# If there is no solution, then 0 is returned.
def find_best_prime_spec(n, c, min_p):
    if min_p == 0:
        min_p = 5
    if min_p < c + 2:
        min_p = c + 2
    lim = 2^32 + 2^16
    e = 1 + c*(n-1)
    max_p = int(sqrt((2^32 + 2^16) / e))
    p = max_p - (max_p % n) + 1
    if (p % 2) == 0:
        p = p - n
    while p >= min_p:
        if c >= p-1:
            return 0
        if (e*p^2 + (2^16 - 1)*p) < (2^32 + 2^16) and is_prime(p):
            K = Zmod(p)
            R = PolynomialRing(K, 'z')
            z = R.gen()
            if (z^n - K(c)).is_irreducible():
                return p
        p = p - 2*n
    return 0

# Given degree n, find and print the best constant c and prime p that
# fulfills the criteria (see find_best_prime_spec()) and maximize p^n.
def find_best_prime(n):
    c = 2
    best_c = 0
    best_p = 0
    best_q = 0
    while true:
        p = find_best_prime_spec(n, c, best_p)
        if p == 0:
            if best_p != 0:
                print "n=%d -> p=%d c=%d log2(q)=%.3f" % (n, best_p, best_c, (log(best_q)/log(2)).n(60))
            return
        q = p^n
        if q > best_q:
            best_c = c
            best_p = p
            best_q = q
        c = c + 1

# Try all prime degrees n, starting at 11. We know that there can be no
# solution for n >= 1291.
for n in range(11, 1291):
    if is_prime(n):
        find_best_prime(n)
