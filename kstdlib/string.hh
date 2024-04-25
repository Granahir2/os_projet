#pragma once
#include <cstddef>
#include <cstdint>
#include "cstring.hh"
#include "memory.hh"
/* Really bare bones templated string implementation.
No reallocation */
#include <utility>

template<typename T>
class basic_string {
public:
	basic_string() : len(0), data() {};
	basic_string(basic_string<T>&& other) : len(other.len), data(std::move(other.data)) {
		other.data.ptr = nullptr;
		other.len = 0;
	}
	basic_string(const basic_string<T>& other) : len(other.len), data(new T[other.len+1]) {
		memcpy(data.ptr, other.c_str(), sizeof(T)*(len+1));
	}

	basic_string(const T* cstr) : len(([&](){size_t i=0; for(; cstr[i] != '\0'; ++i); return i;})()),
				    data(new T[len+1]) {
		memcpy(data.ptr, cstr, sizeof(T)*(len+1));
	}

	~basic_string() = default;

	basic_string<T>& operator=(basic_string<T>&& other) {
		len = other.len;
		data.swap(std::move(other.data));
		return *this;
	}

	operator bool() {return len != 0 && data;}
	operator const T*() {return c_str();}

	T& operator[](size_t offset) const {return data[offset];};

	T* c_str() const {return data.ptr;}
	size_t length() const {return len;};

private:
	friend basic_string<T> operator+<>(const basic_string<T>&, const basic_string<T>&);
	friend basic_string<T> operator+<>(T, const basic_string<T>&);
	friend basic_string<T> operator+<>(const basic_string<T>&, T);

	basic_string(size_t l, smallptr<T[]>&& d) : len(l), data(std::move(d)) {};

	size_t len = 0;
	smallptr<T[]> data;
};

template<typename T>
basic_string<T> operator+(const basic_string<T>& lhs, const basic_string<T>& rhs) {
	auto nptr = smallptr<T[]>(new T[lhs.length() + rhs.length() + 1]);
	for(size_t i = 0; i < lhs.length(); ++i) {
		nptr[i] = lhs[i];
	}
	for(size_t j = 0; j < rhs.length(); ++j) {
		nptr[lhs.length() + j] = rhs[j]; 
	}
	nptr[lhs.length() + rhs.length()] = '\0';
	return basic_string(lhs.length() + rhs.length(), std::move(nptr));
}

template<typename T>
basic_string<T> operator+(const basic_string<T>& lhs, T rhs) {
	auto nptr = smallptr<T[]>(new T[lhs.length() + 2]);
	memcpy(nptr.ptr, lhs.c_str(), sizeof(T)*(lhs.length()));
	nptr[lhs.length()] = rhs;
	nptr[lhs.length() + 1] = '\0';
	return basic_string(lhs.length() + 1, std::move(nptr));
};

template<typename T>
basic_string<T> operator+(T lhs, const basic_string<T>& rhs) {
	auto nptr = smallptr<T[]>(new T[rhs.length() + 2]);
	memcpy(nptr.ptr + 1, rhs.c_str(), sizeof(T)*(rhs.length()));
	nptr[0] = lhs;
	nptr[rhs.length() + 1] = '\0';
	return basic_string(rhs.length() + 1, std::move(nptr));
};

using string = basic_string<char>;
