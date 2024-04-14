#include "string.hh"
#include <exception>
#pragma once
#define _STDEXCEPT(name, parent) class name : public parent { \
public:\
	name() : w() {}\
	explicit name(const char* what_arg) : parent(), w(what_arg) {}\
	explicit name(const string& what_arg) : parent(), w(what_arg) {}\
	const char* what() const noexcept override {return w.c_str();}\
private:\
	string w;\
}; 

_STDEXCEPT(logic_error, std::exception);
_STDEXCEPT(domain_error, logic_error);
_STDEXCEPT(invalid_argument, logic_error);
_STDEXCEPT(length_error, logic_error);
_STDEXCEPT(out_of_range, logic_error);

_STDEXCEPT(runtime_error, std::exception);
_STDEXCEPT(range_error, runtime_error);
_STDEXCEPT(overflow_error, runtime_error);
_STDEXCEPT(underflow_error, runtime_error);

