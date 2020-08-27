#include <paket.hpp>

#include "test.hpp"

using namespace handtruth::pakets;

struct example_paket : public paket<34, fields::varint, fields::varlong, fields::string, fields::boolean,
										fields::byte, fields::uint16, fields::int64, fields::zint<int>, fields::zint<unsigned>> {
	constexpr std::int32_t & varint_f() { return field<0>(); }
	constexpr std::int64_t & varlong_f() { return field<1>(); }
	constexpr std::string & string_f() { return field<2>(); }
	constexpr bool & bool_f() { return field<3>(); }
	constexpr byte_t & byte_f() { return field<4>(); }
	constexpr std::uint16_t & ushort_f() { return field<5>(); }
	constexpr std::int64_t & long_f() { return field<6>(); }
	constexpr int & szint_f() { return field<7>(); }
	constexpr unsigned & uzint_f() { return field<8>(); }
};

const std::size_t buff_sz = 100;

test {
	example_paket ex_paket1;
	ex_paket1.varint_f() = -4;
	ex_paket1.varlong_f() = 4853465723498647;
	ex_paket1.string_f() = "oh my god";
	ex_paket1.bool_f() = true;
	ex_paket1.byte_f() = 23;
	ex_paket1.ushort_f() = 25565;
	ex_paket1.long_f() = 3853465723498647;
	ex_paket1.szint_f() = -456;
	ex_paket1.uzint_f() = 7683;
	byte_t bytes[buff_sz];
	ex_paket1.write(bytes, buff_sz);
	example_paket ex_paket2;
	ex_paket2.read(bytes, buff_sz);
	assert_equals(ex_paket1, ex_paket2);
	bytes[1] = 0;
	assert_fails_with(paket_error, {
		ex_paket2.read(bytes, buff_sz);
	});
	assert_equals("#34:{ -4, 4853465723498647, \"oh my god\", true, 23, 25565, 3853465723498647, -456, 7683 }", std::to_string(ex_paket2));
}
