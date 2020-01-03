//
// Created by pavel on 03.01.20.
//

#include <string>
#include <sys/stat.h>
#include <fstream>
#include <bitset>
#include <iostream>
#include <cstring>
#include "cat.h"
#include "zosfsstruct.h"
#include "inode.h"
#include "directory.h"
#include "datablock.h"

/**
 * Vrati referenci na inode souboru
 * @param s1 nazev souboru
 */
pseudo_inode * getFileINode(filesystem &filesystem_data, std::string &s1) {
    std::fstream input_file;
    input_file.open(filesystem_data.fs_file, std::ios::in | std::ios::out | std::ios::binary);

    pseudo_inode inode;
    pseudo_inode * inode_ptr = nullptr;

    // zjistim, zdali existuje adresar stejneho nazvu
    std::vector<directory_item> directories = getDirectories(filesystem_data);
    for(auto& directory : directories) {
        directory.item_name;
        if(strcmp(s1.c_str(),directory.item_name) == 0) {
            input_file.seekp(filesystem_data.super_block.inode_start_address + (directory.inode-1) * sizeof(pseudo_inode));
            input_file.read(reinterpret_cast<char *>(&inode), sizeof(pseudo_inode));
            if(inode.isDirectory) {
                std::cout << "FILE IS DIRECTORY" << std::endl;
            } else {
                std::cout << "OBSAH" << std::endl;
                inode_ptr = &inode;
            }
            break;
        }
    }

    input_file.close();
    return inode_ptr;
}

/**
 * Vypise obsah bufferu
 * @param buffer obsah
 * @param len velikost bufferu
 */
void printBuffer(char buffer[], int32_t len) {
    for(int i=0 ; i<len; ++i) {
        if(buffer[i] == EOF) break;
        std::cout << buffer[i];
    }
}

/**
 * Nacte datablock do bufferu a vypise jej
 * @param filesystem_data informace filesystemu
 * @param fs_file otevreny soubor
 * @param location pocatecni pozice datablocku
 */
void readDataBlock(filesystem &filesystem_data, std::fstream &fs_file, int32_t location) {
    // priprava bufferu
    char buffer[filesystem_data.super_block.cluster_size];
    memset(buffer, EOF, filesystem_data.super_block.cluster_size);
    // presun na pozici
    fs_file.seekp(location);
    fs_file.read(buffer, filesystem_data.super_block.cluster_size);
    printBuffer(buffer, filesystem_data.super_block.cluster_size);
}

/**
 * Vypise obsah souboru s1
 * @param filesystem_data filesystem
 * @param s1 soubor
 */
void cat(filesystem &filesystem_data, std::string &s1) {
    // najdu odpovidajici inode
    pseudo_inode inode;
    pseudo_inode * inode_ptr = getFileINode(filesystem_data, s1);
    if(inode_ptr != nullptr) {
        inode = *inode_ptr;

        std::fstream fs_file;
        fs_file.open(filesystem_data.fs_file, std::ios::in | std::ios::out | std::ios::binary);

        if(inode.direct1 != 0) {
            readDataBlock(filesystem_data, fs_file, inode.direct1);
        }
        if(inode.direct2 != 0) {
            readDataBlock(filesystem_data, fs_file, inode.direct2);
        }
        if(inode.direct3 != 0) {
            readDataBlock(filesystem_data, fs_file, inode.direct3);
        }
        if(inode.direct4 != 0) {
            readDataBlock(filesystem_data, fs_file, inode.direct4);
        }
        if(inode.direct5 != 0) {
            readDataBlock(filesystem_data, fs_file, inode.direct5);
        }
        if(inode.indirect1 != 0) {
            uint32_t links_per_cluster = filesystem_data.super_block.cluster_size / sizeof(int32_t);
            int32_t links[links_per_cluster];
            fs_file.seekp(inode.indirect1);
            fs_file.read(reinterpret_cast<char *>(&links), sizeof(links));
            for(int i = 0; i < links_per_cluster; i++) {
                if(links[i] != 0) {
                    readDataBlock(filesystem_data, fs_file, links[i]);
                }
            }
        }

        std::cout << std::endl;

        fs_file.close();
    } else {
        return;
    }
}