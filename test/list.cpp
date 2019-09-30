#define PAKET_LIB_EXT
#include <paket.hpp>

#include "test.hpp"

using namespace handtruth::pakets;

struct lists_paket : paket<223, fields::varint, fields::list<std::string>> {
    std::int32_t & varik = field<0>();
    list_wrap<1> listik = field<1>();
};

test {
    lists_paket p;
    p.varik = 54;
    auto & list = p.listik;
    list.emplace_back("stringa");
    list.emplace_back("vodka");
    for (std::string & each : list) {
        assert_true(each == "stringa" || each == "vodka");
    }
    assert_equals("stringa", list.front());
    assert_equals("vodka", list.back());
    assert_equals(2u, list.size());
    list = { "lol", "kek" };
    lists_paket q;
    const std::size_t length = 100;
    byte_t data[length];
    p.write(data, length);
    q.read(data, length);
    assert_equals("lol", q.listik.front());
    assert_equals("kek", q.listik.back());
    assert_equals(p, q);
    assert_equals("#223:{ 54, [\"lol\", \"kek\"] }", std::to_string(q));
}
