#include "fat.hh"
#include "kstdlib/string.hh"
#include "kstdlib/cstdio.hh"

namespace FAT {

FAT_dir_iterator::FAT_dir_iterator(FAT_FileSystem* fat_fs, int initial_stack_size) :
        stack_pointer(0), 
        stack_size(initial_stack_size), 
        short_file_name_counter(0),
        fat_fs(fat_fs), 
        first_cluster_of_current_directory(fat_fs->BPB_RootClus) {}

drit_status FAT_dir_iterator::push(const char* directory_name, size_t current_cluster) {
    locktoken lt = fat_fs->acquire_lock();
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

                    // Found the directory
                    this->first_cluster_of_current_directory = DirEntry.DIR_FstClusLO + (DirEntry.DIR_FstClusHI << 16);
                    stack[stack_pointer++] = {DirEntry, basic_string(directory_name), (size_t)fat_fs->fh->seek(-32, CUR)};
                    return DIR_ENTRY;
                }
            }
        }

        // Check if there is any more cluster
        size_t next_cluster = fat_fs->find_fat_entry(current_cluster);
        // If there is none, we throw error that directory is not found
        if (next_cluster >= fat_fs->LAST_CLUSTER || next_cluster == 0)
            return NP;
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

                    // Found the directory
                    this->first_cluster_of_current_directory = DirEntry.DIR_FstClusLO + (DirEntry.DIR_FstClusHI << 16);
                    stack[stack_pointer++] = {DirEntry, basic_string(directory_name), (size_t)fat_fs->fh->seek(-32, CUR)};
                    return DIR_ENTRY;
                }
            }
        }
        // Directory not found
        return NP;
    }
}

void FAT_dir_iterator::pop() {
    locktoken lt = fat_fs->acquire_lock();
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
    locktoken lt = fat_fs->acquire_lock();
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
                if (DirEntry.DIR_Name[0] == 0xE5) continue;
                if (DirEntry.DIR_Name[0] == 0x00) break;

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
                    return new file(mode, fat_fs, file_size, first_cluster, fat_fs->fh->seek(-32, CUR));
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
                if (DirEntry.DIR_Name[0] == 0xE5) continue;
                if (DirEntry.DIR_Name[0] == 0x00) break;


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
                    return new file(mode, fat_fs, file_size, first_cluster, fat_fs->fh->seek(-32, CUR));
                }
            }
        }
        // File not found
        throw runtime_error("File not found");
    }
}

