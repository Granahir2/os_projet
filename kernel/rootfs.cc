#include "rootfs.hh"
#include "kernel.hh"

/*
 We don't need a stack, .. resolves as a link to the location of the mount point (if mounted on /mnt, 
/mnt/.. is a link to / 
*/

void rootfs_dit::operator<<(const char* str) {
	if(stk_top == -1) {
		int mntid = parent->lookup_mnt(str);
		if (mntid != -1) {
			assert(++stk_top < 32, "[rootfs] Indirection overflow");
			drit_stk[stk_top] = {parent->mnts[mnt_id]->get_iterator(), mntid};
			return DIR_ENTRY;
		} else {
			return NP;
		}
	} else {
		switch((*drit_stk[stk_top].di) << str) {
			case DIR_ENTRY:
				return DIR_ENTRY;
			case LINK_ENTRY:
				const char* p = drit_stk[stk_top].di->readlink(str);
				feed_components(p);
				break;
			case FILE_ENTRY:
				return FILE_ENTRY;
			case UNDERRUN:
				stk_top--;
				return DIR_ENTRY;
			case NP:
				const char* p = drit_stk[stk_top].di->get_canonical_path();
				/*ptot = p @ str */
				int mntid = parent->lookup_mnt(ptot);
				if (mntid != -1) {
					assert(++stk_top < 32, "[rootfs] Indirection overflow");
					drit_stk[stk_top] = {parent->mnts[mnt_id]->get_iterator(), mntid};
					return DIR_ENTRY;
				} else {
					return NP;
				}
		}
	}
}

void rootfs_dit::get_canonical_path() {
	/* concatenate every path in the stack ! */
};
