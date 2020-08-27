#define PAKET_LIB_EXT
#include "paket.hpp"

#include "test.hpp"

test {
	using namespace handtruth::pakets;
	struct : paket<1, fields::int64> {
		fname(lf, 0);
	} paket1, paket2;
	static_assert(sizeof(paket1) == 8u);
	assert_equals(sizeof(paket1), 8u);
	paket1.lf() = 245735678;
	const int sz = 10;
	std::array<byte_t, sz> data;
	assert_equals(sz, paket1.write(data, sz));
	assert_equals(sz, paket2.read(data, sz));
	assert_equals(paket1, paket2);
}
