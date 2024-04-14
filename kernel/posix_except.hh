#include "kstdlib/stdexcept.hh"
#pragma once

/*
Defines POSIX error codes as exception types for convenience inside
kernel code.
*/

_STDEXCEPT(eperm, logic_error)
_STDEXCEPT(einval, logic_error)
_STDEXCEPT(enoent, logic_error)
