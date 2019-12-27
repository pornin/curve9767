#! /usr/bin/env sage

# This script generates test vectors for curve9767.
#
# The hash-to-curve test vectors use SHAKE256. Since Sage (before version 9)
# uses Python 2, and hashlib on Python 2 does not seem to implement SHAKE256
# (at least not with a variable output length), we also import pycryptodome.
# On Ubuntu 18.04, make sure to install the python-pycryptodome package.

import sys
import binascii
import hashlib
import struct
from sage.all import *
from Cryptodome.Hash import SHAKE256
from Cryptodome.Hash import SHA3_256

# Globals: field definition.
#   p = base field modulus
#   q = field cardinal
#   K = field (using z for denoting polynomials)
#   g = multiplicative generator of K*
p = 9767
q = p^19
K.<z> = GF(q, modulus = x^19-2)
g = z+2

# Curve parameters.
A = -3*z^0
B = 2048*z^9
E = EllipticCurve(K, [A, B])
n = 6389436622109970582043832278503799542449455630003248488928817956373993578097
G = E.point([0, 32*z^14])

# Encode a sequence of bytes (integers) into hexadecimal (lowercase).
def to_hex_string(bb):
    s = ""
    for v in bb:
        s += "%0.2x" % v
    return s

# Encode a big integer into bytes, using little-endian convention, and a
# target number of bytes.
def int_to_bin(k, num):
    s = ""
    for i in range(0, num):
        s += struct.pack("B", int(k % 256))
        k = k // 256
    return s

# Encode a big integer into hexadecimal, using little-endian convention, and
# a target number of bytes.
def int_to_hex(k, num):
    bb = []
    for i in range(0, num):
        bb.append(k % 256)
        k = k // 256
    return to_hex_string(bb)

# Encode a scalar into hexadecimal, using little-endian convention.
def scalar_to_hex(k):
    return int_to_hex(k % n, 32)

# Map a big integer to a field element.
def map_to_base(x):
    c = K(0)
    for i in range(0, 19):
        c += (x % p) * (z^i)
        x = x // p
    return c

