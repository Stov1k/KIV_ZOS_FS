//
// Created by pavel on 09.01.20.
//

#include <iostream>
#include <fstream>
#include "ls.h"
#include "zosfsstruct.h"
#include "inode.h"

/**
 * Vypise obsah adresare a1
 * @param filesystem_data filesystem
 * @param a1 adresar
 */
void ls(filesystem &filesystem_data, pseudo_inode &a1) {
    std::fstream input_file;
    input_file.open(filesystem_data.fs_file, std::ios::in);
    if (!input_file.is_open()) {
        std::cout << "Nelze otevrit." << std::endl;
    } else {
        // skok na inode
        input_file.seekp(a1.direct1);

        uint32_t dirs_per_cluster = filesystem_data.super_block.cluster_size / sizeof(directory_item);
        directory_item directories[dirs_per_cluster];

        input_file.read(reinterpret_cast<char *>(&directories), sizeof(directories));
        std::cout << "#" << "\t" << "ND" << "\t" << "NAME" << "\t" << "SIZE" << std::endl;
        for (int i = 0; i < dirs_per_cluster; i++) {
            if (directories[i].inode) {
                pseudo_inode inode;
                input_file.seekp(getINodePosition(filesystem_data, directories[i].inode));
                input_file.read(reinterpret_cast<char *>(&inode), sizeof(pseudo_inode));
                std::cout << i << "\t" << directories[i].inode << "\t" << directories[i].item_name << "\t"
                          << inode.file_size << std::endl;
            }
        }
    }
    input_file.close();
}