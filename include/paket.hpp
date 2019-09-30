#ifndef _PAKET_HEAD
#define _PAKET_HEAD

#include <cinttypes>
#include <tuple>
#include <string>
#include <stdexcept>
#include <vector>
#include <initializer_list>
#include <limits>
#include <array>

namespace handtruth {

namespace pakets {

/**
 * \brief Smallest addresable unit.
 * 
 * This type is used in raw data arrays. This arrays will be decoded to
 * paket or encoded to paket.
 */
typedef std::uint8_t byte_t;

/**
 * Get size of encoded varint.
 * 
 * \param value varint
 * \return size in bytes of encoded varint
 */
std::size_t size_varint(std::int32_t value);

/**
 * Tries to read varint from byte array.
 * 
 */
int read_varint(std::int32_t & value, const byte_t bytes[], std::size_t length);
template <std::size_t N>
inline int read_varint(std::int32_t & value, const std::array<byte_t, N> & bytes, std::size_t length = N) {
	return read_varint(value, bytes.data(), length);
}

int write_varint(std::int32_t value, byte_t bytes[], std::size_t length);

template <std::size_t N>
inline int write_varint(std::int32_t value, std::array<byte_t, N> & bytes, std::size_t length = N) {
	return write_varint(value, bytes, length);
}

std::size_t size_varlong(std::int64_t value);
int read_varlong(const byte_t bytes[], std::size_t length, std::int64_t & value);
int write_varlong(byte_t bytes[], std::size_t length, std::int64_t value);

class paket_error : public std::runtime_error {
public:
	paket_error(const std::string & message) : std::runtime_error(message) {}
};

namespace fields {

	template <typename T>
	struct field {
		typedef T value_type;
		T value;
		field() = default;
		constexpr field(const T & other) : value(other) {}
		T & operator*() noexcept {
			return value;
		}
		const T & operator*() const noexcept {
			return value;
		}
		template <typename other_t>
		bool operator==(const other_t & other) const noexcept {
			return value == other.value;
		}
	};

	struct varint : public field<std::int32_t> {
		varint() = default;
		constexpr varint(const value_type & init) : field(init) {}
		std::size_t size() const noexcept;
		int read(const byte_t bytes[], std::size_t length);
		int write(byte_t bytes[], std::size_t length) const;
		operator std::string() const;
	};

	struct varlong : public field<std::int64_t> {
		varlong() = default;
		constexpr varlong(const value_type & init) : field(init) {}
		std::size_t size() const noexcept;
		int read(const byte_t bytes[], std::size_t length);
		int write(byte_t bytes[], std::size_t length) const;
		operator std::string() const;
	};

	struct string : public field<std::string> {
		string() = default;
		constexpr string(const value_type & init) : field(init) {}
		std::size_t size() const noexcept;
		int read(const byte_t bytes[], std::size_t length);
		int write(byte_t bytes[], std::size_t length) const;
		operator std::string() const;
	};

	template <typename T>
	struct static_size_field : public field<T> {
		static constexpr std::size_t static_size() noexcept {
			return sizeof(typename field<T>::value_type);
		}
	public:
		static_size_field() = default;
		constexpr static_size_field(const typename field<T>::value_type & init) : field<T>(init) {}
		constexpr std::size_t size() const noexcept {
			return static_size();
		}
		int read(const byte_t bytes[], std::size_t length) {
			if (length < static_size())
				return -1;
			field<T>::value = *(reinterpret_cast<const typename field<T>::value_type *>(bytes));
			return static_size();
		}
		int write(byte_t bytes[], std::size_t length) const {
			if (length <= static_size())
				return -1;
			*(reinterpret_cast<typename field<T>::value_type *>(bytes)) = field<T>::value;
			return static_size();
		}
		operator std::string() const {
			return std::to_string(field<T>::value);
		}
	};

