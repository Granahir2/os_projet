#include "fat.hh"
#include "kstdlib/cstdio.hh"
namespace FAT {

FAT_file::FAT_file
    (FAT_FileSystem* fat_fs, 
    size_t file_size, 
    size_t first_cluster_number, 
    size_t address_of_entry_in_parent_directory) : 
    read_write_head_position(0), 
    read_write_head_position_within_cluster(0),
    read_write_head_position_cluster_number(first_cluster_number),
    file_size(file_size),
    first_cluster_number(first_cluster_number),
    fat_fs(fat_fs),
    address_of_entry_in_parent_directory(address_of_entry_in_parent_directory) {
         if (first_cluster_number != 0) 
            fat_fs->fh->seek(fat_fs->cluster_number_to_address(first_cluster_number), SET);
    }

off_t FAT_file::set_position(size_t position) {
    // If the position is already set, return it
    // Also to handle the case when the file is empty
    //printf("Position to be set: %d\nCurrent position: %d\n", position, read_write_head_position);
    if (position == read_write_head_position)
       return position;
    //printf("Setting position to %d\n", position);
    //printf("File size is %d\n", file_size);
    if ((file_size == 0 && position > 0) || (file_size > 0 && position >= file_size))
        throw logic_error("Position is out of file size");
    read_write_head_position = position;
    size_t current_cluster = first_cluster_number;
    while(position >= fat_fs->cluster_size) {
        position -= fat_fs->cluster_size;
        current_cluster = fat_fs->find_fat_entry(current_cluster);
        if (current_cluster == 0)
            throw logic_error("File is too short, this cluster is not allocated");
        else if (current_cluster == fat_fs->LAST_CLUSTER)
            throw logic_error("File is too long, this cluster is not allocated");
        else if (current_cluster == fat_fs->BAD_CLUSTER)
            throw logic_error("Bad cluster");
        else if (current_cluster > fat_fs->number_of_clusters)
            throw logic_error("This cluster is reserved");
    }
    fat_fs->fh->seek(fat_fs->cluster_number_to_address(current_cluster) + position, SET);
    read_write_head_position_within_cluster = position;
    read_write_head_position_cluster_number = current_cluster;
    //printf("Position set to %d\n", position);
    //printf("Read-write head position: %d\n", read_write_head_position);
    //printf("Read-write head position within cluster: %d\n", read_write_head_position_within_cluster);
    //printf("Read-write head position cluster number: %d\n", read_write_head_position_cluster_number);
	if((off_t)position >= 0) {
		return (off_t)position;
	} else {
		throw eoverflow("[seek] Position too large");
	}
}

size_t FAT_file::read(void* buffer, size_t size) {
    //printf("Read-write head position: %d\n", read_write_head_position);
    //printf("File size: %d\n", file_size);
    //printf("Size: %d\n", size);
    if (read_write_head_position + size > file_size)
        size = file_size - read_write_head_position;
    seek(0, CUR); // To update the position (O(1))
    size_t write_head_position_in_buffer = 0;
    size_t read_size = size;
    printf("Reading %d bytes\n", size);
    
    // Read the remaining of the current cluster
    size_t remaining_in_cluster = fat_fs->cluster_size - read_write_head_position_within_cluster;
    if (remaining_in_cluster > size)
        remaining_in_cluster = size;
    fat_fs->fh->read((uint8_t*)(buffer)+write_head_position_in_buffer, remaining_in_cluster);
    read_write_head_position += remaining_in_cluster;
    write_head_position_in_buffer += remaining_in_cluster;
    read_write_head_position_within_cluster = 0;
    size -= remaining_in_cluster;

    printf("Read %d bytes\n", remaining_in_cluster);

    // Read the remaining clusters
    while(size >= fat_fs->cluster_size) {
        size_t current_cluster = fat_fs->find_fat_entry(read_write_head_position_cluster_number);
        if (current_cluster == 0)
            throw logic_error("File is too short, this cluster is not allocated");
        else if (current_cluster == fat_fs->LAST_CLUSTER)
            throw logic_error("File is too long, this cluster is not allocated");
        else if (current_cluster == fat_fs->BAD_CLUSTER)
            throw logic_error("Bad cluster");
        else if (current_cluster > fat_fs->number_of_clusters)
            throw logic_error("This cluster is reserved");
        fat_fs->fh->seek(fat_fs->cluster_number_to_address(current_cluster), SET);
        fat_fs->fh->read((uint8_t*)(buffer)+write_head_position_in_buffer, fat_fs->cluster_size);
        read_write_head_position += fat_fs->cluster_size;
        write_head_position_in_buffer += fat_fs->cluster_size;
        read_write_head_position_cluster_number = current_cluster;
        size -= fat_fs->cluster_size;

        printf("Read %d bytes\n", fat_fs->cluster_size);
    }

    // Read the remaining of the last cluster
    if (size > 0) {
        size_t current_cluster = fat_fs->find_fat_entry(read_write_head_position_cluster_number);
        if (current_cluster == 0)
            throw logic_error("File is too short, this cluster is not allocated");
        else if (current_cluster == fat_fs->LAST_CLUSTER)
            throw logic_error("File is too long, this cluster is not allocated");
        else if (current_cluster == fat_fs->BAD_CLUSTER)
            throw logic_error("Bad cluster");
        else if (current_cluster > fat_fs->number_of_clusters)
            throw logic_error("This cluster is reserved");
        fat_fs->fh->seek(fat_fs->cluster_number_to_address(current_cluster), SET);
        fat_fs->fh->read((uint8_t*)(buffer)+write_head_position_in_buffer, size);
        read_write_head_position += size;
        write_head_position_in_buffer += size;
        read_write_head_position_within_cluster = size;
        read_write_head_position_cluster_number = current_cluster;

        printf("Read %d bytes\n", size);
    }

    return read_size;
}

size_t FAT_file::write(const void* buffer, size_t size) {
    size_t write_size = 0;
    //printf("Writing %d bytes\n", size);
    //set_position(read_write_head_position); // The position is already set and setting it is O(N)
    
    // Write the remaining of the current cluster
    size_t remaining_in_cluster_or_buffer = fat_fs->cluster_size - read_write_head_position_within_cluster;
    if (remaining_in_cluster_or_buffer > size)
        remaining_in_cluster_or_buffer = size;
    // Check if the file is initially empty
    // And if so, allocate the first cluster
    if (file_size == 0) {
        first_cluster_number = fat_fs->find_free_cluster(0);
        if (first_cluster_number == 0)
            throw logic_error("Need memory to write more to file, no free cluster found");

        // Update the first cluster of the directory entry in the parent directory
        size_t first_cluster_number_LO = first_cluster_number & 0xFF;
        size_t first_cluster_number_HI = (first_cluster_number >> 16) & 0xFF;
        fat_fs->fh->seek(address_of_entry_in_parent_directory + 20, SET);
        fat_fs->fh->write(&first_cluster_number_HI, 2);
        fat_fs->fh->seek(address_of_entry_in_parent_directory + 26, SET);
        fat_fs->fh->write(&first_cluster_number_LO, 2);
        
        read_write_head_position_cluster_number = first_cluster_number;
        fat_fs->fh->seek(fat_fs->cluster_number_to_address(first_cluster_number), SET);
    }
    fat_fs->fh->write(buffer+write_size, remaining_in_cluster_or_buffer);
    read_write_head_position += remaining_in_cluster_or_buffer;
    read_write_head_position_within_cluster = 0;
    size -= remaining_in_cluster_or_buffer;
    write_size += remaining_in_cluster_or_buffer;

    //printf("Wrote %d bytes\n", remaining_in_cluster_or_buffer);

    // Write the remaining clusters
    while(size >= fat_fs->cluster_size) {
        size_t current_cluster = fat_fs->find_fat_entry(read_write_head_position_cluster_number);
        
        if (current_cluster == 0)
            throw logic_error("File is too short, this cluster is not allocated");
        else if (current_cluster == fat_fs->BAD_CLUSTER)
            throw logic_error("Bad cluster");
        else if (current_cluster > fat_fs->number_of_clusters) 
            throw logic_error("This cluster is reserved");	
        else if (current_cluster == fat_fs->LAST_CLUSTER) {
            current_cluster = fat_fs->find_free_cluster(read_write_head_position_cluster_number);
            if (current_cluster == 0)
            {
                // No free cluster found, no more space to write
                // End writing prematurely

                // Update the file size if necessary
                if (read_write_head_position > file_size)
                {
                    file_size = read_write_head_position;
                    fat_fs->fh->seek(address_of_entry_in_parent_directory + 28, SET);
                    fat_fs->fh->write(&file_size, 4);
                }
                return write_size;
            }
            else // Update the fh reader to the new cluster
                fat_fs->fh->seek(fat_fs->cluster_number_to_address(current_cluster), SET);
        }
        
        fat_fs->fh->write(buffer+write_size, fat_fs->cluster_size);
        read_write_head_position += fat_fs->cluster_size;
        read_write_head_position_cluster_number = current_cluster;
        size -= fat_fs->cluster_size;
        write_size += fat_fs->cluster_size;

        //printf("Wrote %d bytes\n", fat_fs->cluster_size);
    }

    // Write the remaining of the last cluster
    if (size > 0) {
        size_t current_cluster = fat_fs->find_fat_entry(read_write_head_position_cluster_number);
        if (current_cluster == 0)
            throw logic_error("File is too short, this cluster is not allocated");
        else if (current_cluster == fat_fs->LAST_CLUSTER)
            throw logic_error("Bad cluster");
        else if (current_cluster > fat_fs->number_of_clusters)
            throw logic_error("This cluster is reserved");
        else if (current_cluster == fat_fs->LAST_CLUSTER) {
            current_cluster = fat_fs->find_free_cluster(read_write_head_position_cluster_number);
            if (current_cluster == 0)
            {
                // No free cluster found, no more space to write
                // End writing prematurely

                if (read_write_head_position > file_size)
                {
                    file_size = read_write_head_position;
                    fat_fs->fh->seek(address_of_entry_in_parent_directory + 28, SET);
                    fat_fs->fh->write(&file_size, 4);
                }

                return write_size;
            }
            else // Update the fh reader to the new cluster
                fat_fs->fh->seek(fat_fs->cluster_number_to_address(current_cluster), SET);
        }

        fat_fs->fh->write(buffer+write_size, size);
        read_write_head_position += size;
        read_write_head_position_within_cluster = size;
        read_write_head_position_cluster_number = current_cluster;
        write_size += size;

        //printf("Wrote %d bytes\n", size);
    }

    // Update the file size if necessary
    if (read_write_head_position > file_size)
    {
        file_size = read_write_head_position;
        fat_fs->fh->seek(address_of_entry_in_parent_directory + 28, SET);
        fat_fs->fh->write(&file_size, 4);
    }

    return write_size;
}

off_t FAT_file::seek(off_t offset, seekref whence) {
    switch(whence) {
        case SET:
            return set_position(offset);
            break;
        case CUR:
            // Not the most efficient way, but it works
            // To be improved
            return set_position(read_write_head_position + offset);
            break;
        case END:
            return set_position(file_size + offset);
            break;
        default:
            throw logic_error("Invalid seek reference");
    }
}

}
