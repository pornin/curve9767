/*
 * Enumerate all curves y^2 = x^3-3*x+B in GF(9767^19) with prime order,
 * and B having minimal Hamming weight.
 *
 * This script runs with pari-gp 2.11.2, with the seadata-small.tgz
 * package installed (for efficient curve point counting with SEA).
 * See:
 *    https://pari.math.u-bordeaux.fr/
 *    https://pari.math.u-bordeaux.fr/packages.html
 *
 *
 * Let p = 9767 and q = 9767^19. Note that p is prime and p = 1 mod 19.
 * We define the finite field GF(q) by taking GF(p)[z] (univariate
 * polynomials with coefficients in GF(p)) modulo the irreducible
 * polynomial z^19-2. We want to search for all elliptic curves of
 * equation y^2 = x^3 - 3*x + B, such that B = b*z^i for some b in GF(p)
 * and 0 <= i <= 18. We want curves with prime order.
 *
 * If i = 0, then the curve is also defined over GF(p) with an order close
 * to p, which will divide the order of the curve over GF(q); thus, this
 * cannot yield a curve with prime order. We therefore require 1 <= i <= 18.
 *
 * The Frobenius operator phi(x) = x^p is an automorphism over GF(q); that
 * is, phi(x+y) = phi(x) + phi(y) and phi(x*y) = phi(x) * phi(y) for all
 * x and y in GF(q). Moreover, with our field definition with the modulus
 * z^19-2, we have, for all c in GF(p) and 0 <= i <= 18:
 *    phi(c*z^i) = (2^(i*(p-1)/19))*c*z^i
 * Note that 2^(i*(p-1)/19) is one of the 19th roots of 1 in GF(p).
 *
 * Applying phi() on the curve equation, we obtain an isomorphism from curve:
 *    y^2 = x^3 - 3*x + b*z^i
 * to curve:
 *    y^2 = x^3 - 3*x + (2^(i*(p-1)/19))*b*z^i
 * We get further isomorphic curves by repeatedly applying phi; since
 * phi^19(x) = x, this yields a set of 19 isomorphic curves.
 *
 * This means that we can test for only one curve among the 19th; the other
 * 18 curves have exactly the same order. Whenever we consider a value B,
 * we check whether there is a B*w which is smaller, for some w which is
 * a 19th root of 1 in GF(p); if one of the B*w is smaller, then that B
 * is skipped.
 *
 *
 * This script takes about 1 hour and 40 minutes to fully run on a
 * machine with an Intel Xeon E3-1220 V2 at 3.10 GHz, with 8 GM of RAM,
 * running Linux in 64-bit mode. It finds 23 curves (23 sets of 19
 * curves, taking into account the Frobenius automorphism) with a prime
 * order, two of which being twist of each other.
 *
 * The 23 curves are for the following values of B:
 *
 *   450*z^2   603*z^2   665*z^5   751*z^5   287*z^6   100*z^8   359*z^8
 *   303*z^9   359*z^9   519*z^9  2048*z^9    34*z^10  380*z^10   53*z^12
 *   287*z^12  776*z^13 1751*z^13  458*z^14  489*z^14 1088*z^15  411*z^16
 *  1152*z^16  129*z^18
 *
 * The two curves for B = 359*z^9 and B = 2048*z^9 are quadratic twists of
 * each other. Their respective orders are:
 *
 *    359*z^9 -> 6389436622109970582043832278503799542456269640437724557045939388995057075711
 *   2048*z^9 -> 6389436622109970582043832278503799542449455630003248488928817956373993578097
 */

/* We need some extra RAM for curve point counting. */
default(parisizemax, 100000000)

/* Finite field definitions. */
p = 9767
q = p^19
z = ffgen(x^19-Mod(2, p), 'z)

/* 19-th roots of unity in GF(p). */
rr = polrootsmod(x^19-1, p)

/* Check whether a base field element c is "minimal" in the set of
   values c*w for all 19th roots of 1 in GF(p). Minimality is expressed
   with the cmp() function. */
isminimal19(b) =
{
	for (i = 1, 19,
		if (cmp(rr[i] * b, b) < 0,
			return(0)));
	return(1);
}

/* Test whether the curve y^2 = x^3 - 3*x + B has prime order. If the
   order is prime, then it is returned; otherwise, 0 is returned. */
testcurve(B) =
{
	/* If x^3 - 3*x + B is not irreducible, then there is at least
	   one point with y = 0, which then has order 2, and the curve
	   order is even (hence not prime). */
	if (!polisirreducible(x^3-3*x+B),
		return(0));

	/* Count points with SEA. The ellsea() function will usually abort
	   early and return 0 if the order is not prime. */
	my(E = ellinit([-3, B]));
	my(n = ellsea(E, 1));
	if (!isprime(n), return(0));
	return(n);
}

/* Enumerate all curves y^2 = x^3 - 3*x + b*z^i, and check which one is
   prime. */
findcurves() =
{
	for (i = 1, 18,
		print("Trying b*z^", i);
		for (c = 1, p - 1,
			/* Skip values which are non-minimal in the set
			   of c*w for w a 19th-root of 1. */
			my(b = Mod(c, p));
			if (!isminimal19(b), next);

			/* Try B. */
			my(B = b*z^i);
			my(n = testcurve(B));
			if (!n, next);
			print("PRIME: ", B, " -> ", n);

			/* If the twisted curve also has prime order,
			   report it (it will appear in the list as well). */
			my(nt = 2*(q+1) - n);
			if (isprime(nt),
				print("TWIST OK: ", -B, " -> ", nt))));
}

findcurves()
quit