	struct boolean : public static_size_field<bool> {
		boolean() = default;
		constexpr boolean(const value_type & init) : static_size_field(init) {}
		operator std::string() const {
			return value ? "true" : "false";
		}
	};

	struct byte : public static_size_field<byte_t> {
		byte() = default;
		constexpr byte(const value_type & init) : static_size_field(init) {}
	};

	struct uint16 : public static_size_field<std::uint16_t> {
		uint16() = default;
		constexpr uint16(const value_type & init) : static_size_field(init) {}
	};

	struct int64 : public static_size_field<std::int64_t> {
		int64() = default;
		constexpr int64(const value_type & init) : static_size_field(init) {}
	};

	template <typename T>
	struct list : public field<std::vector<T>> {
		typedef T list_element;

		std::size_t size() const noexcept {
			std::size_t sz = size_varint(static_cast<std::int32_t>(this->value.size()));
			for (const auto & f : this->value)
				sz += f.size();
			return sz;
		}
		int read(const byte_t bytes[], std::size_t length) {
			std::int32_t sz;
			int offset = read_varint(sz, bytes, length);
			if (offset == -1)
				return -1;
			if (sz < 0)
				throw paket_error("list size '" + std::to_string(sz) + "' is lower then 0");
			this->value.resize(sz);
			for (T & f : this->value) {
				int s = f.read(bytes + offset, length - offset);
				if (s == -1)
					return -1;
				offset += s;
			}
			return offset;
		}
		int write(byte_t bytes[], std::size_t length) const {
			int offset = write_varint(static_cast<std::int32_t>(this->value.size()), bytes, length);
			if (offset == -1)
				return -1;
			for (const T & f : this->value) {
				int s = f.write(bytes + offset, length - offset);
				if (s == -1)
					return -1;
				offset += s;
			}
			return offset;
		}
		operator std::string() const {
			if (this->value.empty())
				return "[ ]";
			std::string result = "[" + std::string(this->value[0]);
			for (auto it = this->value.begin() + 1, end = this->value.end(); it != end; it++)
				result += ", " + std::string(*it);
			return result + "]";
		}
	};

