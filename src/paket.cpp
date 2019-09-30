#include "paket.hpp"

#include <exception>
#include <cstring>
#include <type_traits>

namespace handtruth {

namespace pakets {

template <std::size_t max, typename numeric>
inline int read_varnum(const byte_t bytes[], std::size_t length, numeric & value) {
	std::size_t numRead = 0;
    byte_t read;
	value = 0;
    do {
		if (numRead == length)
			return -1;
        read = bytes[numRead];
        numeric tmp = (read & 0b01111111);

		value |= (tmp << (7 * numRead));

        numRead++;
        if (numRead > max) {
            throw paket_error("varint is too big");
        }
    } while ((read & 0b10000000) != 0);
    return numRead;
}

template <typename numeric>
inline int write_varnum(byte_t bytes[], std::size_t length, numeric value) {
	std::size_t numWrite = 0;
	std::make_unsigned_t<numeric> uval = value;
	do {
		if (numWrite == length)
			return -1;
		byte_t temp = static_cast<byte_t>(uval & 0b01111111);
		uval >>= 7;
		if (uval != 0) {
			temp |= 0b10000000;
		}
		if (bytes)
			bytes[numWrite] = temp;
		++numWrite;
	} while (uval != 0);
	return numWrite;
}

std::size_t size_varint(std::int32_t value) {
	return static_cast<std::size_t>(write_varint(value, nullptr, std::numeric_limits<std::size_t>::max()));
}

int read_varint(std::int32_t & value, const byte_t bytes[], std::size_t length) {
	return read_varnum<5>(bytes, length, value);
}

int write_varint(std::int32_t value, byte_t bytes[], std::size_t length) {
	return write_varnum(bytes, length, value);
}

std::size_t size_varlong(std::int64_t value) {
	return static_cast<std::size_t>(write_varlong(nullptr, std::numeric_limits<std::size_t>::max(), value));
}

int read_varlong(const byte_t bytes[], std::size_t length, std::int64_t & value) {
	return read_varnum<10>(bytes, length, value);
}

int write_varlong(byte_t bytes[], std::size_t length, std::int64_t value) {
	return write_varnum(bytes, length, value);
}

std::size_t fields::varint::size() const noexcept {
	return size_varint(value);
}

int fields::varint::read(const byte_t bytes[], std::size_t length) {
	return read_varint(value, bytes, length);
}

int fields::varint::write(byte_t bytes[], std::size_t length) const {
	return write_varint(value, bytes, length);
}

fields::varint::operator std::string() const {
	return std::to_string(value);
}
std::size_t fields::varlong::size() const noexcept {
	return size_varlong(value);
}

int fields::varlong::read(const byte_t bytes[], std::size_t length) {
	return read_varlong(bytes, length, value);
}

int fields::varlong::write(byte_t bytes[], std::size_t length) const {
	return write_varlong(bytes, length, value);
}

fields::varlong::operator std::string() const {
	return std::to_string(value);
}

std::size_t fields::string::size() const noexcept {
	std::size_t length = static_cast<std::size_t>(value.size());
	return size_varint(length) + length;
}

int fields::string::read(const byte_t bytes[], std::size_t length) {
	std::int32_t str_len;
	int s = read_varint(str_len, bytes, length);
	if (s < 0)
		return -1;
	if (str_len < 0)
		throw paket_error("string field is lower than 0");
	std::size_t reminder = length - s;
	std::size_t ustr_len = static_cast<std::size_t>(str_len);
	if (reminder < ustr_len)
		return -1;
	value.assign(reinterpret_cast<const char *>(bytes + s), ustr_len);
	return s + str_len;
}

int fields::string::write(byte_t bytes[], std::size_t length) const {
	std::int32_t str_len = static_cast<std::int32_t>(value.size());
	int s = write_varint(str_len, bytes, length);
	if (s < 0)
		return -1;
	std::size_t reminder = length - s;
	std::size_t ustr_len = static_cast<std::size_t>(str_len);
	if (reminder < ustr_len)
		return -1;
	std::memcpy(bytes + s, value.c_str(), ustr_len);
	return s + str_len;
}

fields::string::operator std::string() const {
	return '"' + value + '"';
}

int head(const byte_t bytes[], std::size_t length, std::int32_t & size, std::int32_t & id) {
	int s = read_varint(size, bytes, length);
	if (s < 0)
		return -1;
	int k = read_varint(id, bytes + s, length - s);
	if (k < 0)
		return -1;
	else
		return s;
}

} // namespace pakets

} // namespace handtruth
