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

/**
 * Vypise obsah bufferu
 * @param buffer obsah
 * @param len velikost bufferu
 */
void printBuffer(std::fstream &output_file, char buffer[], int32_t len) {
    for (int i = 0; i < len; ++i) {
        if (buffer[i] == EOF) break;                // TODO: asi hloupa podminka
        output_file.write(reinterpret_cast<const char *>(&buffer[i]), sizeof(char));
    }
}

/**
 * Nacte datablock do bufferu a vypise jej
 * @param filesystem_data informace filesystemu
 * @param fs_file otevreny soubor na pseudoNTFS
 * @param output_file otevreny soubor na pevnem disku
 * @param location pocatecni pozice datablocku
 */
void readDataBlock(filesystem &filesystem_data, std::fstream &fs_file, std::fstream &output_file, int32_t location) {
    // priprava bufferu
    char buffer[filesystem_data.super_block.cluster_size];
    memset(buffer, EOF, filesystem_data.super_block.cluster_size);
    // presun na pozici
    fs_file.seekp(location);
    fs_file.read(buffer, filesystem_data.super_block.cluster_size);
    printBuffer(output_file, buffer, filesystem_data.super_block.cluster_size);
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
    pseudo_inode *inode_ptr = getFileINode(filesystem_data, s1);
    if (inode_ptr != nullptr) {
        inode = *inode_ptr;

        std::fstream fs_file;
        std::fstream output_file;

        // Vytvoreni / format souboru
        output_file.open(s2, std::ios::binary | std::ios::trunc | std::ios::in | std::ios::out);
        output_file.close();

        fs_file.open(filesystem_data.fs_file, std::ios::in | std::ios::out | std::ios::binary);
        output_file.open(s2, std::ios::in | std::ios::out | std::ios::binary);

        if (inode.direct1 != 0) {
            readDataBlock(filesystem_data, fs_file, output_file, inode.direct1);
        }
        if (inode.direct2 != 0) {
            readDataBlock(filesystem_data, fs_file, output_file, inode.direct2);
        }
        if (inode.direct3 != 0) {
            readDataBlock(filesystem_data, fs_file, output_file, inode.direct3);
        }
        if (inode.direct4 != 0) {
            readDataBlock(filesystem_data, fs_file, output_file, inode.direct4);
        }
        if (inode.direct5 != 0) {
            readDataBlock(filesystem_data, fs_file, output_file, inode.direct5);
        }
        if (inode.indirect1 != 0) {
            uint32_t links_per_cluster = filesystem_data.super_block.cluster_size / sizeof(int32_t);
            int32_t links[links_per_cluster];
            fs_file.seekp(inode.indirect1);
            fs_file.read(reinterpret_cast<char *>(&links), sizeof(links));
            for (int i = 0; i < links_per_cluster; i++) {
                if (links[i] != 0) {
                    readDataBlock(filesystem_data, fs_file, output_file, links[i]);
                }
            }
        }

        std::cout << std::endl;

        output_file.close();
        fs_file.close();
    } else {
        return;
    }
}