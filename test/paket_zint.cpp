#include "paket.hpp"

#include "test.hpp"

test {
    using namespace handtruth::pakets;
    std::array<byte_t, 30> mem {};
    int a, b;
    a = -245;
    write_zint(a, mem.data(), mem.size());
    unsigned az = (unsigned(mem[1]) << 8) | unsigned(mem[0]);
    assert_equals(1003u, az);

    read_zint(b, mem.data(), mem.size());
    assert_equals(a, b);
}
