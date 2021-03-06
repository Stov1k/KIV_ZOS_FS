//
// Created by pavel on 28.12.19.
//

#include <string>
#include <sys/stat.h>
#include <fstream>
#include <bitset>
#include <iostream>
#include <cstring>
#include <cmath>
#include <experimental/filesystem>
#include "incp.h"
#include "zosfsstruct.h"
#include "inode.h"
#include "directory.h"
#include "datablock.h"
#include "cd.h"

/**
 * Vrati velikost souboru mimo FS
 * @param filename umisteni
 * @return velikost souboru
 */
long getFilesize(std::string &filename) {
    struct stat stat_buf;
    int rc = stat(filename.c_str(), &stat_buf);
    return rc == 0 ? stat_buf.st_size : -1;
}

/**
 * Zapis dat z pevneho disku do filesystemu
 * @param filesystem_data filesystem
 * @param datablock datablok
 * @param cpin_file otevreny soubor na pevnem disku
 * @param fs_file otevreny soubor filesystemu
 * @param buffer
 * @return pozice
 */
int32_t writeDatablock(filesystem &filesystem_data, int32_t *datablock, std::ifstream &cpin_file, std::fstream &fs_file,
                       char *buffer) {
    // ziskani datablocku
    getFreeDatablock(filesystem_data, datablock);
    int32_t position_absolute = datablock[0];
    int32_t position_byte = datablock[3];           // poradi bytu v bitmape
    uint8_t bitmap_byte = (uint8_t) datablock[2];
    // nacteni dat do bufferu k zapisu
    cpin_file.read(buffer, filesystem_data.super_block.cluster_size);

    // aktualizace bitmapy
    fs_file.seekp(filesystem_data.super_block.bitmap_start_address + position_byte); // skoci na bitmapu
    fs_file.write(reinterpret_cast<const char *>(&bitmap_byte), sizeof(bitmap_byte));    // zapise upravenou bitmapu

    // aktualizace dat
    int32_t datablock_position = filesystem_data.super_block.data_start_address +
                                 (position_absolute * filesystem_data.super_block.cluster_size);
    fs_file.seekp(datablock_position);
    fs_file.write(buffer, filesystem_data.super_block.cluster_size);

    memset(buffer, EOF, filesystem_data.super_block.cluster_size);        // vymaze obsah bufferu

    return datablock_position;
}

/**
 * Nahraje soubor s1 z pevneho disku do umisteni s2 v pseudoFS
 * @param filesystem_data filesystem
 * @param s1 vstupni soubor na pevnem disku
 * @param s2 umisteni na pseudoFS
 */
