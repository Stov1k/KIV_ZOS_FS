//
// Created by pavel on 12.01.20.
//

#include <iostream>
#include <vector>
#include <iterator>
#include <sstream>
#include <fstream>
#include <cmath>
#include <cstring>
#include <bitset>
#include <sys/stat.h>
#include <cstdio>
#include <experimental/filesystem>
#include "zosfsstruct.h"
#include "incp.h"
#include "inode.h"
#include "directory.h"
#include "datablock.h"
#include "cat.h"
#include "outcp.h"
#include "rm.h"
#include "info.h"
#include "pwd.h"
#include "cd.h"
#include "mkdir.h"
#include "mv.h"
#include "cp.h"
#include "ls.h"
#include "format.h"

/**
 * Naformatovani souboru pseudoFS
 * @param filesystem_data filesystem
 * @param size velikost disku
 */
void format(filesystem &filesystem_data, int32_t size) {
    int32_t remaining_space = size;

    std::fstream input_file;

    // Vytvoreni / format souboru
    input_file.open(filesystem_data.fs_file, std::ios::binary | std::ios::trunc | std::ios::in | std::ios::out);
    input_file.close();

    // existuje cesta ke vstupnimu souboru na pevnem disku?
    if (!std::experimental::filesystem::exists(filesystem_data.fs_file)) {
        std::cout << "CANNOT CREATE FILE" << std::endl;
        return;
    }

    // otevreni souboru pro zapis
    input_file.open(filesystem_data.fs_file, std::ios::binary | std::ios::trunc | std::ios::in | std::ios::out);

    // prazdny 1B pouzivany pri zapisu
    uint8_t byte{0};

    // super block
    filesystem_data.super_block = superblock{};
    std::string signature = "zelenka";
    std::string description = "muj fs";
    std::strncpy(filesystem_data.super_block.signature, signature.c_str(),
                 sizeof(filesystem_data.super_block.signature));
    filesystem_data.super_block.signature[sizeof(filesystem_data.super_block.signature) - 1] = '\0';
    std::strncpy(filesystem_data.super_block.volume_descriptor, description.c_str(),
                 sizeof(filesystem_data.super_block.volume_descriptor));
    filesystem_data.super_block.volume_descriptor[sizeof(filesystem_data.super_block.volume_descriptor) - 1] = '\0';

    int cluster_size = 1024;
    if (size > 314572800) {
        cluster_size = 4096;
    } else if (size > 104857600) {
        cluster_size = 2048;
    }
    remaining_space = remaining_space - sizeof(superblock);
    int inodes = remaining_space / cluster_size;
    remaining_space = remaining_space - (inodes * sizeof(pseudo_inode));
    int bitmap_size = 0;
    while (remaining_space > 0) {
        remaining_space = remaining_space - cluster_size;
        if (bitmap_size % 8 == 0) {
            remaining_space -= 1;
        }
        bitmap_size += 1;
    }
    int correction = bitmap_size %
                     8;                     // velikost FS bude vzdy delitelna 8 (bitmapa pak vyuzije vzdy cely byte)
    bitmap_size = bitmap_size - correction;
    int bitmap_size_bytes = bitmap_size / 8;

    int clusters = bitmap_size;

    filesystem_data.super_block.disk_size = sizeof(superblock) + bitmap_size + inodes + cluster_size * clusters;
    filesystem_data.super_block.cluster_size = cluster_size;
    filesystem_data.super_block.cluster_count = clusters;
    filesystem_data.super_block.bitmap_start_address = sizeof(filesystem_data.super_block);
    filesystem_data.super_block.inode_start_address =
            sizeof(filesystem_data.super_block) + sizeof(byte) * bitmap_size_bytes;
    filesystem_data.super_block.data_start_address =
            sizeof(filesystem_data.super_block) + sizeof(byte) * bitmap_size_bytes + sizeof(pseudo_inode) * inodes;
    input_file.write(reinterpret_cast<const char *>(&filesystem_data.super_block), sizeof(superblock));


    // zapis inodu
    pseudo_inode inode;
    for (int i = 0; i < inodes; i++) {
        inode = pseudo_inode{};
        inode.nodeid = ID_ITEM_FREE;
        input_file.write(reinterpret_cast<const char *>(&inode), sizeof(pseudo_inode));
    }

    // zapis bitmapy
    for (int bmp = 0; bmp < bitmap_size_bytes; bmp++) {
        input_file.write(reinterpret_cast<const char *>(&byte), sizeof(byte));
    }

    // zapis vsech clusteru (data bloku)
    for (int i = 0; i < clusters; i++) {
        // zapis jednoho konkretniho data bloku
        for (int j = 0; j < cluster_size; j++) {
            input_file.write(reinterpret_cast<const char *>(&byte), sizeof(byte));
        }
    }
    input_file.flush();

    // vytvoreni korenoveho adresare
    directory_item dir;
    inode.nodeid = 1;
    inode.type = 1;
    inode.references++;
    inode.direct1 = filesystem_data.super_block.data_start_address;
    inode.direct2 = 0;
    inode.direct3 = 0;
    inode.direct4 = 0;
    inode.direct5 = 0;
    inode.indirect1 = 0;
    inode.indirect2 = 0;
    inode.file_size = filesystem_data.super_block.cluster_size;

    // aktualni adresar
    dir = createDirectoryItem(inode.nodeid, ".");

    int32_t dirs_per_cluster = cluster_size / sizeof(directory_item);

    // predchozi adresar
    directory_item dir_parrent;
    dir_parrent = createDirectoryItem(inode.nodeid, "..");;

    // nastaveni referenci na adresare
    directory_item directories[dirs_per_cluster];
    directories[0] = dir;                           // .
    directories[1] = dir_parrent;                   // ..
    for (int i = 2; i < dirs_per_cluster; i++) {     // dalsi neobsazene
        directories[i] = directory_item{};
        directories[i].inode = 0;   // tj. nic
    }
    filesystem_data.current_dir = inode;

    // vytvoreni korenoveho adresare: uprava bitmapy
    input_file.seekp(filesystem_data.super_block.bitmap_start_address); // skoci na bitmapu
    uint8_t byte1{1};
    input_file.write(reinterpret_cast<const char *>(&byte1), sizeof(byte1));    // zapise upravenou bitmapu

    // vytvoreni korenoveho adresare: zapis dat
    input_file.seekp(filesystem_data.super_block.data_start_address); // skoci na data
    input_file.write(reinterpret_cast<const char *>(&directories), sizeof(directories));

    // vytvoreni korenoveho adresare: zapis inode
    input_file.seekp(filesystem_data.super_block.inode_start_address); // skoci na inody
    input_file.write(reinterpret_cast<const char *>(&inode), sizeof(inode));

    input_file.close();

    std::cout << "OK" << std::endl;
}