//
// Created by pavel on 03.01.20.
//

#include <string>
#include <sys/stat.h>
#include <fstream>
#include <bitset>
#include <iostream>
#include <cstring>
#include <vector>
#include "outcp.h"
#include "zosfsstruct.h"
#include "directory.h"
#include "inode.h"
#include "datablock.h"

/**
 * Vypise obsah bufferu
 * @param buffer obsah
 * @param len velikost bufferu
 */
void printBuffer(std::fstream &output_file, char buffer[], int32_t len) {
    for (int i = 0; i < len; ++i) {
        output_file.write(reinterpret_cast<const char *>(&buffer[i]), sizeof(char));
    }
}

/**
 * Nacte datablock do bufferu a vypise jej
 * @param filesystem_data informace filesystemu
 * @param fs_file otevreny soubor na pseudoNTFS
 * @param output_file otevreny soubor na pevnem disku
 * @param location pocatecni pozice datablocku
 * @param to_export exportovana cast (musi byt rovna nebo mensi nez cluster_size!)
 */
void readDataBlock(filesystem &filesystem_data, std::fstream &fs_file, std::fstream &output_file, int32_t location,
                   long to_export) {
    // priprava bufferu
    char buffer[to_export];
    memset(buffer, EOF, to_export);
    // presun na pozici
    fs_file.seekp(location);
    fs_file.read(buffer, to_export);
    printBuffer(output_file, buffer, to_export);
}

/**
 * Nahraje soubor s1 z pseudoNTFS do umisteni s2 na pevnem disku
 * @param filesystem_data filesystem
 * @param s1 soubor pseudoNTFS
 * @param s2 soubor na pevnem disku
 */
void outcp(filesystem &filesystem_data, std::string &s1, std::string &s2) {
    // najdu odpovidajici inode
    pseudo_inode inode;
    pseudo_inode *inode_ptr = getFileINode(filesystem_data, filesystem_data.current_dir, s1);
    if (inode_ptr != nullptr) {
        inode = *inode_ptr;

        std::fstream fs_file;
        std::fstream output_file;

        // Vytvoreni / format souboru
        output_file.open(s2, std::ios::binary | std::ios::trunc | std::ios::in | std::ios::out);
        output_file.close();

        fs_file.open(filesystem_data.fs_file, std::ios::in | std::ios::out | std::ios::binary);
        output_file.open(s2, std::ios::in | std::ios::out | std::ios::binary);

        // platne adresy na databloky
        std::vector<int32_t> addresses = usedDatablockByINode(filesystem_data, fs_file, inode, false);

        long remaining = inode.file_size;                               // zbyva exportovat bytu
        long to_export = filesystem_data.super_block.cluster_size;      // bude exportovano nasledujici iteraci

        // prochazeni adres databloku
        for (auto &address : addresses) {
            if (to_export > remaining) to_export = remaining;
            readDataBlock(filesystem_data, fs_file, output_file, address, to_export);
            remaining -= to_export;
        }

        std::cout << std::endl;

        addDatablockToINode(filesystem_data, fs_file, inode);

        output_file.close();
        fs_file.close();
    } else {
        return;
    }
}