	template <> struct list<std::int32_t> : public list<varint> {};
	template <> struct list<std::int64_t> : public list<varlong> {};
	template <> struct list<std::string> : public list<string> {};
	template <> struct list<bool> : public list<boolean> {};
	template <> struct list<byte_t> : public list<byte> {};
	template <> struct list<std::uint16_t> : public list<uint16> {};
}

template <typename Iter>
struct list_wrapper_iterator {
	Iter iter;
	list_wrapper_iterator(const Iter & it) : iter(it) {}
	typedef typename Iter::value_type field_type;
	typedef typename field_type::value_type value_type;
	typedef value_type & reference;
	typedef value_type * pointer;
	typedef typename Iter::difference_type difference_type;
	// Iterator
	list_wrapper_iterator(const list_wrapper_iterator &) = default;
    list_wrapper_iterator & operator=(const list_wrapper_iterator &) = default;
    list_wrapper_iterator & operator++() {
		++iter;
		return *this;
	}
	list_wrapper_iterator operator++(int) {
		return iter++;
	}
    reference operator*() const {
		return iter->value;
	}
    friend void swap(list_wrapper_iterator & lhs, list_wrapper_iterator & rhs) {
		std::swap(lhs.iter, rhs.iter);
	}
    pointer operator->() const {
		return iter.operator->();
	}
    friend bool operator==(const list_wrapper_iterator & lhs, const list_wrapper_iterator & rhs) {
		return lhs.iter == rhs.iter;
	}
    friend bool operator!=(const list_wrapper_iterator & lhs, const list_wrapper_iterator & rhs) {
		return lhs.iter != rhs.iter;
	}
	// Bidirectional
    list_wrapper_iterator & operator--() {
		--iter;
		return *this;
	}
    list_wrapper_iterator operator--(int) {
		return iter--;
	}
	// Random access
    friend bool operator<(const list_wrapper_iterator & lhs, const list_wrapper_iterator & rhs) {
		return lhs.iter < rhs.iter;
	}
    friend bool operator>(const list_wrapper_iterator & lhs, const list_wrapper_iterator & rhs) {
		return lhs.iter > rhs.iter;
	}
    friend bool operator<=(const list_wrapper_iterator & lhs, const list_wrapper_iterator & rhs) {
		return lhs.iter <= rhs.iter;
	}
    friend bool operator>=(const list_wrapper_iterator & lhs, const list_wrapper_iterator & rhs) {
		return lhs.iter >= rhs.iter;
	}
    list_wrapper_iterator& operator+=(difference_type n) {
		iter += n;
		return *this;
	}
    friend list_wrapper_iterator operator+(const list_wrapper_iterator & it, difference_type n) {
		return it.iter + n;
	}
    friend list_wrapper_iterator operator+(difference_type n, const list_wrapper_iterator & it) {
		return n + it.iter;
	}
    list_wrapper_iterator & operator-=(difference_type n) {
		iter -= n;
		return *this;
	}
    friend list_wrapper_iterator operator-(const list_wrapper_iterator & it, difference_type n) {
		return it.iter - n;
	}
    friend difference_type operator-(const list_wrapper_iterator & lhs, const list_wrapper_iterator & rhs) {
		return lhs.iter - rhs.iter;
	}
    reference operator[](difference_type n) const {
		return iter[n].value;
	}
};

template <typename List>
class list_wrapper {
	List & ref;
public:
	typedef typename List::value_type field_type;
	typedef typename List::size_type size_type;
	typedef typename field_type::value_type value_type;
	typedef value_type & reference;
	typedef const value_type & const_reference;
	typedef value_type * pointer;
	typedef const value_type * const_pointer;
	typedef list_wrapper_iterator<typename std::vector<field_type>::iterator> iterator;
	typedef list_wrapper_iterator<typename std::vector<field_type>::const_iterator> const_iterator;
	typedef list_wrapper_iterator<typename std::vector<field_type>::reverse_iterator> reverse_iterator;
	typedef list_wrapper_iterator<typename std::vector<field_type>::const_reverse_iterator> const_reverse_iterator;

	constexpr list_wrapper(List & vector) : ref(vector) {};

	list_wrapper & operator=(const std::vector<value_type> & other) {
		ref.clear();
		ref.reserve(other.size());
		for (const auto & each : other)
			ref.emplace_back(each);
		return *this;
	};

	list_wrapper & operator=(std::initializer_list<value_type> other) {
		ref.clear();
		ref.reserve(other.size());
		for (const auto & each : other)
			ref.emplace_back(each);
		return *this;
	}

	operator std::vector<value_type>() const {
		std::vector<value_type> result;
		result.reserve(ref.size());
		for (const auto & each : ref)
			result.push_back(each);
		return result;
	}

