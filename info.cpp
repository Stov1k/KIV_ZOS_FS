//
// Created by pavel on 06.01.20.
//

#include <fstream>
#include <vector>
#include <iostream>
#include "info.h"
#include "zosfsstruct.h"
#include "directory.h"

/**
 * Vypise adresy databloku
 * @param filesystem_data filesystem
 * @param fs_file otevreny soubor filesystemu
 * @param inode soubor
 */
void printDatablocksAddresses(filesystem &filesystem_data, std::fstream &fs_file, pseudo_inode &inode) {
    if (inode.direct1 != 0) {
        std::cout << inode.direct1 << " ";
    }
    if (inode.direct2 != 0) {
        std::cout << inode.direct2 << " ";
    }
    if (inode.direct3 != 0) {
        std::cout << inode.direct3 << " ";
    }
    if (inode.direct4 != 0) {
        std::cout << inode.direct4 << " ";
    }
    if (inode.direct5 != 0) {
        std::cout << inode.direct5 << " ";
    }
    if (inode.indirect1 != 0) {
        std::cout << inode.indirect1 << " ";
        uint32_t links_per_cluster = filesystem_data.super_block.cluster_size / sizeof(int32_t);
        int32_t links[links_per_cluster];
        fs_file.seekp(inode.indirect1);
        fs_file.read(reinterpret_cast<char *>(&links), sizeof(links));
        for (int i = 0; i < links_per_cluster; i++) {
            if (links[i] != 0) {
                std::cout << links[i] << " ";
            }
        }
    }
    if (inode.indirect2 != 0) {
        std::cout << inode.indirect2 << " ";
        uint32_t links_per_cluster = filesystem_data.super_block.cluster_size / sizeof(int32_t);
        int32_t links[links_per_cluster];
        fs_file.seekp(inode.indirect2);
        fs_file.read(reinterpret_cast<char *>(&links), sizeof(links));
        for (int i = 0; i < links_per_cluster; i++) {
            if (links[i] != 0) {
                std::cout << links[i] << " ";
                int32_t sublinks[links_per_cluster];
                fs_file.seekp(links[i]);
                fs_file.read(reinterpret_cast<char *>(&sublinks), sizeof(sublinks));
                for (int j = 0; j < links_per_cluster; j++) {
                    if (sublinks[j] != 0) {
                        std::cout << sublinks[j] << " ";
                    }
                }
            }
        }
    }
}

/**
 * Vypise informace o souboru/adresari s1/a1
 * @param filesystem_data filesystem
 * @param name nazev
 */
void info(filesystem &filesystem_data, std::string &name) {
    std::fstream fs_file;
    fs_file.open(filesystem_data.fs_file, std::ios::in | std::ios::out | std::ios::binary);

    pseudo_inode *inode_ptr = nullptr;
    pseudo_inode inode;
    std::string dir_name;

    // zjistim, zdali existuje adresar stejneho nazvu
    std::vector<directory_item> directories = getDirectories(filesystem_data);
    for (auto &directory : directories) {
        if (strcmp(name.c_str(), directory.item_name) == 0) {
            dir_name = directory.item_name;
            fs_file.seekp(
                    filesystem_data.super_block.inode_start_address + (directory.inode - 1) * sizeof(pseudo_inode));
            fs_file.read(reinterpret_cast<char *>(&inode), sizeof(pseudo_inode));
            inode_ptr = &inode;
            break;
        }
    }

    if (inode_ptr != nullptr) {
        std::cout << "NAME\t – SIZE\t – INODE\t – LINKS" << std::endl;
        std::cout << dir_name << "\t – " << inode.file_size << "\t – " << inode.nodeid << "\t – ";
        printDatablocksAddresses(filesystem_data, fs_file, inode);
        std::cout << std::endl;
    } else {
        std::cout << "FILE NOT FOUND" << std::endl;
    }
    fs_file.close();
}