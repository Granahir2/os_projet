#include "kstdlib/stdexcept.hh"
#pragma once

/*
Defines POSIX error codes as exception types for convenience inside
kernel code.
*/

_STDEXCEPT(eperm, runtime_error)
_STDEXCEPT(einval, runtime_error)
_STDEXCEPT(enoent, runtime_error)
_STDEXCEPT(eaccess, runtime_error)
_STDEXCEPT(eoverflow, runtime_error)
_STDEXCEPT(enxio, runtime_error)
_STDEXCEPT(ebusy, runtime_error)
_STDEXCEPT(enomem, runtime_error)
