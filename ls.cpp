//
// Created by pavel on 09.01.20.
//

#include <iostream>
#include <fstream>
#include <vector>
#include "ls.h"
#include "zosfsstruct.h"
#include "inode.h"
#include "datablock.h"
#include "directory.h"


/**
 * Vypise obsah adresare a1
 * @param filesystem_data filesystem
 * @param a1 adresar
 */
void ls(filesystem &filesystem_data, pseudo_inode &a1) {
    std::fstream fs_file;
    fs_file.open(filesystem_data.fs_file, std::ios::in);

    // platne adresy na soubory a adresare
    std::vector<int32_t> addresses = usedDatablockByINode(filesystem_data, fs_file, a1, false);

    // adresare databloku
    uint32_t dirs_per_cluster = dirsPerCluster(filesystem_data);
    directory_item directories[dirs_per_cluster];

    // vypise hlavicku
    //std::cout << "NAME" << "\t" << "ND" << "\t" << "SIZE" << "\t" << std::endl;

    // prochazeni adres databloku
    for (auto &address : addresses) {
        fs_file.seekp(address);     // prejde na adresu databloku
        fs_file.read(reinterpret_cast<char *>(&directories), sizeof(directories));
        for (int i = 0; i < dirs_per_cluster; i++) {
            if (directories[i].inode) {
                pseudo_inode inode;
                fs_file.seekp(getINodePosition(filesystem_data, directories[i].inode));
                fs_file.read(reinterpret_cast<char *>(&inode), sizeof(pseudo_inode));
                if(inode.isDirectory) {
                    std::cout << " + ";
                } else {
                    std::cout << " - ";
                }
                std::cout << directories[i].item_name << "\t"
                          << directories[i].inode << "\t" << inode.file_size << std::endl;
            }
        }
    }

    fs_file.close();
}

/**
 * Vypise obsah adresare a1
 * @param filesystem_data filesystem
 * @param a1 adresar
 */
void ls(filesystem &filesystem_data, std::string &a1) {
    pseudo_inode a1_inode;
    pseudo_inode *a1_inode_ptr = iNodeByLocation(filesystem_data, a1, false);
    if (nullptr != a1_inode_ptr) {
        a1_inode = *a1_inode_ptr;
        if (!a1_inode.isDirectory) {
            std::cout << "FILE IS NOT DIRECTORY" << std::endl;
            return;
        } else {
            ls(filesystem_data, a1_inode);
        }
    } else {
        std::cout << "PATH NOT FOUND" << std::endl;
        return;
    }
}

/**
 * Vypise obsah adresare a1
 * @param filesystem_data filesystem
 */
void ls(filesystem &filesystem_data) {
    ls(filesystem_data, filesystem_data.current_dir);
}