#include "inner.h"
#include "sha3.h"

/* see curve9767.h */
void
curve9767_hash_to_curve(curve9767_point *Q, shake_context *sc)
{
	/*
	 * We obtain 96 bytes from the SHAKE context, split into two
	 * 48-byte seeds. Each seed is mapped to a field element, and
	 * Icart's map is used to convert that element to a curve
	 * point. Finally, the two points are added together.
	 */
	uint8_t seed[96];
	field_element u;
	curve9767_point T;

	shake_extract(sc, seed, sizeof seed);
	curve9767_inner_gf_map_to_base(u.v, seed);
	curve9767_inner_Icart_map(Q, u.v);
	curve9767_inner_gf_map_to_base(u.v, seed + 48);
	curve9767_inner_Icart_map(&T, u.v);
	curve9767_point_add(Q, Q, &T);
}
