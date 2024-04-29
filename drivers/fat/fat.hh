#pragma once
#include "kernel/fs/fs.hh"
#include "kernel/fs/templates.hh"
#include "kstdlib/string.hh"

namespace FAT {

class FAT_dir_iterator;
class FAT_file;
class FAT_FileSystem;

const int FAT12 = 0;
const int FAT16 = 1;
const int FAT32 = 2;

const uint16_t ATTR_READ_ONLY = 0x01;
const uint16_t ATTR_HIDDEN = 0x02;
const uint16_t ATTR_SYSTEM = 0x04;
const uint16_t ATTR_VOLUME_ID = 0x08;
const uint16_t ATTR_DIRECTORY = 0x10;
const uint16_t ATTR_ARCHIVE = 0x20;
const uint16_t ATTR_LONG_NAME = 0x0F;
const uint16_t LAST_LONG_ENTRY = 0x40;

struct FAT_dir_entry {
    uint8_t DIR_Name[11];
    uint8_t DIR_Attr;
    uint8_t DIR_NTRes;
    uint8_t DIR_CrtTimeTenth;
    uint16_t DIR_CrtTime;
    uint16_t DIR_CrtDate;
    uint16_t DIR_LstAccDate;
    uint16_t DIR_FstClusHI;
    uint16_t DIR_WrtTime;
    uint16_t DIR_WrtDate;
    uint16_t DIR_FstClusLO;
    uint32_t DIR_FileSize;
} __attribute__((packed));

struct FAT_Long_File_Name_entry {
    uint8_t LDIR_Ord;
    uint16_t LDIR_Name1[5];
    uint8_t LDIR_Attr;
    uint8_t LDIR_Type;
    uint8_t LDIR_Chksum;
    uint16_t LDIR_Name2[6];
    uint16_t LDIR_FstClusLO;
    uint16_t LDIR_Name3[2];
} __attribute__((packed));

struct FAT_dir_entry_with_full_name {
    FAT_dir_entry entry;
    string full_name;
};


class FAT_file {
public:
    size_t read(void* buffer, size_t size);
    size_t write(const void* buffer, size_t size);
    void seek(off_t offset, seekref whence);
    FAT_file(FAT_FileSystem* fat_fs, size_t file_size, size_t first_cluster_number);

private:
    size_t read_write_head_position;
    size_t read_write_head_position_within_cluster;
    size_t read_write_head_position_cluster_number;
    size_t file_size;
    size_t first_cluster_number;
    FAT_FileSystem* fat_fs;

    void set_position(size_t position);
};

using file = perm_fh<FAT::FAT_file>;

class FAT_dir_iterator {
public:
    FAT_dir_iterator(FAT_FileSystem* fat_fs, int initial_stack_size = 32);
    drit_status push(const char* directory_name, size_t cluster_number = SIZE_MAX);
    void pop();
    size_t depth();
    string operator[](size_t index);
    smallptr<filehandler> open_file(const char* file_name, int mode, size_t cluster_number = SIZE_MAX);

private:
    FAT_dir_entry_with_full_name stack[32];
    unsigned int stack_pointer;
    unsigned int stack_size;
    bool is_root;

    FAT_FileSystem* const fat_fs;
    unsigned short first_cluster_of_current_directory;
};

using dit = coh_dit<FAT::FAT_dir_iterator>;
class FAT_FileSystem : public fs {
friend FAT_dir_iterator;
friend FAT_file;
public:
    FAT_FileSystem(filehandler* fh, bool verbose = false);
    dit* get_iterator() { return new dit(this);}
    
    void read(void* buffer, size_t size);
    void write(const void* buffer, size_t size);

private:
    //dit* dit_ptr;

    size_t FATSz;
    filehandler* fh;
    short FATType;

    uint16_t BPB_RsvdSecCnt;
    uint8_t BPB_SecPerClus;
    uint16_t BPB_BytsPerSec;
    uint16_t BPB_RootEntCnt;
    uint8_t BPB_NumFATs;
    uint32_t BPB_FATSz;
    uint32_t BPB_RootClus;

    size_t cluster_size;
    size_t number_of_entries_per_cluster;
    size_t number_of_clusters;
    size_t number_of_FAT_entries;
    size_t root_directory_begin_address_for_FAT12_and_FAT16;
    size_t first_data_sector;

    uint32_t BAD_CLUSTER;
    uint32_t LAST_CLUSTER;

    size_t cluster_number_to_address(size_t cluster_number);
    size_t find_fat_entry(size_t cluster_number, unsigned int FatNumber = 1);
    size_t cluster_number_to_fat_entry_index(size_t cluster_number, unsigned int FatNumber = 1);
    size_t find_free_cluster(size_t last_cluster_number);
    void write_fat_entry(size_t cluster_number, size_t value, unsigned int FatNumber = 1);
};  

using filesystem = FAT_FileSystem;

}
