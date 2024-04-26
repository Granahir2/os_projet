#pragma once
#include <cstddef>
#include <type_traits>

/*
Minimalistic smart pointer classes. A smallptr object represents
ownership of the pointed-to object.
*/

template<class T>
class smallptr {
public:
	smallptr(T* q = nullptr) : ptr(q) {}
	smallptr(smallptr& other) = delete;
	smallptr(smallptr&& other) {ptr = other.ptr; other.ptr = nullptr;}
	
	template<class S>
	smallptr(smallptr<S>&& other) requires (std::is_base_of<T, S>::value) {
		ptr = other.ptr; other.ptr = nullptr;
	} 

	~smallptr() {delete ptr;}

	void swap(smallptr&& other) {auto tmp = other.ptr; other.ptr = ptr; ptr = tmp;}

	operator bool() {return ptr != nullptr;}
	T* ptr;
};


template<class T>
class smallptr<T[]> {
public:
	smallptr(T* q = nullptr) : ptr(q) {}
	smallptr(smallptr& other) = delete;
	smallptr(smallptr&& other) {ptr = other.ptr; other.ptr = nullptr;}

	template<class S>
	smallptr(smallptr<S[]>&& other) requires (std::is_base_of<T, S>::value) {
		ptr = other.ptr; other.ptr = nullptr;
	} 

	~smallptr() {delete[] ptr;}



	void swap(smallptr&& other) {auto tmp = other.ptr; other.ptr = ptr; ptr = tmp;}

	operator bool() const {return ptr != nullptr;}
	T& operator [](size_t s) const {return ptr[s];}

	T* ptr;
};
