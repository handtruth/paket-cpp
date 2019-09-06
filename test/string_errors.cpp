#include <paket.hpp>

#include "test.hpp"

using namespace handtruth::pakets;

const std::size_t buff_sz = 100;

test {
	struct : public paket<2508, fields::string> {
		constexpr std::string & str() { return field<0>(); }
	} strp1, strp2;
	strp1.str() = "tree";
	byte_t bytes[buff_sz];
	assert_equals(-1, strp1.write(bytes, 3));
	assert_equals(-1, strp1.write(bytes, 4));
	assert_equals(8, strp1.write(bytes, 8));
	bytes[0] = 0;
	assert_equals(-1, strp2.read(bytes, 3));
	assert_equals(-1, strp2.read(bytes, 4));
	bytes[3] = 255;
	bytes[4] = 255;
	bytes[5] = 255;
	bytes[6] = 255;
	bytes[7] = 127;
	assert_fails_with(paket_error, {
		strp2.read(bytes, buff_sz);
	});
	assert_equals(8, strp1.write(bytes, 8));
	assert_equals(8, strp2.read(bytes, 8));
	assert_equals(strp1, strp2);
}
