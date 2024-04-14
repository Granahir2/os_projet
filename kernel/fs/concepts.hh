#pragma once
#include "kstdlib/string.hh"

typedef size_t time_t;
typedef size_t off_t;


typedef uint16_t mode_t;
enum mode : mode_t {NONE = 0, RDONLY = 1, WRONLY = 2, RW = 3};
enum seekref {SET, CUR, END}; // Tells us whether to offset from the beginning, current pos or end of a file.
enum drit_status {DIR_ENTRY, FILE_ENTRY, LINK_ENTRY, UNDERRUN, NP};

struct statbuf { // Result from stat
	time_t tcreat;
	time_t tmod;

	int mode;
	uint16_t auth; // Authorization word; like Linux
};


template<typename T>
concept Seekable = requires (T x) {
	{x.seek(off_t(), seekref::SET)} -> std::same_as<off_t>;
};

template<typename T>
concept ImplementsFile = requires (T raw_f) {
	{raw_f.write((const void*)(nullptr), size_t())}	-> std::same_as<size_t>;
	{raw_f.read((void*)(nullptr), size_t())}        -> std::same_as<size_t>;
};

class filehandler;
template<typename T>
concept ImplementsDirIterator = requires (T raw_it) {
	{raw_it.pop()};
	{raw_it.push((const char*)nullptr)} -> std::same_as<drit_status>;
	{raw_it.depth()} -> std::same_as<size_t>;
	{raw_it[(size_t)(0)] } -> std::convertible_to<string>;
	{raw_it.open_file((const char*)nullptr, (int)0)} -> std::convertible_to<smallptr<filehandler>>;
};