	reference at(size_type pos) {
		return ref.at(pos).value;
	}
	const_reference at(size_type pos) const {
		return ref.at(pos).value;
	}
	reference operator[](size_type pos) {
		return ref[pos].value;
	}
	const_reference operator[](size_type pos) const {
		return ref[pos].value;
	}
	reference front() {
		return ref.front().value;
	}
	const_reference front() const {
		return ref.front().value;
	}
	reference back() {
		return ref.back().value;
	}
	const_reference back() const {
		return ref.back().value;
	}
	bool empty() const noexcept {
		return ref.empty();
	}
	size_type size() const noexcept {
		return ref.size();
	}
	constexpr size_type max_size() const noexcept {
		return std::numeric_limits<int>::max() / sizeof(field_type);
	}
	void reserve(size_type new_cap) {
		ref.reserve(new_cap);
	}
	size_type capacity() const noexcept {
		return ref.capacity();
	}
	void shrink_to_fit() {
		ref.shrink_to_fit();
	}
	void clear() noexcept {
		ref.clear();
	}
	void push_back(const_reference value) {
		ref.emplace_back(value);
	}
	void push_back(value_type && value) {
		ref.emplace_back(value);
	}
	template <class... Args> 
	iterator emplace(const_iterator pos, Args &&... args) {
		return ref.emplace(pos.iter, args...);
	}
	template <class... Args>
	reference emplace_back(Args &&... args) {
		return ref.emplace_back(args...).value;
	}
	iterator begin() noexcept {
		return ref.begin();
	}
	const_iterator begin() const noexcept {
		return ref.begin();
	}
	const_iterator cbegin() const noexcept {
		return ref.cbegin();
	}
	iterator end() noexcept {
		return ref.end();
	}
	const_iterator end() const noexcept {
		return ref.end();
	}
	const_iterator cend() const noexcept {
		return ref.cend();
	}
	reverse_iterator rbegin() noexcept {
		return ref.rbegin();
	}
	const_reverse_iterator rbegin() const noexcept {
		return ref.rbegin();
	}
	const_reverse_iterator crbegin() const noexcept {
		return ref.crbegin();
	}
	iterator erase(const_iterator pos) {
		return ref.erase(pos.iter);
	}
	iterator erase(const_iterator first, const_iterator last) {
		return ref.erase(first.iter, last.iter);
	}
	void resize(size_type count) {
		ref.resize(count);
	}
	void resize(size_type count, const value_type & value) {
		ref.resize(count, field_type(value));
	}
	iterator insert(const_iterator pos, const value_type & value) {
		return ref.insert(pos.iter, field_type(value));
	}
	iterator insert(const_iterator pos, value_type && value) {
		return ref.insert(pos.iter, field_type(value));
	}
};

int head(const byte_t bytes[], std::size_t length, std::int32_t & size, std::int32_t & id);

template <std::int32_t paket_id, typename ...fields_t>
class paket : public std::tuple<fields_t...> {
private:
	template <typename first, typename ...other>
	static std::size_t size_field(const first & field, const other &... fields) noexcept {
		return field.size() + size_field(fields...);
	}
	static constexpr std::size_t size_field() noexcept {
		return 0;
	}
public:
	paket() {}
	std::size_t size() const {
		auto size_them = [](auto const &... e) -> std::size_t {
			return size_field(e...);
		};
		return std::apply(size_them, (const std::tuple<fields_t...> &) *this);
	}
	template <std::size_t i>
	using field_type = typename std::tuple_element<i, std::tuple<fields_t...>>::type;

	template <std::size_t i>
	using value_type = typename field_type<i>::value_type;

	template <int i>
	constexpr value_type<i> & field() noexcept {
		return std::get<i>(*this).value;
	}
	template <int i>
	constexpr const value_type<i> & field() const noexcept {
		return std::get<i>(*this).value;
	}

	template <std::size_t i>
	using list_wrap = list_wrapper<value_type<i>>;
	template <std::size_t i>
	using list_const_wrap = list_wrapper<const value_type<i>>;

	template <int i>
	constexpr field_type<i> & wrapper() noexcept {
		return std::get<i>(*this);
	}
	template <int i>
	constexpr const field_type<i> & wrapper() const noexcept {
		return std::get<i>(*this);
	}