# Encode a polynomial into a sequence of 32 bytes (returned as a list
# of integers).
def polynomial_encode(tt):
    bb = []
    for i in range(0, 6):
        x0 = int(tt[3 * i + 0])
        x1 = int(tt[3 * i + 1])
        x2 = int(tt[3 * i + 2])
        l0 = x0 % (2^11)
        l1 = x1 % (2^11)
        l2 = x2 % (2^11)
        h0 = x0 // (2^11)
        h1 = x1 // (2^11)
        h2 = x2 // (2^11)
        v = l0 + (2^11) * l1 + (2^22) * l2 + (2^33)*(h0 + 5*h1 + 25*h2)
        bb.append(v % (2^8))
        bb.append((v // (2^8)) % (2^8))
        bb.append((v // (2^16)) % (2^8))
        bb.append((v // (2^24)) % (2^8))
        bb.append((v // (2^32)) % (2^8))
    x3 = int(tt[18])
    bb.append(x3 % (2^8))
    bb.append(x3 // (2^8))
    return bb

# Encode a field element into a sequence of 32 bytes (returned as a list
# of integers).
def gf_encode(t):
    return polynomial_encode(t.polynomial())

# Test whether a field element is "negative".
def gf_is_neg(t):
    tt = t.polynomial()
    for i in range(0, 19):
        v = int(tt[18 - i])
        if v != 0:
            return (v >= ((p+1) // 2))
    return False

# Encode a curve point into a sequence of bytes (list of integers).
def curve9767_encode(Q):
    xy = Q.xy()
    bb = gf_encode(xy[0])
    if gf_is_neg(xy[1]):
        bb[31] += 64
    return bb

# Convert bytes (list of integers) into a binary string.
def to_binary_string(bb):
    r = ""
    for v in bb:
        r += struct.pack("B", v)
    return r

# Copy a polynomial into a sequence of integers.
def polynomial_to_sequence_of_int(tt):
    uu = []
    for i in range(0, 19):
        uu.append(int(tt[i]))
    return uu

# Apply Icart's map on field element u.
def Icart_map(u):
    if u == 0:
        return 0*G
    v = (3*A - u^4) / (6*u)
    x = (v^2 - B - (u^6)/27)^((2*q-1)/3) + (u^2)/3
    y = u*x + v
    if y^2 != x^3+A*x+B:
        raise Exception("Icart's map failed!")
    return E.point([x, y])

# Compute the hash-to-curve process on a given input string.
def hash_to_curve(m):
    shake = SHAKE256.new()
    shake.update(m)
    seed1 = shake.read(int(48))
    x = 0
    for i in range(0, 48):
        x += ord(seed1[i])*(2^(8*i))
    Q1 = Icart_map(map_to_base(x))
    seed2 = shake.read(int(48))
    x = 0
    for i in range(0, 48):
        x += ord(seed2[i])*(2^(8*i))
    Q2 = Icart_map(map_to_base(x))
    return Q1 + Q2

# Generate a key pair from a seed (secret scalar s, additional secret t,
# and public key Q = s*G).
def keygen(seed):
    shake = SHAKE256.new()
    shake.update(b'curve9767-keygen:')
    shake.update(seed)
    v = shake.read(int(64))
    s = 0
    for i in range(0, 64):
        s += ord(v[i])*(2^(8*i))
    s = s % n
    t = shake.read(int(32))
    Q = s*G
    return (s, t, Q)

# Test vectors for scalars.
def make_test_vectors_scalars():
    print "# ============================================================"
    print "# Scalar tests."
    print "#"
    print "# Each sequence of eight lines consists in a raw (large) integer b1,"
    print "# the integer a1 = b1 mod n (where n is the curve order), a raw"
    print "# integer b2, the integer a2 = b2 mod n, then a1 + a2 mod n,"
    print "# a1 - a2 mod n, -a1 mod n, and a1*a2 mod n. All integers use"
    print "# unsigned little-endian encoding (over 96 bytes for the two raw"
    print "# integers, 32 bytes for the six other integers)."
    print ""

    for i in range(0, 20):
        shake = SHAKE256.new()
        shake.update("curve9767-test3:%0.3d" % i)
        b1 = 0
        for c in shake.read(int(96)):
            b1 = (b1 * 256) + ord(c)
        b2 = 0
        for c in shake.read(int(96)):
            b2 = (b2 * 256) + ord(c)
        a1 = b1 % n
        a2 = b2 % n
        print int_to_hex(b1, 96)
        print int_to_hex(a1, 32)
        print int_to_hex(b2, 96)
        print int_to_hex(a2, 32)
        print int_to_hex((a1 + a2) % n, 32)
        print int_to_hex((a1 - a2) % n, 32)
        print int_to_hex(-a1 % n, 32)
        print int_to_hex((a1 * a2) % n, 32)
        print ""

# Test vectors for decoding: generate various sequences of bytes that
# should be accepted or rejected.
def make_test_vectors_decode():
    print "# ============================================================"
    print "# Encode/decode test vectors."
    print "#"
    print "# Each non-comment/blank line is a byte sequence followed by an"
    print "# integer of value 0 or 1, depending on whether the byte sequence"
    print "# should be rejected or accepted by the decoder, respectively."
    print ""

    # Start with a base point: we multiply the conventional generator
    # by a big integer obtained from a hash; we could use random
    # generation, but we want deterministic generation of the test
    # vectors.
    k = 0
    for c in hashlib.sha256("curve9767-test1").digest():
        k = (k * 256) + ord(c)
    Q = k*G

    # Q and -Q should encode to valid strings.
    print "# Normal points"
    print to_hex_string(curve9767_encode(Q)), 1
    print to_hex_string(curve9767_encode(-Q)), 1
    print ""

    # With the top bit of the last byte set to 1, this should not be
    # a valid string.
    print "# Top bit of last byte"
    bb = curve9767_encode(Q)
    bb[31] += 128
    print to_hex_string(bb), 0
    print ""

    # For each index i: find a point whose X coordinate has a coefficient
    # of value 0 at index i; the encoding with a value 0 should work,
    # not the encoding with a value 9767. Also make a similar test with
    # a value 1 encoded as 9768.
    for i in range(0, 19):
        print "# Range check on coefficient", i
        Q2 = Q
        while True:
            tt = Q2.xy()[0].polynomial()
            if tt[i].is_zero():
                print to_hex_string(polynomial_encode(tt)), 1
                uu = polynomial_to_sequence_of_int(tt)
                uu[i] = int(p)
                print to_hex_string(polynomial_encode(uu)), 0
                break
            Q2 = Q2 + G
        while True:
            tt = Q2.xy()[0].polynomial()
            if tt[i] == 1:
                print to_hex_string(polynomial_encode(tt)), 1
                uu = polynomial_to_sequence_of_int(tt)
                uu[i] = int(p + 1)
                print to_hex_string(polynomial_encode(uu)), 0
                break
            Q2 = Q2 + G
        print ""

    # For each index i multiple of 3: find a point whose X coordinate has
    # three coefficients at indices i, i+1 and i+2 such that the seven
    # top bits of the 40-bit group should encode a value of 0, 1 or 2;
    # when these bits are set to 125, 126 or 127, the encoding should be
    # rejected.
    for j in range(0, 6):
        i = 3 * j
        print "# Base-5 encoding of triplet of coefficient at index", i
        Q2 = Q
        while True:
            tt = Q2.xy()[0].polynomial()
            x0 = int(tt[i+0]) // 2048
            x1 = int(tt[i+1]) // 2048
            x2 = int(tt[i+2]) // 2048
            v = x0 + 5*x1 + 25*x2
            if v <= 2:
                print to_hex_string(polynomial_encode(tt)), 1
                uu = polynomial_to_sequence_of_int(tt)
                uu[i+2] += 5*2048
                print to_hex_string(polynomial_encode(uu)), 0
                break
            Q2 = Q2 + G
        print ""

    # Encoding should be rejected if it decodes as X such that X^3+A*X+B
    # is not a square, but accepted otherwise.
    print "# Reject points not on the curve"
    for i in range(0, 40):
        k = 0
        for c in hashlib.sha256("curve9767-test1:%0.3d" % i).digest():
            k = (k * 256) + ord(c)
        X = g^k
        print to_hex_string(gf_encode(X)), int((X^3+A*X+B).is_square())
    print ""

# Basic test vectors for curve operations.
def make_test_vectors_curve_basic():
    print "# ============================================================"
    print "# Basic curve test vectors."
    print "#"
    print "# Each sequence of values encodes the following:"
    print "#     k1    integer modulo n (curve order), hex, little-endian"
    print "#     k2    integer modulo n (curve order), hex, little-endian"
    print "#     Q1 = k1*G"
    print "#     Q2 = k2*G"
    print "#     2*Q1"
    print "#     2*Q2"
    print "#     Q1 + Q2"
    print "#     Q1 - Q2"
    print ""
    for i in range(0, 20):
        k1 = 0
        for c in hashlib.sha256("curve9767-test2:%0.3d" % (2*i)).digest():
            k1 = (k1 * 256) + ord(c)
        k1 = k1 % n
        k2 = 0
        for c in hashlib.sha256("curve9767-test2:%0.3d" % (2*i+1)).digest():
            k2 = (k2 * 256) + ord(c)
        k2 = k2 % n
        Q1 = k1*G
        Q2 = k2*G
        print "# Test", i + 1
        print scalar_to_hex(k1)
        print scalar_to_hex(k2)
        print to_hex_string(curve9767_encode(Q1))
        print to_hex_string(curve9767_encode(Q2))
        print to_hex_string(curve9767_encode(2*Q1))
        print to_hex_string(curve9767_encode(2*Q2))
        print to_hex_string(curve9767_encode(Q1+Q2))
        print to_hex_string(curve9767_encode(Q1-Q2))
        print ""

# Test vectors for mapping seeds to field elements.
def make_test_vectors_map_to_base():
    print "# ============================================================"
    print "# Map to base tests."
    print "#"
    print "# Each sequence of two lines consists in a raw integer (48 bytes"
    print "# with little-endian encoding), followed by the encoding of the"
    print "# mapped field element (32 bytes)."
    print ""
    x = 0
    print int_to_hex(x, 48)
    print to_hex_string(gf_encode(map_to_base(x)))
    print ""
    x = 2^384-1
    print int_to_hex(x, 48)
    print to_hex_string(gf_encode(map_to_base(x)))
    print ""
    for i in range(0, 20):
        x = 0
        for c in hashlib.sha384("curve9767-test4:%0.3d" % (3*i)).digest():
            x = (x * 256) + ord(c)
        print int_to_hex(x, 48)
        print to_hex_string(gf_encode(map_to_base(x)))
        print ""

# Test vectors for Icart's map.
def make_test_vectors_Icart_map():
    print "# ============================================================"
    print "# Icart's map tests."
    print "#"
    print "# Each sequence of two lines consists in an encoded field element"
    print "# (32 bytes), followed by the curve point to which that element"
    print "# is mapped with Icart's map."
    print "# If the field element is zero, then the point-at-infinity should"
    print "# be obtained; in that case, the second line contains the (invalid)"
    print "# all-ones point encoding (32 bytes of value 0xFF)."
    print ""
    print "0000000000000000000000000000000000000000000000000000000000000000"
    print "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"
    print ""
    for i in range(0, 20):
        x = 0
        for c in hashlib.sha384("curve9767-test5:%0.3d" % (3*i)).digest():
            x = (x * 256) + ord(c)
        u = map_to_base(x)
        if u == 0:
            continue
        print to_hex_string(gf_encode(u))
        print to_hex_string(curve9767_encode(Icart_map(u)))
        print ""

# Test vectors for hash-to-curve.
def make_test_vectors_hash_to_curve():
    print "# ============================================================"
    print "# Hash-to-curve tests."
    print "#"
    print "# Each sequence of two lines consists in a message (ASCII string)"
    print "# followed by the encoding of the curve point obtained by applying"
    print "# the hash-to-curve process to that message. SHAKE256 is used for"
    print "# the first step (seed extraction). The message consists of the"
    print "# contents of the string literal, excluding the starting and"
    print "# ending double-quote characters."
    print ""
    for i in range(0, 20):
        m = "curve9767-test6:%0.3d" % i
        print "\"%s\"" % m
        print to_hex_string(curve9767_encode(hash_to_curve(m)))
        print ""

# Test vectors for ECDH.
def make_test_vectors_ECDH():
    print "# ============================================================"
    print "# ECDH tests."
    print "#"
    print "# Each sequence of seven lines consists in a seed, followed by"
    print "# the generated secret scalar s, the corresponding public key Q,"
    print "# the public key Q2 from the peer, the corresponding shared"
    print "# secret, and invalid point Q3, and the pseudo-shared secret"
    print "# that would have resulted if Q3 had been received."
    print ""
    for i in range(0, 20):
        seed = hashlib.sha256("curve9767-test7:%0.3d" % (3 * i)).digest()
        (s, t, Q) = keygen(seed)
        seed2 = hashlib.sha256("curve9767-test7:%0.3d" % (3 * i + 1)).digest()
        (s2, t2, Q2) = keygen(seed2)
        T = s*Q2
        xy = T.xy()
        pms1 = to_binary_string(gf_encode(xy[0]))

        pp = hashlib.sha256("curve9767-test7:%0.3d" % (3 * i + 2)).digest()
        while true:
            k = 0
            for c in pp:
                k = (k * 256) + ord(c)
            X = g^k
            if not (X^3+A*X+B).is_square():
                bb = gf_encode(X)
                if ord(pp[31]) >= 0x80:
                    bb[31] += 64
                Q3b = to_binary_string(bb)
                break
            pp = hashlib.sha256(pp).digest()
        shake = SHAKE256.new()
        shake.update(b'curve9767-ecdh-failed:')
        shake.update(int_to_bin(s, 32))
        shake.update(Q3b)
        pms2 = shake.read(int(32))

        shake = SHAKE256.new()
        shake.update(b'curve9767-ecdh:')
        shake.update(pms1)
        secret1 = shake.read(int(32))

        shake = SHAKE256.new()
        shake.update(b'curve9767-ecdh:')
        shake.update(pms2)
        secret2 = shake.read(int(32))

        print binascii.hexlify(seed)
        print scalar_to_hex(s)
        print to_hex_string(curve9767_encode(Q))
        print to_hex_string(curve9767_encode(Q2))
        print binascii.hexlify(secret1)
        print binascii.hexlify(Q3b)
        print binascii.hexlify(secret2)
        print ""

# Test vectors for signatures.
def make_test_vectors_signatures():
    print "# ============================================================"
    print "# Signature tests."
    print "#"
    print "# Each sequence of six lines consists in a seed, followed by"
    print "# the generated secret scalar s, the additional secret t, the"
    print "# public key Q, a test message (ASCII string, not hex), and"
    print "# the corresponding signature value. The hash function is"
    print "# SHA3-256 (OID = 2.16.840.1.101.3.4.2.8)."
    print ""
    for i in range(0, 20):
        seed = hashlib.sha256("curve9767-test8:%0.3d" % i).digest()
        (s, t, Q) = keygen(seed)
        msg = "curve9767-test9:%0.3d" % i

        # Hash message with SHA3-256.
        sha3 = SHA3_256.new()
        sha3.update(msg)
        hv = sha3.digest()

        # Make per-signature secret scalar k.
        shake = SHAKE256.new()
        shake.update(b'curve9767-sign-k:')
        shake.update(t)
        shake.update(b'2.16.840.1.101.3.4.2.8:')
        shake.update(hv)
        kb = shake.read(int(64))
        k = 0
        for j in range(0, 64):
            k += ord(kb[j])*(2^(8*j))
        k = k % n
        if k == 0:
            k = 1

        # Commitment point.
        C = k*G
        Cb = to_binary_string(curve9767_encode(C))

        # Compute challenge.
        shake = SHAKE256.new()
        shake.update(b'curve9767-sign-e:')
        shake.update(Cb)
        shake.update(to_binary_string(curve9767_encode(Q)))
        shake.update(b'2.16.840.1.101.3.4.2.8:')
        shake.update(hv)
        eb = shake.read(int(64))
        e = 0
        for j in range(0, 64):
            e += ord(eb[j])*(2^(8*j))
        e = e % n

        # Respond to challenge.
        d = k + e*s

        # Print out values.
        print binascii.hexlify(seed)
        print scalar_to_hex(s)
        print binascii.hexlify(t)
        print to_hex_string(curve9767_encode(Q))
        print "%s" % msg
        print "%s%s" % (binascii.hexlify(Cb), scalar_to_hex(d))
        print ""

# Monte-Carlo test vectors.
def make_test_vectors_curve_monte_carlo():
    print "# ============================================================"
    print "# Monte-Carlo curve test vectors."
    print "#"
    print "# Point multiplications are performed repeatedly:"
    print "#   1. Set Q <- 2*G"
    print "#   2. Set cc <- 0"
    print "#   3. Do 10000 times:"
    print "#      a. Encode Q to 32 bytes."
    print "#      b. Interpret the bytes as integer k (unsigned little-endian)."
    print "#      c. Q <- k*Q"
    print "#      d. cc <- cc + 1"
    print "#      e. if (cc = 0 mod 1000), print Q"
    print "#"
    print "# This process should print 10 points (encoded hex strings)."
    print ""
    # We cannot start at Q=G because that one has X=0, leading to
    # multiplication by 0 and the point-at-infinity, that cannot be encoded.
    Q = 2*G
    for cc in range(1, 10001):
        bb = curve9767_encode(Q)
        k = 0
        for i in range(0, 32):
            k = k * 256 + bb[31 - i]
        Q = k*Q
        if cc % 1000 == 0:
            print to_hex_string(curve9767_encode(Q))

# Main program
make_test_vectors_scalars()
make_test_vectors_decode()
make_test_vectors_curve_basic()
make_test_vectors_map_to_base()
make_test_vectors_Icart_map()
make_test_vectors_hash_to_curve()
make_test_vectors_ECDH()
make_test_vectors_signatures()
make_test_vectors_curve_monte_carlo()
