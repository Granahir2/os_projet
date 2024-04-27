#include "kstdlib/stdexcept.hh"
#include "kernel/fs/fs.hh"
#include "fat.hh"

// Implmenetation refers to Microsoft's Documentation of FAT system (c) Microsoft Corporation 2004
// https://academy.cba.mit.edu/classes/networking_communications/SD/FAT.pdf

namespace FAT {

FAT_FileSystem::FAT_FileSystem(filehandler* fh)
{
    this->fh = fh;

    unsigned int BPB_RsvdSecCnt;
    unsigned int BPB_TotSec16;
    unsigned int BPB_FATSz16;
    unsigned int BPB_TotSec32;

    fh->seek(0x0B, SET);
    fh->read(&this->BPB_BytsPerSec, 2);
    fh->read(&this->BPB_SecPerClus, 1);
    fh->read(&BPB_RsvdSecCnt, 2);
    fh->read(&this->BPB_NumFATs, 1);
    fh->read(&this->BPB_RootEntCnt, 2);    
    fh->read(&BPB_TotSec16, 2);
    fh->seek(0x16, SET);
    fh->read(&BPB_FATSz16, 2);
    fh->seek(0x20, SET);
    fh->read(&BPB_TotSec32, 4);

    if (this->BPB_BytsPerSec != 512 || 
        this->BPB_BytsPerSec != 1024 || 
        this->BPB_BytsPerSec != 2048 || 
        this->BPB_BytsPerSec != 4096)
        throw logic_error("Corrupted FAT filesystem: Invalid BPB_BytsPerSec\n");
    if (BPB_RsvdSecCnt == 0)
        throw logic_error("Corrupted FAT filesystem: BPB_RsvdSecCnt is zero\n");
    if (this->BPB_NumFATs == 0)
        throw logic_error("Corrupted FAT filesystem: BPB_NumFATs is zero\n");
    if (BPB_TotSec16 != 0)
    {
        // Supposedly FAT12 or FAT16
        if (BPB_TotSec32 != 0)
            throw logic_error("Corrupted FAT filesystem: Both BPB_TotSec16 and BPB_TotSec32 are non-zero\n");
        if (BPB_FATSz16 == 0)
            throw logic_error("Corrupted FAT filesystem: BPB_FATSz16 is zero in FAT12/FAT16\n");
        
        this->number_of_clusters = (BPB_TotSec16 - (BPB_RsvdSecCnt + (this->BPB_NumFATs * BPB_FATSz16))) / BPB_SecPerClus;
        this->FATSz = BPB_FATSz16;
        if (this->BPB_RootEntCnt < 4085)
            this->FATType = FAT12;
        else if (this->BPB_RootEntCnt < 65525)
            this->FATType = FAT16;
        else 
            throw logic_error("Corrupted FAT filesystem: BPB_RootEntCnt is too large for FAT12/FAT16\n");
        
        throw logic_error("FAT12/FAT16 not supported\n");
    }
    else
    {
        // Supposedly FAT32
        if (BPB_TotSec32 == 0)
            throw logic_error("Corrupted FAT filesystem: Both BPB_TotSec16 and BPB_TotSec32 are zero\n");
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
        this->cluster_size = this->BPB_SecPerClus * this->BPB_BytsPerSec;
        this->number_of_entries_per_cluster = this->cluster_size / 32;
        this->number_of_clusters = (BPB_TotSec32 - (BPB_RsvdSecCnt + (this->BPB_NumFATs * BPB_FATSz32))) / BPB_SecPerClus;
    }

    this->BPB_BytsPerSec = BPB_BytsPerSec;
    this->BPB_SecPerClus = BPB_SecPerClus;
    this->BPB_NumFATs = BPB_NumFATs;
    this->BPB_RootEntCnt = BPB_RootEntCnt;
}

size_t FAT_FileSystem::cluster_number_to_address(size_t cluster_number)
{
    return ((cluster_number - 2) * BPB_SecPerClus) + (BPB_RsvdSecCnt + (BPB_NumFATs * FATSz));
}

size_t FAT_FileSystem::find_fat_entry(size_t cluster_number, unsigned int FatNumber)
{   
    // Section 4.1: Determination of FAT entry for a cluster number
    size_t FAT_Offset;
    size_t FATEntry;
    size_t ThisFATSecNum;
    size_t ThisFATEntOffset;
    if (FATType == FAT12)
    {
        FAT_Offset = cluster_number + (cluster_number / 2);
        ThisFATSecNum = BPB_RsvdSecCnt + (FAT_Offset / BPB_BytsPerSec) + (FatNumber - 1) * FATSz;
        ThisFATEntOffset = FAT_Offset % BPB_BytsPerSec;
        
        if (ThisFATEntOffset == (BPB_BytsPerSec - 1))
        {
            // FAT entry spans two sectors
            ThisFATSecNum = BPB_RsvdSecCnt + (FAT_Offset / BPB_BytsPerSec);
            ThisFATEntOffset = FAT_Offset % BPB_BytsPerSec;
            fh->seek(ThisFATEntOffset, SET);
            fh->read(&FATEntry, 1);
            fh->seek(1, CUR);
            unsigned char FATEntry2;
            fh->read(&FATEntry2, 1);
            FATEntry = (FATEntry2 << 4) | (FATEntry >> 4);
        }

        fh->seek(ThisFATSecNum * BPB_BytsPerSec + ThisFATEntOffset, SET);
        fh->read(&FATEntry, 2);
        if (cluster_number & 1)
            FATEntry >>= 4;
        else 
            FATEntry &= 0x0FFF;
        return FATEntry;
    }
    else if (FATType == FAT16) 
    {
        FAT_Offset = cluster_number * 2;
        ThisFATSecNum = BPB_RsvdSecCnt + (FAT_Offset / BPB_BytsPerSec) + (FatNumber - 1) * FATSz;
        ThisFATEntOffset = FAT_Offset % BPB_BytsPerSec;

        fh->seek(ThisFATSecNum * BPB_BytsPerSec + ThisFATEntOffset, SET);
        fh->read(&FATEntry, 2);
        return FATEntry;
    }
    else if (FATType == FAT32)
    {
        FAT_Offset = cluster_number * 4;
        ThisFATSecNum = BPB_RsvdSecCnt + (FAT_Offset / BPB_BytsPerSec) + (FatNumber - 1) * FATSz;
        ThisFATEntOffset = FAT_Offset % BPB_BytsPerSec;

        fh->seek(ThisFATSecNum * BPB_BytsPerSec + ThisFATEntOffset, SET);
        fh->read(&FATEntry, 4);
        FATEntry &= 0x0FFFFFFF;
        return FATEntry;
    }
    else throw logic_error("Invalid FAT type\n");
}

}
