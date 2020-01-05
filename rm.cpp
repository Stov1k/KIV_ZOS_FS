//
// Created by pavel on 05.01.20.
//

#include <string>
#include <fstream>
#include <vector>
#include <iostream>
#include "rm.h"
#include "zosfsstruct.h"
#include "directory.h"

/**
 * Vrati, zdali adresar je prazdny
 * @param filesystem_data filesystem
 * @param fs_file otevreny soubor filesystemu
 * @param inode adresar
 * @return je prazdny?
 */
bool isDirectoryEmpty(filesystem &filesystem_data, std::fstream &fs_file, pseudo_inode &inode) {

    uint32_t dirs_per_cluster = filesystem_data.super_block.cluster_size / sizeof(directory_item);
    directory_item directories[dirs_per_cluster];

    if (inode.direct1 != 0) {
        fs_file.seekp(inode.direct1);
        fs_file.read(reinterpret_cast<char *>(&directories), sizeof(directories));
        for (int i = 2; i < dirs_per_cluster; i++) {
            if (directories[i].inode) return false;
        }
    }
    if (inode.direct2 != 0) {
        fs_file.seekp(inode.direct2);
        fs_file.read(reinterpret_cast<char *>(&directories), sizeof(directories));
        for (int i = 0; i < dirs_per_cluster; i++) {
            if (directories[i].inode) return false;
        }
    }
    if (inode.direct3 != 0) {
        fs_file.seekp(inode.direct3);
        fs_file.read(reinterpret_cast<char *>(&directories), sizeof(directories));
        for (int i = 0; i < dirs_per_cluster; i++) {
            if (directories[i].inode) return false;
        }
    }
    if (inode.direct4 != 0) {
        fs_file.seekp(inode.direct4);
        fs_file.read(reinterpret_cast<char *>(&directories), sizeof(directories));
        for (int i = 0; i < dirs_per_cluster; i++) {
            if (directories[i].inode) return false;
        }
    }
    if (inode.direct5 != 0) {
        fs_file.seekp(inode.direct5);
        fs_file.read(reinterpret_cast<char *>(&directories), sizeof(directories));
        for (int i = 0; i < dirs_per_cluster; i++) {
            if (directories[i].inode) return false;
        }
    }
    if (inode.indirect1 != 0) {
        uint32_t links_per_cluster = filesystem_data.super_block.cluster_size / sizeof(int32_t);
        int32_t links[links_per_cluster];
        fs_file.seekp(inode.indirect1);
        fs_file.read(reinterpret_cast<char *>(&links), sizeof(links));
        for (int i = 0; i < links_per_cluster; i++) {
            if (links[i] != 0) {
                fs_file.seekp(links[i]);
                fs_file.read(reinterpret_cast<char *>(&directories), sizeof(directories));
                for (int i = 0; i < dirs_per_cluster; i++) {
                    if (directories[i].inode) return false;
                }
            }
        }
    }
    if (inode.indirect2 != 0) {
        uint32_t links_per_cluster = filesystem_data.super_block.cluster_size / sizeof(int32_t);
        int32_t links[links_per_cluster];
        fs_file.seekp(inode.indirect2);
        fs_file.read(reinterpret_cast<char *>(&links), sizeof(links));
        for (int i = 0; i < links_per_cluster; i++) {
            if (links[i] != 0) {
                int32_t sublinks[links_per_cluster];
                fs_file.seekp(links[i]);
                fs_file.read(reinterpret_cast<char *>(&sublinks), sizeof(sublinks));
                for (int j = 0; j < links_per_cluster; j++) {
                    if (sublinks[j] != 0) {
                        fs_file.seekp(sublinks[j]);
                        fs_file.read(reinterpret_cast<char *>(&directories), sizeof(directories));
                        for (int i = 0; i < dirs_per_cluster; i++) {
                            if (directories[i].inode) return false;
                        }
                    }
                }
            }
        }
    }
    return true;
}

/**
 * Smaze prazdny adresar a1
 * @param filesystem_data fileszystem
 * @param a1 nazev adresare
 */
void rmdir(filesystem &filesystem_data, std::string &a1) {
    std::fstream fs_file;
    fs_file.open(filesystem_data.fs_file, std::ios::in | std::ios::out | std::ios::binary);

    pseudo_inode * inode_ptr = nullptr;
    pseudo_inode inode;

    // zjistim, zdali existuje adresar stejneho nazvu
    std::vector<directory_item> directories = getDirectories(filesystem_data);
    for (auto &directory : directories) {
        directory.item_name;
        if (strcmp(a1.c_str(), directory.item_name) == 0) {
            fs_file.seekp(filesystem_data.super_block.inode_start_address + (directory.inode - 1) * sizeof(pseudo_inode));
            fs_file.read(reinterpret_cast<char *>(&inode), sizeof(pseudo_inode));
            if (inode.isDirectory) {
                inode_ptr = &inode;
            }
            break;
        }
    }

    if (inode_ptr != nullptr) {
        inode = *inode_ptr;
        bool empty = isDirectoryEmpty(filesystem_data, fs_file, inode);
        if(empty) {
            std::cout << "OK" << std::endl;
        } else {
            std::cout << "NOT EMPTY" << std::endl;
        }
    } else {
        std::cout << "FILE NOT FOUND" << std::endl;
    }

    fs_file.close();
}