dirlist_token FAT_dir_iterator::list(size_t current_cluster, dirlist_token* filename_list_head, dirlist_token* filename_list_tail)
{
    locktoken lt = fat_fs->acquire_lock();
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
                if (DirEntry.DIR_Name[0] == 0xE5) continue;
                if (DirEntry.DIR_Name[0] == 0x00) break;

                // Calculate checksum
                uint8_t checksum = 0;
                for (size_t j = 0; j < 11; j++)
                    checksum = ((checksum & 1) ? 0x80 : 0) + (checksum >> 1) + DirEntry.DIR_Name[j];
                if (there_is_long_name && checksum != current_checksum)
                    throw runtime_error("Corrupted directory entry: checksum mismatch");
                there_is_long_name = false;

                // Add new file name to the list
                if (strcmp(found_directory_Name, ".") == 0 || strcmp(found_directory_Name, "..") == 0
			|| strcmp(found_directory_Name, "") == 0)
                {
                    // Skip . and ..
                    continue;
                }
                dirlist_token* new_token = new dirlist_token;
                new_token->name = basic_string(found_directory_Name);
                new_token->is_directory = DirEntry.DIR_Attr & ATTR_DIRECTORY;
                new_token->file_size = DirEntry.DIR_FileSize;
                if (filename_list_head == nullptr)
                {
                    filename_list_head = new_token;
                } else {
                    new_token->next.ptr = filename_list_head;
                    filename_list_head = new_token;
                }
            }
        }

        // Check if there is any more cluster
        size_t next_cluster = fat_fs->find_fat_entry(current_cluster);
        // If there is none, we throw error that file is not found
        if (next_cluster >= fat_fs->LAST_CLUSTER || next_cluster == 0)
        {
		if(filename_list_head == nullptr) // Empty directory case
			throw empty_directory();
            dirlist_token head = {filename_list_head->name, filename_list_head->is_directory, filename_list_head->next.ptr};
            filename_list_head->next.ptr = nullptr;
            delete filename_list_head;
            return head;
        }
        // Else, we set the current cluster to the next cluster, and repeat
        else 
           return list(next_cluster, filename_list_head, filename_list_tail);
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
                if (DirEntry.DIR_Name[0] == 0xE5) continue;
                if (DirEntry.DIR_Name[0] == 0x00) break;


                // Calculate checksum
                uint8_t checksum = 0;
                for (size_t j = 0; j < 11; j++)
                    checksum = ((checksum & 1) ? 0x80 : 0) + (checksum >> 1) + DirEntry.DIR_Name[j];
                if (checksum != current_checksum)
                    throw runtime_error("Corrupted directory entry: checksum mismatch");

                // Add new file name to the list
                if (strcmp(found_directory_Name, ".") == 0 || strcmp(found_directory_Name, "..") == 0
			|| strcmp(found_directory_Name, "") == 0)
                {
                    // Skip . and ..
                    continue;
                }
                dirlist_token* new_token = new dirlist_token;
                new_token->name = basic_string(found_directory_Name);
                new_token->is_directory = DirEntry.DIR_Attr & ATTR_DIRECTORY;
                new_token->file_size = DirEntry.DIR_FileSize;
                if (filename_list_head == nullptr)
                {
                    filename_list_head = new_token;
                }
                else
                {
                    new_token->next.ptr = filename_list_head;
                    filename_list_head = new_token;
                }
            }
        }

	if(filename_list_head == nullptr) // Empty directory case
		throw empty_directory();

        dirlist_token head = {filename_list_head->name, filename_list_head->is_directory, filename_list_head->next.ptr};
        filename_list_head->next.ptr = nullptr;
        delete filename_list_head;
        return head;
    }
}

