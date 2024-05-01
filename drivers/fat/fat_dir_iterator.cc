#include "fat.hh"
#include "kstdlib/string.hh"
#include "kstdlib/cstdio.hh"

namespace FAT {

FAT_dir_iterator::FAT_dir_iterator(FAT_FileSystem* fat_fs, int initial_stack_size) :
        stack_pointer(0), 
        stack_size(initial_stack_size), 
        fat_fs(fat_fs), 
        first_cluster_of_current_directory(fat_fs->BPB_RootClus) {}

drit_status FAT_dir_iterator::push(const char* directory_name, size_t current_cluster) {
    if (stack_pointer == stack_size) 
        throw runtime_error("Stack overflow");
    
    FAT_dir_entry DirEntry;
    FAT_Long_File_Name_entry LFNEntry;
    uint8_t Attribute;

    // Buffer to contain long name
    char found_directory_Name [256];
    found_directory_Name[0] = '\0';
    int cnt = 0;
    int number_of_LFN_entries = -1;
    int current_checksum = -1;
    bool there_is_long_name = false;

    if (fat_fs->FATType == FAT32 || stack_pointer > 0) // If it is not root directory or FAT32
    {
        if (current_cluster == SIZE_MAX)
            current_cluster = this->first_cluster_of_current_directory;

        // Traverse through directory to find the directory entry corresponding to the directory name
        // and push it to the stack
        size_t start_address = fat_fs->cluster_number_to_address(current_cluster);
        fat_fs->fh->seek(start_address, SET);
        
        for (uint16_t i = 0; i < fat_fs->number_of_entries_per_cluster; i++)
        {
            // check attribute byte
            fat_fs->fh->seek(11, CUR);
            fat_fs->fh->read(&Attribute, 1);
            fat_fs->fh->seek(-12, CUR);

            // check if it is a directory or a LFN
            if ((number_of_LFN_entries > 0 && Attribute != ATTR_LONG_NAME) || 
                (Attribute == ATTR_LONG_NAME && number_of_LFN_entries == 0))
                throw runtime_error("Corrupted directory entry");
            else if (Attribute == ATTR_LONG_NAME)
            {
                // LFN

                fat_fs->fh->read(&LFNEntry, 32);
                // Sanity check
                if(number_of_LFN_entries == -1) // begin of LFN
                {
                    if (!(LFNEntry.LDIR_Ord & LAST_LONG_ENTRY))
                        throw runtime_error("Corrupted LFN entry: this is supposed to be the last entry");
                    number_of_LFN_entries = LFNEntry.LDIR_Ord & 0x1F, 
                    current_checksum = LFNEntry.LDIR_Chksum;
                    there_is_long_name = true;
                }
                else if (LFNEntry.LDIR_Ord & LAST_LONG_ENTRY)
                    throw runtime_error("Corrupted LFN entry: this is not supposed to be the last entry");
                else if (number_of_LFN_entries != LFNEntry.LDIR_Ord)
                    throw runtime_error("Corrupted LFN entry: sequence number mismatch");
                else if (current_checksum != LFNEntry.LDIR_Chksum)
                    throw runtime_error("Corrupted LFN entry: checksum mismatch");

                // Add the name to the buffer
                cnt = (number_of_LFN_entries-1) * 13;
                for (size_t j = 0; j < 5; j++)
                {
                    if (cnt >= 256) throw runtime_error("Corrupted LFN entry: name too long");
                    if (LFNEntry.LDIR_Name1[j] == 0xFFFF) break;
                    found_directory_Name[cnt++] = LFNEntry.LDIR_Name1[j];
                }
                for (size_t j = 0; j < 6; j++)
                {
                    if (cnt >= 256) throw runtime_error("Corrupted LFN entry: name too long");
                    if (LFNEntry.LDIR_Name2[j] == 0xFFFF) break;
                    found_directory_Name[cnt++] = LFNEntry.LDIR_Name2[j];
                }
                for (size_t j = 0; j < 2; j++)
                {
                    if (cnt >= 256) throw runtime_error("Corrupted LFN entry: name too long");
                    if (LFNEntry.LDIR_Name3[j] == 0xFFFF) break;
                    found_directory_Name[cnt++] = LFNEntry.LDIR_Name3[j];
                }
                number_of_LFN_entries--;
            }
            else
            {
                // Directory
                // Reset LFN variables
                number_of_LFN_entries = -1;
                cnt = 0;

                fat_fs->fh->read(&DirEntry, 32);

                if (DirEntry.DIR_Name[0] == 0xE5) continue;
                if (DirEntry.DIR_Name[0] == 0x00) break;

                // Calculate checksum
                uint8_t checksum = 0;
                for (size_t j = 0; j < 11; j++)
                    checksum = ((checksum & 1) ? 0x80 : 0) + (checksum >> 1) + DirEntry.DIR_Name[j];
                if (there_is_long_name && checksum != current_checksum)
                    throw runtime_error("Corrupted directory entry: checksum mismatch");
                there_is_long_name = false;

                if (strcmp(found_directory_Name, directory_name) == 0)
                {
                    // Check if it is a directory
                    if (!(DirEntry.DIR_Attr & ATTR_DIRECTORY))
                        return FILE_ENTRY;
                        //throw runtime_error("Found a file with matching name, but it is not a directory");

                    // Found the directory
                    this->first_cluster_of_current_directory = DirEntry.DIR_FstClusLO + (DirEntry.DIR_FstClusHI << 16);
                    stack[stack_pointer++] = {DirEntry, basic_string(directory_name), fat_fs->fh->seek(-32, CUR)};
                    return DIR_ENTRY;
                }
            }
        }

        // Check if there is any more cluster
        size_t next_cluster = fat_fs->find_fat_entry(current_cluster);
        // If there is none, we throw error that directory is not found
        if (next_cluster >= fat_fs->LAST_CLUSTER || next_cluster == 0)
            return NP;
        //throw runtime_error("Directory not found");
        // Else, we set the current cluster to the next cluster, and repeat
        else { 
            return push(directory_name, next_cluster);
        }
    }
    else
    {
        fat_fs->fh->seek(fat_fs->root_directory_begin_address_for_FAT12_and_FAT16, SET);

        for (uint16_t i = 0; i < fat_fs->BPB_RootEntCnt; i++)
        {
            // check attribute byte
            fat_fs->fh->seek(11, CUR);
            fat_fs->fh->read(&Attribute, 1);
            fat_fs->fh->seek(-12, CUR);

            // check if it is a directory or a LFN
            if ((number_of_LFN_entries > 0 && Attribute != ATTR_LONG_NAME) || 
                (Attribute == ATTR_LONG_NAME && number_of_LFN_entries == 0))
                throw runtime_error("Corrupted directory entry");
            else if (Attribute == ATTR_LONG_NAME)
            {
                // LFN
                fat_fs->fh->read(&LFNEntry, 32);
                // Sanity check
                if(number_of_LFN_entries == -1) // begin of LFN
                {
                    number_of_LFN_entries = LFNEntry.LDIR_Ord & 0x1F, 
                    current_checksum = LFNEntry.LDIR_Chksum;
                    there_is_long_name = true;
                }
                else if (number_of_LFN_entries != LFNEntry.LDIR_Ord)
                    throw runtime_error("Corrupted LFN entry: sequence number mismatch");
                else if (current_checksum != LFNEntry.LDIR_Chksum)
                    throw runtime_error("Corrupted LFN entry: checksum mismatch");

                // Add the name to the buffer
                cnt = (number_of_LFN_entries-1) * 13;
                for (size_t j = 0; j < 5; j++)
                {
                    if (cnt >= 256) throw runtime_error("Corrupted LFN entry: name too long");
                    if (LFNEntry.LDIR_Name1[j] == 0xFFFF) break;
                    found_directory_Name[cnt++] = LFNEntry.LDIR_Name1[j];
                }
                for (size_t j = 0; j < 6; j++)
                {
                    if (cnt >= 256) throw runtime_error("Corrupted LFN entry: name too long");
                    if (LFNEntry.LDIR_Name2[j] == 0xFFFF) break;
                    found_directory_Name[cnt++] = LFNEntry.LDIR_Name2[j];
                }
                for (size_t j = 0; j < 2; j++)
                {
                    if (cnt >= 256) throw runtime_error("Corrupted LFN entry: name too long");
                    if (LFNEntry.LDIR_Name3[j] == 0xFFFF) break;
                    found_directory_Name[cnt++] = LFNEntry.LDIR_Name3[j];
                }
                number_of_LFN_entries--;
            }
            else
            {
                // Directory
                number_of_LFN_entries = -1;
                fat_fs->fh->read(&DirEntry, 32);

                if (DirEntry.DIR_Name[0] == 0xE5) continue;
                if (DirEntry.DIR_Name[0] == 0x00) break;

                // Calculate checksum
                uint8_t checksum = 0;
                for (size_t j = 0; j < 11; j++)
                    checksum = ((checksum & 1) ? 0x80 : 0) + (checksum >> 1) + DirEntry.DIR_Name[j];
                if (there_is_long_name && checksum != current_checksum)
                    throw runtime_error("Corrupted directory entry: checksum mismatch");
                there_is_long_name = false;
                
                if (strcmp(found_directory_Name, directory_name) == 0)
                {
                    // Check if it is a directory
                    if (!(DirEntry.DIR_Attr & ATTR_DIRECTORY))
                        return FILE_ENTRY;
                        //throw runtime_error("Found a file with matching name, but it is not a directory");

                    // Found the directory
                    this->first_cluster_of_current_directory = DirEntry.DIR_FstClusLO + (DirEntry.DIR_FstClusHI << 16);
                    stack[stack_pointer++] = {DirEntry, basic_string(directory_name), fat_fs->fh->seek(-32, CUR)};
                    return DIR_ENTRY;
                }
            }
        }
        // Directory not found
        return NP;
    }
}

void FAT_dir_iterator::pop() {
    if (stack_pointer == 0)
        throw runtime_error("Stack underflow");
    stack_pointer--;
    if (stack_pointer == 0)
        first_cluster_of_current_directory = fat_fs->BPB_RootClus;
    else
        first_cluster_of_current_directory = stack[stack_pointer - 1].entry.DIR_FstClusLO + (stack[stack_pointer - 1].entry.DIR_FstClusHI << 16);
}

size_t FAT_dir_iterator::depth() {
    return stack_pointer;
}

string FAT_dir_iterator::operator[](size_t index) {
    if (index >= stack_pointer)
        throw runtime_error("Index out of bounds");
    return stack[index].full_name;
}

smallptr<file> FAT_dir_iterator::open_file(const char* file_name, int mode, size_t current_cluster) {
    // Traverse through directory to find the file
    FAT_dir_entry DirEntry;
    FAT_Long_File_Name_entry LFNEntry;
    uint8_t Attribute;
    bool there_is_long_name = false;

    // Buffer to contain long name
    char found_directory_Name [256];
    found_directory_Name[0] = '\0';
    int cnt = 0;
    int number_of_LFN_entries = -1;
    int current_checksum = -1;
    
    if (fat_fs->FATType == FAT32 || stack_pointer > 0) // If it is not root directory or FAT32
    {
        if (current_cluster == SIZE_MAX)
            current_cluster = this->first_cluster_of_current_directory;

        size_t start_address = fat_fs->cluster_number_to_address(current_cluster);
        fat_fs->fh->seek(start_address, SET);
        for (uint16_t i = 0; i < fat_fs->number_of_entries_per_cluster; i++)
        {
            // check attribute byte
            fat_fs->fh->seek(11, CUR);
            fat_fs->fh->read(&Attribute, 1);
            fat_fs->fh->seek(-12, CUR);

            // check if it is a directory or a LFN
            if ((number_of_LFN_entries > 0 && Attribute != ATTR_LONG_NAME) || 
                (Attribute == ATTR_LONG_NAME && number_of_LFN_entries == 0))
                throw runtime_error("Corrupted directory entry");
            else if (Attribute == ATTR_LONG_NAME)
            {
                // LFN

                fat_fs->fh->read(&LFNEntry, 32);
                // Sanity check
                if(number_of_LFN_entries == -1) // begin of LFN
                {
                    if (!(LFNEntry.LDIR_Ord & LAST_LONG_ENTRY))
                        throw runtime_error("Corrupted LFN entry: this is supposed to be the last entry");
                    number_of_LFN_entries = LFNEntry.LDIR_Ord & 0x1F, 
                    current_checksum = LFNEntry.LDIR_Chksum;
                    cnt = (number_of_LFN_entries-1) * 13;
                    there_is_long_name = true;
                }
                else if (number_of_LFN_entries != LFNEntry.LDIR_Ord)
                    throw runtime_error("Corrupted LFN entry: sequence number mismatch");
                else if (current_checksum != LFNEntry.LDIR_Chksum)
                    throw runtime_error("Corrupted LFN entry: checksum mismatch");

                // Add the name to the buffer
                cnt = (number_of_LFN_entries-1) * 13;
                for (size_t j = 0; j < 5; j++)
                {
                    if (cnt >= 256) throw runtime_error("Corrupted LFN entry: name too long");
                    if (LFNEntry.LDIR_Name1[j] == 0xFFFF) break;
                    found_directory_Name[cnt++] = LFNEntry.LDIR_Name1[j];
                }
                for (size_t j = 0; j < 6; j++)
                {
                    if (cnt >= 256) throw runtime_error("Corrupted LFN entry: name too long");
                    if (LFNEntry.LDIR_Name2[j] == 0xFFFF) break;
                    found_directory_Name[cnt++] = LFNEntry.LDIR_Name2[j];
                }
                for (size_t j = 0; j < 2; j++)
                {
                    if (cnt >= 256) throw runtime_error("Corrupted LFN entry: name too long");
                    if (LFNEntry.LDIR_Name3[j] == 0xFFFF) break;
                    found_directory_Name[cnt++] = LFNEntry.LDIR_Name3[j];
                }
                number_of_LFN_entries--;
            }
            else
            {
                // Directory
                // Reset LFN variables
                number_of_LFN_entries = -1;
                cnt = 0;
                
                fat_fs->fh->read(&DirEntry, 32);

                // Calculate checksum
                uint8_t checksum = 0;
                for (size_t j = 0; j < 11; j++)
                    checksum = ((checksum & 1) ? 0x80 : 0) + (checksum >> 1) + DirEntry.DIR_Name[j];
                if (there_is_long_name && checksum != current_checksum)
                    throw runtime_error("Corrupted directory entry: checksum mismatch");
                there_is_long_name = false;

                if (strcmp(found_directory_Name, file_name) == 0)
                {
                    // Check if it is a file
                    if (DirEntry.DIR_Attr & ATTR_DIRECTORY)
                        throw runtime_error("Found a directory with matching name, but it is not a file");
                    
                    // Found the file
                    size_t file_size = DirEntry.DIR_FileSize;
                    size_t first_cluster = DirEntry.DIR_FstClusLO + (DirEntry.DIR_FstClusHI << 16);
                    return new file(mode, fat_fs/*->fh*/, file_size, first_cluster, fat_fs->fh->seek(-32, CUR));
                }
            }
        }

        // Check if there is any more cluster
        size_t next_cluster = fat_fs->find_fat_entry(current_cluster);
        // If there is none, we throw error that file is not found
        if (next_cluster >= fat_fs->LAST_CLUSTER || next_cluster == 0)
            throw runtime_error("File not found");
        // Else, we set the current cluster to the next cluster, and repeat
        else 
           return open_file(file_name, mode, next_cluster);
    }
    else 
    {
        fat_fs->fh->seek(fat_fs->root_directory_begin_address_for_FAT12_and_FAT16, SET);

        for (uint16_t i = 0; i < fat_fs->BPB_RootEntCnt; i++)
        {
            // check attribute byte
            fat_fs->fh->seek(11, CUR);
            fat_fs->fh->read(&Attribute, 1);
            fat_fs->fh->seek(-12, CUR);

            // check if it is a directory or a LFN
            if ((number_of_LFN_entries > 0 && Attribute != ATTR_LONG_NAME) || 
                (Attribute == ATTR_LONG_NAME && number_of_LFN_entries == 0))
                throw runtime_error("Corrupted directory entry");
            else if (Attribute == ATTR_LONG_NAME)
            {
                // LFN

                fat_fs->fh->read(&LFNEntry, 32);
                // Sanity check
                if(number_of_LFN_entries == -1) // begin of LFN
                {
                    if (!(LFNEntry.LDIR_Ord & LAST_LONG_ENTRY))
                        throw runtime_error("Corrupted LFN entry: this is supposed to be the last entry");
                    number_of_LFN_entries = LFNEntry.LDIR_Ord & 0x1F, 
                    current_checksum = LFNEntry.LDIR_Chksum;
                }
                else if (number_of_LFN_entries != LFNEntry.LDIR_Ord)
                    throw runtime_error("Corrupted LFN entry: sequence number mismatch");
                else if (current_checksum != LFNEntry.LDIR_Chksum)
                    throw runtime_error("Corrupted LFN entry: checksum mismatch");

                // Add the name to the buffer
                cnt = (number_of_LFN_entries-1) * 13;
                for (size_t j = 0; j < 5; j++)
                {
                    if (cnt >= 256) throw runtime_error("Corrupted LFN entry: name too long");
                    if (LFNEntry.LDIR_Name1[j] == 0xFFFF) break;
                    found_directory_Name[cnt++] = LFNEntry.LDIR_Name1[j];
                }
                for (size_t j = 0; j < 6; j++)
                {
                    if (cnt >= 256) throw runtime_error("Corrupted LFN entry: name too long");
                    if (LFNEntry.LDIR_Name2[j] == 0xFFFF) break;
                    found_directory_Name[cnt++] = LFNEntry.LDIR_Name2[j];
                }
                for (size_t j = 0; j < 2; j++)
                {
                    if (cnt >= 256) throw runtime_error("Corrupted LFN entry: name too long");
                    if (LFNEntry.LDIR_Name3[j] == 0xFFFF) break;
                    found_directory_Name[cnt++] = LFNEntry.LDIR_Name3[j];
                }
                number_of_LFN_entries--;
            }
            else
            {
                // Directory
                number_of_LFN_entries = -1;
                fat_fs->fh->read(&DirEntry, 32);

                // Calculate checksum
                uint8_t checksum = 0;
                for (size_t j = 0; j < 11; j++)
                    checksum = ((checksum & 1) ? 0x80 : 0) + (checksum >> 1) + DirEntry.DIR_Name[j];
                if (checksum != current_checksum)
                    throw runtime_error("Corrupted directory entry: checksum mismatch");

                if (strcmp(found_directory_Name, file_name) == 0)
                {
                    // Check if it is a file
                    if (DirEntry.DIR_Attr & ATTR_DIRECTORY)
                        throw runtime_error("Found a directory with matching name, but it is not a file");
                    
                    // Found the file
                    size_t file_size = DirEntry.DIR_FileSize;
                    size_t first_cluster = DirEntry.DIR_FstClusLO + (DirEntry.DIR_FstClusHI << 16);
                    return new file(mode, fat_fs/*->fh*/, file_size, first_cluster, fat_fs->fh->seek(-32, CUR));
                }
            }
        }
        // File not found
        throw runtime_error("File not found");
    }
}

}