	constexpr std::int32_t id() const noexcept {
		return paket_id;
	}
private:
	template <typename first, typename ...other>
	static int write_field(byte_t bytes[], std::size_t length, const first & field, const other &... fields) {
		int s = field.write(bytes, length);
		if (s < 0)
			return -1;
		int comp_size = write_field(bytes + s, length - s, fields...);
		if (comp_size < 0)
			return -1;
		return s + comp_size;
	}
	static int write_field(byte_t *, std::size_t) {
		return 0;
	}
	template <typename first, typename ...other>
	static int read_field(const byte_t bytes[], std::size_t length, first & field, other &... fields) {
		int s = field.read(bytes, length);
		if (s < 0)
			return -1;
		int comp_size = read_field(bytes + s, length - s, fields...);
		if (comp_size < 0)
			return -1;
		return s + comp_size;
	}
	static int read_field(const byte_t *, std::size_t) {
		return 0;
	}
public:
	int write(byte_t bytes[], std::size_t length) const {
		// HEAD
		// size of size
		int k = write_varint(size_varint(paket_id) + size(), bytes, length);
		if (k < 0)
			return -1;
		// size of id
		int s = write_varint(paket_id, bytes + k, length - k);
		if (s < 0)
			return -1;
		s += k;
		// BODY
		auto write_them = [bytes, length, s](auto const &... e) -> int {
			return write_field(bytes + s, length - s, e...);
		};
		int comp_size = std::apply(write_them, (const std::tuple<fields_t...> &) *this);
		if (comp_size < 0)
			return -1;
		else
			return comp_size + s;
	}
	template <std::size_t N>
	inline int write(std::array<byte_t, N> & bytes, std::size_t length = N) const {
		return write(bytes.data(), length);
	}
	int read(const byte_t bytes[], std::size_t length) {
		std::int32_t size;
		std::int32_t id;
		// HEAD
		int k = read_varint(size, bytes, length);
		if (k < 0)
			return -1;
		if ((std::size_t)(size + k) > length)
			return -1;
		int s = read_varint(id, bytes + k, length - k);
		if (s < 0)
			return -1;
		if (id != paket_id)
			throw paket_error("wrong paket id (" + std::to_string(paket_id) + " expected, got " + std::to_string(id) + ")");
		int l = k + s;
		// BODY
		auto read_them = [bytes, length, l](auto &... e) -> int {
			return read_field(bytes + l, length - l, e...);
		};
		int comp_size = std::apply(read_them, (std::tuple<fields_t...> &) *this);
		if (comp_size < 0)
			return -1;
		if (size != comp_size + s)
			throw paket_error("wrong paket size (" + std::to_string(size) + " expected, got " + std::to_string(comp_size + s) + ")");
		else
			return comp_size + l;
	}
	template <std::size_t N>
	inline int read(const std::array<byte_t, N> & bytes, std::size_t length = N) {
		return read(bytes.data(), length);
	}
private:
	template <typename first, typename ...other>
	static std::string enum_next_as_string(const first & field, const other &... fields) {
		return std::string(field) + ((", " + std::string(fields)) + ...);
	}
	template <typename first>
	static std::string enum_next_as_string(const first & field) {
		return std::string(field);
	}

	static std::string enum_next_as_string() {
		return std::string();
	}

public:
	std::string enumerate_as_string() const {
		auto enum_them = [](auto const &... e) -> std::string {
			return enum_next_as_string(e...);
		};
		return std::apply(enum_them, (const std::tuple<fields_t...> &) *this);
	}
};

} // namespace pakets

} // namespace handtruth

namespace std {

	template <int id, typename ...fields_t>
	string to_string(const handtruth::pakets::paket<id, fields_t...> & paket) {
		std::string s = paket.enumerate_as_string();
		return "#" + to_string(id) + ":{ " + s + " }";
	}

} // namespace std

#ifdef PAKET_LIB_EXT

#	define fname(name, n) \
		constexpr value_type<n> & name() noexcept { return field<n>(); }\
		constexpr const value_type<n> & name() const noexcept { return field<n>(); }
#	define lname(name, n) \
		constexpr list_wrap<n> name() noexcept { return field<n>(); }\
		constexpr const list_const_wrap<n> name() const noexcept { return field<n>(); }
#	define ename(name, n, type) \
		fname(name##_numeric, n) \
		constexpr type name() const noexcept { return type(name##_numeric()); } \
		constexpr void name(type c) noexcept { name##_numeric() = std::int32_t(c); }

#endif // PAKET_LIB_EXT

#endif // _PAKET_HEAD
