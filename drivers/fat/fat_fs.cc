#include "kstdlib/stdexcept.hh"
#include "kernel/fs/fs.hh"
#include "fat.hh"
#include "kstdlib/cstdio.hh"

// Implementation refers to Microsoft's Documentation of FAT system (c) Microsoft Corporation 2004
// https://academy.cba.mit.edu/classes/networking_communications/SD/FAT.pdf

namespace FAT {

FAT_FileSystem::FAT_FileSystem(filehandler* fh, bool verbose)
{
    this->fh = fh;

    uint16_t BPB_TotSec16; // Seems to make more sense given the reads ?
    uint16_t BPB_FATSz16;
    uint32_t BPB_TotSec32;

    fh->seek(0x0B, SET);
    fh->read(&this->BPB_BytsPerSec, 2);
    fh->read(&this->BPB_SecPerClus, 1);
    fh->read(&this->BPB_RsvdSecCnt, 2);
    fh->read(&this->BPB_NumFATs, 1);
    fh->read(&this->BPB_RootEntCnt, 2);    
    fh->read(&BPB_TotSec16, 2);
    fh->seek(0x16, SET);
    fh->read(&BPB_FATSz16, 2);
    fh->seek(0x20, SET);
    fh->read(&BPB_TotSec32, 4);

    if (this->BPB_BytsPerSec != 512 &&
        this->BPB_BytsPerSec != 1024 && 
        this->BPB_BytsPerSec != 2048 && 
        this->BPB_BytsPerSec != 4096)
        throw logic_error("Corrupted FAT filesystem: Invalid BPB_BytsPerSec\n");
    if (this->BPB_RsvdSecCnt == 0)
        throw logic_error("Corrupted FAT filesystem: BPB_RsvdSecCnt is zero\n");
    if (this->BPB_NumFATs == 0)
        throw logic_error("Corrupted FAT filesystem: BPB_NumFATs is zero\n");
    if (BPB_TotSec32 != 0 && BPB_TotSec16 != 0)
        throw logic_error("Corrupted FAT filesystem: Both BPB_TotSec16 and BPB_TotSec32 are non-zero\n");
    if (BPB_TotSec32 == 0 && BPB_TotSec16 == 0)
        throw logic_error("Corrupted FAT filesystem: Both BPB_TotSec16 and BPB_TotSec32 are zero\n");
    if (BPB_FATSz16 != 0) 
    {
        // Supposedly FAT12 or FAT16
        if (this->BPB_RootEntCnt == 0)
            throw logic_error("Corrupted FAT filesystem: BPB_RootEntCnt is zero in FAT12/FAT16\n");
        
        this->number_of_clusters = ((BPB_TotSec16 ? BPB_TotSec16 : BPB_TotSec32) - (this->BPB_RsvdSecCnt + (this->BPB_NumFATs * BPB_FATSz16))) / BPB_SecPerClus;
        this->FATSz = BPB_FATSz16;
        if (this->number_of_clusters < 4085)
            this->FATType = FAT12, 
            this->BAD_CLUSTER = 0x0FF7,
            this->LAST_CLUSTER = 0x0FFF;
        else if (this->number_of_clusters < 65525)
            this->FATType = FAT16, 
            this->BAD_CLUSTER = 0xFFF7,
            this->LAST_CLUSTER = 0xFFFF;
        else 
            throw logic_error("Corrupted FAT filesystem: The number of clusters is too large for FAT12/FAT16\n");
        
        this->root_directory_begin_address_for_FAT12_and_FAT16 = 
            this->BPB_RsvdSecCnt * BPB_BytsPerSec + // FAT begin
            (this->BPB_NumFATs * BPB_FATSz16 * BPB_BytsPerSec); // FAT size
        this->BPB_RootClus = 0;
        //throw logic_error("FAT12/FAT16 not supported\n");
    }
    else
    {
        // Supposedly FAT32
        if (BPB_FATSz16 != 0)
            throw logic_error("Corrupted FAT filesystem: BPB_FATSz16 is non-zero in FAT32\n");
        if (this->BPB_RootEntCnt != 0)
            throw logic_error("Corrupted FAT filesystem: BPB_RootEntCnt is non-zero in FAT32\n");
        unsigned int BPB_FATSz32;
        unsigned char BPB_FSVer;
        unsigned int BPB_BkBootSec;
        unsigned int BPB_Reserved[3];
        fh->seek(0x24, SET);
        fh->read(&BPB_FATSz32, 4);
        if (BPB_FATSz32 == 0)
            throw logic_error("Corrupted FAT filesystem: BPB_FATSz32 is zero in FAT32\n");

        fh->seek(0x2A, SET);
        fh->read(&BPB_FSVer, 2);
        if (BPB_FSVer != 0)
            throw logic_error("Corrupted FAT filesystem: BPB_FSVer is non-zero\n");

        fh->seek(0x2C, SET);
        fh->read(&this->BPB_RootClus, 4);

        fh->seek(0x32, SET);
        fh->read(&BPB_BkBootSec, 2);
        if (BPB_BkBootSec != 0 || BPB_BkBootSec != 6)
            throw logic_error("Corrupted FAT filesystem: BPB_BkBootSec is not 0 or 6\n");
        fh->read(BPB_Reserved, 3 * 4);
        if (BPB_Reserved[0] != 0 || BPB_Reserved[1] != 0 || BPB_Reserved[2] != 0)
            throw logic_error("Corrupted FAT filesystem: Reserved fields are not zero\n");

        this->FATSz = BPB_FATSz32;
        this->FATType = FAT32;

        // Calculate number of entries per cluster
        this->number_of_clusters = (BPB_TotSec32 - (this->BPB_RsvdSecCnt + (this->BPB_NumFATs * BPB_FATSz32))) / BPB_SecPerClus;
        this->BAD_CLUSTER = 0x0FFFFFF7;
        this->LAST_CLUSTER = 0x0FFFFFFF;
    }
    this->cluster_size = this->BPB_SecPerClus * this->BPB_BytsPerSec;
    this->number_of_entries_per_cluster = this->cluster_size / 32;

    this->BPB_BytsPerSec = BPB_BytsPerSec;
    this->BPB_SecPerClus = BPB_SecPerClus;
    this->BPB_NumFATs = BPB_NumFATs;
    this->BPB_RootEntCnt = BPB_RootEntCnt;
    this->number_of_FAT_entries = FATSz * BPB_BytsPerSec / 4;
    this->first_data_sector = this->BPB_RsvdSecCnt + (this->BPB_NumFATs * FATSz);
    if (this->FATType == FAT12 || this->FATType == FAT16)
        this->first_data_sector += ((this->BPB_RootEntCnt * 32 + (BPB_BytsPerSec - 1)) / BPB_BytsPerSec);
    if (this->FATType == FAT12)
        printf("FAT12 filesystem detected\n");
    else if (this->FATType == FAT16)
        printf("FAT16 filesystem detected\n");
    else if (this->FATType == FAT32)
        printf("FAT32 filesystem detected\n");
    else
        throw logic_error("Invalid FAT type\n");

    if (verbose)
    {
        // Print the information about the filesystem
        printf("Sector size: %d\n", this->BPB_BytsPerSec);
        printf("Sectors per cluster: %d\n", this->BPB_SecPerClus);
        printf("Number of FATs: %d\n", this->BPB_NumFATs);
        printf("Number of entries in the root directory: %d\n", this->BPB_RootEntCnt);
        printf("Number of sectors in the filesystem: %d\n", BPB_TotSec32);
        printf("Number of sectors per FAT: %d\n", this->FATSz);
        printf("Number of clusters: %d\n", this->number_of_clusters);
        printf("Number of FAT entries: %d\n", this->number_of_FAT_entries);
        printf("Root directory begin address: %d\n", this->root_directory_begin_address_for_FAT12_and_FAT16);
        printf("First cluster of the root directory: %d\n", this->BPB_RootClus);
        printf("First data sector: %d\n", this->first_data_sector);
    }
}

size_t FAT_FileSystem::cluster_number_to_address(size_t cluster_number)
{
    if (cluster_number < 2)
        throw logic_error("In function cluster_number_to_address(): Invalid cluster number\n");
    return ((cluster_number - 2) * BPB_SecPerClus + first_data_sector) * BPB_BytsPerSec;
}

size_t FAT_FileSystem::cluster_number_to_fat_entry_index(size_t cluster_number, unsigned int FatNumber)
{
    if (cluster_number < 2)
        throw logic_error("In function cluster_number_to_fat_entry_index(): Invalid cluster number\n");

    // Section 4.1: Determination of FAT entry for a cluster number
    size_t FAT_Offset;
    if (FATType == FAT12) FAT_Offset = cluster_number + (cluster_number / 2);
    else if (FATType == FAT16) FAT_Offset = cluster_number * 2;
    else if (FATType == FAT32) FAT_Offset = cluster_number * 4;
    else throw logic_error("Invalid FAT type\n");

    return FAT_Offset + (BPB_RsvdSecCnt + FatNumber * FATSz) * BPB_BytsPerSec;
}

size_t FAT_FileSystem::find_fat_entry(size_t cluster_number, unsigned int FatNumber)
{   
    if (cluster_number < 2)
        throw logic_error("In function find_fat_entry(): Invalid cluster number\n");
    
    // Section 4.1: Determination of FAT entry for a cluster number
    size_t FATEntry;
    
    fh->seek(cluster_number_to_fat_entry_index(cluster_number, FatNumber), SET);
    if (FATType == FAT12)
    {
        fh->read(&FATEntry, 2);
        if (cluster_number & 1)
            FATEntry >>= 4;
        else
            FATEntry &= 0x0FFF;
    }
    else if (FATType == FAT16) 
    {
        fh->read(&FATEntry, 2);
        return FATEntry;
    }
    else if (FATType == FAT32)
    {
        fh->read(&FATEntry, 4);
        FATEntry &= 0x0FFFFFFF;
    }
    else throw logic_error("Invalid FAT type\n");
    return FATEntry;
}

size_t FAT_FileSystem::find_free_cluster(size_t last_cluster_number) {
    // Start going through the FAT table
    size_t FATEntry = 0;
    size_t next_cluster_number = 0;
    fh->seek(BPB_RsvdSecCnt * BPB_BytsPerSec, SET);
    for (unsigned int i = 0; i < number_of_FAT_entries; i++)
    {
        if (FATType == FAT12)
        {
            if (i & 1)
            {
                fh->read(&FATEntry, 2);
                FATEntry >>= 4;
            }
            else
            {
                fh->read(&FATEntry, 2);
                FATEntry &= 0x0FFF;
            }
        }
        else if (FATType == FAT16)
        {
            fh->read(&FATEntry, 2);
        }
        else if (FATType == FAT32)
        {
            fh->read(&FATEntry, 4);
            FATEntry &= 0x0FFFFFFF;
        }
        else throw logic_error("Invalid FAT type\n");
        if (FATEntry == 0)
        {
            next_cluster_number = i;
            break;
        }
    }
    if (next_cluster_number != 0)
    {
        for (int i = 0; i < BPB_NumFATs; i++)
        {
            if (last_cluster_number != 0)
                write_fat_entry(last_cluster_number, next_cluster_number, i);
            write_fat_entry(next_cluster_number, LAST_CLUSTER, i);
        }
    }
    return next_cluster_number;
}

void FAT_FileSystem::write_fat_entry(size_t cluster_number, size_t FATEntry, unsigned int FatNumber)
{
    if (cluster_number < 2)
        throw logic_error("Invalid cluster number\n");
    size_t old_FAT_Entry;
    fh->seek(cluster_number_to_fat_entry_index(cluster_number, FatNumber), SET);
    if (FATType == FAT12)
    {
        if (cluster_number & 1)
        {
            FATEntry <<= 4;
            fh->read(&old_FAT_Entry, 2);
            old_FAT_Entry &= 0x000F;
        }
        else
        {
            FATEntry &= 0x0FFF;
            fh->read(&old_FAT_Entry, 2);
            old_FAT_Entry &= 0xF000;
        }
        FATEntry |= old_FAT_Entry;
        fh->seek(-2, CUR);
        fh->write(&FATEntry, 2);
    }
    else if (FATType == FAT16) 
    {
        fh->write(&FATEntry, 2);
    }
    else if (FATType == FAT32)
    {
        FATEntry &= 0x0FFFFFFF;
        fh->read(&old_FAT_Entry, 4);
        fh->seek(-4, CUR);
        old_FAT_Entry &= 0xF0000000;
        FATEntry |= old_FAT_Entry;
        fh->write(&FATEntry, 4);
    }
    else throw logic_error("Invalid FAT type\n");
}

}