void incp(filesystem &filesystem_data, std::string &s1, std::string &s2) {

    // existuje cesta ke vstupnimu souboru na pevnem disku?
    if (!std::experimental::filesystem::exists(s1)) {
        std::cout << "FILE NOT FOUND" << std::endl;
        return;
    }

    // cesta rozdelena na adresare
    std::vector<std::string> to_segments = splitPath(s2);

    pseudo_inode *to_dir_ptr = cd(filesystem_data, s2, false, false);
    pseudo_inode to_dir = filesystem_data.current_dir;
    if (to_dir_ptr != nullptr) {
        to_dir = *to_dir_ptr;
    } else {
        std::cout << "PATH NOT FOUND" << std::endl;
        return;
    }

    // zjisti, zdali jiz soubor stejneho nazvu existuje
    if (isDirectoryExists(filesystem_data, to_dir, to_segments.back())) {
        std::cout << "EXIST" << std::endl;
        return;
    }
    directory_item dir = createDirectoryItem(0, to_segments.back());

    // spocte velikost input
    long filesize = getFilesize(s1);

    // potreba volnych datablocku
    int32_t links_per_cluster = linksPerCluster(filesystem_data);
    int32_t available_datablocks = availableDatablocks(filesystem_data);
    int32_t maximum_datablocks = maximumDatablocksPerINode(filesystem_data);
    int32_t datablock_needed = ceil((double) (filesize) / (double) (filesystem_data.super_block.cluster_size));
    if (datablock_needed > 5) {
        datablock_needed += ceil((double) datablock_needed / (double) links_per_cluster) + 1;
    }
    if (datablock_needed > available_datablocks) {
        int32_t free_space = available_datablocks * filesystem_data.super_block.cluster_size;
        std::cout << "NOT ENOUGH SPACE (FREE: " << free_space << " REQUIRED: " << filesize << ")" << std::endl;
        return;
    }
    if (datablock_needed > maximum_datablocks) {
        int32_t max_file = maximum_datablocks * filesystem_data.super_block.cluster_size;
        std::cout << "FILE IS TOO LARGE (MAXIMUM: " << max_file << " REQUIRED: " << filesize << ")" << std::endl;
        return;
    }

    // zajisti volny inode
    pseudo_inode *inode_ptr = getFreeINode(filesystem_data);
    pseudo_inode inode;
    if (inode_ptr != nullptr) {
        inode = *inode_ptr;
    } else {
        std::cout << "FREE INODE NOT FOUND" << std::endl;
        return;
    }

    // vytvor adresar
    std::fstream fs_file;
    fs_file.open(filesystem_data.fs_file, std::ios::in | std::ios::out | std::ios::binary);
    fs_file.seekp(to_dir.direct1);
    uint32_t dirs_per_cluster = filesystem_data.super_block.cluster_size / sizeof(directory_item);
    directory_item dirs[dirs_per_cluster];
    fs_file.read(reinterpret_cast<char *>(&dirs), sizeof(dirs));
    for (int i = 0; i < dirs_per_cluster; i++) {
        if (!dirs[i].inode) {
            std::strncpy(dirs[i].item_name, dir.item_name, sizeof(dir.item_name));
            dirs[i].inode = inode.nodeid;
            break;
        }
    }

    std::ifstream cpin_file(s1, std::ios::in | std::ios::binary);
    // provest zapis
    int writed = 0;
    char buffer[filesystem_data.super_block.cluster_size];
    int32_t datablock[4];

    // vymaze obsah bufferu
    memset(buffer, EOF, filesystem_data.super_block.cluster_size);

    // 1P - prvni datablock
    if (writed < filesize) {
        inode.direct1 = writeDatablock(filesystem_data, datablock, cpin_file, fs_file, buffer);
        writed += sizeof(buffer);
    }

    // 2P - druhy datablock
    if (writed < filesize) {
        inode.direct2 = writeDatablock(filesystem_data, datablock, cpin_file, fs_file, buffer);
        writed += sizeof(buffer);
    }

    // 3P - treti datablock
    if (writed < filesize) {
        inode.direct3 = writeDatablock(filesystem_data, datablock, cpin_file, fs_file, buffer);
        writed += sizeof(buffer);
    }

    // 4P - ctvrty datablock
    if (writed < filesize) {
        inode.direct4 = writeDatablock(filesystem_data, datablock, cpin_file, fs_file, buffer);
        writed += sizeof(buffer);
    }

    // 5P - paty datablock
    if (writed < filesize) {
        inode.direct5 = writeDatablock(filesystem_data, datablock, cpin_file, fs_file, buffer);
        writed += sizeof(buffer);
    }

    // 1N - prvni neprimy
    if (writed < filesize) {
        inode.indirect1 = createIndirectDatablock(filesystem_data, fs_file);

        int32_t links[links_per_cluster];
        fs_file.seekp(inode.indirect1);
        fs_file.read(reinterpret_cast<char *>(&links), sizeof(links));

        int iteration = 0;
        while (writed < filesize && iteration < links_per_cluster) {
            links[iteration] = writeDatablock(filesystem_data, datablock, cpin_file, fs_file, buffer);

            // aktualizace odkazu
            fs_file.seekp(inode.indirect1);
            fs_file.write(reinterpret_cast<const char *>(&links), filesystem_data.super_block.cluster_size);

            writed += sizeof(buffer);
            iteration++;
        }
    }

    // 2N - druhy neprimy
    if (writed < filesize) {
        inode.indirect2 = createIndirectDatablock(filesystem_data, fs_file);

        int32_t links[links_per_cluster];
        fs_file.seekp(inode.indirect2);
        fs_file.read(reinterpret_cast<char *>(&links), sizeof(links));

        int iteration = 0;
        while (writed < filesize && iteration < links_per_cluster) {
            links[iteration] = createIndirectDatablock(filesystem_data, fs_file);

            // aktualizace odkazu
            fs_file.seekp(inode.indirect2);
            fs_file.write(reinterpret_cast<const char *>(&links), filesystem_data.super_block.cluster_size);

            // nacteni odkazu
            int32_t sublinks[links_per_cluster];
            fs_file.seekp(links[iteration]);
            fs_file.read(reinterpret_cast<char *>(&sublinks), sizeof(sublinks));

            int j = 0;
            while (writed < filesize && j < links_per_cluster) {
                sublinks[j] = writeDatablock(filesystem_data, datablock, cpin_file, fs_file, buffer);

                // aktualizace odkazu
                fs_file.seekp(links[iteration]);
                fs_file.write(reinterpret_cast<const char *>(&sublinks), filesystem_data.super_block.cluster_size);

                writed += sizeof(buffer);
                j++;
            }

            iteration++;
        }

    }

    inode.file_size = filesize;     // puvodni velikost souboru, nutne pro export

    // aktualizace podslozek v nadrazenem adresari
    fs_file.seekp(to_dir.direct1); // skoci na data
    fs_file.write(reinterpret_cast<const char *>(&dirs), sizeof(dirs));

    // zapis inode
    inode.type = 0;
    inode.references++;
    fs_file.seekp(getINodePosition(filesystem_data, inode.nodeid));
    fs_file.write(reinterpret_cast<const char *>(&inode), sizeof(pseudo_inode));

    cpin_file.close();
    fs_file.close();

    std::cout << "OK" << std::endl;
}