//*
void FAT_dir_iterator::create_file(const char* file_name, bool is_dir, size_t current_cluster, bool checked) 
{
    locktoken lt = fat_fs->acquire_lock();
    // First, we have to make sure that no already existing file or folder has the same name
    if (not checked)
    {
        drit_status check = push(file_name, 0);
        if (check != NP) 
        {
            pop();
            throw logic_error("There have been files or folders with the same name.");
        }
    }

    // We calculate the directory entry
    string s = basic_string(file_name);
    uint16_t number_of_LFN_entries = (s.length() + 12) / 13;

    // Finally, we need to find space in the directory folder to save these entries
    // Which is the pain in the butt

    uint8_t empty_entries_count = 0;
    uint8_t beginning_of_empty_entries = 0;
    uint8_t first_name_byte;
    
    if (fat_fs->FATType == FAT32 || stack_pointer > 0) // If it is not root directory or FAT32
    {
        if (current_cluster == SIZE_MAX)
            current_cluster = this->first_cluster_of_current_directory;

        size_t start_address = fat_fs->cluster_number_to_address(current_cluster);
        fat_fs->fh->seek(start_address, SET);
        for (uint16_t i = 0; i < fat_fs->number_of_entries_per_cluster; i++)
        {
            // check the first byte
            fat_fs->fh->read(&first_name_byte, 1);

            // check if it is a directory or a LFN
            if (first_name_byte == 0xe5)
            {
                if (empty_entries_count == 0) beginning_of_empty_entries = start_address + i*fat_fs->cluster_size;
                empty_entries_count++;
                if (empty_entries_count >= number_of_LFN_entries + 1) break;
            }
            else if (first_name_byte == 0)
            {
                if (fat_fs->number_of_entries_per_cluster >= (size_t)(number_of_LFN_entries + i + 1))
                {
                    empty_entries_count = number_of_LFN_entries + 1;
                    beginning_of_empty_entries = start_address + i*fat_fs->cluster_size;
                }
                else
                {
                    empty_entries_count = fat_fs->number_of_entries_per_cluster - i;
                }
                break;
            }
            else empty_entries_count = 0;
        }

        // If there is not enough space, we move to the next cluster
        if (empty_entries_count < number_of_LFN_entries + 1)
        {
            // Check if there is any more cluster
            size_t next_cluster = fat_fs->find_fat_entry(current_cluster);
            // If there is none, we allocate a new cluster
            if (next_cluster >= fat_fs->LAST_CLUSTER || next_cluster == 0) next_cluster = fat_fs->find_free_cluster(current_cluster);
            create_file(file_name, is_dir, next_cluster, true);
        }
        else 
        {
            // But if there is enough space, we put the entries
            FAT_dir_entry DIR_ENTRY;
            uint8_t short_file_name_length = s.length();
            for (uint8_t i = 0; i < 11; i++) DIR_ENTRY.DIR_Name[i] = 0;
            if (is_dir) 
            {
                if (short_file_name_length > 6) short_file_name_length = 6;
                for (uint8_t i = 0; i < short_file_name_length; i++) DIR_ENTRY.DIR_Name[i] = s[i];
            }
            else 
            {
                // We need to take care of the extension in this case
                uint8_t last_dot = short_file_name_length - 1;
                while(last_dot && s[last_dot] != '.') last_dot--;

                short_file_name_length = last_dot;
                if (short_file_name_length > 6) short_file_name_length = 6;
                for (uint8_t i = 0; i < short_file_name_length; i++) DIR_ENTRY.DIR_Name[i] = s[i];

                short_file_name_length = s.length() - last_dot;
                if (short_file_name_length > 3) short_file_name_length = 3;
                for (uint8_t i = 0; i < short_file_name_length; i++) DIR_ENTRY.DIR_Name[i + 9] = s[last_dot + i];
            }
            DIR_ENTRY.DIR_Name[7] = '~';
            DIR_ENTRY.DIR_Name[8] = short_file_name_counter++;

            if (is_dir) DIR_ENTRY.DIR_Attr = ATTR_DIRECTORY;
            DIR_ENTRY.DIR_NTRes = 0;

            // File/folder is initially empty
            DIR_ENTRY.DIR_FstClusHI = 0;
            DIR_ENTRY.DIR_FstClusLO = 0;

            // TODO: set the good create time

            // Then we calculate the checksum
            uint8_t checksum = 0;
            for (uint8_t j = 0; j < 11; j++)
                checksum = ((checksum & 1) ? 0x80 : 0) + (checksum >> 1) + DIR_ENTRY.DIR_Name[j];

            // Then we create the LFN entries for the file name.
            FAT_Long_File_Name_entry LFN[number_of_LFN_entries];
            size_t offset = 0;
            for (uint16_t i = 0; i < number_of_LFN_entries; i++)
            {
                LFN[i].LDIR_Ord = i + 1;
                LFN[i].LDIR_Attr = ATTR_LONG_NAME;
                LFN[i].LDIR_FstClusLO = 0;
                LFN[i].LDIR_Chksum = checksum;
                if (offset + 5 > s.length())
                {
                    for (uint8_t j = 0; j < s.length() - offset; j++) LFN[i].LDIR_Name1[j] = s[j + offset];
                    for (uint8_t j = s.length() - offset; j < 5; j++) LFN[i].LDIR_Name1[j] = 0;
                    for (uint8_t j = 0; j < 6; j++) LFN[i].LDIR_Name2[j] = 0;
                    for (uint8_t j = 0; j < 2; j++) LFN[i].LDIR_Name3[j] = 0;
                    break;
                }
                else
                {
                    for (uint8_t j = 0; j < 5; j++) LFN[i].LDIR_Name1[j] = s[j + offset];
                    offset += 5;
                }

                if (offset + 6 > s.length())
                {
                    for (uint8_t j = 0; j < s.length() - offset; j++) LFN[i].LDIR_Name2[j] = s[j + offset];
                    for (uint8_t j = s.length() - offset; j < 6; j++) LFN[i].LDIR_Name2[j] = 0;
                    for (uint8_t j = 0; j < 2; j++) LFN[i].LDIR_Name3[j] = 0;
                    break;
                }
                else
                {
                    for (uint8_t j = 0; j < 6; j++) LFN[i].LDIR_Name2[j] = s[j + offset];
                    offset += 6;
                }

                if (offset + 5 > s.length())
                {
                    for (uint8_t j = 0; j < 2; j++) LFN[i].LDIR_Name3[j] = s[j + offset];
                    for (uint8_t j = s.length() - offset; j < 2; j++) LFN[i].LDIR_Name3[j] = 0;
                    break;
                }
                else
                {
                    for (uint8_t j = 0; j < 2; j++) LFN[i].LDIR_Name3[j] = s[j + offset];
                    offset += 2;
                }
            }
            
            LFN[number_of_LFN_entries - 1].LDIR_Ord |= 0x40;

            fat_fs->fh->seek(beginning_of_empty_entries, SET);
            for (int i = number_of_LFN_entries - 1; i >= 0; i--)
            {
                fat_fs->fh->write(&LFN[i], sizeof(LFN[i]));
            }
            fat_fs->fh->write(&DIR_ENTRY, sizeof(DIR_ENTRY));
        }
    }
    else 
    {
        fat_fs->fh->seek(fat_fs->root_directory_begin_address_for_FAT12_and_FAT16, SET);

        for (uint16_t i = 0; i < fat_fs->BPB_RootEntCnt; i++)
        {
            // check the first byte
            fat_fs->fh->read(&first_name_byte, 1);

            // check if it is a directory or a LFN
            if (first_name_byte == 0xe5)
            {
                if (empty_entries_count == 0) beginning_of_empty_entries = fat_fs->root_directory_begin_address_for_FAT12_and_FAT16 + i*fat_fs->cluster_size;
                empty_entries_count++;
                if (empty_entries_count >= number_of_LFN_entries + 1) break;
            }
            else if (first_name_byte == 0)
            {
                if (fat_fs->number_of_entries_per_cluster >= number_of_LFN_entries + (size_t)i + 1) 
                {
                    empty_entries_count = number_of_LFN_entries + 1;
                    beginning_of_empty_entries = fat_fs->root_directory_begin_address_for_FAT12_and_FAT16 + i*fat_fs->cluster_size;
                }
                else
                {
                    empty_entries_count = fat_fs->number_of_entries_per_cluster - i;
                }
                break;
            }
            else empty_entries_count = 0;
        }

        if (empty_entries_count >= number_of_LFN_entries + 1)
        {
            // If there is enough space, we put the entries
            FAT_dir_entry DIR_ENTRY;
            uint8_t short_file_name_length = s.length();
            for (uint8_t i = 0; i < 11; i++) DIR_ENTRY.DIR_Name[i] = 0;
            if (is_dir) 
            {
                if (short_file_name_length > 6) short_file_name_length = 6;
                for (uint8_t i = 0; i < short_file_name_length; i++) DIR_ENTRY.DIR_Name[i] = s[i];
            }
            else 
            {
                // We need to take care of the extension in this case
                uint8_t last_dot = short_file_name_length - 1;
                while(last_dot && s[last_dot] != '.') last_dot--;

                short_file_name_length = last_dot;
                if (short_file_name_length > 6) short_file_name_length = 6;
                for (uint8_t i = 0; i < short_file_name_length; i++) DIR_ENTRY.DIR_Name[i] = s[i];

                short_file_name_length = s.length() - last_dot;
                if (short_file_name_length > 3) short_file_name_length = 3;
                for (uint8_t i = 0; i < short_file_name_length; i++) DIR_ENTRY.DIR_Name[i + 9] = s[last_dot + i];
            }
            DIR_ENTRY.DIR_Name[7] = '~';
            DIR_ENTRY.DIR_Name[8] = short_file_name_counter++;

            if (is_dir) DIR_ENTRY.DIR_Attr = ATTR_DIRECTORY;
            DIR_ENTRY.DIR_NTRes = 0;

            // File/folder is initially empty
            DIR_ENTRY.DIR_FstClusHI = 0;
            DIR_ENTRY.DIR_FstClusLO = 0;

            // TODO: set the good create time

            // Then we calculate the checksum
            uint8_t checksum = 0;
            for (uint8_t j = 0; j < 11; j++)
                checksum = ((checksum & 1) ? 0x80 : 0) + (checksum >> 1) + DIR_ENTRY.DIR_Name[j];

            // Then we create the LFN entries for the file name.
            FAT_Long_File_Name_entry LFN[number_of_LFN_entries];
            size_t offset = 0;
            for (uint16_t i = 0; i < number_of_LFN_entries; i++)
            {
                LFN[i].LDIR_Ord = i + 1;
                LFN[i].LDIR_Attr = ATTR_LONG_NAME;
                LFN[i].LDIR_FstClusLO = 0;
                LFN[i].LDIR_Chksum = checksum; // Unsure
                if (offset + 5 > s.length())
                {
                    for (uint8_t j = 0; j < s.length() - offset; j++) LFN[i].LDIR_Name1[j] = s[j + offset];
                    for (uint8_t j = s.length() - offset; j < 5; j++) LFN[i].LDIR_Name1[j] = 0;
                    for (uint8_t j = 0; j < 6; j++) LFN[i].LDIR_Name2[j] = 0;
                    for (uint8_t j = 0; j < 2; j++) LFN[i].LDIR_Name3[j] = 0;
                    break;
                }
                else
                {
                    for (uint8_t j = 0; j < 5; j++) LFN[i].LDIR_Name1[j] = s[j + offset];
                    offset += 5;
                }

                if (offset + 6 > s.length())
                {
                    for (uint8_t j = 0; j < s.length() - offset; j++) LFN[i].LDIR_Name2[j] = s[j + offset];
                    for (uint8_t j = s.length() - offset; j < 6; j++) LFN[i].LDIR_Name2[j] = 0;
                    for (uint8_t j = 0; j < 2; j++) LFN[i].LDIR_Name3[j] = 0;
                    break;
                }
                else
                {
                    for (uint8_t j = 0; j < 6; j++) LFN[i].LDIR_Name2[j] = s[j + offset];
                    offset += 6;
                }

                if (offset + 5 > s.length())
                {
                    for (uint8_t j = 0; j < 2; j++) LFN[i].LDIR_Name3[j] = s[j + offset];
                    for (uint8_t j = s.length() - offset; j < 2; j++) LFN[i].LDIR_Name3[j] = 0;
                    break;
                }
                else
                {
                    for (uint8_t j = 0; j < 2; j++) LFN[i].LDIR_Name3[j] = s[j + offset];
                    offset += 2;
                }
            }
            
            LFN[number_of_LFN_entries - 1].LDIR_Ord |= 0x40;

            fat_fs->fh->seek(beginning_of_empty_entries, SET);
            for (int i = number_of_LFN_entries - 1; i >= 0; i--)
            {
                fat_fs->fh->write(&LFN[i], sizeof(LFN[i]));
            }
            fat_fs->fh->write(&DIR_ENTRY, sizeof(DIR_ENTRY));
        }
        else
        {
            throw logic_error("Failed to make a new file/folder, due to lack of space in root directory, which cannot be extended in FAT12/FAT16.");
        }
    }
}
//*/

}
