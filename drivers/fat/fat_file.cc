#include "fat.hh"

namespace FAT {

void FAT_file::set_position(size_t position) {
    if (position >= file_size)
        throw logic_error("Position is out of file size");
    read_write_head_position = position;
    size_t current_cluster = first_cluster_number;
    while(position >= fat_fs->cluster_size) {
        position -= fat_fs->cluster_size;
        current_cluster = fat_fs->find_fat_entry(current_cluster);
        if (current_cluster == 0)
            throw logic_error("File is too short, this cluster is not allocated");
        else if (current_cluster == 0xFFFFFFFF)
            throw logic_error("File is too long, this cluster is not allocated");
        else if (current_cluster == 0xFFFFFF7)
            throw logic_error("Bad cluster");
        else if (current_cluster > fat_fs->number_of_clusters)
            throw logic_error("This cluster is reserved");
    }
    fat_fs->fh->seek(fat_fs->cluster_number_to_address(current_cluster) + position, SET);
    read_write_head_position_within_cluster = position;
    read_write_head_position_cluster_number = current_cluster;
}

size_t FAT_file::read(void* buffer, size_t size) {
    size_t read_size = size;
    if (read_write_head_position + size > file_size)
        size = file_size - read_write_head_position;
    set_position(read_write_head_position);
    
    // Read the remaining of the current cluster
    size_t remaining_in_cluster = fat_fs->cluster_size - read_write_head_position_within_cluster;
    fat_fs->fh->read(buffer, remaining_in_cluster);
    read_write_head_position += remaining_in_cluster;
    read_write_head_position_within_cluster = 0;
    size -= remaining_in_cluster;

    // Read the remaining clusters
    while(size >= fat_fs->cluster_size) {
        size_t current_cluster = fat_fs->find_fat_entry(read_write_head_position_cluster_number);
        if (current_cluster == 0)
            throw logic_error("File is too short, this cluster is not allocated");
        else if (current_cluster == 0xFFFFFFFF)
            throw logic_error("File is too long, this cluster is not allocated");
        else if (current_cluster == 0xFFFFFF7)
            throw logic_error("Bad cluster");
        else if (current_cluster > fat_fs->number_of_clusters)
            throw logic_error("This cluster is reserved");
        fat_fs->fh->read(buffer, fat_fs->cluster_size);
        read_write_head_position += fat_fs->cluster_size;
        read_write_head_position_cluster_number = current_cluster;
        size -= fat_fs->cluster_size;
    }

    // Read the remaining of the last cluster
    if (size > 0) {
        size_t current_cluster = fat_fs->find_fat_entry(read_write_head_position_cluster_number);
        if (current_cluster == 0)
            throw logic_error("File is too short, this cluster is not allocated");
        else if (current_cluster == 0xFFFFFFFF)
            throw logic_error("File is too long, this cluster is not allocated");
        else if (current_cluster == 0xFFFFFF7)
            throw logic_error("Bad cluster");
        else if (current_cluster > fat_fs->number_of_clusters)
            throw logic_error("This cluster is reserved");
        fat_fs->fh->read(buffer, size);
        read_write_head_position += size;
        read_write_head_position_within_cluster = size;
        read_write_head_position_cluster_number = current_cluster;
    }

    return read_size;
}

size_t FAT_file::write(const void* buffer, size_t size) {
    size_t write_size = size;
    if (read_write_head_position + size > file_size)
        size = file_size - read_write_head_position;
    set_position(read_write_head_position);
    
    // Read the remaining of the current cluster
    size_t remaining_in_cluster = fat_fs->cluster_size - read_write_head_position_within_cluster;
    fat_fs->fh->write(buffer, remaining_in_cluster);
    read_write_head_position += remaining_in_cluster;
    read_write_head_position_within_cluster = 0;
    size -= remaining_in_cluster;

    // Read the remaining clusters
    while(size >= fat_fs->cluster_size) {
        size_t current_cluster = fat_fs->find_fat_entry(read_write_head_position_cluster_number);
        if (current_cluster == 0)
            throw logic_error("File is too short, this cluster is not allocated");
        else if (current_cluster == 0xFFFFFFFF)
            throw logic_error("File is too long, this cluster is not allocated");
        else if (current_cluster == 0xFFFFFF7)
            throw logic_error("Bad cluster");
        else if (current_cluster > fat_fs->number_of_clusters)
            throw logic_error("This cluster is reserved");
        fat_fs->fh->write(buffer, fat_fs->cluster_size);
        read_write_head_position += fat_fs->cluster_size;
        read_write_head_position_cluster_number = current_cluster;
        size -= fat_fs->cluster_size;
    }

    // Read the remaining of the last cluster
    if (size > 0) {
        size_t current_cluster = fat_fs->find_fat_entry(read_write_head_position_cluster_number);
        if (current_cluster == 0)
            throw logic_error("File is too short, this cluster is not allocated");
        else if (current_cluster == 0xFFFFFFFF)
            throw logic_error("File is too long, this cluster is not allocated");
        else if (current_cluster == 0xFFFFFF7)
            throw logic_error("Bad cluster");
        else if (current_cluster > fat_fs->number_of_clusters)
            throw logic_error("This cluster is reserved");
        fat_fs->fh->write(buffer, size);
        read_write_head_position += size;
        read_write_head_position_within_cluster = size;
        read_write_head_position_cluster_number = current_cluster;
    }

    return write_size;
}

void FAT_file::seek(off_t offset, seekref whence) {
    switch(whence) {
        case SET:
            set_position(offset);
            break;
        case CUR:
            // Not the most efficient way, but it works
            // To be improved
            set_position(read_write_head_position + offset);
            break;
        case END:
            set_position(file_size + offset);
            break;
        default:
            throw logic_error("Invalid seek reference");
    }
}

}