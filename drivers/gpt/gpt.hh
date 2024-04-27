#include "kernel/fs/fs.hh"
#include "kernel/fs/templates.hh"

namespace gpt {

using guid_t = uint64_t[4];

struct __attribute__((packed)) part_name {
	const void* name;
	size_t name_size;
};

struct __attribute__((packed)) partition_entry {
	guid_t type;
	guid_t part_guid;

	size_t start_lba;
	size_t end_lba;
	uint64_t attribs;
	part_name pn;
	uint16_t blk_size;
};

class driver;

class raw_partition {
public:
	const partition_entry partinfo;
	driver* const parent;

	raw_partition(partition_entry pe, driver* p); 
	~raw_partition();
	
	off_t  seek(off_t off, seekref whence);
	size_t read(void* buf, size_t sz) const;
	size_t write(const void* buf, size_t sz) const;
private:
	size_t offset = 0;
};

using partition = perm_fh<raw_partition>;

class driver {
friend raw_partition;
public:
	driver(filehandler* to_bind, uint16_t log_sec = 512);

	smallptr<partition> get_part(unsigned int by_id, int mode);
	smallptr<partition> get_part(guid_t by_guid, int mode);
	smallptr<partition> get_part(part_name by_name, int mode);

	partition_entry get_partinfo(unsigned int id);	
	
private:
	filehandler* const fh;	
	const uint16_t blk_size;
	size_t refcnt;

	unsigned int num_part;
	unsigned int partdesc_size;

	smallptr<unsigned char[]> partentry_arr;

};

}
