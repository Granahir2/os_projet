#include "gpt.hh"
#include "kernel/posix_except.hh"

namespace gpt {

driver::driver(filehandler* to_bind, uint16_t log_sec) : fh(to_bind), blk_size(log_sec) {
	// Read the GPT header.
	fh->seek(0, SET);
	smallptr<unsigned char[]> lba0(new unsigned char[16]);
	fh->read(lba0.ptr, 16);
	if(lba0[4] != 0xee) {
		throw runtime_error("Illegal PMBR found. Disk not GPT-formatted?");
	}

	unsigned int pth_lba = *((unsigned int *)(&lba0[0x8]));
	constexpr int pth_size = 0x5c;
	smallptr<char[]> pt_header(new char[pth_size]);	

	fh->seek(blk_size*pth_lba, SET);
	if(memcmp(pt_header.ptr, "EFI PART", 8) != 0) {
		throw runtime_error("Illegal GPT header. Disk corrupted?");
	}

	// TODO : as of now, we don't check the CRCs / alternate header

	num_part = *((unsigned int*)(&pt_header[0x50]));
	partdesc_size = *((unsigned int*)(&pt_header[0x54]));		

	size_t pte_lba = *((size_t*)(&pt_header[0x48]));
	auto p = smallptr<unsigned char[]>(new unsigned char[num_part * partdesc_size]);
	partentry_arr.swap(std::move(p));

	fh->seek(blk_size*pte_lba, SET);
	fh->read(partentry_arr.ptr, num_part*partdesc_size);
};

smallptr<partition> driver::get_part(unsigned int by_id, int mode) {
	if(by_id >= num_part) {
		throw eaccess();
	}

	if((mode & (fh->stat()).mode) != mode) { // Mode on partitions is covariant
		throw eperm();
	}

	partition_entry pe = get_partinfo(by_id);

	return smallptr<partition>(new partition(mode, pe, this));
};

smallptr<partition> driver::get_part(guid_t by_guid, int mode) {
	for(unsigned i = 0; i < num_part; ++i) {
		if(memcmp(&by_guid, &partentry_arr[partdesc_size*i + 0x10], 0x10) == 0) {
			return get_part(i, mode);
		}
	}
	throw eaccess();
}

smallptr<partition> driver::get_part(part_name name, int mode) {
	if(name.name_size != partdesc_size - 0x38) {
		throw eaccess();	
	}

	for(unsigned i = 0; i < num_part; ++i) {
		if(memcmp(name.name, &partentry_arr[partdesc_size*i + 0x38], partdesc_size - 0x38) == 0) {
			return get_part(i, mode);
		}
	}
	throw eaccess();
}

partition_entry driver::get_partinfo(unsigned int id) {
	if(id >= num_part) {throw eaccess();}
	partition_entry pe;
	memcpy(&pe, &partentry_arr[id * 0x38], 0x38);
	pe.pn.name = (const void*)(&partentry_arr[0x38]);
	pe.pn.name_size = partdesc_size - 0x38;
	pe.blk_size = blk_size;
	return pe;
}


raw_partition::raw_partition(partition_entry pe, driver* p) : partinfo(pe), parent(p) {
	if(p){
		parent->refcnt++;
	} else {throw logic_error("Cannot open partition orphan partition");}
}


raw_partition::~raw_partition() {if(parent && parent->refcnt) {--parent->refcnt;}}

off_t raw_partition::seek(off_t off, seekref whence) {
	switch(whence) {
		case SET:
			if(off > 0 && (size_t)(off) < (partinfo.end_lba + 1 - partinfo.start_lba)*partinfo.blk_size) {
				return offset = off;
			} else {throw einval();}
		case CUR:
			return seek(off + offset, SET);
		case END:
			if(off <= 0 && (size_t)(-off) <= (partinfo.end_lba + 1 - partinfo.start_lba)*partinfo.blk_size - 1) {
				return offset = (partinfo.end_lba + 1 - partinfo.start_lba)*partinfo.blk_size - 1 + off;
			} else {throw einval();}
		default:
			throw einval();
	}
}


size_t raw_partition::read(void* buf, size_t cnt) const {
	parent->fh->seek(offset + partinfo.start_lba*partinfo.blk_size, SET);
	/* Here particularly but elsewhere too we don't guard against overflows... */
	if(offset + cnt >= (partinfo.end_lba + 1 - partinfo.start_lba) * partinfo.blk_size) {
		cnt = (partinfo.end_lba + 1 - partinfo.start_lba) * partinfo.blk_size - offset;
	}
	return parent->fh->read(buf, cnt);
};

size_t raw_partition::write(const void* buf, size_t cnt) const {
	parent->fh->seek(offset + partinfo.start_lba*partinfo.blk_size, SET);
	if(offset + cnt >= (partinfo.end_lba + 1 - partinfo.start_lba) * partinfo.blk_size) {
		cnt = (partinfo.end_lba + 1 - partinfo.start_lba) * partinfo.blk_size - offset;
	}
	return parent->fh->write(buf, cnt);
};

}
