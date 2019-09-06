#include <paket.hpp>

#include "test.hpp"

using namespace handtruth::pakets;

const std::size_t buff_sz = 100;

test {
	struct : public paket<-1> {} p1;
	byte_t bytes[buff_sz];
	assert_equals(6, p1.write(bytes, 6));
	bytes[5] = 255;
	assert_fails_with(paket_error, {
		p1.read(bytes, buff_sz);
	});
}
