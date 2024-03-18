#pragma once

#include "x86/memory.hh"

namespace mem {
	struct phmem_resolvant {
		enum {OK, NOT_MAPPED, NOT_CANONICAL} status;
		x64::phaddr resolved;
	};

	phmem_resolvant resolve_to_phmem(x64::linaddr to_res);
}
