#include "test.hpp"

#include <paket.hpp>

using namespace handtruth::pakets;

const std::size_t buff_sz = 100;

test {
	class : public paket<42> {} ep1;
	byte_t bytes[buff_sz];
	ep1.write(bytes, buff_sz);
	std::int32_t size, id;
	head(bytes, buff_sz, size, id);
	assert_equals(42, id);
	assert_equals(1, size);
	assert_equals(-1, head(bytes, 0, size, id));
	assert_equals(-1, head(bytes, 1, size, id));
}
