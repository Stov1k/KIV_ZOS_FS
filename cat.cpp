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
 * Vypise obsah bufferu
 * @param buffer obsah
 * @param len velikost bufferu
 */
void printBuffer(char buffer[], int32_t len) {
    for (int i = 0; i < len; ++i) {
        if (buffer[i] == EOF) break;
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
    pseudo_inode *inode_ptr = getFileINode(filesystem_data, s1);
    if (inode_ptr != nullptr) {
        inode = *inode_ptr;

        std::fstream fs_file;
        fs_file.open(filesystem_data.fs_file, std::ios::in | std::ios::out | std::ios::binary);

        // platne adresy na databloky
        std::vector<int32_t> addresses = usedDatablockByINode(filesystem_data, fs_file, inode);

        // prochazeni adres databloku
        for (auto &address : addresses) {
            readDataBlock(filesystem_data, fs_file, address);
        }

        std::cout << std::endl;

        fs_file.close();
    } else {
        return;
    }
}